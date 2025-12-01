# Storage Manager - Enterprise Storage Management System

Sistema completo de gestión de almacenamiento empresarial con monitoreo, backups, optimización de rendimiento, gestión de procesos y automatización.

## 👥 Equipo

- **Patricio Dávila Assad** - Parts 6-10 (Monitoring, Backup, Performance, IPC, Kernel Module)
- **Diego Cristobal Gael Serna Domínguez** - Parts 1-4 (RAID, LVM, Filesystems, Virtual Memory)
- **Angel Valencia Saavedra** - Parts 5, 11-12 (Security, Process Management, Automation)

---

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

---

## 🚀 Instalación Rápida

```bash
# 1. Clonar o descargar el proyecto
cd ~/storage_manager

# 2. Hacer ejecutable el script de setup
chmod +x setup.sh

# 3. Ejecutar instalación completa (requiere root)
sudo ./setup.sh install

# O usar el menú interactivo
sudo ./setup.sh
```

---

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

---

## 🔧 Compilación Manual

```bash
# Compilar todo el proyecto
make all

# Compilar solo el módulo del kernel
make kernel

# Instalar scripts de automatización
sudo make install-automation

# Instalar servicios systemd
sudo make install-systemd

# Ejecutar tests
make test

# Limpiar archivos compilados
make clean
```

---

## 📂 Estructura del Proyecto

```
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
├── systemd/                  # Servicios systemd
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

---

## 💻 Uso del Sistema

### 1. Iniciar el Daemon

```bash
# Método 1: Systemd (recomendado)
sudo systemctl start storage_daemon
sudo systemctl enable storage_daemon  # Auto-inicio

# Método 2: Manual en foreground (debugging)
sudo ./bin/storage_daemon -f

# Método 3: Manual en background
sudo ./bin/storage_daemon

# Verificar estado
sudo systemctl status storage_daemon
ps aux | grep storage_daemon
cat /var/run/storage_mgr.pid
```

### 2. Cliente CLI - Comandos Disponibles

#### Comandos de Monitoreo

```bash
# Ver estadísticas de dispositivo
sudo storage_cli monitor stats sda

# Iniciar monitoreo continuo (cada 5 segundos)
sudo storage_cli monitor start 5

# Detener monitoreo
sudo storage_cli monitor stop
```

#### Comandos de Backup

```bash
# Crear backup completo
sudo storage_cli backup create /data /backup full

# Crear backup incremental
sudo storage_cli backup create /data /backup incremental

# Listar backups disponibles
sudo storage_cli backup list

# Restaurar backup
sudo storage_cli backup restore backup-20250527-143022 /restore

# Verificar integridad de backup
sudo storage_cli backup verify backup-20250527-143022
```

#### Comandos de Performance

```bash
# Ejecutar benchmark
sudo storage_cli perf benchmark sda /mnt/data/testfile

# Ajustar configuración
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048

# Obtener recomendaciones por workload
sudo storage_cli perf recommend sda database
# Workloads disponibles: database, web, fileserver, general
```

#### Comandos Generales

```bash
# Ver estado del daemon
storage_cli status

# Ayuda completa
storage_cli help
```

### 3. Módulo del Kernel

```bash
# Cargar módulo
cd kernel_module
sudo make          # Compilar primero
sudo make install  # Luego instalar

# Ver estadísticas
cat /proc/storage_stats

# Resetear estadísticas
echo "reset" | sudo tee /proc/storage_stats

# Habilitar/deshabilitar debug
echo "debug on" | sudo tee /proc/storage_stats
echo "debug off" | sudo tee /proc/storage_stats

# Ver logs del kernel
dmesg | grep storage_stats

# Descargar módulo
sudo make uninstall
```

### 4. Scripts de Automatización

#### Health Check

```bash
# Ejecutar verificación manual
sudo health_check.sh

# Ver logs
sudo tail -f /var/log/storage_health.log

# Ver reportes generados
cat /var/log/storage_health_report_*.txt
```

#### Backup Automatizado

```bash
# Backup completo
sudo auto_backup.sh --full /mnt/data

