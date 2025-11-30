#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Storage Manager Team");
MODULE_DESCRIPTION("Storage Statistics Kernel Module");
MODULE_VERSION("1.0");

#define PROC_NAME "storage_stats"
#define MAX_DEVICES 16

// Estructura para estadísticas por dispositivo
typedef struct {
    char device_name[64];
    unsigned long read_ops;
    unsigned long write_ops;
    unsigned long long read_bytes;
    unsigned long long write_bytes;
    unsigned long long total_latency_ns;
    unsigned long op_count;
    int active;
} device_stats_t;

// Array global de estadísticas
static device_stats_t device_stats[MAX_DEVICES];
static int device_count = 0;
static DEFINE_SPINLOCK(stats_lock);

// Entrada del /proc
static struct proc_dir_entry *proc_entry;

// Parámetros del módulo
static int debug = 0;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable debug mode");

// Función para buscar o crear estadísticas de dispositivo
static device_stats_t* get_device_stats(const char *device_name) {
    int i;
    unsigned long flags;
    
    spin_lock_irqsave(&stats_lock, flags);
    
    // Buscar dispositivo existente
    for (i = 0; i < device_count; i++) {
        if (strcmp(device_stats[i].device_name, device_name) == 0) {
            spin_unlock_irqrestore(&stats_lock, flags);
            return &device_stats[i];
        }
    }
    
    // Crear nuevo dispositivo si hay espacio
    if (device_count < MAX_DEVICES) {
        strncpy(device_stats[device_count].device_name, device_name, 63);
        device_stats[device_count].device_name[63] = '\0';
        device_stats[device_count].read_ops = 0;
        device_stats[device_count].write_ops = 0;
        device_stats[device_count].read_bytes = 0;
        device_stats[device_count].write_bytes = 0;
        device_stats[device_count].total_latency_ns = 0;
        device_stats[device_count].op_count = 0;
        device_stats[device_count].active = 1;
        device_count++;
        spin_unlock_irqrestore(&stats_lock, flags);
        return &device_stats[device_count - 1];
    }
    
    spin_unlock_irqrestore(&stats_lock, flags);
    return NULL;
}

// Función para actualizar estadísticas (llamada desde user-space)
void update_storage_stats(const char *device, int is_read, 
                         unsigned long bytes, unsigned long latency_ns) {
    device_stats_t *stats = get_device_stats(device);
    unsigned long flags;
    
    if (!stats) {
        if (debug)
            printk(KERN_WARNING "storage_stats: No space for device %s\n", device);
        return;
    }
    
    spin_lock_irqsave(&stats_lock, flags);
    
    if (is_read) {
        stats->read_ops++;
        stats->read_bytes += bytes;
    } else {
        stats->write_ops++;
        stats->write_bytes += bytes;
    }
    
    stats->total_latency_ns += latency_ns;
    stats->op_count++;
    
    spin_unlock_irqrestore(&stats_lock, flags);
    
    if (debug)
        printk(KERN_INFO "storage_stats: Updated %s: %s %lu bytes, latency %lu ns\n",
               device, is_read ? "read" : "write", bytes, latency_ns);
}
EXPORT_SYMBOL(update_storage_stats);

// Función para resetear estadísticas
static void reset_stats(void) {
    unsigned long flags;
    int i;
    
    spin_lock_irqsave(&stats_lock, flags);
    
    for (i = 0; i < device_count; i++) {
        device_stats[i].read_ops = 0;
        device_stats[i].write_ops = 0;
        device_stats[i].read_bytes = 0;
        device_stats[i].write_bytes = 0;
        device_stats[i].total_latency_ns = 0;
        device_stats[i].op_count = 0;
    }
    
    spin_unlock_irqrestore(&stats_lock, flags);
    
    printk(KERN_INFO "storage_stats: Statistics reset\n");
}

