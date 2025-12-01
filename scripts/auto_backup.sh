#!/bin/bash
#
# auto_backup.sh - Sistema de backup automatizado
# Parte 12: Automation & Scripting
#

set -e

# Configuración
BACKUP_ROOT="/backup"
LOG_FILE="/var/log/storage_backup.log"
KEEP_BACKUPS=7
DATE=$(date '+%Y%m%d_%H%M%S')
DATE_SIMPLE=$(date '+%Y-%m-%d %H:%M:%S')

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Función de logging
log_message() {
    local level=$1
    shift
    echo "[$DATE_SIMPLE] [$level] $*" | tee -a "$LOG_FILE"
}

# Verificar root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Error: Este script debe ejecutarse como root${NC}"
        exit 1
    fi
}

# Crear estructura de directorios de backup
setup_backup_dirs() {
    log_message "INFO" "Configurando directorios de backup..."
    
    mkdir -p "$BACKUP_ROOT"/{full,incremental,snapshots,logs}
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Directorios de backup creados${NC}"
        log_message "INFO" "Directorios de backup configurados"
    else
        echo -e "${RED}✗ Error creando directorios de backup${NC}"
        log_message "ERROR" "Fallo al crear directorios de backup"
        exit 1
    fi
}

# Backup completo con tar
backup_full_tar() {
    local source_path=$1
    local backup_name="full_${DATE}.tar.gz"
    local backup_path="$BACKUP_ROOT/full/$backup_name"
    
    log_message "INFO" "Iniciando backup completo: $source_path"
    echo -e "${BLUE}→ Creando backup completo...${NC}"
    
    if tar -czf "$backup_path" -C "$(dirname "$source_path")" "$(basename "$source_path")" 2>> "$LOG_FILE"; then
        local size=$(du -h "$backup_path" | awk '{print $1}')
        echo -e "${GREEN}✓ Backup completo creado: $backup_name ($size)${NC}"
        log_message "INFO" "Backup completo exitoso: $backup_name ($size)"
        
        # Calcular checksum
        local checksum=$(sha256sum "$backup_path" | awk '{print $1}')
        echo "$checksum  $backup_name" > "$backup_path.sha256"
        log_message "INFO" "Checksum: $checksum"
        
        echo "$backup_path"
        return 0
    else
        echo -e "${RED}✗ Error en backup completo${NC}"
        log_message "ERROR" "Fallo en backup completo de $source_path"
        return 1
    fi
}

# Backup incremental con rsync
backup_incremental_rsync() {
    local source_path=$1
    local backup_name="incremental_${DATE}"
    local backup_path="$BACKUP_ROOT/incremental/$backup_name"
    
    log_message "INFO" "Iniciando backup incremental: $source_path"
    echo -e "${BLUE}→ Creando backup incremental...${NC}"
    
    if ! command -v rsync &> /dev/null; then
        echo -e "${RED}✗ rsync no está instalado${NC}"
        log_message "ERROR" "rsync no disponible"
        return 1
    fi
    
    mkdir -p "$backup_path"
    
    if rsync -av --delete "$source_path/" "$backup_path/" >> "$LOG_FILE" 2>&1; then
        local size=$(du -sh "$backup_path" | awk '{print $1}')
        echo -e "${GREEN}✓ Backup incremental creado: $backup_name ($size)${NC}"
        log_message "INFO" "Backup incremental exitoso: $backup_name ($size)"
        
        echo "$backup_path"
        return 0
    else
        echo -e "${RED}✗ Error en backup incremental${NC}"
        log_message "ERROR" "Fallo en backup incremental de $source_path"
        return 1
    fi
}

# Verificar integridad del backup
verify_backup() {
    local backup_path=$1
    
    log_message "INFO" "Verificando backup: $backup_path"
    echo -e "${BLUE}→ Verificando integridad...${NC}"
    
    if [ -f "$backup_path.sha256" ]; then
        if sha256sum -c "$backup_path.sha256" &> /dev/null; then
            echo -e "${GREEN}✓ Verificación exitosa${NC}"
            log_message "INFO" "Backup verificado: OK"
            return 0
        else
            echo -e "${RED}✗ Verificación fallida - checksum no coincide${NC}"
            log_message "ERROR" "Fallo verificación de $backup_path"
            return 1
        fi
    else
        echo -e "${YELLOW}⚠ No hay checksum para verificar${NC}"
        log_message "WARN" "Sin checksum para $backup_path"
        return 1
    fi
}