# Backup incremental
sudo auto_backup.sh --incremental /mnt/data

# Listar backups
sudo auto_backup.sh --list

# Verificar backup
sudo auto_backup.sh --verify /backup/full/full_20250101.tar.gz

# Restaurar backup
sudo auto_backup.sh --restore /backup/full/full_20250101.tar.gz /restore

# Limpiar backups antiguos (mantener últimos 7)
sudo auto_backup.sh --cleanup --keep 7

# Modo dry-run (prueba sin ejecutar)
sudo auto_backup.sh --dry-run

# Ver ayuda
sudo auto_backup.sh --help
```

#### Reporte de Rendimiento

```bash
# Generar reporte automático
sudo perf_report.sh

# Generar con nombre específico
sudo perf_report.sh --output /tmp/mi_reporte.txt

# Ver reporte
cat /var/log/performance_report_*.txt
```

### 5. Systemd - Automatización

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

# Detener servicios
sudo systemctl stop storage_backup.timer
sudo systemctl stop storage_health.timer

# Deshabilitar
sudo systemctl disable storage_backup.timer
```

### 6. Configuración con Cron (Alternativa)

```bash
# Editar crontab
sudo crontab -e

# Agregar:
# Health check cada hora
0 * * * * /usr/local/bin/health_check.sh

# Backup completo diario a las 2 AM
0 2 * * * /usr/local/bin/auto_backup.sh --full /mnt/data

# Backup incremental cada 6 horas
0 */6 * * * /usr/local/bin/auto_backup.sh --incremental /mnt/data

# Reporte semanal los lunes a las 8 AM
0 8 * * 1 /usr/local/bin/perf_report.sh

# Limpieza mensual (día 1 a las 3 AM)
0 3 1 * * /usr/local/bin/auto_backup.sh --cleanup --keep 10
```

---

## 📊 Escenarios de Uso Real

### Escenario 1: Servidor de Base de Datos

```bash
# 1. Iniciar daemon
sudo systemctl start storage_daemon

# 2. Ver estadísticas actuales
sudo storage_cli monitor stats sda

# 3. Optimizar para workload de base de datos
sudo storage_cli perf recommend sda database
# Responder 'y' para aplicar optimizaciones

# 4. Iniciar monitoreo continuo
sudo storage_cli monitor start 10

# 5. Configurar backup automático con snapshot LVM
sudo auto_backup.sh --full /var/lib/mysql
sudo systemctl enable storage_backup.timer
```

### Escenario 2: Backup Automático Diario

```bash
# Script diario (/usr/local/bin/daily_backup.sh)
#!/bin/bash
/usr/local/bin/auto_backup.sh --incremental /data
/usr/local/bin/auto_backup.sh --cleanup --keep 7

# Hacer ejecutable
sudo chmod +x /usr/local/bin/daily_backup.sh

# Agregar a cron (2 AM)
sudo crontab -e
# Agregar: 0 2 * * * /usr/local/bin/daily_backup.sh
```

### Escenario 3: Análisis de Performance

```bash
# 1. Benchmark antes de optimizar
sudo storage_cli perf benchmark sda /mnt/data/test > before.txt

# 2. Aplicar optimizaciones
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048

# 3. Benchmark después
sudo storage_cli perf benchmark sda /mnt/data/test > after.txt

# 4. Comparar resultados
diff before.txt after.txt
```

### Escenario 4: Monitoreo y Alertas

```bash
# 1. Habilitar health check automático
sudo systemctl enable storage_health.timer
sudo systemctl start storage_health.timer

# 2. Configurar alertas por email
sudo nano /usr/local/bin/health_check.sh
# Editar: EMAIL_ALERT="admin@empresa.com"

# 3. Ver próximas ejecuciones
sudo systemctl list-timers storage_health.timer

# 4. Ver reportes generados
ls -lh /var/log/storage_health_report_*.txt
```

---

## 🧪 Testing

### Tests Automatizados

```bash
# Ejecutar todos los tests
sudo make test

# Tests individuales
sudo ./bin/test_monitor
sudo ./bin/test_backup
sudo ./bin/test_perf
sudo ./bin/test_ipc
sudo ./bin/test_daemon
sudo ./bin/test_security

# Test rápido completo
sudo ./quick_test.sh
```

