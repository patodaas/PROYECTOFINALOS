#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/backup_engine.h"

#define TEST_SOURCE "/tmp/backup_test_source"
#define TEST_DEST "/tmp/backup_test_dest"
#define TEST_RESTORE "/tmp/backup_test_restore"

// Crear datos de prueba
int create_test_data(void) {
    printf("\n=== Creating Test Data ===\n");
    
    // Crear directorio fuente
    mkdir(TEST_SOURCE, 0755);
    
    // Crear algunos archivos de prueba
    char filepath[512];
    
    for (int i = 1; i <= 5; i++) {
        snprintf(filepath, sizeof(filepath), "%s/file%d.txt", TEST_SOURCE, i);
        FILE *fp = fopen(filepath, "w");
        if (fp) {
            fprintf(fp, "Test data for file %d\n", i);
            fprintf(fp, "This is a test backup\n");
            fprintf(fp, "Line 3\nLine 4\nLine 5\n");
            fclose(fp);
            printf("✓ Created %s\n", filepath);
        }
    }
    
    // Crear un subdirectorio
    snprintf(filepath, sizeof(filepath), "%s/subdir", TEST_SOURCE);
    mkdir(filepath, 0755);
    
    snprintf(filepath, sizeof(filepath), "%s/subdir/nested.txt", TEST_SOURCE);
    FILE *fp = fopen(filepath, "w");
    if (fp) {
        fprintf(fp, "Nested file content\n");
        fclose(fp);
        printf("✓ Created %s\n", filepath);
    }
    
    printf("✓ Test data created in %s\n", TEST_SOURCE);
    return 0;
}

void test_full_backup(void) {
    printf("\n=== Test 1: Full Backup ===\n");
    
    if (backup_create(TEST_SOURCE, TEST_DEST, BACKUP_FULL) == 0) {
        printf("✓ Full backup completed successfully\n");
    } else {
        printf("✗ Full backup failed\n");
    }
}

void test_incremental_backup(void) {
    printf("\n=== Test 2: Incremental Backup ===\n");
    
    // Modificar algunos archivos
    printf("Modifying test data...\n");
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/file1.txt", TEST_SOURCE);
    
    FILE *fp = fopen(filepath, "a");
    if (fp) {
        fprintf(fp, "Additional line added\n");
        fclose(fp);
        printf("✓ Modified %s\n", filepath);
    }
    
    // Crear nuevo archivo
    snprintf(filepath, sizeof(filepath), "%s/newfile.txt", TEST_SOURCE);
    fp = fopen(filepath, "w");
    if (fp) {
        fprintf(fp, "This is a new file\n");
        fclose(fp);
        printf("✓ Created %s\n", filepath);
    }
    
    sleep(1);
    
    // Hacer backup incremental
    if (backup_create(TEST_SOURCE, TEST_DEST, BACKUP_INCREMENTAL) == 0) {
        printf("✓ Incremental backup completed successfully\n");
    } else {
        printf("✗ Incremental backup failed\n");
    }
}

void test_backup_list(void) {
    printf("\n=== Test 3: List Backups ===\n");
    
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_list(&backups, &count) == 0) {
        printf("✓ Found %d backup(s)\n\n", count);
        
        for (int i = 0; i < count; i++) {
            const char *type_str = backups[i].type == BACKUP_FULL ? "Full" :
                                  backups[i].type == BACKUP_INCREMENTAL ? "Incremental" :
                                  "Differential";
            
            printf("Backup %d:\n", i + 1);
            printf("  ID:        %s\n", backups[i].backup_id);
            printf("  Type:      %s\n", type_str);
            printf("  Timestamp: %s", ctime(&backups[i].timestamp));
            printf("  Size:      %.2f KB\n", backups[i].size_bytes / 1024.0);
            printf("  Success:   %s\n", backups[i].success ? "Yes" : "No");
            printf("\n");
        }
        
        free(backups);
    } else {
        printf("✗ Failed to list backups\n");
    }
}

