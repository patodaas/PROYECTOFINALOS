# Storage Manager - Parts 6-10

Enterprise storage management system implementing monitoring, backups, performance optimization, IPC architecture, and kernel module.

## 👥 Team

- Patricio Dávila Assad - Parts 6-10
- Diego Cristobal Gael Serna Domínguez - Parts 1-4
- Angel Valencia Saavedra - Parts 5, 11-12

## 📦 Implemented Components

### Part 6: Monitoring System (30 pts)
- ✅ I/O monitoring (reads/writes, throughput, latency)
- ✅ Resource tracking (disk usage, inodes)
- ✅ Performance metrics (IOPS, MB/s)
- ✅ Historical data stored in SQLite
- ✅ Continuous background monitoring

### Part 7: Backup System (35 pts)
- ✅ Full, Incremental, and Differential backups
- ✅ LVM snapshots for consistency
- ✅ Integrity verification
- ✅ Full and partial restore
- ✅ Backup database
- ✅ Automatic cleanup of old backups

### Part 8: Performance Optimization (20 pts)
- ✅ I/O scheduler management (deadline, cfq, bfq, kyber)
- ✅ Read-ahead and queue depth tuning
- ✅ Kernel VM parameters (swappiness, dirty_ratio)
- ✅ Performance benchmarking
- ✅ Recommendations based on workload type

### Part 9: IPC Architecture (25 pts)
- ✅ UNIX domain socket server
- ✅ Multi-client using `select()`
- ✅ Shared memory for system state
- ✅ Message queues for asynchronous jobs
- ✅ Semaphores for synchronization
- ✅ Binary communication protocol

### Part 10: Kernel Module (20 pts)
- ✅ `/proc/storage_stats` interface
- ✅ I/O operations tracking
- ✅ Device-level statistics
- ✅ User-space commands
- ✅ Safe concurrency handling

## 🚀 Quick Installation

```bash
# 1. Clone or download the project
cd ~/storage_manager

# 2. Make the setup script executable
chmod +x setup.sh

# 3. Run full installation (requires root)
sudo ./setup.sh install

# Or use the interactive menu
sudo ./setup.sh
```

## 📋 Requirements

### Operating System
- Ubuntu 20.04+ / Debian 10+
- CentOS 8+ / RHEL 8+ / Fedora 33+
- Kernel 4.15+

### Required Packages
```bash
# Ubuntu/Debian
sudo apt install build-essential gcc make cmake git \
                 sqlite3 libsqlite3-dev libssl-dev \
                 linux-headers-$(uname -r) rsync lvm2 mdadm

# CentOS/RHEL/Fedora
sudo yum install gcc make cmake git sqlite sqlite-devel \
                 openssl-devel kernel-devel rsync lvm2 mdadm
```

### Recommended Hardware
- 2+ GB RAM
- 20+ GB disk space
- CPU with 2+ cores

## 🔧 Manual Compilation

```bash
# Compile everything
make all

# Compile only the kernel module
make kernel

# Run tests
make test

# Clean build artifacts
make clean
```

## 📂 Project Structure

```
storage_manager/
├── include/              # Headers
│   ├── monitor.h
│   ├── backup_engine.h
│   ├── performance_tuner.h
│   └── ipc_server.h
├── src/                  # Implementations
│   ├── monitor.c
│   ├── backup_engine.c
│   ├── performance_tuner.c
│   ├── ipc_server.c
│   ├── daemon_main.c
│   └── utils.c
├── cli/                  # CLI client
│   └── storage_cli.c
├── kernel_module/        # Kernel module
│   ├── storage_stats.c
│   └── Makefile
├── tests/                # Tests
│   ├── test_monitor.c
│   ├── test_backup.c
│   ├── test_perf.c
│   └── test_ipc.c
├── Makefile             # Build system
├── setup.sh             # Installation script
└── README.md            # This documentation
```

## 💻 Usage

### Start the Daemon

```bash
# Method 1: Systemd (recommended)
sudo systemctl start storage_mgr
sudo systemctl enable storage_mgr  # Auto-start on boot

# Method 2: Manual in foreground (for debugging)
sudo ./bin/storage_daemon -f

# Method 3: Manual in background
sudo ./bin/storage_daemon
```

### CLI Client

#### Monitoring Commands

```bash
# View device stats
sudo storage_cli monitor stats sda

# Start continuous monitoring (every 5 seconds)
sudo storage_cli monitor start 5

# Stop monitoring
sudo storage_cli monitor stop
```

#### Backup Commands

```bash
# Create full backup
sudo storage_cli backup create /data /backup full

# Create incremental backup
sudo storage_cli backup create /data /backup incremental

# List backups
sudo storage_cli backup list

# Restore backup
sudo storage_cli backup restore backup-20250527-143022 /restore

# Verify integrity
sudo storage_cli backup verify backup-20250527-143022
```

#### Performance Commands

```bash
# Run benchmark
sudo storage_cli perf benchmark sda /mnt/data/testfile

# Tune configuration
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048

# Get recommendations
sudo storage_cli perf recommend sda database
```

#### General Commands

```bash
# Check daemon status
storage_cli status

# Help
storage_cli help
```

### Kernel Module

> ⚠️ **Important:** Before running `sudo make install`, you must first compile the module with `sudo make` inside the `kernel_module` folder.

