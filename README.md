# Storage Manager - Enterprise Storage Management System

Sistema completo de gestión de almacenamiento empresarial con monitoreo, backups, optimización de rendimiento, gestión de procesos y automatización.

## 👥 Equipo

- **Patricio Dávila Assad** - Parts 6-10 (Monitoring, Backup, Performance, IPC, Kernel Module)
- **Diego Cristobal Gael Serna Domínguez** - Parts 1-4 (RAID, LVM, Filesystems, Virtual Memory)
- **Angel Valencia Saavedra** - Parts 5, 11-12 (Security, Process Management, Automation)

***

## 📦 Componentes Implementados

### Part 1-4: Core Storage Management (Diego Cristobal Gael Serna Domínguez)
- ✅ RAID Management (RAID 0, 1, 5, 10)
- ✅ LVM Implementation (PV, VG, LV, Snapshots)
- ✅ Filesystem Operations (ext4, xfs, btrfs)
- ✅ Virtual Memory Management

### Part 5: Security Features (Angel Valencia Saavedra)
- ✅ Access Control Lists (ACLs)
- ✅ LUKS Encryption
- ✅ Advanced Permissions (chattr)
- ✅ Audit Logging

### Part 6: Monitoring System (30 pts) - Patricio Dávila Assad
- ✅ I/O monitoring (reads/writes, throughput, latency)
- ✅ Resource tracking (disk usage, inodes)
- ✅ Performance metrics (IOPS, MB/s)
- ✅ Historical data stored in SQLite
- ✅ Continuous background monitoring

### Part 7: Backup System (35 pts) - Patricio Dávila Assad
- ✅ Full, Incremental, and Differential backups
- ✅ LVM snapshots for consistency
- ✅ Integrity verification with SHA256
- ✅ Full and partial restore
- ✅ Backup database
- ✅ Automatic cleanup of old backups

### Part 8: Performance Optimization (20 pts) - Patricio Dávila Assad
- ✅ I/O scheduler management (deadline, cfq, bfq, kyber)
- ✅ Read-ahead and queue depth tuning
- ✅ Kernel VM parameters (swappiness, dirty_ratio)
- ✅ Performance benchmarking
- ✅ Workload-based recommendations

### Part 9: IPC Architecture (25 pts) - Patricio Dávila Assad
- ✅ UNIX domain socket server
- ✅ Multi-client using `select()`
- ✅ Shared memory for system state
- ✅ Message queues for asynchronous jobs
- ✅ Semaphores for synchronization
- ✅ Binary communication protocol

### Part 10: Kernel Module (20 pts) - Patricio Dávila Assad
- ✅ `/proc/storage_stats` interface
- ✅ I/O operations tracking
- ✅ Device-level statistics
- ✅ User-space commands
- ✅ Safe concurrency handling

### Part 11: Process Management (15 pts) - Angel Valencia Saavedra
- ✅ Proper daemonization (double fork)
- ✅ PID file management
- ✅ Signal handling (SIGTERM, SIGHUP, SIGCHLD, SIGUSR1)
- ✅ Worker process management
- ✅ Resource cleanup and zombie prevention

### Part 12: Automation & Scripting (15 pts) - Angel Valencia Saavedra
- ✅ Health check script with alerting
- ✅ Automated backup script with verification
- ✅ Performance reporting script
- ✅ Systemd integration (timers and services)
- ✅ Cron compatibility

***

## 🚀 Instalación Rápida

```bash
# 1. Clonar o descargar el proyecto
cd ~/storage_manager

# 2. Hacer ejecutable el script de setup
chmod +x setup.sh

# 3. Ejecutar instalación completa (requiere root)
sudo ./setup.sh install

# O usar el menú interactivo (si está habilitado)
sudo ./setup.sh
```

***

## 📋 Requisitos del Sistema

### Operating System
- Ubuntu 20.04+ / Debian 10+
- CentOS 8+ / RHEL 8+ / Fedora 33+
- Kernel 4.15+

### Paquetes Requeridos

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential gcc make cmake git \
                    sqlite3 libsqlite3-dev libssl-dev \
                    linux-headers-$(uname -r) rsync lvm2 mdadm \
                    sysstat mailutils

# CentOS/RHEL/Fedora
sudo yum install -y gcc make cmake git sqlite sqlite-devel \
                    openssl-devel kernel-devel rsync lvm2 mdadm \
                    sysstat mailx
```

### Hardware Recomendado
- 2+ GB RAM
- 20+ GB espacio en disco
- CPU con 2+ cores

***

## 🔧 Compilación Manual

```bash
# Compilar todo el proyecto
make all

# Ejecutar tests core
make test-core

# Compilar solo el módulo del kernel
cd kernel_module
make
cd ..

