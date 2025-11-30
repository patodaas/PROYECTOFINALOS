#!/bin/bash

# Script de instalación para Storage Manager - Partes 6-10
# Autor: Patricio Dávila Assad
# Descripción: Instala dependencias y configura el entorno

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Funciones de logging
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Verificar si somos root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "Este script debe ejecutarse como root"
        echo "Usa: sudo $0"
        exit 1
    fi
}

# Detectar distribución
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        VERSION=$VERSION_ID
    else
        log_error "No se pudo detectar la distribución"
        exit 1
    fi
    
    log_info "Distribución detectada: $DISTRO $VERSION"
}

# Instalar dependencias
install_dependencies() {
    log_info "Instalando dependencias..."
    
    if [ "$DISTRO" = "ubuntu" ] || [ "$DISTRO" = "debian" ]; then
        apt-get update
        apt-get install -y \
            build-essential \
            gcc \
            make \
            cmake \
            git \
            sqlite3 \
            libsqlite3-dev \
            libssl-dev \
            linux-headers-$(uname -r) \
            rsync \
            lvm2 \
            mdadm
            
    elif [ "$DISTRO" = "centos" ] || [ "$DISTRO" = "rhel" ] || [ "$DISTRO" = "fedora" ]; then
        yum install -y \
            gcc \
            make \
            cmake \
            git \
            sqlite \
            sqlite-devel \
            openssl-devel \
            kernel-devel \
            rsync \
            lvm2 \
            mdadm
            
    else
        log_warn "Distribución no reconocida, instala manualmente las dependencias"
        return 1
    fi
    
    log_info "✓ Dependencias instaladas"
}

# Crear estructura de directorios
create_directories() {
    log_info "Creando directorios..."
    
    mkdir -p /var/lib/storage_mgr
    mkdir -p /var/run
    mkdir -p /backup
    mkdir -p /var/log/storage_mgr
    
    # Permisos
    chmod 755 /var/lib/storage_mgr
    chmod 755 /backup
    chmod 755 /var/log/storage_mgr
    
    log_info "✓ Directorios creados"
}

# Crear dispositivos loop para testing
create_loop_devices() {
    log_info "Creando dispositivos loop para pruebas..."
    
    if [ ! -d "/tmp/loop_devices" ]; then
        mkdir -p /tmp/loop_devices
    fi
    
    for i in {0..7}; do
        if [ ! -f "/tmp/loop_devices/disk$i.img" ]; then
            dd if=/dev/zero of=/tmp/loop_devices/disk$i.img bs=1M count=1024 2>/dev/null
            log_info "✓ Creado disk$i.img (1GB)"
        fi
        
        # Intentar asociar con loop device si está disponible
        if [ ! -e "/dev/loop$i" ]; then
            mknod /dev/loop$i b 7 $i 2>/dev/null || true
        fi
        
        # Asociar imagen con loop device
        losetup /dev/loop$i /tmp/loop_devices/disk$i.img 2>/dev/null || true
    done
    
    log_info "✓ Dispositivos loop configurados"
    log_info "  Usa: losetup -a para ver dispositivos activos"
}

# Compilar proyecto
compile_project() {
    log_info "Compilando proyecto..."
    
    # Crear directorios necesarios
    mkdir -p include src cli tests kernel_module obj bin
    
    # Compilar código principal
    make clean 2>/dev/null || true
    make all
    
    log_info "✓ Proyecto compilado"
}

# Compilar módulo del kernel
compile_kernel_module() {
    log_info "Compilando módulo del kernel..."
    
    cd kernel_module
    make clean 2>/dev/null || true
    make
    cd ..
    
    log_info "✓ Módulo del kernel compilado"
}

# Cargar módulo del kernel
load_kernel_module() {
    log_info "Cargando módulo del kernel..."
    
    cd kernel_module
    
    # Descargar si ya está cargado
    rmmod storage_stats 2>/dev/null || true
    
    # Cargar módulo
    insmod storage_stats.ko
    
    # Verificar
    if lsmod | grep -q storage_stats; then
        log_info "✓ Módulo storage_stats cargado"
        
        # Mostrar entrada /proc
        if [ -f /proc/storage_stats ]; then
            log_info "✓ Entrada /proc/storage_stats creada"
        fi
    else
        log_error "No se pudo cargar el módulo"
    fi
    
    cd ..
}

