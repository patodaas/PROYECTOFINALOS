# Storage Manager - Partes 6-10

Sistema de gestión de almacenamiento empresarial implementando monitoreo, backups, optimización de rendimiento, arquitectura IPC y módulo del kernel.

## 👥 Equipo

- Patricio Dávila Assad - Partes 6-10
- Diego Cristobal Gael Serna Domínguez - Partes 1-4
- Angel Valencia Saavedra - Partes 5,11-12

## 📦 Componentes Implementados

### Parte 6: Sistema de Monitoreo (30 pts)
- ✅ Monitoreo de I/O (reads/writes, throughput, latency)
- ✅ Tracking de recursos (disk usage, inodes)
- ✅ Métricas de rendimiento (IOPS, MB/s)
- ✅ Datos históricos con SQLite
- ✅ Monitoreo continuo en background

### Parte 7: Sistema de Backups (35 pts)
- ✅ Backups Full, Incremental y Differential
- ✅ Snapshots LVM para consistencia
- ✅ Verificación de integridad
- ✅ Restauración completa y parcial
- ✅ Base de datos de backups
- ✅ Limpieza automática de backups antiguos

### Parte 8: Optimización de Rendimiento (20 pts)
- ✅ Gestión de I/O schedulers (deadline, cfq, bfq, kyber)
- ✅ Ajuste de read-ahead y queue depth
- ✅ Parámetros VM del kernel (swappiness, dirty_ratio)
- ✅ Benchmarking de rendimiento
- ✅ Recomendaciones por tipo de workload

### Parte 9: Arquitectura IPC (25 pts)
- ✅ Servidor UNIX domain sockets
- ✅ Multi-cliente con select()
- ✅ Shared memory para estado del sistema
- ✅ Message queues para jobs asíncronos
- ✅ Semáforos para sincronización
- ✅ Protocolo binario de comunicación

### Parte 10: Módulo del Kernel (20 pts)
- ✅ Interfaz /proc/storage_stats
- ✅ Tracking de operaciones I/O
- ✅ Estadísticas por dispositivo
- ✅ Comandos desde user-space
- ✅ Manejo seguro de concurrencia

## 🚀 Instalación Rápida

```bash
# 1. Clonar o descargar el proyecto
cd ~/storage_manager

# 2. Hacer ejecutable el script de setup
chmod +x setup.sh

# 3. Ejecutar instalación completa (requiere root)
sudo ./setup.sh install

# O seguir el menú interactivo
sudo ./setup.sh
```

## 📋 Requisitos

### Sistema Operativo
- Ubuntu 20.04+ / Debian 10+
- CentOS 8+ / RHEL 8+ / Fedora 33+
- Kernel 4.15+

### Paquetes Necesarios
```bash
# Ubuntu/Debian
sudo apt install build-essential gcc make cmake git \
                 sqlite3 libsqlite3-dev libssl-dev \
                 linux-headers-$(uname -r) rsync lvm2 mdadm

# CentOS/RHEL/Fedora
sudo yum install gcc make cmake git sqlite sqlite-devel \
                 openssl-devel kernel-devel rsync lvm2 mdadm
```

### Hardware Recomendado
- 2+ GB RAM
- 20+ GB espacio en disco
- CPU con 2+ cores

## 🔧 Compilación Manual

```bash
# Compilar todo
make all

# Compilar solo el módulo del kernel
make kernel

# Ejecutar tests
make test

# Limpiar
make clean
```

## 📂 Estructura del Proyecto

```
storage_manager/
├── include/              # Headers
│   ├── monitor.h
│   ├── backup_engine.h
│   ├── performance_tuner.h
│   └── ipc_server.h
├── src/                  # Implementaciones
│   ├── monitor.c
│   ├── backup_engine.c
│   ├── performance_tuner.c
│   ├── ipc_server.c
│   ├── daemon_main.c
│   └── utils.c
├── cli/                  # Cliente CLI
│   └── storage_cli.c
├── kernel_module/        # Módulo del kernel
│   ├── storage_stats.c
│   └── Makefile
├── tests/                # Tests
│   ├── test_monitor.c
│   ├── test_backup.c
│   ├── test_perf.c
│   └── test_ipc.c
├── Makefile             # Build system
├── setup.sh             # Script de instalación
└── README.md            # Esta documentación
```