# Limpiar archivos compilados
make clean
```

***

## 📂 Estructura del Proyecto

```text
storage_manager/
├── include/                  # Headers
│   ├── monitor.h
│   ├── backup_engine.h
│   ├── performance_tuner.h
│   ├── ipc_server.h
│   ├── daemon.h
│   ├── security_manager.h
│   ├── raid_manager.h
│   ├── lvm_manager.h
│   ├── filesystem_ops.h
│   └── memory_manager.h
│
├── src/                      # Implementaciones
│   ├── monitor.c
│   ├── backup_engine.c
│   ├── performance_tuner.c
│   ├── ipc_server.c
│   ├── daemon.c
│   ├── daemon_main.c
│   ├── security_manager.c
│   ├── raid_manager.c
│   ├── lvm_manager.c
│   ├── filesystem_ops.c
│   ├── memory_manager.c
│   └── utils.c
│
├── cli/                      # Cliente CLI
│   └── storage_cli.c
│
├── kernel_module/            # Módulo del kernel
│   ├── storage_stats.c
│   └── Makefile
│
├── scripts/                  # Scripts de automatización
│   ├── health_check.sh       # Verificación de salud
│   ├── auto_backup.sh        # Backup automatizado
│   └── perf_report.sh        # Reportes de rendimiento
│
├── systemd/                  # Servicios systemd (opcional)
│   ├── storage_daemon.service
│   ├── storage_backup.service
│   ├── storage_backup.timer
│   ├── storage_health.service
│   └── storage_health.timer
│
├── tests/                    # Tests
│   ├── test_monitor.c
│   ├── test_backup.c
│   ├── test_perf.c
│   ├── test_ipc.c
│   ├── test_daemon.c
│   ├── test_security.c
│   ├── test_raid.c
│   └── test_lvm.c
│
├── docs/                     # Documentación
│   ├── DESIGN.md
│   ├── USER_MANUAL.md
│   └── API_REFERENCE.md
│
├── bin/                      # Binarios (generados)
├── obj/                      # Archivos objeto (generados)
├── Makefile                  # Build system
├── setup.sh                  # Script de instalación
├── quick_test.sh             # Tests rápidos
└── README.md                 # Esta documentación
```

***

## 💻 Uso del Sistema

### 1. Iniciar el Daemon

```bash
# Método 1: Manual (demo/proyecto)
cd ~/storage_manager
sudo ./bin/storage_daemon    # Dejar corriendo en una terminal

# Verificar estado desde otra terminal
./bin/storage_cli status
cat /var/run/storage_mgr.pid
ps aux | grep storage_daemon
```

***

### 2. Cliente CLI - Comandos Más Importantes

#### Comandos generales

```bash
# Ayuda general
./bin/storage_cli help

# Ver estado del daemon y del sistema
./bin/storage_cli status
```

#### Monitoreo

```bash
# Ver estadísticas de un dispositivo (ajusta sda/sda1 según tu VM)
./bin/storage_cli monitor stats sda
# o
./bin/storage_cli monitor stats sda1
```

#### Backup (vía CLI)

```bash
# Listar backups registrados en la base de datos
./bin/storage_cli backup list

# (Opcional, según lo que tengas configurado)
# ./bin/storage_cli backup create /mnt/data /backup full
# ./bin/storage_cli backup verify BACKUP_ID
# ./bin/storage_cli backup restore BACKUP_ID /restore
```

#### Performance

```bash
# Ejecutar benchmark (ajusta el dispositivo)
./bin/storage_cli perf benchmark sda /tmp/perf_test

# Obtener recomendaciones según tipo de carga (por ejemplo database)
./bin/storage_cli perf recommend sda database
```

***

## 3. Módulo del Kernel

```bash
cd ~/storage_manager/kernel_module

# Compilar módulo
make

# Cargar módulo
sudo insmod storage_stats.ko

# Ver estadísticas expuestas por el módulo
cat /proc/storage_stats

# Descargar módulo
sudo rmmod storage_stats
```

Salida típica esperada:

```text
Storage Statistics Module v1.0
================================

No devices tracked yet.
```

***

## 4. Scripts de Automatización

### Health Check

```bash
cd ~/storage_manager

# Ejecutar verificación manual
sudo ./scripts/health_check.sh

# Ver logs
sudo tail -f /var/log/storage_health.log

# Ver reportes generados
ls -lh /var/log/storage_health_report_*.txt
```

### Backup Automatizado

```bash
cd ~/storage_manager

# Preparar datos de prueba
sudo mkdir -p /mnt/data
sudo touch /mnt/data/demo1.txt
sudo touch /mnt/data/demo2.txt

# Modo dry-run (prueba sin ejecutar cambios)
sudo ./scripts/auto_backup.sh --dry-run