# Crear archivo de servicio systemd
create_systemd_service() {
    log_info "Creando servicio systemd..."
    
    cat > /etc/systemd/system/storage_mgr.service << 'EOF'
[Unit]
Description=Storage Manager Daemon
After=network.target

[Service]
Type=forking
ExecStart=/usr/local/bin/storage_daemon
PIDFile=/var/run/storage_mgr.pid
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    
    log_info "✓ Servicio systemd creado"
    log_info "  Usa: systemctl start storage_mgr"
    log_info "       systemctl enable storage_mgr"
}

# Menú de instalación
show_menu() {
    echo ""
    echo "╔════════════════════════════════════════════════╗"
    echo "║  Storage Manager - Setup Script (Parts 6-10)  ║"
    echo "╚════════════════════════════════════════════════╝"
    echo ""
    echo "1) Instalación completa (recomendado)"
    echo "2) Instalar solo dependencias"
    echo "3) Compilar proyecto"
    echo "4) Compilar e instalar módulo del kernel"
    echo "5) Crear dispositivos loop para testing"
    echo "6) Crear servicio systemd"
    echo "7) Ejecutar tests"
    echo "8) Salir"
    echo ""
    read -p "Selecciona una opción: " choice
    
    case $choice in
        1)
            full_install
            ;;
        2)
            install_dependencies
            ;;
        3)
            compile_project
            ;;
        4)
            compile_kernel_module
            load_kernel_module
            ;;
        5)
            create_loop_devices
            ;;
        6)
            create_systemd_service
            ;;
        7)
            run_tests
            ;;
        8)
            exit 0
            ;;
        *)
            log_error "Opción inválida"
            show_menu
            ;;
    esac
}

# Instalación completa
full_install() {
    log_info "=== Iniciando instalación completa ==="
    
    detect_distro
    install_dependencies
    create_directories
    create_loop_devices
    compile_project
    compile_kernel_module
    load_kernel_module
    
    # Copiar binarios
    cp bin/storage_daemon /usr/local/bin/ 2>/dev/null || true
    cp bin/storage_cli /usr/local/bin/ 2>/dev/null || true
    
    create_systemd_service
    
    log_info ""
    log_info "=== ✓ Instalación completada ==="
    log_info ""
    log_info "Próximos pasos:"
    log_info "  1. Iniciar daemon: systemctl start storage_mgr"
    log_info "  2. Ver módulo kernel: cat /proc/storage_stats"
    log_info "  3. Probar CLI: storage_cli status"
    log_info "  4. Ejecutar tests: make test"
    log_info ""
}

# Ejecutar tests
run_tests() {
    log_info "Ejecutando tests..."
    
    if [ -f "bin/test_monitor" ]; then
        log_info "=== Test: Monitor ==="
        ./bin/test_monitor
    fi
    
    if [ -f "bin/test_backup" ]; then
        log_info "=== Test: Backup ==="
        ./bin/test_backup
    fi
    
    if [ -f "bin/test_perf" ]; then
        log_info "=== Test: Performance ==="
        ./bin/test_perf
    fi
    
    if [ -f "bin/test_ipc" ]; then
        log_info "=== Test: IPC ==="
        ./bin/test_ipc
    fi
}

# Main
main() {
    check_root
    
    if [ $# -eq 0 ]; then
        show_menu
    else
        case "$1" in
            install)
                full_install
                ;;
            deps)
                detect_distro
                install_dependencies
                ;;
            compile)
                compile_project
                ;;
            kernel)
                compile_kernel_module
                load_kernel_module
                ;;
            test)
                run_tests
                ;;
            *)
                echo "Uso: $0 [install|deps|compile|kernel|test]"
                exit 1
                ;;
        esac
    fi
}

main "$@"
