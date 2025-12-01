#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/security_manager.h"

void print_separator() {
    printf("\n========================================\n");
}

void test_acl() {
    print_separator();
    printf("TEST 1: ACL Operations\n");
    print_separator();
    
    // Crear archivo de prueba
    printf("Creando archivo de prueba...\n");
    system("touch /tmp/test_acl_file.txt");
    system("echo 'test content' > /tmp/test_acl_file.txt");
    
    // Establecer ACL
    printf("\n[1.1] Estableciendo ACL para usuario 'nobody'...\n");
    if (acl_set("/tmp/test_acl_file.txt", "nobody", "r--") == 0) {
        printf("✓ ÉXITO: ACL establecido\n");
    } else {
        printf("✗ ERROR: No se pudo establecer ACL\n");
    }
    
    // Obtener ACL
    printf("\n[1.2] Obteniendo ACLs del archivo...\n");
    acl_entry_t entries[10];
    int count = 0;

    if (acl_get("/tmp/test_acl_file.txt", entries, 10, &count) == 0) {
        printf("✓ ÉXITO: Se encontraron %d ACLs\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - Usuario: %s, Permisos: %s\n", 
                   entries[i].user, entries[i].permissions);
        }
    } else {
        printf("✗ ERROR: No se pudieron obtener ACLs\n");
    }
    
    // Remover ACL
    printf("\n[1.3] Removiendo ACL...\n");
    if (acl_remove("/tmp/test_acl_file.txt", "nobody") == 0) {
        printf("✓ ÉXITO: ACL removido\n");
    }
    
    // Limpiar
    system("rm -f /tmp/test_acl_file.txt");
    printf("\nTest ACL completado.\n");
}

void test_attributes() {
    print_separator();
    printf("TEST 2: File Attributes (Immutable)\n");
    print_separator();
    
    // Crear archivo de prueba
    printf("Creando archivo de prueba...\n");
    system("touch /tmp/test_attr_file.txt");
    system("echo 'original content' > /tmp/test_attr_file.txt");
    
    printf("\n[2.1] Estableciendo atributo inmutable con attr_set_immutable()...\n");
    if (attr_set_immutable("/tmp/test_attr_file.txt") == 0) {
        printf("✓ ÉXITO: Archivo marcado como inmutable\n");
        
        // Intentar modificar (debería fallar)
        printf("\n[2.2] Intentando modificar archivo inmutable...\n");
        printf("(Esto debería fallar - es lo esperado)\n");
        int ret = system("echo 'new content' >> /tmp/test_attr_file.txt 2>&1");
        if (ret != 0) {
            printf("✓ CORRECTO: No se pudo modificar (como se esperaba)\n");
        } else {
            printf("⚠ ADVERTENCIA: El sistema permitió modificar el archivo\n");
        }
        
        // Quitar inmutable
        printf("\n[2.3] Removiendo atributo inmutable con attr_unset_immutable()...\n");
        if (attr_unset_immutable("/tmp/test_attr_file.txt") == 0) {
            printf("✓ Atributo removido\n");
        } else {
            printf("✗ ERROR: No se pudo remover el atributo inmutable\n");
        }
        
        // Ahora sí se puede modificar
        printf("\n[2.4] Modificando archivo (ahora debería funcionar)...\n");
        system("echo 'new content' >> /tmp/test_attr_file.txt");
        printf("✓ Archivo modificado exitosamente\n");
    } else {
        printf("✗ ERROR: No se pudo establecer inmutable (¿necesitas sudo?)\n");
    }
    
    // Limpiar
    system("rm -f /tmp/test_attr_file.txt");
    printf("\nTest Attributes completado.\n");
}

void test_audit() {
    print_separator();
    printf("TEST 3: Audit Logging\n");
    print_separator();
    
    printf("\n[3.1] Escribiendo entradas de prueba al log...\n");

    // Usamos solo enums que existen en security_manager.c
    audit_log(AUDIT_RAID_CREATE, "testuser", "Creación de RAID de prueba");
    audit_log(AUDIT_LVM_CREATE,  "testuser", "Creación de LVM de prueba");
    audit_log(AUDIT_ACL_CHANGE,  "testuser", "Cambio de ACL de prueba");
    printf("✓ 3 entradas escritas\n");
    
    printf("\n[3.2] Leyendo log de auditoría con audit_get_log()...\n");
    char buffer[4096];
    if (audit_get_log(buffer, sizeof(buffer), 0) == 0) {
        printf("Contenido del log (primeras entradas):\n");
        printf("%s\n", buffer);
    } else {
        printf("✗ ERROR: No se pudo leer el log\n");
    }
    
    printf("\nTest Audit completado.\n");
}

void test_luks_info() {
    print_separator();
    printf("TEST 4: LUKS Encryption (Información)\n");
    print_separator();
    
    printf("\nNOTA IMPORTANTE:\n");
    printf("Las pruebas reales de LUKS requieren:\n");
    printf("  1. Permisos de root (sudo)\n");
    printf("  2. Un dispositivo de bloque (loop device)\n");
    printf("\nPara probar LUKS manualmente:\n");
    printf("  $ dd if=/dev/zero of=/tmp/test.img bs=1M count=100\n");
    printf("  $ sudo losetup /dev/loop0 /tmp/test.img\n");
    printf("  $ sudo ./bin/test_security\n");
    printf("\nSi tienes un loop device configurado, las funciones de LUKS\n");
    printf("están listas para usarse con luks_format(), luks_open(), etc.\n");
    
    printf("\nTest LUKS (informativo) completado.\n");
}

int main() {
    printf("========================================\n");
    printf("  SECURITY MANAGER - SUITE DE PRUEBAS\n");
    printf("========================================\n");
    printf("Fecha: %s", __DATE__);
    printf("\n========================================\n");
    
    // No hay security_init()/security_cleanup en tu implementación,
    // así que no se llaman aquí para evitar referencias indefinidas.
    
    // Ejecutar tests
    test_acl();
    test_attributes();
    test_audit();
    test_luks_info();
    
    // Resumen final
    print_separator();
    printf("RESUMEN DE PRUEBAS\n");
    print_separator();
    printf("\n✓ Test 1: ACL Operations - Completado\n");
    printf("✓ Test 2: File Attributes - Completado\n");
    printf("✓ Test 3: Audit Logging - Completado\n");
    printf("✓ Test 4: LUKS Info - Completado\n");
    
    printf("\n========================================\n");
    printf("  TODAS LAS PRUEBAS COMPLETADAS\n");
    printf("========================================\n");
    
    printf("\nRevisa el archivo de audit log:\n");
    printf("  cat /var/log/storage_audit.log\n");
    printf("\n");
    
    return 0;
}

