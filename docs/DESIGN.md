# Storage Manager – System Design

## 1. System Architecture
Storage Manager uses a modular client-server architecture.

### Main Components:
- **`storage_daemon`** — A persistent background process that centralizes and coordinates storage operations:
  - RAID, LVM, filesystems, memory, monitoring, security, backup orchestration, and performance tuning
- **`storage_cli`** — A lightweight, user triggered client program that sends commands via IPC or calls fast local module functions when appropriate
- The codebase is organized into C modules based on responsibility:
  - RAID → `raid_manager.c / .h`
  - LVM → `lvm_manager.c / .h`
  - Filesystems → `filesystem_ops.c / .h`
  - Memory → `memory_manager.c / .h`
  - Security → `security_manager.c / .h`
  - Monitoring → `monitor.c / .h`
  - Backups → `backup_engine.c / .h`
  - Performance Tuning → `performance_tuner.c / .h`
  - Daemon & IPC → `daemon.c`, `daemon_main.c`, `ipc_server.c`
- **Kernel module (`kernel_module/storage_stats.c`)**
  - Exposes live I/O statistics in `/proc/storage_stats`
- **Automation Scripts**
  - `health_check.sh`, `auto_backup.sh`, `perf_report.sh`
- **Service Management**
  - Systemd services and timers handle:
    - Daemon startup
    - Scheduled backups and health checks

---

## 2. Component Interaction
### CLI client behavior:
- Reads quick metrics using direct C module calls
- Sends complex operations to daemon through Unix IPC socket (`storage_mgr.sock`):
  1. Connect
  2. Send binary request structure
  3. Wait and decode binary response

### Daemon behavior:
- Runs in the background
- Uses `select()` to handle multiple clients concurrently
- Creates worker processes for long running tasks to avoid blocking
- Dispatches request types to the proper module: RAID, LVM, backup, monitor, etc.

### Kernel module behavior:
- Maintains internal counters of I/O activity
- Generates human readable summaries when `/proc/storage_stats` is read via `cat`

### Script interaction:
- Uses native system tools: `mdadm`, `df`, `vgs`, `lvs`, `mkfs`, `tar`, `rsync`, `dmesg`, `ps`
- Produces automated reports and checksum validated backups

---

## 3. Data Structure Summary (Design Stage)
Designed structures include:

### RAID structure contains:
- Array device path, RAID level, disk list, state, failed disk counter

### Monitor structure contains:
- Device name, read/write operations, transferred bytes, latency averages, queue depth

### Filesystem structure contains:
- Device, mountpoint, type, mount options, size/availability metrics

### IPC Request structure contains:
- Protocol version, request ID, command type, payload size, binary buffer

### IPC Response structure contains:
- Request ID, status code (`0` success, `<0` error), error message if any, binary buffer

---

## 4. Design Decisions
✅ Code divided into modules for readability and testability  
✅ `daemon + CLI` split to avoid a monolithic single binary  
✅ Reuses native Linux storage tools instead of reinventing low-level logic  
✅ `SQLite` used for backup history catalog (easy binary integration)

---

## 5. Trade-offs
⚠ One monolithic daemon simplifies academic development, but reduces real scalability  
⚠ Binary protocol is efficient but not human inspectable like JSON  
⚠ Some monitoring state is ephemeral in memory, not persisted, for implementation simplicity  

---
