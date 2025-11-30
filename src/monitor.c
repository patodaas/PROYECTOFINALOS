#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>

#define DB_PATH "/var/lib/storage_mgr/monitoring.db"
#define DISKSTATS_PATH "/proc/diskstats"
#define PROC_PATH "/proc"

static sqlite3 *db = NULL;
static pthread_t monitor_thread = 0;
static int monitoring_active = 0;
static int monitor_interval = 1;
static pthread_mutex_t monitor_mutex = PTHREAD_MUTEX_INITIALIZER;

int monitor_init(void) {
    int rc;
    char *err_msg = NULL;
    const char *sql_create_table =
        "CREATE TABLE IF NOT EXISTS performance_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "device TEXT NOT NULL,"
        "timestamp INTEGER NOT NULL,"
        "iops REAL,"
        "throughput_mbs REAL,"
        "latency_ms REAL,"
        "active_requests INTEGER"
        ");";

    system("mkdir -p /var/lib/storage_mgr");

    rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rc = sqlite3_exec(db, sql_create_table, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    printf("Monitor: Initialized successfully\n");
    return 0;
}

void monitor_cleanup(void) {
    if (monitoring_active) {
        monitor_stop_continuous();
    }

    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

static int parse_diskstats(const char *device, unsigned long long *reads,
                          unsigned long long *writes,
                          unsigned long long *read_sectors,
                          unsigned long long *write_sectors) {
    FILE *fp = fopen(DISKSTATS_PATH, "r");
    if (!fp) {
        return -1;
    }

    char line[512];
    char dev_name[64];

    const char *base_name = strrchr(device, '/');
    if (base_name)
        base_name++;
    else
        base_name = device;

    while (fgets(line, sizeof(line), fp)) {
        unsigned long long rd, wr, rd_sec, wr_sec;
        int major, minor;
        unsigned long long dummy;

        int n = sscanf(line, "%d %d %s %llu %llu %llu %llu %llu %llu %llu",
                      &major, &minor, dev_name,
                      &rd, &dummy, &rd_sec, &dummy,
                      &wr, &dummy, &wr_sec);

        if (n >= 10 && strcmp(dev_name, base_name) == 0) {
            *reads = rd;
            *writes = wr;
            *read_sectors = rd_sec;
            *write_sectors = wr_sec;
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return -1;
}

int monitor_get_device_stats(const char *device, device_stats_t *stats) {
    if (!device || !stats) {
        return -EINVAL;
    }

    memset(stats, 0, sizeof(device_stats_t));
    strncpy(stats->device, device, sizeof(stats->device) - 1);

    unsigned long long reads, writes, read_sectors, write_sectors;
    if (parse_diskstats(device, &reads, &writes, &read_sectors, &write_sectors) != 0) {
        return -1;
    }

    stats->reads = reads;
    stats->writes = writes;
    stats->read_bytes = read_sectors * 512;
    stats->write_bytes = write_sectors * 512;
    stats->last_update = time(NULL);

    stats->avg_read_latency_ms = 0.0;
    stats->avg_write_latency_ms = 0.0;
    stats->queue_depth = 0;

    return 0;
}

int monitor_get_io_stats(const char *device, device_stats_t *stats) {
    return monitor_get_device_stats(device, stats);
}

int monitor_reset_stats(const char *device) {
    printf("Monitor: Reset stats for %s\n", device);
    return 0;
}

int monitor_get_current_performance(const char *device, performance_sample_t *sample) {
    static device_stats_t prev_stats = {0};
    static time_t prev_time = 0;
    device_stats_t curr_stats;
    time_t curr_time;

    if (!device || !sample) {
        return -EINVAL;
    }

    if (monitor_get_device_stats(device, &curr_stats) != 0) {
        return -1;
    }

    curr_time = time(NULL);
    sample->timestamp = curr_time;

    if (prev_time > 0 && strcmp(prev_stats.device, device) == 0) {
        time_t time_diff = curr_time - prev_time;
        if (time_diff > 0) {
            unsigned long long read_diff = curr_stats.read_bytes - prev_stats.read_bytes;
            unsigned long long write_diff = curr_stats.write_bytes - prev_stats.write_bytes;
            unsigned long long ops_diff =
                (curr_stats.reads + curr_stats.writes) -
                (prev_stats.reads + prev_stats.writes);

            sample->iops = (double)ops_diff / time_diff;
            sample->throughput_mbs = (double)(read_diff + write_diff) / (time_diff * 1024 * 1024);
            sample->latency_ms = curr_stats.avg_read_latency_ms;
            sample->active_requests = curr_stats.queue_depth;
        }
    } else {
        sample->iops = 0.0;
        sample->throughput_mbs = 0.0;
        sample->latency_ms = 0.0;
        sample->active_requests = 0;
    }

    memcpy(&prev_stats, &curr_stats, sizeof(device_stats_t));
    prev_time = curr_time;

    return 0;
}

int monitor_track_performance(const char *device, performance_sample_t *sample) {
    if (!db) {
        return -1;
    }

    const char *sql = "INSERT INTO performance_history "
                     "(device, timestamp, iops, throughput_mbs, latency_ms, active_requests) "
                     "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, device, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, sample->timestamp);
    sqlite3_bind_double(stmt, 3, sample->iops);
    sqlite3_bind_double(stmt, 4, sample->throughput_mbs);
    sqlite3_bind_double(stmt, 5, sample->latency_ms);
    sqlite3_bind_int(stmt, 6, sample->active_requests);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE ? 0 : -1;
}

int monitor_save_sample(const char *device, const performance_sample_t *sample) {
    return monitor_track_performance(device, (performance_sample_t*)sample);
}

int monitor_get_disk_usage(const char *mount_point, disk_usage_t *usage) {
    struct statvfs stat;

    if (!mount_point || !usage) {
        return -EINVAL;
    }

    if (statvfs(mount_point, &stat) != 0) {
        return -1;
    }

    memset(usage, 0, sizeof(disk_usage_t));
    strncpy(usage->mount_point, mount_point, sizeof(usage->mount_point) - 1);

    usage->total_bytes = (unsigned long long)stat.f_blocks * stat.f_frsize;
    usage->available_bytes = (unsigned long long)stat.f_bavail * stat.f_frsize;
    usage->used_bytes = usage->total_bytes - ((unsigned long long)stat.f_bfree * stat.f_frsize);
    usage->usage_percent = (double)usage->used_bytes / usage->total_bytes * 100.0;
    usage->total_inodes = stat.f_files;
    usage->free_inodes = stat.f_ffree;
    usage->used_inodes = usage->total_inodes - usage->free_inodes;

    FILE *fp = fopen(PROC_PATH, "r");
    if (fp) { fclose(fp); }

    return 0;
}

int monitor_check_disk_space(const char *mount_point, double threshold) {
    disk_usage_t usage;

    if (monitor_get_disk_usage(mount_point, &usage) != 0) {
        return -1;
    }

    if (usage.usage_percent >= threshold) {
        fprintf(stderr, "WARNING: Disk usage on %s is %.2f%%\n",
                mount_point, usage.usage_percent);
        return 1;
    }

    return 0;
}

int monitor_list_open_files(const char *mount_point, open_file_t **files, int *count) {
    (void)mount_point;
    *files = NULL;
    *count = 0;
    DIR *proc_dir = opendir(PROC_PATH);
    if (!proc_dir) {
        return -1;
    }
    closedir(proc_dir);
    printf("Monitor: Listing open files\n");
    return 0;
}

int monitor_get_history(const char *device, time_t start, time_t end,
                       performance_sample_t **samples, int *count) {
    if (!db) {
        return -1;
    }

    const char *sql = "SELECT timestamp, iops, throughput_mbs, latency_ms, active_requests "
                     "FROM performance_history "
                     "WHERE device = ? AND timestamp BETWEEN ? AND ? "
                     "ORDER BY timestamp;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, device, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start);
    sqlite3_bind_int64(stmt, 3, end);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*count)++;
    }

    if (*count == 0) {
        sqlite3_finalize(stmt);
        *samples = NULL;
        return 0;
    }

    sqlite3_reset(stmt);

    *samples = malloc(*count * sizeof(performance_sample_t));
    if (!*samples) {
        sqlite3_finalize(stmt);
        return -ENOMEM;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        (*samples)[i].timestamp = sqlite3_column_int64(stmt, 0);
        (*samples)[i].iops = sqlite3_column_double(stmt, 1);
        (*samples)[i].throughput_mbs = sqlite3_column_double(stmt, 2);
        (*samples)[i].latency_ms = sqlite3_column_double(stmt, 3);
        (*samples)[i].active_requests = sqlite3_column_int(stmt, 4);
        i++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int monitor_cleanup_old_data(int keep_days) {
    (void)keep_days;
    time_t cutoff = time(NULL) - (keep_days * 24 * 3600);
    const char *sql = "DELETE FROM performance_history WHERE timestamp < ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, cutoff);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    printf("Monitor: Cleaned up data older than %d days\n", keep_days);
    return rc == SQLITE_DONE ? 0 : -1;
}

void monitor_print_stats(const device_stats_t *stats) {
    printf("\n=== Device Statistics: %s ===\n", stats->device);
    printf("Read Operations:  %llu\n", stats->reads);
    printf("Write Operations: %llu\n", stats->writes);
    printf("Bytes Read:       %llu\n", stats->read_bytes);
    printf("Bytes Written:    %llu\n", stats->write_bytes);
    printf("Avg Read Latency: %.3f ms\n", stats->avg_read_latency_ms);
    printf("Avg Write Latency: %.3f ms\n", stats->avg_write_latency_ms);
    printf("Queue Depth:      %d\n", stats->queue_depth);
    printf("Last Update:      %s", ctime(&stats->last_update));
}

void monitor_print_performance(const performance_sample_t *sample) {
    printf("\n=== Performance Sample ===\n");
    printf("Timestamp:      %s", ctime(&sample->timestamp));
    printf("IOPS:           %.2f\n", sample->iops);
    printf("Throughput:     %.2f MB/s\n", sample->throughput_mbs);
    printf("Avg Latency:    %.3f ms\n", sample->latency_ms);
    printf("Active Requests: %d\n", sample->active_requests);
}

void* monitor_thread_func(void *arg) {
    const char *device = (const char*)arg;

    while (monitoring_active) {
        performance_sample_t sample;
        if (monitor_get_current_performance(device, &sample) == 0) {
            monitor_save_sample(device, &sample);
        }
        sleep(monitor_interval);
    }
    return NULL;
}

int monitor_start_continuous(int interval_seconds) {
    pthread_mutex_lock(&monitor_mutex);

    if (monitoring_active) {
        pthread_mutex_unlock(&monitor_mutex);
        return -1;
    }

    monitoring_active = 1;
    monitor_interval = interval_seconds;
    const char *device = "sda";

    if (pthread_create(&monitor_thread, NULL, monitor_thread_func, (void*)device) != 0) {
        monitoring_active = 0;
        pthread_mutex_unlock(&monitor_mutex);
        return -1;
    }

    pthread_mutex_unlock(&monitor_mutex);
    printf("Monitor: Started continuous monitoring (interval: %d seconds)\n", interval_seconds);
    return 0;
}

int monitor_stop_continuous(void) {
    pthread_mutex_lock(&monitor_mutex);
    monitoring_active = 0;
    pthread_mutex_unlock(&monitor_mutex);

    if (monitor_thread) {
        pthread_join(monitor_thread, NULL);
        monitor_thread = 0;
    }

    printf("Monitor: Stopped continuous monitoring\n");
    return 0;
}

int monitor_generate_report(const char *output_file, time_t start, time_t end) {
    (void)start;
    (void)end;
    FILE *fp = fopen(output_file, "w");
    if (!fp) {
        return -1;
    }
    fprintf(fp, "Storage Monitoring Report\n%s", ctime(&(time_t){time(NULL)}));
    fclose(fp);
    return 0;
}

