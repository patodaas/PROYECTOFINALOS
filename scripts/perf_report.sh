#!/bin/bash
#
# perf_report.sh - Generador de reportes de rendimiento
#

set -e

OUTPUT_FILE="/var/log/performance_report_$(date +%Y%m%d_%H%M%S).txt"
DATE=$(date '+%Y-%m-%d %H:%M:%S')

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Error: Ejecutar como root${NC}"
        exit 1
    fi
}

generate_report() {
    {
        echo "========================================="
        echo "  REPORTE DE RENDIMIENTO"
        echo "  Fecha: $DATE"
        echo "========================================="
        echo ""
        
        echo "--- Sistema ---"
        echo "Hostname: $(hostname)"
        echo "Kernel: $(uname -r)"
        echo "Uptime: $(uptime -p)"
        echo ""
        
        echo "--- CPU ---"
        mpstat 2 1 2>/dev/null || echo "mpstat no disponible"
        echo ""
        
        echo "--- Memoria ---"
        free -h
        echo ""
        
        echo "--- Disco ---"
        df -h
        echo ""
        
        echo "--- I/O ---"
        iostat -x 2 2 2>/dev/null || echo "iostat no disponible"
        echo ""
        
        if [ -f /proc/mdstat ]; then
            echo "--- RAID ---"
            cat /proc/mdstat
            echo ""
        fi
        
        if command -v vgs &> /dev/null; then
            echo "--- LVM ---"
            vgs 2>/dev/null
            lvs 2>/dev/null
            echo ""
        fi
        
    } > "$OUTPUT_FILE"
}

show_help() {
    echo "Uso: $0 [--output FILE]"
}

main() {
    check_root
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --output)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                shift
                ;;
        esac
    done
    
    echo -e "${BLUE}→ Generando reporte...${NC}"
    generate_report
    echo -e "${GREEN}✓ Reporte generado: $OUTPUT_FILE${NC}"
}

main "$@"
