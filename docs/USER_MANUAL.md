# Storage Manager – User Manual

## 1. Introduction
Storage Manager is a Linux storage administration system designed to mimic real-world tools used by system administrators. The system integrates:

- RAID, LVM, and filesystem management
- I/O and resource monitoring
- Automated backup system (full, incremental, differential)
- Disk performance benchmarking and tuning
- Volume and directory security tools
- Central storage daemon and CLI client
- Kernel module for advanced storage statistics

## 2. Installation and Requirements
### System Requirements:
- Linux distribution (Ubuntu, Debian recommended)
- Recommended packages:
  - Build tools and C compiler (`gcc`, `make`)
  - `sqlite3`
  - `mdadm`
  - `lvm2`
  - `rsync`
  - `mailutils`
  - Kernel headers (`linux-headers-$(uname -r)`)

### Compilation:
```bash
cd ~/storage_manager
make all
```

### Installation Script:
```bash
chmod +x setup.sh
sudo ./setup.sh install
```
The script builds the system, installs binaries, prepares log/backup directories, and registers systemd services if configured.

## 3. Running the Daemon
### Start Manually:
```bash
sudo ./bin/storage_daemon
```

### Verify Execution:
```bash
./bin/storage_cli status
cat /var/run/storage_mgr.pid
ps aux | grep storage_daemon
```

## 4. Main CLI Commands
### General:
```bash
./bin/storage_cli help
./bin/storage_cli status
```

### Monitoring:
```bash
./bin/storage_cli monitor stats sda
```

### Backup via CLI:
```bash
sudo ./bin/storage_cli backup create /mnt/data /backup full
sudo ./bin/storage_cli backup create /mnt/data /backup incremental
./bin/storage_cli backup list
./bin/storage_cli backup verify BACKUP_ID
sudo ./bin/storage_cli backup restore BACKUP_ID /restore/path
```

### Performance:
```bash
./bin/storage_cli perf benchmark sda /tmp/perf_test
./bin/storage_cli perf recommend sda database
sudo ./bin/storage_cli perf tune sda --scheduler=deadline --readahead=2048
```

### LVM and Filesystems (Loop device test example):
```bash
sudo ./bin/storage_cli lvm pv-create /dev/loop0
sudo ./bin/storage_cli lvm vg-create vg0 /dev/loop0
sudo ./bin/storage_cli lvm lv-create vg0 data 512
sudo ./bin/storage_cli fs create /dev/vg0/data ext4 --label=data_vol
sudo ./bin/storage_cli fs mount /dev/vg0/data /mnt/data ext4
```

## 5. Automation Scripts
### Health Check:
```bash
sudo ./scripts/health_check.sh
sudo tail -n 50 /var/log/storage_health.log
ls -lh /var/log/storage_health_report_*.txt
```

### Auto Backup:
```bash
sudo ./scripts/auto_backup.sh --dry-run
sudo ./scripts/auto_backup.sh --full /mnt/data
sudo ./scripts/auto_backup.sh --incremental /mnt/data
sudo ./scripts/auto_backup.sh --list
sudo ./scripts/auto_backup.sh --verify /backup/full/NAME.tar.gz
sudo ./scripts/auto_backup.sh --restore /backup/full/NAME.tar.gz /restore/path
```

## 6. Troubleshooting
### Daemon fails to start:
- Check if already running: `ps aux`
- Remove stale PID: `sudo rm -f /var/run/storage_mgr.pid`
- View service logs if systemd is used:
  ```bash
  sudo journalctl -u storage_daemon
  ```

### Backup failures:
- Check available space: `df -h /backup`
- Review log: `sudo cat /var/log/storage_backup.log`

### Health check errors:
- Open latest system report:
  ```bash
  sudo less /var/log/storage_health_report_*.txt
  ```

## 7. FAQ
**Q: Do I need root privileges?**  
A: Yes, for all disk, volume, filesystem, and backup operations.

**Q: Can I delete the logs?**  
A: Yes. They will be recreated during the next system run. History will be lost.

**Q: Where are backups stored?**  
A: Default directories include `/backup/full`, `/backup/incremental`, and checksum/log files within `/backup`.

---