## 💻 Uso

### Iniciar el Daemon

```bash
# Método 1: Systemd (recomendado)
sudo systemctl start storage_mgr
sudo systemctl enable storage_mgr  # Auto-inicio

# Método 2: Manual en foreground (para debugging)
sudo ./bin/storage_daemon -f

# Método 3: Manual en background
sudo ./bin/storage_daemon
```

### Cliente CLI

#### Comandos de Monitoreo

```bash
# Ver estadísticas de un dispositivo
sudo storage_cli monitor stats sda

# Iniciar monitoreo continuo (cada 5 segundos)
sudo storage_cli monitor start 5

# Detener monitoreo
sudo storage_cli monitor stop
```

#### Comandos de Backup

```bash
# Crear backup full
sudo storage_cli backup create /data /backup full

# Crear backup incremental
sudo storage_cli backup create /data /backup incremental

# Listar backups
sudo storage_cli backup list

# Restaurar backup
sudo storage_cli backup restore backup-20250527-143022 /restore

# Verificar integridad
sudo storage_cli backup verify backup-20250527-143022
```

#### Comandos de Performance

```bash
# Ejecutar benchmark
sudo storage_cli perf benchmark sda /mnt/data/testfile

# Ajustar configuración
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048

# Obtener recomendaciones
sudo storage_cli perf recommend sda database
```

#### Comandos Generales

```bash
# Ver estado del daemon
storage_cli status

# Ayuda
storage_cli help
```

### Módulo del Kernel

```bash
# Cargar módulo
cd kernel_module
sudo make install

# Ver estadísticas
cat /proc/storage_stats

# Resetear estadísticas
echo "reset" | sudo tee /proc/storage_stats

# Debug on/off
echo "debug on" | sudo tee /proc/storage_stats
echo "debug off" | sudo tee /proc/storage_stats

# Ver logs del kernel
dmesg | grep storage_stats

# Descargar módulo
sudo make uninstall
```

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
```

### Crear Dispositivos Loop para Testing

```bash
# Crear imágenes de disco
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

## 📊 Ejemplos de Uso Real

### Escenario 1: Monitoreo de Servidor de Base de Datos

```bash
# 1. Iniciar daemon
sudo systemctl start storage_mgr

# 2. Ver estadísticas actuales
sudo storage_cli monitor stats sda

# 3. Optimizar para workload de base de datos
sudo storage_cli perf recommend sda database
# Responder 'y' para aplicar

# 4. Iniciar monitoreo continuo
sudo storage_cli monitor start 10

# 5. Crear backup con snapshot LVM
sudo storage_cli backup create /var/lib/mysql /backup full
```

### Escenario 2: Backup Automático con Snapshots

```bash
# Script de backup diario (guardar como /usr/local/bin/daily_backup.sh)
#!/bin/bash
/usr/local/bin/storage_cli backup create /data /backup incremental
/usr/local/bin/storage_cli backup cleanup 7  # Mantener últimos 7

# Agregar a cron (ejecutar a las 2 AM)
sudo crontab -e
# Agregar: 0 2 * * * /usr/local/bin/daily_backup.sh
```

### Escenario 3: Análisis de Performance

```bash
# 1. Benchmark antes de optimizar
sudo storage_cli perf benchmark sda /mnt/data/test > before.txt

# 2. Aplicar optimizaciones
sudo storage_cli perf tune sda --scheduler=deadline --readahead=2048
sudo storage_cli perf tune sda --vm-swappiness=10 --vm-dirty-ratio=15

# 3. Benchmark después
sudo storage_cli perf benchmark sda /mnt/data/test > after.txt

# 4. Comparar resultados
diff before.txt after.txt
```

