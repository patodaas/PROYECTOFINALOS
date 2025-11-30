#!/bin/bash

# Script para configurar loop devices para testing
# Debe ejecutarse como root

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Este script debe ejecutarse como root${NC}"
    echo "Usa: sudo ./setup_loops.sh"
    exit 1
fi

echo -e "${YELLOW}Configurando loop devices para testing...${NC}\n"

# Limpiar configuración previa
echo "Limpiando loop devices previos..."
losetup -D 2>/dev/null || true
mdadm --stop /dev/md* 2>/dev/null || true

# Crear directorio de trabajo
mkdir -p /tmp/storage_test
cd /tmp/storage_test

# Contador de éxitos
success=0
failed=0

# Crear 6 imágenes de disco y asociarlas
for i in {0..5}; do
    echo -n "Creando disk${i}.img... "
    
    # Crear imagen
    if dd if=/dev/zero of=disk${i}.img bs=1M count=200 2>/dev/null; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${RED}FAIL${NC}"
        ((failed++))
        continue
    fi
    
    # Intentar asociar con loop device específico
    echo -n "Asociando con loop device... "
    if losetup /dev/loop${i} disk${i}.img 2>/dev/null; then
        echo -e "${GREEN}/dev/loop${i}${NC}"
        ((success++))
    else
        # Si falla, usar --find
        LOOP=$(losetup --find --show disk${i}.img 2>/dev/null)
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}${LOOP}${NC}"
            ((success++))
        else
            echo -e "${RED}FAIL${NC}"
            ((failed++))
        fi
    fi
done

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Loop Devices Configurados: ${success}/6${NC}"
echo -e "${GREEN}========================================${NC}"

if [ $success -lt 4 ]; then
    echo -e "${RED}ERROR: Se necesitan al menos 4 loop devices${NC}"
    echo "Intenta reiniciar la VM y ejecutar de nuevo"
    exit 1
fi

# Mostrar loop devices activos
echo ""
echo "Loop devices activos:"
losetup -a | grep storage_test || echo "Ninguno (¿error?)"

# Verificar que podemos acceder a los dispositivos
echo ""
echo "Verificando acceso a dispositivos:"
for i in {0..5}; do
    if [ -b "/dev/loop${i}" ]; then
        SIZE=$(blockdev --getsize64 /dev/loop${i} 2>/dev/null || echo "0")
        if [ "$SIZE" -gt 0 ]; then
            echo -e "  ${GREEN}✓${NC} /dev/loop${i} ($(numfmt --to=iec-i --suffix=B $SIZE))"
        fi
    fi
done

echo ""
echo -e "${GREEN}✓ Setup completo${NC}"
echo ""
echo "Puedes ejecutar ahora:"
echo "  sudo make test"
echo ""
echo "O para probar solo RAID:"
echo "  sudo ./build/storage_test"
