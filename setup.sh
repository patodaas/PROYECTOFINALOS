#!/bin/bash

# Script de instalación unificado para Storage Manager
# Integra todas las partes del proyecto (1-14)
# Combina install.sh (partes 1-5) y setup.sh (partes 6-10)

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Banner
show_banner() {
    clear
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║     ENTERPRISE STORAGE MANAGER - INSTALLATION SCRIPT      ║"
    echo "║                  Complete System Setup                     ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo -e "${NC}\n"
}

# Funciones de utilidad
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Verificar si es root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Este script debe ejecutarse como root"
        echo "Usa: sudo $0"
        exit 1
    fi
    print_success "Running as root"
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

# Instalar dependencias completas
install_dependencies() {
    print_header "Installing Dependencies"
    
    echo "Updating package list..."
    
    if [ "$DISTRO" = "ubuntu" ] || [ "$DISTRO" = "debian" ]; then
        apt update -qq
        
        echo "Installing build tools..."
        apt install -y build-essential gcc make cmake git >/dev/null 2>&1
        print_success "Build tools installed"
        
        echo "Installing storage tools..."
        apt install -y mdadm lvm2 xfsprogs btrfs-progs rsync >/dev/null 2>&1
        print_success "Storage tools installed"
        
        echo "Installing security tools..."
        apt install -y cryptsetup acl attr >/dev/null 2>&1
        print_success "Security tools installed"
        
        echo "Installing monitoring tools..."
        apt install -y sysstat iotop lsof >/dev/null 2>&1
        print_success "Monitoring tools installed"
        
        echo "Installing development tools..."
        apt install -y valgrind gdb strace >/dev/null 2>&1
        print_success "Development tools installed"
        
        echo "Installing system libraries..."
        apt install -y sqlite3 libsqlite3-dev libssl-dev >/dev/null 2>&1
        print_success "System libraries installed"
        
        echo "Installing kernel headers..."
        apt install -y linux-headers-$(uname -r) >/dev/null 2>&1
        print_success "Kernel headers installed"
        
        echo "Installing automation tools..."
        apt install -y mailutils >/dev/null 2>&1 || true
        print_success "Automation tools installed"
            
    elif [ "$DISTRO" = "centos" ] || [ "$DISTRO" = "rhel" ] || [ "$DISTRO" = "fedora" ]; then
        yum install -y gcc make cmake git \
                       mdadm lvm2 xfsprogs rsync \
                       cryptsetup \
                       sysstat \
                       valgrind gdb strace \
                       sqlite sqlite-devel openssl-devel \
                       kernel-devel \
                       mailx >/dev/null 2>&1
        print_success "All packages installed"
    else
        log_warn "Distribución no reconocida, instala manualmente las dependencias"
        return 1
    fi
    
    print_success "All dependencies installed"
}

# Verificar instalación
verify_installation() {
    print_header "Verifying Installation"
    
    local tools=("gcc" "make" "mdadm" "lvm" "cryptsetup" "setfacl" "sqlite3" "rsync")
    local all_ok=1
    
    for tool in "${tools[@]}"; do
        if command -v $tool >/dev/null 2>&1; then
            print_success "$tool found"
        else
            print_error "$tool not found"
            all_ok=0
        fi
    done
    
    if [ $all_ok -eq 0 ]; then
        print_error "Some tools are missing. Please check installation."
        exit 1
    fi
    
    print_success "All tools verified"
}

# Crear estructura completa de directorios
create_directories() {
    print_header "Creating Directory Structure"
    
    # Directorios del proyecto
    mkdir -p src include tests scripts docs build cli kernel_module obj bin systemd
    print_success "Project directories created"
    
    # Directorios del sistema
    mkdir -p /var/lib/storage_mgr
    mkdir -p /var/run
    mkdir -p /backup/{full,incremental,snapshots,logs}
    mkdir -p /var/log/storage_mgr
    mkdir -p /mnt/test_data
    mkdir -p /mnt/test_xfs
    mkdir -p /tmp/storage_test
    
    # Permisos
    chmod 755 /var/lib/storage_mgr
    chmod 755 /backup
    chmod 755 /var/log/storage_mgr
    
    print_success "System directories created"
    
    # Verificar que los archivos fuente existen
    if [ ! -f "src/utils.c" ] && [ ! -f "src/monitor.c" ]; then
        print_warning "Source files not found. Make sure to copy all .c and .h files"
        print_warning "Run this script from the project root directory"
    fi
}