# Limpiar backups antiguos
cleanup_old_backups() {
    local keep=$KEEP_BACKUPS
    
    log_message "INFO" "Limpiando backups antiguos (mantener: $keep)"
    echo -e "${BLUE}→ Limpiando backups antiguos...${NC}"
    
    # Limpiar backups completos
    local full_count=$(ls -1 "$BACKUP_ROOT/full/" 2>/dev/null | wc -l)
    if [ $full_count -gt $keep ]; then
        ls -t "$BACKUP_ROOT/full/" | tail -n +$((keep + 1)) | while read old_backup; do
            rm -f "$BACKUP_ROOT/full/$old_backup"
            rm -f "$BACKUP_ROOT/full/$old_backup.sha256"
            echo -e "${YELLOW}  Eliminado: $old_backup${NC}"
            log_message "INFO" "Backup antiguo eliminado: $old_backup"
        done
    fi
    
    echo -e "${GREEN}✓ Limpieza completada${NC}"
}

# Listar backups existentes
list_backups() {
    echo "========================================="
    echo "  BACKUPS DISPONIBLES"
    echo "========================================="
    
    echo -e "\n${BLUE}Backups Completos:${NC}"
    if [ -d "$BACKUP_ROOT/full" ]; then
        ls -lh "$BACKUP_ROOT/full/" 2>/dev/null | grep "^-" | awk '{print "  " $9 " (" $5 ")"}'
    else
        echo "  Ninguno"
    fi
    
    echo -e "\n${BLUE}Backups Incrementales:${NC}"
    if [ -d "$BACKUP_ROOT/incremental" ]; then
        ls -ld "$BACKUP_ROOT/incremental/"*/ 2>/dev/null | awk '{print "  " $9}'
    else
        echo "  Ninguno"
    fi
    
    echo ""
}

# Restaurar backup
restore_backup() {
    local backup_file=$1
    local restore_path=$2
    
    log_message "INFO" "Restaurando backup: $backup_file a $restore_path"
    echo -e "${BLUE}→ Restaurando backup...${NC}"
    
    if [ ! -f "$backup_file" ]; then
        echo -e "${RED}✗ Backup no encontrado: $backup_file${NC}"
        log_message "ERROR" "Backup no encontrado: $backup_file"
        return 1
    fi
    
    mkdir -p "$restore_path"
    
    if tar -xzf "$backup_file" -C "$restore_path" 2>> "$LOG_FILE"; then
        echo -e "${GREEN}✓ Backup restaurado en: $restore_path${NC}"
        log_message "INFO" "Backup restaurado exitosamente en $restore_path"
        return 0
    else
        echo -e "${RED}✗ Error restaurando backup${NC}"
        log_message "ERROR" "Fallo al restaurar $backup_file"
        return 1
    fi
}

# Modo dry-run
dry_run() {
    echo "========================================="
    echo "  MODO DRY-RUN (Simulación)"
    echo "========================================="
    echo ""
    echo "Configuración:"
    echo "  - Directorio de backup: $BACKUP_ROOT"
    echo "  - Mantener últimos: $KEEP_BACKUPS backups"
    echo "  - Log file: $LOG_FILE"
    echo ""
    echo "No se realizaron cambios."
}

# Mostrar ayuda
show_help() {
    cat << HELP
Uso: $0 [OPCIONES]

Opciones:
    --full SOURCE          Crear backup completo de SOURCE
    --incremental SOURCE   Crear backup incremental de SOURCE
    --list                 Listar backups disponibles
    --restore FILE DEST    Restaurar backup FILE a DEST
    --cleanup              Limpiar backups antiguos
    --verify FILE          Verificar integridad de backup
    --dry-run              Simular sin hacer cambios
    --keep N               Mantener N backups (default: $KEEP_BACKUPS)
    --help                 Mostrar esta ayuda

Ejemplos:
    $0 --full /mnt/data
    $0 --incremental /mnt/data
    $0 --list
HELP
}

# Función principal
main() {
    check_root
    setup_backup_dirs
    
    local action=""
    local source=""
    local dest=""
    local backup_file=""
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --full)
                action="full"
                source="$2"
                shift 2
                ;;
            --incremental)
                action="incremental"
                source="$2"
                shift 2
                ;;
            --list)
                action="list"
                shift
                ;;
            --restore)
                action="restore"
                backup_file="$2"
                dest="$3"
                shift 3
                ;;
            --cleanup)
                action="cleanup"
                shift
                ;;
            --verify)
                action="verify"
                backup_file="$2"
                shift 2
                ;;
            --dry-run)
                dry_run
                exit 0
                ;;
            --keep)
                KEEP_BACKUPS="$2"
                shift 2
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                echo "Opción desconocida: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    case $action in
        full)
            backup_full_tar "$source"
            cleanup_old_backups
            ;;
        incremental)
            backup_incremental_rsync "$source"
            cleanup_old_backups
            ;;
        list)
            list_backups
            ;;
        restore)
            restore_backup "$backup_file" "$dest"
            ;;
        cleanup)
            cleanup_old_backups
            ;;
        verify)
            verify_backup "$backup_file"
            ;;
        *)
            show_help
            ;;
    esac
}

main "$@"