# Backup completo de /mnt/data
sudo ./scripts/auto_backup.sh --full /mnt/data

# Listar backups
sudo ./scripts/auto_backup.sh --list

# (Opcional) Verificar/Restaurar usando rutas reales que te muestre --list
# sudo ./scripts/auto_backup.sh --verify /backup/full/NOMBRE.tar.gz
# sudo ./scripts/auto_backup.sh --restore /backup/full/NOMBRE.tar.gz /restore
```

### Reporte de Rendimiento

```bash
cd ~/storage_manager

# Generar reporte automático
sudo ./scripts/perf_report.sh

# Ver reporte
ls -lh /var/log/performance_report_*.txt
```

***

## 5. Systemd - Automatización (Opcional)

Si decides usar systemd en lugar de lanzarlo a mano:

```bash
# Habilitar servicios
sudo systemctl enable storage_daemon
sudo systemctl enable storage_backup.timer
sudo systemctl enable storage_health.timer

# Iniciar servicios
sudo systemctl start storage_daemon
sudo systemctl start storage_backup.timer
sudo systemctl start storage_health.timer

# Ver estado
sudo systemctl status storage_daemon
sudo systemctl list-timers storage_*

# Ver logs
sudo journalctl -u storage_daemon -f
sudo journalctl -u storage_backup.service -n 50
sudo journalctl -u storage_health.service -n 50
```

***

## 📊 Escenarios de Uso Real

### Escenario 1: Verificar instalación y core

```bash
cd ~/storage_manager
sudo make all
sudo make test-core

sudo ./bin/storage_daemon          # terminal 1

# terminal 2
./bin/storage_cli status
./bin/storage_cli memory status
./bin/storage_cli monitor stats sda
```

### Escenario 2: Health check + backup script

```bash
cd ~/storage_manager

sudo mkdir -p /mnt/data
sudo touch /mnt/data/demo1.txt

sudo ./scripts/auto_backup.sh --full /mnt/data
sudo ./scripts/auto_backup.sh --list

sudo ./scripts/health_check.sh
sudo tail -n 20 /var/log/storage_health.log
```

### Escenario 3: Kernel module

```bash
cd ~/storage_manager/kernel_module
make
sudo insmod storage_stats.ko
cat /proc/storage_stats
sudo rmmod storage_stats
```

***

## 🧪 Testing

```bash
cd ~/storage_manager

# Ejecutar tests core del proyecto
sudo make test-core

# Test rápido (si está configurado)
sudo ./quick_test.sh
```

***

## 🗄️ Logs y Directorios Importantes

```text
/var/log/
  ├── storage_health.log              # Health checks
  ├── storage_backup.log              # Backups
  ├── storage_health_report_*.txt     # Reportes de salud
  └── performance_report_*.txt        # Reportes de rendimiento

/var/run/
  └── storage_mgr.pid                 # PID del daemon

/backup/
  ├── full/                           # Backups completos
  ├── incremental/                    # Backups incrementales
  ├── snapshots/                      # Snapshots LVM
  └── logs/                           # Logs de backup
```

***

## 🤝 Contribuciones

Este proyecto es parte de un trabajo académico del curso de Sistemas Operativos.

**Distribución de Trabajo:**
- **Patricio Dávila Assad:** Parts 6-10
- **Diego Cristobal Gael Serna Domínguez:** Parts 1-4
- **Angel Valencia Saavedra:** Parts 5, 11-12

***

## 📄 Licencia

Proyecto Académico - Universidad Autónoma de Guadalajara - 2025  
Uso exclusivo para fines educativos.

***

## 📞 Soporte y Ayuda

**Comandos de Diagnóstico:**

```bash
# Estado completo del sistema
sudo ./scripts/health_check.sh
sudo ./scripts/perf_report.sh
./bin/storage_cli status

# Estado del daemon
ps aux | grep storage_daemon
cat /var/run/storage_mgr.pid

# Verificar instalación básica
ls -l bin/
ls -l scripts/
ls -l docs/

# Ver logs
sudo journalctl -u storage_daemon -n 50
sudo tail -n 50 /var/log/storage_health.log
sudo tail -n 50 /var/log/storage_backup.log
```

**Flujo recomendado para demo:**

1. `sudo make all && sudo make test-core`  
2. `sudo ./bin/storage_daemon` (dejar corriendo)  
3. `./bin/storage_cli status`  
4. `./bin/storage_cli monitor stats sda`  
5. `sudo ./scripts/auto_backup.sh --full /mnt/data && --list`  
6. `sudo ./scripts/health_check.sh`  
7. Kernel module: `make`, `insmod`, `cat /proc/storage_stats`, `rmmod`.