# Crear dispositivos loop para testing
create_loop_devices() {
    print_header "Creating Loop Devices for Testing"
    
    log_info "Creando dispositivos loop para pruebas..."
    
    # Limpiar dispositivos previos
    log_info "Cleaning previous loop devices..."
    losetup -D 2>/dev/null || true
    mdadm --stop /dev/md* 2>/dev/null || true
    
    if [ ! -d "/tmp/loop_devices" ]; then
        mkdir -p /tmp/loop_devices
    fi
    
    local success=0
    local failed=0
    
    for i in {0..7}; do
        if [ ! -f "/tmp/loop_devices/disk$i.img" ]; then
            if dd if=/dev/zero of=/tmp/loop_devices/disk$i.img bs=1M count=1024 2>/dev/null; then
                log_info "✓ Created disk$i.img (1GB)"
            else
                log_warn "Failed to create disk$i.img"
                ((failed++))
                continue
            fi
        fi
        
        # Intentar asociar con loop device
        if [ ! -e "/dev/loop$i" ]; then
            mknod /dev/loop$i b 7 $i 2>/dev/null || true
        fi
        
        if losetup /dev/loop$i /tmp/loop_devices/disk$i.img 2>/dev/null; then
            log_info "✓ Associated /dev/loop$i"
            ((success++))
        else
            # Intentar con --find
            LOOP=$(losetup --find --show /tmp/loop_devices/disk$i.img 2>/dev/null)
            if [ $? -eq 0 ]; then
                log_info "✓ Associated $LOOP"
                ((success++))
            else
                log_warn "Failed to associate loop device for disk$i"
                ((failed++))
            fi
        fi
    done
    
    print_success "Loop devices configured: $success/8"
    log_info "Use: losetup -a to see active devices"
    
    # Verificar que tenemos al menos 4 dispositivos
    if [ $success -lt 4 ]; then
        print_error "Need at least 4 loop devices for testing"
        print_warning "Try rebooting the VM and running again"
        return 1
    fi
}

# Compilar proyecto completo
compile_project() {
    print_header "Compiling Project"
    
    if [ ! -f "Makefile" ]; then
        print_error "Makefile not found"
        exit 1
    fi
    
    echo "Cleaning previous build..."
    make clean >/dev/null 2>&1 || true
    
    echo "Compiling all components..."
    if make -j$(nproc); then
        print_success "Compilation successful"
    else
        print_error "Compilation failed"
        echo "Check the error messages above"
        exit 1
    fi
    
    # Verificar binarios
    if [ -f "bin/storage_daemon" ] && [ -f "bin/storage_cli" ]; then
        print_success "Main binaries compiled"
    else
        print_error "Main binaries not found"
        exit 1
    fi
}

# Compilar módulo del kernel
compile_kernel_module() {
    print_header "Compiling Kernel Module"
    
    if [ ! -d "kernel_module" ]; then
        print_error "kernel_module directory not found"
        return 1
    fi
    
    cd kernel_module
    
    echo "Cleaning previous build..."
    make clean 2>/dev/null || true
    
    echo "Compiling kernel module..."
    if make; then
        print_success "Kernel module compiled"
    else
        print_error "Kernel module compilation failed"
        cd ..
        return 1
    fi
    
    cd ..
    print_success "Kernel module ready"
}

# Cargar módulo del kernel
load_kernel_module() {
    print_header "Loading Kernel Module"
    
    if [ ! -f "kernel_module/storage_stats.ko" ]; then
        print_error "Kernel module not found. Compile it first."
        return 1
    fi
    
    cd kernel_module
    
    # Descargar si ya está cargado
    echo "Unloading previous module..."
    rmmod storage_stats 2>/dev/null || true
    
    # Cargar módulo
    echo "Loading module..."
    if insmod storage_stats.ko; then
        print_success "Module loaded"
    else
        print_error "Failed to load module"
        cd ..
        return 1
    fi
    
    # Verificar
    if lsmod | grep -q storage_stats; then
        print_success "Module storage_stats is active"
        
        if [ -f /proc/storage_stats ]; then
            print_success "/proc/storage_stats interface created"
        fi
    else
        print_error "Module not loaded properly"
        cd ..
        return 1
    fi
    
    cd ..
}

# Instalar binarios
install_binaries() {
    print_header "Installing Binaries"
    
    if [ -f "bin/storage_daemon" ]; then
        cp bin/storage_daemon /usr/local/bin/
        chmod +x /usr/local/bin/storage_daemon
        print_success "storage_daemon installed"
    fi
    
    if [ -f "bin/storage_cli" ]; then
        cp bin/storage_cli /usr/local/bin/
        chmod +x /usr/local/bin/storage_cli
        print_success "storage_cli installed"
    fi
}

