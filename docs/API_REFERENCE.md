# Storage Manager – API Reference

## 1. Overview
This document describes the main public interfaces of the C modules. Exact prototypes are available inside the `include/` directory headers.

---

## 2. RAID Manager
### Core Structure:
A RAID array object contains:
- Array name (e.g., `/dev/md0`)
- RAID level (`0`, `1`, `5`, `10`)
- Number of devices
- Device list (`/dev/loopX`, `/dev/sdX`)
- Current state (`active`, `degraded`, `failed`)
- Failed disk counter

### Supported Operations:
- Create RAID array
- Query and populate array status
- Mark/Add/Remove/Stop disks

### Return Convention:
- `0` = success, negative value = logical/system failure

---

## 3. LVM Manager
### Public Functions Support:
- Create physical volume (`PV`)
- Create volume group (`VG`) from PV(s)
- Create logical volume (`LV`) (size in MB)
- Extend LV and sync FS size

### Return Convention:
- `0` = success, negative = tool/system execution error

---

## 4. Filesystem Operations
### Main Capabilities:
- Create filesystem (`ext4`, `xfs`, `btrfs`) on a device with optional label
- Mount/Unmount with configurable flags
- Integrity repair (`fsck`, `xfs_repair`, etc.)
- Filesystem resize after volume extension

### Return Convention:
- `0` = success, negative = tool/system execution error

---

## 5. Memory Manager
### Core Structure:
Memory metrics include:
- Total/Free/Available/RAM Cache/Buffers
- Swap total and available
- Memory pressure factor (`0.0` to `1.0`)

### Public Function:
- Populate structure from `/proc/meminfo`

---

## 6. Monitor Module
### Core Structure:
Device statistics include:
- Device name
- Read/write operations counter
- Bytes transferred
- Average read/write latency (ms)
- Queue depth

### Public Function:
- Populate statistics from `/sys/` and `/proc/`

---

## 7. Backup Engine
### Core Structure:
Each backup object includes:
- Backup ID (string)
- Timestamp
- Type (`full`, `incremental`, `differential`)
- Source and destination path
- Total size (bytes)
- SHA256 Checksum
- Backup success indicator

### Public Functions:
- `backup_create() → string ID`
- `backup_verify()`
- `backup_restore()`
- `backup_list()`
- `backup_cleanup_old()` (limit by N backups)

---

## 8. Performance Tuner
### Supported Load Types:
- `database`, `web_server`, `file_server`, `general`

### Core Profile Structure:
Tuning suggestions include:
- I/O Scheduler
- Readhead size (KB)
- Queue depth
- Kernel parameters (`vm.swappiness`, `vm.dirty_ratio`)

### Public Functions:
- Benchmark execution, report generation
- Populate recommended tuning profile
- Apply tuning profile to a real device

---

## 9. IPC Server
### Main Workflow:
- Initialize UNIX socket
- Main select loop for multi-client handling
- Decode client binary request
- Use module dispatchers to compose binary response

---

## 10. Error Style Convention
- `0` = success
- Negative = logical/system error
- Errors translated to human-readable messages by CLI and daemon layers

---
