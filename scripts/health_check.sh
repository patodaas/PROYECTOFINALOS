#!/bin/bash
#
# health_check.sh - Sistema de verificación de salud del almacenamiento
# Parte 12: Automation & Scripting
#

set -e

# Configuración
LOG_FILE="/var/log/storage_health.log"
EMAIL_ALERT="admin@localhost"
ALERT_THRESHOLD_DISK=90
ALERT_THRESHOLD_INODE=90
DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Función de logging
log_message() {
    local level=$1
    shift
    echo "[$DATE] [$level] $*" | tee -a "$LOG_FILE"
}

# Función para enviar alertas por email (opcional)
send_alert() {
    local subject=$1
    local message=$2
    
    # Si tienes mailutils instalado, descomenta:
    # echo "$message" | mail -s "$subject" "$EMAIL_ALERT"
    
    log_message "ALERT" "$subject: $message"
}

# Verificar si se ejecuta como root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Error: Este script debe ejecutarse como root${NC}"
        exit 1
    fi
}

# Verificar estado de arrays RAID
check_raid() {
    log_message "INFO" "Verificando arrays RAID..."
    
    if [ ! -f /proc/mdstat ]; then
        log_message "INFO" "RAID no configurado (esto es normal)"
        return 0
    fi
    
    local raid_arrays=$(cat /proc/mdstat | grep "^md" | awk '{print $1}')
    
    if [ -z "$raid_arrays" ]; then
        log_message "INFO" "No hay arrays RAID configurados (esto es normal)"
        return 0
    fi
    
    for array in $raid_arrays; do
        local status=$(mdadm --detail /dev/$array 2>/dev/null | grep "State :" | awk '{print $3}')
        
        if [ "$status" != "clean" ] && [ "$status" != "active" ]; then
            echo -e "${RED}✗ RAID /dev/$array está degradado: $status${NC}"
            send_alert "RAID Alert" "Array /dev/$array en estado: $status"
            log_message "ERROR" "RAID /dev/$array degradado: $status"
            return 1
        else
            echo -e "${GREEN}✓ RAID /dev/$array: $status${NC}"
            log_message "INFO" "RAID /dev/$array: OK ($status)"
        fi
    done
    
    return 0
}

# Verificar volúmenes LVM
check_lvm() {
    log_message "INFO" "Verificando volúmenes LVM..."
    
    if ! command -v vgs &> /dev/null; then
        log_message "INFO" "LVM no instalado (esto es normal)"
        return 0
    fi
    
    local vg_count=$(vgs --noheadings 2>/dev/null | wc -l)
    
    if [ "$vg_count" -eq 0 ]; then
        log_message "INFO" "No hay Volume Groups configurados (esto es normal)"
        return 0
    fi
    
    # Verificar Volume Groups
    vgs --noheadings -o vg_name,vg_attr 2>/dev/null | while read vg attrs; do
        if [[ $attrs == *"p"* ]]; then
            echo -e "${RED}✗ Volume Group $vg tiene problemas${NC}"
            log_message "ERROR" "VG $vg con problemas en atributos: $attrs"
            return 1
        else
            echo -e "${GREEN}✓ Volume Group $vg: OK${NC}"
            log_message "INFO" "VG $vg: OK"
        fi
    done
    
    # Verificar Logical Volumes
    lvs --noheadings -o lv_name,vg_name,lv_attr 2>/dev/null | while read lv vg attrs; do
        if [[ $attrs == *"i"* ]]; then
            echo -e "${RED}✗ Logical Volume $vg/$lv inactivo${NC}"
            log_message "ERROR" "LV $vg/$lv inactivo"
            return 1
        else
            echo -e "${GREEN}✓ Logical Volume $vg/$lv: OK${NC}"
            log_message "INFO" "LV $vg/$lv: OK"
        fi
    done
    
    return 0
}

# Verificar salud de filesystems
check_filesystem() {
    log_message "INFO" "Verificando filesystems..."
    
    local exit_code=0
    
    # Obtener todos los puntos de montaje (excepto virtuales)
    df -h -x tmpfs -x devtmpfs -x squashfs 2>/dev/null | tail -n +2 | while read line; do
        local device=$(echo $line | awk '{print $1}')
        local mount=$(echo $line | awk '{print $6}')
        local use=$(echo $line | awk '{print $5}' | sed 's/%//')
        
        # Ignorar sistemas de archivos especiales
        if [[ "$mount" == "/sys"* ]] || [[ "$mount" == "/proc"* ]] || [[ "$mount" == "/dev"* ]]; then
            continue
        fi
        
        # Verificar que use sea un número válido
        if ! [[ "$use" =~ ^[0-9]+$ ]]; then
            continue
        fi
        
        # Verificar uso de disco
        if [ "$use" -ge "$ALERT_THRESHOLD_DISK" ]; then
            echo -e "${RED}✗ $mount: ${use}% usado (umbral: ${ALERT_THRESHOLD_DISK}%)${NC}"
            send_alert "Disk Space Alert" "$mount está al ${use}% de capacidad"
            log_message "ERROR" "$mount al ${use}% de capacidad"
            exit_code=1
        elif [ "$use" -ge 75 ]; then
            echo -e "${YELLOW}⚠ $mount: ${use}% usado${NC}"
            log_message "WARN" "$mount al ${use}% de capacidad"
        else
            echo -e "${GREEN}✓ $mount: ${use}% usado${NC}"
            log_message "INFO" "$mount: OK (${use}%)"
        fi
        
        # Verificar inodos (solo para filesystems normales)
        local inode_use=$(df -i "$mount" 2>/dev/null | tail -1 | awk '{print $5}' | sed 's/%//')
        
        # Solo verificar si es un número válido
        if [[ "$inode_use" =~ ^[0-9]+$ ]]; then
            if [ "$inode_use" -ge "$ALERT_THRESHOLD_INODE" ]; then
                echo -e "${RED}✗ $mount: ${inode_use}% inodos usados${NC}"
                send_alert "Inode Alert" "$mount tiene ${inode_use}% inodos usados"
                log_message "ERROR" "$mount: ${inode_use}% inodos usados"
                exit_code=1
            fi
        fi
    done
    
    return $exit_code
}