# Instalar scripts de automatización
install_scripts() {
    print_header "Installing Automation Scripts"
    
    if [ -d "scripts" ]; then
        if [ -f "scripts/health_check.sh" ]; then
            cp scripts/health_check.sh /usr/local/bin/
            chmod +x /usr/local/bin/health_check.sh
            print_success "health_check.sh installed"
        fi
        
        if [ -f "scripts/auto_backup.sh" ]; then
            cp scripts/auto_backup.sh /usr/local/bin/
            chmod +x /usr/local/bin/auto_backup.sh
            print_success "auto_backup.sh installed"
        fi
        
        if [ -f "scripts/perf_report.sh" ]; then
            cp scripts/perf_report.sh /usr/local/bin/
            chmod +x /usr/local/bin/perf_report.sh
            print_success "perf_report.sh installed"
        fi
    fi
}

# Crear servicios systemd
create_systemd_services() {
    print_header "Creating Systemd Services"
    
    # Daemon principal
    cat > /etc/systemd/system/storage_daemon.service << 'EOF'
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
    print_success "storage_daemon.service created"
    
    # Backup service
    cat > /etc/systemd/system/storage_backup.service << 'EOF'
[Unit]
Description=Storage Manager Backup Service
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/auto_backup.sh --incremental /mnt/data
EOF
    print_success "storage_backup.service created"
    
    # Backup timer
    cat > /etc/systemd/system/storage_backup.timer << 'EOF'
[Unit]
Description=Storage Manager Backup Timer
Requires=storage_backup.service

[Timer]
OnCalendar=02:00
Persistent=true

[Install]
WantedBy=timers.target
EOF
    print_success "storage_backup.timer created"
    
    # Health check service
    cat > /etc/systemd/system/storage_health.service << 'EOF'
[Unit]
Description=Storage Manager Health Check
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/health_check.sh
EOF
    print_success "storage_health.service created"
    
    # Health check timer
    cat > /etc/systemd/system/storage_health.timer << 'EOF'
[Unit]
Description=Storage Manager Health Check Timer
Requires=storage_health.service

[Timer]
OnCalendar=hourly
Persistent=true

[Install]
WantedBy=timers.target
EOF
    print_success "storage_health.timer created"
    
    # Reload systemd
    systemctl daemon-reload
    print_success "Systemd daemon reloaded"
    
    echo ""
    log_info "To enable services, run:"
    log_info "  systemctl enable storage_daemon"
    log_info "  systemctl enable storage_backup.timer"
    log_info "  systemctl enable storage_health.timer"
}

# Ejecutar tests
run_tests() {
    print_header "Running Tests"
    
    if [ ! -d "bin" ] || [ ! -f "bin/test_monitor" ]; then
        print_error "Test binaries not found. Run compilation first."
        return 1
    fi
    
    print_warning "Tests will modify system storage configuration"
    print_warning "Make sure you're running on a VM or test system"
    read -p "Continue? (y/n): " confirm
    
    if [ "$confirm" != "y" ]; then
        echo "Tests cancelled"
        return 0
    fi
    
    # Test monitor
    if [ -f "bin/test_monitor" ]; then
        log_info "=== Test: Monitor ==="
        ./bin/test_monitor || true
    fi
    
    # Test backup
    if [ -f "bin/test_backup" ]; then
        log_info "=== Test: Backup ==="
        ./bin/test_backup || true
    fi
    
    # Test performance
    if [ -f "bin/test_perf" ]; then
        log_info "=== Test: Performance ==="
        ./bin/test_perf || true
    fi
    
    # Test IPC
    if [ -f "bin/test_ipc" ]; then
        log_info "=== Test: IPC ==="
        ./bin/test_ipc || true
    fi
    
    # Test daemon
    if [ -f "bin/test_daemon" ]; then
        log_info "=== Test: Daemon ==="
        ./bin/test_daemon || true
    fi
    
    print_success "Tests completed"
}

