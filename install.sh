#!/bin/bash

# Script de instalación automática
# Enterprise Storage Manager - Partes 1-5

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Verificar si es root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Este script debe ejecutarse como root"
        echo "Usa: sudo ./install.sh"
        exit 1
    fi
    print_success "Running as root"
}

# Instalar dependencias
install_dependencies() {
    print_header "Installing Dependencies"
    
    echo "Updating package list..."
    apt update -qq
    
    echo "Installing build tools..."
    apt install -y build-essential gcc make cmake git >/dev/null 2>&1
    print_success "Build tools installed"
    
    echo "Installing storage tools..."
    apt install -y mdadm lvm2 xfsprogs btrfs-progs >/dev/null 2>&1
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
}

# Verificar instalación
verify_installation() {
    print_header "Verifying Installation"
    
    local tools=("gcc" "make" "mdadm" "lvm" "cryptsetup" "setfacl")
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

# Crear estructura de directorios
create_structure() {
    print_header "Creating Project Structure"
    
    mkdir -p src include tests scripts docs build
    print_success "Directories created"
    
    # Verificar que los archivos fuente existen
    if [ ! -f "src/utils.c" ]; then
        print_warning "Source files not found. Make sure to copy all .c and .h files"
        print_warning "Run this script from the project root directory"
    fi
}

# Compilar proyecto
compile_project() {
    print_header "Compiling Project"
    
    if [ ! -f "Makefile" ]; then
        print_error "Makefile not found"
        exit 1
    fi
    
    echo "Cleaning previous build..."
    make clean >/dev/null 2>&1 || true
    
    echo "Compiling..."
    if make -j$(nproc); then
        print_success "Compilation successful"
    else
        print_error "Compilation failed"
        exit 1
    fi
}

# Crear directorios de prueba
setup_test_environment() {
    print_header "Setting Up Test Environment"
    
    mkdir -p /tmp/storage_test
    mkdir -p /mnt/test_data
    mkdir -p /mnt/test_xfs
    
    print_success "Test environment ready"
}

# Menú principal
show_menu() {
    print_header "Enterprise Storage Manager - Setup"
    
    echo "Select an option:"
    echo ""
    echo "  1) Full installation (recommended for first time)"
    echo "  2) Install dependencies only"
    echo "  3) Compile project only"
    echo "  4) Run tests"
    echo "  5) Clean everything"
    echo "  6) Exit"
    echo ""
    read -p "Enter option [1-6]: " option
    
    case $option in
        1)
            full_installation
            ;;
        2)
            check_root
            install_dependencies
            verify_installation
            ;;
        3)
            compile_project
            ;;
        4)
            run_tests
            ;;
        5)
            clean_all
            ;;
        6)
            exit 0
            ;;
        *)
            print_error "Invalid option"
            exit 1
            ;;
    esac
}

# Instalación completa
full_installation() {
    print_header "Full Installation"
    
    check_root
    install_dependencies
    verify_installation
    create_structure
    compile_project
    setup_test_environment
    
    print_header "Installation Complete!"
    
    echo ""
    echo "Next steps:"
    echo "  1. Make sure all source files (.c and .h) are in place"
    echo "  2. Run: make"
    echo "  3. Run tests: sudo make test"
    echo ""
    
    print_success "Setup completed successfully!"
}

# Ejecutar tests
run_tests() {
    print_header "Running Tests"
    
    if [ ! -f "build/storage_test" ]; then
        print_error "Test program not found. Run compilation first."
        exit 1
    fi
    
    print_warning "Tests will modify system storage configuration"
    print_warning "Make sure you're running on a VM or test system"
    read -p "Continue? (y/n): " confirm
    
    if [ "$confirm" != "y" ]; then
        echo "Tests cancelled"
        exit 0
    fi
    
    ./build/storage_test
}

# Limpieza
clean_all() {
    print_header "Cleaning Everything"
    
    check_root
    
    print_warning "This will stop all RAID arrays, remove LVM volumes, and clean loop devices"
    read -p "Continue? (y/n): " confirm
    
    if [ "$confirm" != "y" ]; then
        echo "Cleaning cancelled"
        exit 0
    fi
    
    echo "Stopping services..."
    make distclean 2>/dev/null || true
    
    echo "Removing test files..."
    rm -rf /tmp/storage_test 2>/dev/null || true
    rm -rf /mnt/test_data /mnt/test_xfs 2>/dev/null || true
    
    echo "Cleaning build directory..."
    rm -rf build/* 2>/dev/null || true
    
    print_success "Cleaning complete"
}

# Script principal
main() {
    clear
    
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║   ENTERPRISE STORAGE MANAGER - INSTALLATION SCRIPT    ║"
    echo "║              Partes 1-5: Setup & Testing              ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo -e "${NC}\n"
    
    # Si se pasa argumento, ejecutar directamente
    if [ $# -gt 0 ]; then
        case $1 in
            "--full"|"-f")
                full_installation
                ;;
            "--deps"|"-d")
                check_root
                install_dependencies
                verify_installation
                ;;
            "--compile"|"-c")
                compile_project
                ;;
            "--test"|"-t")
                run_tests
                ;;
            "--clean")
                clean_all
                ;;
            "--help"|"-h")
                echo "Usage: sudo ./install.sh [option]"
                echo ""
                echo "Options:"
                echo "  -f, --full      Full installation"
                echo "  -d, --deps      Install dependencies only"
                echo "  -c, --compile   Compile project only"
                echo "  -t, --test      Run tests"
                echo "  --clean         Clean everything"
                echo "  -h, --help      Show this help"
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