### Crear Dispositivos Loop para Testing

```bash
# Crear imágenes de disco (1GB cada una)
for i in {0..7}; do
    dd if=/dev/zero of=/tmp/disk$i.img bs=1M count=1024
    sudo losetup /dev/loop$i /tmp/disk$i.img
done

# Verificar
losetup -a

# Limpiar después
for i in {0..7}; do
    sudo losetup -d /dev/loop$i
    rm /tmp/disk$i.img
done
```

### Test de Integración Completo

```bash
# 1. Crear datos de prueba
sudo mkdir -p /mnt/test_data
sudo dd if=/dev/urandom of=/mnt/test_data/file1.bin bs=1M count=50

# 2. Backup completo
sudo auto_backup.sh --full /mnt/test_data

# 3. Modificar datos
sudo dd if=/dev/urandom of=/mnt/test_data/file2.bin bs=1M count=25

# 4. Backup incremental
sudo auto_backup.sh --incremental /mnt/test_data

# 5. Simular pérdida de datos
sudo rm -rf /mnt/test_data/*

# 6. Restaurar
LATEST_BACKUP=$(ls -t /backup/full/*.tar.gz | head -1)
sudo auto_backup.sh --restore $LATEST_BACKUP /mnt/test_data

# 7. Verificar
ls -lh /mnt/test_data/

# 8. Health check
sudo health_check.sh

# 9. Generar reporte
sudo perf_report.sh
```

---

## 🗄️ Bases de Datos y Logs

### Bases de Datos

```bash
# Monitoring DB
sqlite3 /var/lib/storage_mgr/monitoring.db
> SELECT * FROM performance_history ORDER BY timestamp DESC LIMIT 10;
> .schema
> .exit

# Backups DB
sqlite3 /var/lib/storage_mgr/backups.db
> SELECT backup_id, timestamp, type, size_bytes FROM backups;
> .exit
```

### Logs del Sistema

```bash
# Ubicación de logs
/var/log/
├── storage_health.log                    # Health checks
├── storage_backup.log                    # Backups
├── storage_health_report_*.txt           # Reportes de salud
└── performance_report_*.txt              # Reportes de rendimiento

/var/run/
└── storage_mgr.pid                       # PID del daemon

/backup/
├── full/                                 # Backups completos
├── incremental/                          # Backups incrementales
├── snapshots/                            # Snapshots LVM
└── logs/                                 # Logs de backup
```

### Ver Logs

```bash
# Daemon logs
sudo journalctl -u storage_daemon -f

# Health check logs
sudo tail -f /var/log/storage_health.log

# Backup logs
sudo tail -f /var/log/storage_backup.log

# Kernel module logs
dmesg | grep storage_stats

# System logs
tail -f /var/log/syslog | grep storage
```

---

## 🐛 Troubleshooting

### Problema: Daemon no inicia

```bash
# Verificar si ya está corriendo
ps aux | grep storage_daemon

# Ver logs de error
sudo journalctl -u storage_daemon -n 50

# Limpiar PID file obsoleto
sudo rm /var/run/storage_mgr.pid

# Reintentar
sudo systemctl restart storage_daemon
```

### Problema: Módulo del kernel no carga

```bash
# Ver errores
dmesg | tail -20

# Verificar headers del kernel
uname -r
ls /lib/modules/$(uname -r)/build

# Reinstalar headers
sudo apt install linux-headers-$(uname -r)

# Recompilar
cd kernel_module
sudo make clean && sudo make
sudo make install
```

### Problema: Errores de permisos

```bash
# Verificar ejecución como root
sudo storage_cli status

# Verificar permisos de directorios
sudo ls -la /var/lib/storage_mgr
sudo ls -la /backup

# Recrear directorios
sudo mkdir -p /var/lib/storage_mgr /backup
sudo chmod 755 /var/lib/storage_mgr /backup
```

### Problema: Backup falla

