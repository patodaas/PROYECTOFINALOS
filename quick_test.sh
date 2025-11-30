#!/bin/bash

# Quick Test Script - Storage Manager Parts 6-10
# Tests básicos para verificar que todo funciona

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════╗"
echo "║  Storage Manager - Quick Test Suite           ║"
echo "╚════════════════════════════════════════════════╝"
echo ""

# Verificar root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: Este script debe ejecutarse como root${NC}"
    echo "Usa: sudo $0"
    exit 1
fi

# Test 1: Verificar compilación
echo -e "${YELLOW}[1/8]${NC} Verificando binarios compilados..."
if [ -f "bin/storage_daemon" ] && [ -f "bin/storage_cli" ]; then
    echo -e "${GREEN}✓${NC} Binarios encontrados"
else
    echo -e "${RED}✗${NC} Binarios no encontrados. Ejecuta: make all"
    exit 1
fi

# Test 2: Verificar módulo del kernel
echo -e "${YELLOW}[2/8]${NC} Verificando módulo del kernel..."
if [ -f "kernel_module/storage_stats.ko" ]; then
    echo -e "${GREEN}✓${NC} Módulo compilado"
    
    # Intentar cargar
    rmmod storage_stats 2>/dev/null || true
    insmod kernel_module/storage_stats.ko
    
    if lsmod | grep -q storage_stats; then
        echo -e "${GREEN}✓${NC} Módulo cargado"
        
        if [ -f "/proc/storage_stats" ]; then
            echo -e "${GREEN}✓${NC} /proc/storage_stats existe"
        fi
    else
        echo -e "${RED}✗${NC} No se pudo cargar el módulo"
    fi
else
    echo -e "${RED}✗${NC} Módulo no compilado. Ejecuta: make kernel"
fi

# Test 3: Verificar directorios
echo -e "${YELLOW}[3/8]${NC} Verificando directorios..."
for dir in /var/lib/storage_mgr /backup /var/log/storage_mgr; do
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $dir existe"
    else
        mkdir -p "$dir"
        echo -e "${YELLOW}⚠${NC} Creado $dir"
    fi
done

# Test 4: Test de monitoreo
echo -e "${YELLOW}[4/8]${NC} Probando sistema de monitoreo..."
if ./bin/test_monitor > /tmp/test_monitor.log 2>&1; then
    echo -e "${GREEN}✓${NC} Test de monitoreo pasó"
else
    echo -e "${RED}✗${NC} Test de monitoreo falló (ver /tmp/test_monitor.log)"
fi

# Test 5: Test de backups
echo -e "${YELLOW}[5/8]${NC} Probando sistema de backups..."
if ./bin/test_backup > /tmp/test_backup.log 2>&1; then
    echo -e "${GREEN}✓${NC} Test de backups pasó"
else
    echo -e "${RED}✗${NC} Test de backups falló (ver /tmp/test_backup.log)"
fi

# Test 6: Iniciar daemon
echo -e "${YELLOW}[6/8]${NC} Probando daemon..."

# Matar daemon si está corriendo
if [ -f /var/run/storage_mgr.pid ]; then
    kill $(cat /var/run/storage_mgr.pid) 2>/dev/null || true
    sleep 1
fi

# Iniciar daemon en foreground con timeout
timeout 5s ./bin/storage_daemon -f > /tmp/daemon.log 2>&1 &
DAEMON_PID=$!
sleep 2

if kill -0 $DAEMON_PID 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Daemon iniciado (PID: $DAEMON_PID)"
    
    # Test 7: Test de CLI
    echo -e "${YELLOW}[7/8]${NC} Probando cliente CLI..."
    
    if ./bin/storage_cli status > /tmp/cli_test.log 2>&1; then
        echo -e "${GREEN}✓${NC} Comando CLI funcionó"
    else
        echo -e "${YELLOW}⚠${NC} CLI puede no estar comunicándose con el daemon"
    fi
    
    # Matar daemon
    kill $DAEMON_PID 2>/dev/null || true
else
    echo -e "${RED}✗${NC} Daemon no pudo iniciar"
fi

# Test 8: Verificar /proc/storage_stats
echo -e "${YELLOW}[8/8]${NC} Verificando módulo del kernel..."
if [ -f /proc/storage_stats ]; then
    echo "Contenido de /proc/storage_stats:"
    head -20 /proc/storage_stats
    echo -e "${GREEN}✓${NC} Módulo del kernel funcional"
else
    echo -e "${RED}✗${NC} /proc/storage_stats no encontrado"
fi

# Resumen
echo ""
echo "╔════════════════════════════════════════════════╗"
echo "║  Resumen de Tests                              ║"
echo "╚════════════════════════════════════════════════╝"
echo ""

# Contar éxitos/fallos (simplificado)
echo "Tests completados. Revisa los logs en /tmp/ para detalles."
echo ""
echo "Próximos pasos:"
echo "  1. Iniciar daemon: systemctl start storage_mgr"
echo "  2. Ver status: storage_cli status"
echo "  3. Probar monitor: storage_cli monitor stats sda"
echo "  4. Ver módulo kernel: cat /proc/storage_stats"
echo ""

# Limpiar
rmmod storage_stats 2>/dev/null || true

echo -e "${GREEN}Tests completados!${NC}"