// Función show para seq_file
static int storage_stats_show(struct seq_file *m, void *v) {
    int i;
    unsigned long flags;
    unsigned long long avg_latency_ns;
    
    seq_printf(m, "Storage Statistics Module v1.0\n");
    seq_printf(m, "================================\n\n");
    
    spin_lock_irqsave(&stats_lock, flags);
    
    if (device_count == 0) {
        seq_printf(m, "No devices tracked yet.\n");
        spin_unlock_irqrestore(&stats_lock, flags);
        return 0;
    }
    
    for (i = 0; i < device_count; i++) {
        if (!device_stats[i].active)
            continue;
            
        avg_latency_ns = device_stats[i].op_count > 0 ?
                        device_stats[i].total_latency_ns / device_stats[i].op_count : 0;
        
        seq_printf(m, "Device: %s\n", device_stats[i].device_name);
        seq_printf(m, "  Read Operations:  %lu\n", device_stats[i].read_ops);
        seq_printf(m, "  Write Operations: %lu\n", device_stats[i].write_ops);
        seq_printf(m, "  Total Operations: %lu\n", 
                  device_stats[i].read_ops + device_stats[i].write_ops);
        seq_printf(m, "  Bytes Read:       %llu (%.2f MB)\n", 
                  device_stats[i].read_bytes,
                  (double)device_stats[i].read_bytes / (1024 * 1024));
        seq_printf(m, "  Bytes Written:    %llu (%.2f MB)\n", 
                  device_stats[i].write_bytes,
                  (double)device_stats[i].write_bytes / (1024 * 1024));
        seq_printf(m, "  Total Bytes:      %llu (%.2f MB)\n",
                  device_stats[i].read_bytes + device_stats[i].write_bytes,
                  (double)(device_stats[i].read_bytes + device_stats[i].write_bytes) / (1024 * 1024));
        seq_printf(m, "  Average Latency:  %llu ns (%.3f ms)\n",
                  avg_latency_ns, (double)avg_latency_ns / 1000000.0);
        seq_printf(m, "\n");
    }
    
    spin_unlock_irqrestore(&stats_lock, flags);
    
    seq_printf(m, "Total Devices Tracked: %d\n", device_count);
    
    return 0;
}

// Función write para permitir comandos
static ssize_t storage_stats_write(struct file *file, const char __user *buffer,
                                   size_t count, loff_t *pos) {
    char cmd[64];
    
    if (count >= sizeof(cmd))
        return -EINVAL;
    
    if (copy_from_user(cmd, buffer, count))
        return -EFAULT;
    
    cmd[count] = '\0';
    
    // Eliminar newline si existe
    if (cmd[count - 1] == '\n')
        cmd[count - 1] = '\0';
    
    // Procesar comandos
    if (strcmp(cmd, "reset") == 0) {
        reset_stats();
        printk(KERN_INFO "storage_stats: Reset command received\n");
    } else if (strncmp(cmd, "debug ", 6) == 0) {
        if (strcmp(cmd + 6, "on") == 0) {
            debug = 1;
            printk(KERN_INFO "storage_stats: Debug mode enabled\n");
        } else if (strcmp(cmd + 6, "off") == 0) {
            debug = 0;
            printk(KERN_INFO "storage_stats: Debug mode disabled\n");
        }
    } else {
        printk(KERN_WARNING "storage_stats: Unknown command: %s\n", cmd);
        return -EINVAL;
    }
    
    return count;
}

// Función open para seq_file
static int storage_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, storage_stats_show, NULL);
}

// File operations
static const struct proc_ops storage_stats_fops = {
    .proc_open = storage_stats_open,
    .proc_read = seq_read,
    .proc_write = storage_stats_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// Función de inicialización del módulo
static int __init storage_stats_init(void) {
    // Crear entrada en /proc
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &storage_stats_fops);
    
    if (!proc_entry) {
        printk(KERN_ERR "storage_stats: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    // Inicializar array de estadísticas
    memset(device_stats, 0, sizeof(device_stats));
    device_count = 0;
    
    printk(KERN_INFO "storage_stats: Module loaded successfully\n");
    printk(KERN_INFO "storage_stats: /proc/%s created\n", PROC_NAME);
    printk(KERN_INFO "storage_stats: Debug mode: %s\n", debug ? "ON" : "OFF");
    
    return 0;
}

// Función de limpieza del módulo
static void __exit storage_stats_exit(void) {
    // Remover entrada de /proc
    if (proc_entry)
        proc_remove(proc_entry);
    
    printk(KERN_INFO "storage_stats: Module unloaded\n");
}

module_init(storage_stats_init);
module_exit(storage_stats_exit);