```bash
# Verificar espacio en disco
df -h /backup

# Verificar permisos
ls -ld /backup

# Crear directorios faltantes
sudo mkdir -p /backup/{full,incremental,snapshots,logs}

# Ver log de error
sudo tail -50 /var/log/storage_backup.log

# Ejecutar en modo debug
sudo bash -x /usr/local/bin/auto_backup.sh --full /mnt/data
```

### Problema: Timers no se ejecutan

```bash
# Verificar que están habilitados
sudo systemctl list-timers storage_*

# Habilitar si es necesario
sudo systemctl enable storage_backup.timer
sudo systemctl start storage_backup.timer

# Ver próxima ejecución
sudo systemctl list-timers --all
```

---

## 📈 Métricas y Rendimiento

### System Overhead
- **CPU:** < 1% en monitoreo normal
- **Memoria:** ~50 MB
- **Disco:** ~10 MB (bases de datos)

### Capacidades
- **Clientes simultáneos:** 64
- **Dispositivos monitoreados:** 16
- **Backups concurrentes:** 4
- **Samples históricos:** Ilimitados (con cleanup)

### Performance Tips

**Para Backups:**
- Usar compresión para backups completos
- Backups incrementales para cambios frecuentes
- Programar backups en horas de bajo uso
- Verificar integridad después de cada backup

**Para Monitoreo:**
- Ajustar intervalos según carga del sistema
- Usar `nice`/`ionice` para procesos de background
- Limpiar datos históricos antiguos regularmente

---

## 🔒 Seguridad

### Características de Seguridad

- **Root requerido:** Todas las operaciones privilegiadas
- **IPC seguro:** UNIX domain sockets con permisos 666
- **Logs auditables:** Todas las operaciones registradas
- **Integridad:** Checksums SHA256 en backups
- **Encriptación:** Soporte LUKS para volúmenes

### Permisos Recomendados

```bash
# Scripts (solo root)
sudo chmod 700 /usr/local/bin/*.sh

# Logs (solo root)
sudo chmod 600 /var/log/storage_*.log

# Directorio de backups
sudo chmod 700 /backup
```

### Audit Trail

```bash
# Ver audit trail
sudo grep "\[INFO\]\|\[ERROR\]\|\[WARN\]" /var/log/storage_health.log
```

---

## 📚 Referencias

- [Linux Device Drivers](https://lwn.net/Kernel/LDD3/)
- [Advanced Linux Programming](http://advancedlinuxprogramming.com/)
- [LVM HOWTO](https://tldp.org/HOWTO/LVM-HOWTO/)
- [Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
- [Linux RAID Wiki](https://raid.wiki.kernel.org/)
- [Systemd Documentation](https://www.freedesktop.org/wiki/Software/systemd/)

---

## 🤝 Contribuciones

Este proyecto es parte de un trabajo académico del curso de Sistemas Operativos.

**Distribución de Trabajo:**
- **Patricio Dávila Assad:** Parts 6-10
- **Diego Cristobal Gael Serna Domínguez:** Parts 1-4
- **Angel Valencia Saavedra:** Parts 5, 11-12

---

## 📄 Licencia

Proyecto Académico - Universidad Autónoma de Guadalajara - 2025

Uso exclusivo para fines educativos.

---

## 📞 Soporte y Ayuda

**Comandos de Diagnóstico:**

```bash
# Estado completo del sistema
sudo health_check.sh
sudo perf_report.sh
sudo storage_cli status

# Estado del daemon
sudo systemctl status storage_daemon
ps aux | grep storage_daemon

# Verificar instalación
which storage_daemon health_check.sh auto_backup.sh

# Ver todos los logs
sudo journalctl -u storage_daemon -u storage_backup.service -u storage_health.service --since today
```

**¿Necesitas ayuda?**

1. Revisa la sección de Troubleshooting
2. Ejecuta: `storage_cli help`
3. Revisa los logs en `/var/log/`
4. Consulta la documentación en `docs/`

---

**Última Actualización:** Noviembre 30, 2025  
**Versión:** 1.0  
**Universidad Autónoma de Guadalajara**  
**Curso:** Linux Systems Programming