# Limpieza completa
clean_all() {
    print_header "Cleaning Everything"
    
    check_root
    
    print_warning "This will stop all RAID arrays, remove LVM volumes, and clean loop devices"
    read -p "Continue? (y/n): " confirm
    
    if [ "$confirm" != "y" ]; then
        echo "Cleaning cancelled"
        return 0
    fi
    
    echo "Stopping services..."
    systemctl stop storage_daemon 2>/dev/null || true
    systemctl stop storage_backup.timer 2>/dev/null || true
    systemctl stop storage_health.timer 2>/dev/null || true
    
    echo "Unloading kernel module..."
    rmmod storage_stats 2>/dev/null || true
    
    echo "Cleaning RAID arrays..."
    mdadm --stop /dev/md* 2>/dev/null || true
    
    echo "Cleaning loop devices..."
    losetup -D 2>/dev/null || true
    
    echo "Removing test files..."
    rm -rf /tmp/storage_test 2>/dev/null || true
    rm -rf /tmp/loop_devices 2>/dev/null || true
    rm -rf /mnt/test_data /mnt/test_xfs 2>/dev/null || true
    
    echo "Cleaning build directory..."
    make clean 2>/dev/null || true
    make distclean 2>/dev/null || true
    rm -rf build/* obj/* bin/* 2>/dev/null || true
    
    echo "Cleaning kernel module..."
    cd kernel_module && make clean 2>/dev/null || true
    cd ..
    
    print_success "Cleaning complete"
}

# Instalación completa
full_installation() {
    print_header "Full Installation"
    
    check_root
    detect_distro
    install_dependencies
    verify_installation
    create_directories
    create_loop_devices
    compile_project
    compile_kernel_module
    load_kernel_module
    install_binaries
    install_scripts
    create_systemd_services
    
    print_header "Installation Complete!"
    
    echo ""
    echo "Next steps:"
    echo "  1. Start daemon: systemctl start storage_daemon"
    echo "  2. Enable services:"
    echo "     systemctl enable storage_daemon"
    echo "     systemctl enable storage_backup.timer"
    echo "     systemctl enable storage_health.timer"
    echo "  3. View kernel module: cat /proc/storage_stats"
    echo "  4. Test CLI: storage_cli status"
    echo "  5. Run tests: sudo make test"
    echo ""
    
    print_success "Setup completed successfully!"
}

# Menú principal
show_menu() {
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║  Storage Manager - Setup Script                       ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "Select an option:"
    echo ""
    echo "  1) Full installation (recommended for first time)"
    echo "  2) Install dependencies only"
    echo "  3) Compile project only"
    echo "  4) Compile and load kernel module"
    echo "  5) Create loop devices for testing"
    echo "  6) Install systemd services"
    echo "  7) Install automation scripts"
    echo "  8) Run tests"
    echo "  9) Clean everything"
    echo " 10) Exit"
    echo ""
    read -p "Enter option [1-10]: " option
    
    case $option in
        1)
            full_installation
            ;;
        2)
            check_root
            detect_distro
            install_dependencies
            verify_installation
            ;;
        3)
            compile_project
            ;;
        4)
            compile_kernel_module
            load_kernel_module
            ;;
        5)
            check_root
            create_loop_devices
            ;;
        6)
            check_root
            create_systemd_services
            ;;
        7)
            check_root
            install_scripts
            ;;
        8)
            run_tests
            ;;
        9)
            clean_all
            ;;
        10)
            exit 0
            ;;
        *)
            print_error "Invalid option"
            exit 1
            ;;
    esac
}

# Main
main() {
    show_banner
    
    # Si se pasa argumento, ejecutar directamente
    if [ $# -gt 0 ]; then
        case "$1" in
            "--full"|"-f"|"install")
                check_root
                full_installation
                ;;
            "--deps"|"-d"|"deps")
                check_root
                detect_distro
                install_dependencies
                verify_installation
                ;;
            "--compile"|"-c"|"compile")
                compile_project
                ;;
            "--kernel"|"-k"|"kernel")
                check_root
                compile_kernel_module
                load_kernel_module
                ;;
            "--test"|"-t"|"test")
                run_tests
                ;;
            "--clean")
                clean_all
                ;;
            "--help"|"-h")
                echo "Usage: sudo ./setup.sh [option]"
                echo ""
                echo "Options:"
                echo "  -f, --full, install     Full installation"
                echo "  -d, --deps, deps        Install dependencies only"
                echo "  -c, --compile, compile  Compile project only"
                echo "  -k, --kernel, kernel    Compile and load kernel module"
                echo "  -t, --test, test        Run tests"
                echo "  --clean                 Clean everything"
                echo "  -h, --help              Show this help"
                echo ""
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    else
        show_menu
    fi
}

# Ejecutar script
main "$@"