# Verificar espacio en disco
check_disk_space() {
    log_message "INFO" "Verificando espacio en disco..."
    
    df -h | grep -vE '^Filesystem|tmpfs|cdrom|loop|sys|proc|dev' | awk '{print $5 " " $1}' | while read output; do
        local usage=$(echo $output | awk '{print $1}' | sed 's/%//g')
        local partition=$(echo $output | awk '{print $2}')
        
        # Verificar que usage sea un número
        if ! [[ "$usage" =~ ^[0-9]+$ ]]; then
            continue
        fi
        
        if [ $usage -ge $ALERT_THRESHOLD_DISK ]; then
            echo -e "${RED}✗ Partición $partition al ${usage}%${NC}"
            send_alert "Disk Alert" "Partición $partition al ${usage}%"
            log_message "ERROR" "Partición $partition al ${usage}%"
        fi
    done
}

# Verificar servicios del daemon
check_daemon() {
    log_message "INFO" "Verificando daemon de almacenamiento..."
    
    if [ -f /var/run/storage_mgr.pid ]; then
        local pid=$(cat /var/run/storage_mgr.pid)
        if ps -p $pid > /dev/null 2>&1; then
            echo -e "${GREEN}✓ Daemon corriendo (PID: $pid)${NC}"
            log_message "INFO" "Daemon activo (PID: $pid)"
            return 0
        else
            echo -e "${RED}✗ Daemon no está corriendo (PID file obsoleto)${NC}"
            log_message "ERROR" "Daemon no activo - PID file obsoleto"
            return 1
        fi
    else
        echo -e "${YELLOW}⚠ Daemon no está corriendo (esto es normal si no lo has iniciado)${NC}"
        log_message "INFO" "Daemon no activo"
        return 0
    fi
}

# Verificar logs de errores del kernel
check_kernel_errors() {
    log_message "INFO" "Verificando errores del kernel..."
    
    local error_count=$(dmesg 2>/dev/null | grep -i "error\|fail" | grep -i "sd\|md\|dm" | tail -10 | wc -l)
    
    if [ $error_count -gt 0 ]; then
        echo -e "${YELLOW}⚠ Se encontraron $error_count mensajes en dmesg${NC}"
        log_message "INFO" "Mensajes en kernel: $error_count"
    else
        echo -e "${GREEN}✓ No hay mensajes críticos en kernel${NC}"
        log_message "INFO" "Sin mensajes críticos en kernel"
    fi
}

# Generar reporte de salud
generate_report() {
    local report_file="/var/log/storage_health_report_$(date +%Y%m%d_%H%M%S).txt"
    
    {
        echo "========================================="
        echo "  REPORTE DE SALUD DEL ALMACENAMIENTO"
        echo "  Fecha: $DATE"
        echo "========================================="
        echo ""
        
        echo "--- Estado del Sistema ---"
        uptime
        echo ""
        
        echo "--- Uso de Disco ---"
        df -h
        echo ""
        
        echo "--- Uso de Inodos ---"
        df -i
        echo ""
        
        if [ -f /proc/mdstat ]; then
            echo "--- Estado RAID ---"
            cat /proc/mdstat
            echo ""
        fi
        
        if command -v vgs &> /dev/null; then
            echo "--- Volume Groups ---"
            vgs 2>/dev/null || echo "No hay VGs"
            echo ""
            
            echo "--- Logical Volumes ---"
            lvs 2>/dev/null || echo "No hay LVs"
            echo ""
        fi
        
        echo "--- Procesos con Mayor I/O ---"
        if command -v iotop &> /dev/null; then
            iotop -b -n 1 2>/dev/null | head -20 || echo "iotop no disponible"
        else
            echo "iotop no instalado"
        fi
        echo ""
        
        echo "--- Mensajes del Kernel ---"
        dmesg 2>/dev/null | grep -i "error\|fail" | tail -10 || echo "Sin acceso a dmesg"
        
    } > "$report_file"
    
    echo -e "${GREEN}Reporte generado: $report_file${NC}"
    log_message "INFO" "Reporte generado: $report_file"
}

# Función principal
main() {
    echo "========================================="
    echo "  VERIFICACIÓN DE SALUD DEL SISTEMA"
    echo "  $(date)"
    echo "========================================="
    echo ""
    
    check_root
    
    local overall_status=0
    
    check_raid
    echo ""
    
    check_lvm
    echo ""
    
    check_filesystem || overall_status=1
    echo ""
    
    check_disk_space
    echo ""
    
    check_daemon
    echo ""
    
    check_kernel_errors
    echo ""
    
    generate_report
    
    echo "========================================="
    if [ $overall_status -eq 0 ]; then
        echo -e "${GREEN}✓ Estado general: SALUDABLE${NC}"
        log_message "INFO" "Health check completado: SALUDABLE"
    else
        echo -e "${YELLOW}⚠ Estado general: REVISAR (algunos filesystems están llenos)${NC}"
        log_message "WARN" "Health check completado: REVISAR"
    fi
    echo "========================================="
    
    exit $overall_status
}

# Ejecutar
main "$@"