```bash
cd kernel_module
sudo make       # Compile first
sudo make install  # Then install

# View statistics
cat /proc/storage_stats

# Reset statistics
echo "reset" | sudo tee /proc/storage_stats

# Debug on/off
echo "debug on" | sudo tee /proc/storage_stats
echo "debug off" | sudo tee /proc/storage_stats

# View kernel logs
dmesg | grep storage_stats

# Remove module
sudo make uninstall
```

## 🧪 Testing

### Automated Tests

```bash
# Run all tests
sudo make test

# Individual tests
sudo ./bin/test_monitor
sudo ./bin/test_backup
sudo ./bin/test_perf
sudo ./bin/test_ipc
```

### Create Loop Devices for Testing

```bash
# Create disk images
for i in {0..7}; do
    dd if=/dev/zero of=/tmp/disk$i.img bs=1M count=1024
    sudo losetup /dev/loop$i /tmp/disk$i.img
 done

# Verify
losetup -a

# Cleanup
for i in {0..7}; do
    sudo losetup -d /dev/loop$i
    rm /tmp/disk$i.img
 done
```

## 📊 Real Usage Examples

### Scenario 1: Database Server Monitoring

```bash
# 1. Start daemon
sudo systemctl start storage_mgr

# 2. Check current stats
sudo storage_cli monitor stats sda

# 3. Optimize for database workload
sudo storage_cli perf recommend sda database
# Respond 'y' to apply

# 4. Start continuous monitoring
sudo storage_cli monitor start 10

# 5. Create backup with LVM snapshot
sudo storage_cli backup create /var/lib/mysql /backup full
```

### Scenario 2: Automated Backups with Snapshots

```bash
# Daily backup script (/usr/local/bin/daily_backup.sh)
#!/bin/bash
/usr/local/bin/storage_cli backup create /data /backup incremental
/usr/local/bin/storage_cli backup cleanup 7  # Keep last 7

# Add to cron (run at 2 AM)
sudo crontab -e
# Add: 0 2 * * * /usr/local/bin/daily_backup.sh
```

### Scenario 3: Performance Analysis

```bash
# 1. Benchmark before tuning
sudo storage_cli perf benchmark sda /mnt/data/test > before.txt

# 2. Apply optimizations
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048
sudo storage_cli perf tune sda --vm-swappiness=10 --vm-dirty-ratio=15

# 3. Benchmark after
sudo storage_cli perf benchmark sda /mnt/data/test > after.txt

# 4. Compare results
diff before.txt after.txt
```

## 🗄️ Databases

### Monitoring
```bash
# Location
/var/lib/storage_mgr/monitoring.db

# Inspect
sqlite3 /var/lib/storage_mgr/monitoring.db
> SELECT * FROM performance_history ORDER BY timestamp DESC LIMIT 10;
> .schema
> .exit
```

### Backups
```bash
# Location
/var/lib/storage_mgr/backups.db

# Inspect
sqlite3 /var/lib/storage_mgr/backups.db
> SELECT backup_id, timestamp, type, size_bytes FROM backups;
> .exit
```

## 🐛 Troubleshooting

### Daemon Does Not Start

```bash
# Check logs
sudo journalctl -u storage_mgr -f

# Check permissions
ls -la /var/run/storage_mgr.pid
ls -la /var/run/storage_mgr.sock

# Clean and restart
sudo rm /var/run/storage_mgr.* 2>/dev/null
sudo systemctl restart storage_mgr
```

### Kernel Module Fails to Load

```bash
# Check errors
dmesg | tail -20

# Check kernel headers
uname -r
ls /lib/modules/$(uname -r)/build

# Reinstall headers
sudo apt install linux-headers-$(uname -r)

# Recompile
cd kernel_module
sudo make clean && sudo make
sudo make install
```

### Permission Errors

```bash
# Ensure running as root
sudo storage_cli status

# Check directory permissions
sudo ls -la /var/lib/storage_mgr
sudo ls -la /backup

# Recreate with correct permissions
sudo mkdir -p /var/lib/storage_mgr /backup
sudo chmod 755 /var/lib/storage_mgr /backup
```

## 📈 Metrics and Performance

### System Overhead
- CPU: < 1% during normal monitoring
- Memory: ~50 MB
- Disk: ~10 MB (databases)

### Capabilities
- Simultaneous clients: 64
- Monitored devices: 16
- Concurrent backups: 4
- Historical samples: Unlimited (with cleanup)

## 🔒 Security

- **Root required**: All privileged operations
- **Secure IPC**: UNIX domain sockets with 666 permissions
- **Auditable logs**: All operations are logged
- **Integrity**: SHA256 checksums for backups

## 📝 Logs

```bash
# Daemon logs
sudo journalctl -u storage_mgr -f

# Kernel module logs
dmesg | grep storage_stats

# System logs
tail -f /var/log/syslog | grep storage
```
## 📚 References

- [Linux Device Drivers](https://lwn.net/Kernel/LDD3/)
- [Advanced Linux Programming](http://advancedlinuxprogramming.com/)
- [LVM HOWTO](https://tldp.org/HOWTO/LVM-HOWTO/)
- [Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)

## 🤝 Contributions

This project is part of an academic assignment. Parts 6-10 were implemented by Patricio Dávila Assad.

## 📄 License

Academic project - Universidad Autónoma de Guadalajara - 2025

---

**Need help?** Check the Troubleshooting section or run:

```bash
storage_cli help
man storage_cli  # If man pages installed
```

