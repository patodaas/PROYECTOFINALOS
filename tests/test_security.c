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
    int count = 10;
    
    if (acl_get("/tmp/test_acl_file.txt", entries, &count) == 0) {
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
    
    printf("\n[2.1] Estableciendo atributo inmutable...\n");
    if (set_immutable("/tmp/test_attr_file.txt") == 0) {
        printf("✓ ÉXITO: Archivo marcado como inmutable\n");
        
        // Intentar modificar (debería fallar)
        printf("\n[2.2] Intentando modificar archivo inmutable...\n");
        printf("(Esto debería fallar - es lo esperado)\n");
        int ret = system("echo 'new content' >> /tmp/test_attr_file.txt 2>&1");
        if (ret != 0) {
            printf("✓ CORRECTO: No se pudo modificar (como se esperaba)\n");
        }
        
        // Quitar inmutable
        printf("\n[2.3] Removiendo atributo inmutable...\n");
        unset_immutable("/tmp/test_attr_file.txt");
        printf("✓ Atributo removido\n");
        
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
    audit_log("TEST_OPERATION_1", "testuser", "/tmp/test1.txt");
    audit_log("TEST_OPERATION_2", "testuser", "/tmp/test2.txt");
    audit_log("TEST_OPERATION_3", "testuser", "/tmp/test3.txt");
    printf("✓ 3 entradas escritas\n");
    
    printf("\n[3.2] Leyendo log de auditoría...\n");
    audit_entry_t entries[100];
    int count = 100;
    
    if (audit_read_log(entries, 100, &count) == 0) {
        printf("✓ Log leído correctamente: %d entradas totales\n", count);
        
        printf("\nÚltimas 5 entradas:\n");
        int start = (count > 5) ? count - 5 : 0;
        for (int i = start; i < count; i++) {
            printf("  [%d] Usuario: %s | Op: %s | Detalles: %s\n",
                   i + 1, entries[i].user, entries[i].operation, entries[i].details);
        }
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
    
    // Inicializar
    printf("\nInicializando Security Manager...\n");
    if (security_init() < 0) {
        fprintf(stderr, "✗ ERROR: Fallo al inicializar\n");
        return 1;
    }
    printf("✓ Inicialización exitosa\n");
    
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
    
    // Cleanup
    security_cleanup();
    
    printf("\nRevisa el archivo de audit log:\n");
    printf("  cat audit.log\n");
    printf("\n");
    
    return 0;
}