## 🗄️ Bases de Datos

### Monitoreo
```bash
# Ubicación
/var/lib/storage_mgr/monitoring.db

# Inspeccionar
sqlite3 /var/lib/storage_mgr/monitoring.db
> SELECT * FROM performance_history ORDER BY timestamp DESC LIMIT 10;
> .schema
> .exit
```

### Backups
```bash
# Ubicación
/var/lib/storage_mgr/backups.db

# Inspeccionar
sqlite3 /var/lib/storage_mgr/backups.db
> SELECT backup_id, timestamp, type, size_bytes FROM backups;
> .exit
```

## 🐛 Troubleshooting

### El daemon no inicia

```bash
# Verificar logs
sudo journalctl -u storage_mgr -f

# Verificar permisos
ls -la /var/run/storage_mgr.pid
ls -la /var/run/storage_mgr.sock

# Limpiar y reiniciar
sudo rm /var/run/storage_mgr.* 2>/dev/null
sudo systemctl restart storage_mgr
```

### El módulo del kernel no carga

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
make clean && make
sudo make install
```

### Errores de permisos

```bash
# Verificar que se ejecuta como root
sudo storage_cli status

# Verificar permisos de directorios
sudo ls -la /var/lib/storage_mgr
sudo ls -la /backup

# Recrear con permisos correctos
sudo mkdir -p /var/lib/storage_mgr /backup
sudo chmod 755 /var/lib/storage_mgr /backup
```

## 📈 Métricas y Rendimiento

### Overhead del Sistema
- CPU: < 1% en monitoreo normal
- Memoria: ~50 MB
- Disco: ~10 MB (bases de datos)

### Capacidades
- Clientes simultáneos: 64
- Dispositivos monitoreados: 16
- Backups concurrentes: 4
- Samples históricos: Ilimitados (con cleanup)

## 🔒 Seguridad

- **Requiere root**: Todas las operaciones privilegiadas
- **IPC seguro**: UNIX domain sockets con permisos 666
- **Logs auditables**: Todas las operaciones se registran
- **Integridad**: Checksums SHA256 en backups

## 📝 Logs

```bash
# Logs del daemon
sudo journalctl -u storage_mgr -f

# Logs del kernel module
dmesg | grep storage_stats

# Logs del sistema
tail -f /var/log/syslog | grep storage
```

## 🎓 Para la Presentación

### Demostración Recomendada (15-20 min)

1. **Introducción** (2 min)
   - Arquitectura general
   - Componentes implementados

2. **Demo en Vivo** (10 min)
   - Iniciar daemon
   - Monitorear dispositivo real
   - Crear backup incremental
   - Aplicar optimización de performance
   - Mostrar módulo del kernel

3. **Código Notable** (5 min)
   - Shared memory IPC
   - Kernel module proc interface
   - Backup engine con rsync

4. **Q&A** (3-5 min)

### Slides Sugeridos

1. Título y equipo
2. Arquitectura del sistema
3. Parte 6: Monitoreo (capturas)
4. Parte 7: Backups (demo)
5. Parte 8: Performance (benchmarks)
6. Parte 9: IPC (diagramas)
7. Parte 10: Kernel Module (código)
8. Desafíos y soluciones
9. Resultados y métricas
10. Conclusiones

## 📚 Referencias

- [Linux Device Drivers](https://lwn.net/Kernel/LDD3/)
- [Advanced Linux Programming](http://advancedlinuxprogramming.com/)
- [LVM HOWTO](https://tldp.org/HOWTO/LVM-HOWTO/)
- [Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)

## 🤝 Contribuciones

Este proyecto es parte de un trabajo académico. Las partes 6-10 fueron implementadas por [tu nombre].

## 📄 Licencia

Proyecto académico - Universidad [Nombre] - 2025

---

**¿Necesitas ayuda?** Revisa la sección de Troubleshooting o ejecuta:
```bash
storage_cli help
man storage_cli  # Si instalaste las man pages
```