void test_backup_verify(void) {
    printf("\n=== Test 4: Verify Backup ===\n");
    
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_list(&backups, &count) == 0 && count > 0) {
        const char *backup_id = backups[0].backup_id;
        printf("Verifying backup: %s\n", backup_id);
        
        if (backup_verify(backup_id) == 0) {
            printf("✓ Backup verification passed\n");
        } else {
            printf("✗ Backup verification failed\n");
        }
        
        free(backups);
    } else {
        printf("✗ No backups available to verify\n");
    }
}

void test_backup_restore(void) {
    printf("\n=== Test 5: Restore Backup ===\n");
    
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_list(&backups, &count) == 0 && count > 0) {
        const char *backup_id = backups[0].backup_id;
        printf("Restoring backup: %s\n", backup_id);
        
        mkdir(TEST_RESTORE, 0755);
        
        if (backup_restore(backup_id, TEST_RESTORE) == 0) {
            printf("✓ Backup restored successfully to %s\n", TEST_RESTORE);
            
            // Verificar que los archivos fueron restaurados
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/file1.txt", TEST_RESTORE);
            
            struct stat st;
            if (stat(filepath, &st) == 0) {
                printf("✓ Verified restored file: %s\n", filepath);
            } else {
                printf("⚠  Warning: Could not verify restored file\n");
            }
        } else {
            printf("✗ Backup restore failed\n");
        }
        
        free(backups);
    } else {
        printf("✗ No backups available to restore\n");
    }
}

void test_backup_cleanup(void) {
    printf("\n=== Test 6: Cleanup Old Backups ===\n");
    
    backup_info_t *backups = NULL;
    int count = 0;
    
    if (backup_list(&backups, &count) == 0) {
        printf("Current backup count: %d\n", count);
        free(backups);
        
        if (count > 1) {
            printf("Keeping only the latest backup...\n");
            
            if (backup_cleanup_old(1) == 0) {
                printf("✓ Cleanup completed\n");
                
                // Verificar nuevo conteo
                if (backup_list(&backups, &count) == 0) {
                    printf("New backup count: %d\n", count);
                    free(backups);
                }
            } else {
                printf("✗ Cleanup failed\n");
            }
        } else {
            printf("Not enough backups to test cleanup\n");
        }
    }
}

void cleanup_test_data(void) {
    printf("\n=== Cleaning Up Test Data ===\n");
    
    char cmd[512];
    
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_SOURCE);
    system(cmd);
    printf("✓ Removed %s\n", TEST_SOURCE);
    
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_RESTORE);
    system(cmd);
    printf("✓ Removed %s\n", TEST_RESTORE);
    
    // Nota: No eliminamos TEST_DEST para que puedas inspeccionar los backups
    printf("ℹ  Backups preserved in %s\n", TEST_DEST);
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Backup Engine Test Suite              ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    // Verificar permisos
    if (geteuid() != 0) {
        printf("\n⚠️  Warning: Some operations may require root privileges\n");
        printf("   Run with sudo for full functionality\n\n");
    }
    
    // Inicializar backup engine
    if (backup_init(NULL) != 0) {
        fprintf(stderr, "Failed to initialize backup engine\n");
        return 1;
    }
    
    // Crear datos de prueba
    create_test_data();
    
    // Ejecutar tests
    test_full_backup();
    sleep(2);  // Esperar un poco entre backups
    
    test_incremental_backup();
    test_backup_list();
    test_backup_verify();
    test_backup_restore();
    test_backup_cleanup();
    
    // Limpiar
    cleanup_test_data();
    backup_cleanup();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Backup Tests Completed                ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Next steps:\n");
    printf("  1. Check backup database: /var/lib/storage_mgr/backups.db\n");
    printf("  2. Inspect backups in: %s\n", TEST_DEST);
    printf("  3. Try CLI: ./bin/storage_cli backup list\n");
    printf("\n");
    
    return 0;
}