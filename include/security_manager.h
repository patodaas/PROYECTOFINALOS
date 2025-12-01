#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include "common.h"
#include <time.h>

/* ========== Access Control Lists (ACL) ========== */

/* Estructura de entrada ACL */
typedef struct {
    char user[64];              // Nombre de usuario
    char permissions[16];       // Permisos (e.g., "rwx", "r-x")
    int is_default;             // 1 si es ACL por defecto, 0 si no
} acl_entry_t;

/**
 * Establece una ACL para un usuario en un archivo/directorio
 * @param path: Ruta del archivo/directorio
 * @param user: Nombre de usuario
 * @param perms: Permisos (formato: "rwx", "r-x", etc.)
 * @return: SUCCESS o código de error
 */
int acl_set(const char *path, const char *user, const char *perms);

/**
 * Establece una ACL por defecto para un directorio
 * @param path: Ruta del directorio
 * @param user: Nombre de usuario
 * @param perms: Permisos
 * @return: SUCCESS o código de error
 */
int acl_set_default(const char *path, const char *user, const char *perms);

/**
 * Obtiene todas las ACLs de un archivo/directorio
 * @param path: Ruta del archivo/directorio
 * @param entries: Array de entradas ACL a llenar
 * @param max_entries: Tamaño máximo del array
 * @param count: Número de entradas encontradas (salida)
 * @return: SUCCESS o código de error
 */
int acl_get(const char *path, acl_entry_t *entries, int max_entries, int *count);

/**
 * Remueve una ACL de un usuario
 * @param path: Ruta del archivo/directorio
 * @param user: Nombre de usuario
 * @return: SUCCESS o código de error
 */
int acl_remove(const char *path, const char *user);

/**
 * Remueve todas las ACLs de un archivo/directorio
 * @param path: Ruta del archivo/directorio
 * @return: SUCCESS o código de error
 */
int acl_remove_all(const char *path);

/**
 * Aplica ACLs recursivamente a un directorio
 * @param path: Ruta del directorio
 * @param user: Nombre de usuario
 * @param perms: Permisos
 * @return: SUCCESS o código de error
 */
int acl_set_recursive(const char *path, const char *user, const char *perms);

/* ========== Encriptación LUKS ========== */

/* Información de volumen encriptado */
typedef struct {
    char device[256];           // Dispositivo físico (e.g., /dev/loop0)
    char name[64];              // Nombre del mapping
    char dm_path[MAX_PATH];     // Ruta del device mapper (e.g., /dev/mapper/secure)
    char dm_name[64];           // Nombre del device mapper
    int is_open;                // 1 si está abierto, 0 si no
    char cipher[64];            // Algoritmo de cifrado
    int key_size;               // Tamaño de clave en bits
} encrypted_volume_t;

/**
 * Formatea un dispositivo con LUKS
 * @param device: Dispositivo a encriptar
 * @param passphrase: Contraseña
 * @return: SUCCESS o código de error
 */
int luks_format(const char *device, const char *passphrase);

/**
 * Formatea con opciones avanzadas
 * @param device: Dispositivo
 * @param passphrase: Contraseña
 * @param cipher: Algoritmo de cifrado (NULL para default)
 * @param key_size: Tamaño de clave en bits (0 para default)
 * @return: SUCCESS o código de error
 */
int luks_format_advanced(const char *device, const char *passphrase,
                         const char *cipher, int key_size);

/**
 * Abre un volumen LUKS
 * @param device: Dispositivo encriptado
 * @param name: Nombre para el mapping
 * @param passphrase: Contraseña
 * @return: SUCCESS o código de error
 */
int luks_open(const char *device, const char *name, const char *passphrase);

/**
 * Cierra un volumen LUKS
 * @param name: Nombre del mapping
 * @return: SUCCESS o código de error
 */
int luks_close(const char *name);

/**
 * Verifica si un dispositivo está encriptado con LUKS
 * @param device: Dispositivo a verificar
 * @return: 1 si es LUKS, 0 si no, negativo en error
 */
int luks_is_luks(const char *device);

/**
 * Obtiene información de un volumen LUKS
 * @param device: Dispositivo encriptado
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int luks_get_info(const char *device, encrypted_volume_t *info);

/**
 * Estado de un volumen LUKS
 * @param name: Nombre del mapping
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int luks_status(const char *name, encrypted_volume_t *info);

/**
 * Cambia la contraseña de un volumen LUKS
 * @param device: Dispositivo
 * @param old_pass: Contraseña actual
 * @param new_pass: Nueva contraseña
 * @return: SUCCESS o código de error
 */
int luks_change_password(const char *device, const char *old_pass, 
                         const char *new_pass);

/**
 * Lista todos los volúmenes LUKS abiertos
 * @param volumes: Array de estructuras a llenar
 * @param max_volumes: Tamaño máximo del array
 * @param count: Número de volúmenes encontrados (salida)
 * @return: SUCCESS o código de error
 */
int luks_list_open(encrypted_volume_t *volumes, int max_volumes, int *count);

/* ========== Atributos Avanzados de Archivos ========== */

/* Flags de atributos */
#define ATTR_IMMUTABLE    0x00000010  // Archivo inmutable (chattr +i)
#define ATTR_APPEND_ONLY  0x00000020  // Solo append (chattr +a)
#define ATTR_NO_DUMP      0x00000040  // No dump (chattr +d)
#define ATTR_SECURE_DEL   0x00000400  // Borrado seguro (chattr +s)

/**
 * Establece atributos de un archivo
 * @param path: Ruta del archivo
 * @param flags: Flags de atributos (OR de ATTR_*)
 * @return: SUCCESS o código de error
 */
int attr_set(const char *path, unsigned int flags);

/**
 * Remueve atributos de un archivo
 * @param path: Ruta del archivo
 * @param flags: Flags de atributos a remover
 * @return: SUCCESS o código de error
 */
int attr_unset(const char *path, unsigned int flags);

/**
 * Obtiene atributos de un archivo
 * @param path: Ruta del archivo
 * @param flags: Flags de atributos (salida)
 * @return: SUCCESS o código de error
 */
int attr_get(const char *path, unsigned int *flags);

/**
 * Obtiene atributos como string
 * @param path: Ruta del archivo
 * @param attrs: Buffer para atributos
 * @return: SUCCESS o código de error
 */
int get_attributes(const char *path, char *attrs);

/**
 * Hace un archivo inmutable
 * @param path: Ruta del archivo
 * @return: SUCCESS o código de error
 */
int attr_set_immutable(const char *path);
int set_immutable(const char *path);

/**
 * Hace un archivo de solo append
 * @param path: Ruta del archivo
 * @return: SUCCESS o código de error
 */
int attr_set_append_only(const char *path);
int set_append_only(const char *path);

/**
 * Remueve inmutabilidad de un archivo
 * @param path: Ruta del archivo
 * @return: SUCCESS o código de error
 */
int attr_unset_immutable(const char *path);
int unset_immutable(const char *path);
int unset_append_only(const char *path);

/* ========== Auditoría ========== */

/* Tipo de operación para auditoría */
typedef enum {
    AUDIT_RAID_CREATE,
    AUDIT_RAID_MODIFY,
    AUDIT_LVM_CREATE,
    AUDIT_LVM_MODIFY,
    AUDIT_FS_MOUNT,
    AUDIT_FS_UNMOUNT,
    AUDIT_ENCRYPT,
    AUDIT_DECRYPT,
    AUDIT_ACL_CHANGE,
    AUDIT_SECURITY_EVENT
} audit_operation_t;

/* Estructura de entrada de auditoría */
typedef struct {
    time_t timestamp;
    char user[64];
    char operation[128];
    char details[256];
    int success;
} audit_entry_t;

/**
 * Registra una operación en el log de auditoría
 * @param operation: Tipo de operación
 * @param user: Usuario que realizó la operación
 * @param details: Detalles de la operación
 * @return: SUCCESS o código de error
 */
int audit_log(audit_operation_t operation, const char *user, const char *details);
int audit_log(const char *operation, const char *user, const char *details);

/**
 * Obtiene entradas del log de auditoría
 * @param output: Buffer para el output
 * @param size: Tamaño del buffer
 * @param num_entries: Número de entradas a obtener (0 para todas)
 * @return: SUCCESS o código de error
 */
int audit_get_log(char *output, size_t size, int num_entries);

/**
 * Lee entradas del log de auditoría
 * @param entries: Array de entradas a llenar
 * @param max_entries: Tamaño máximo del array
 * @param count: Número de entradas encontradas (salida)
 * @return: SUCCESS o código de error
 */
int audit_read_log(audit_entry_t *entries, int max_entries, int *count);

/**
 * Limpia el log de auditoría (requiere permisos especiales)
 * @return: SUCCESS o código de error
 */
int audit_clear_log(void);

/**
 * Verifica la integridad del log de auditoría
 * @return: SUCCESS si íntegro, ERROR si manipulado
 */
int audit_verify_integrity(void);

/* ========== Utilidades ========== */

/**
 * Inicializa el gestor de seguridad
 * @return: SUCCESS o código de error
 */
int security_init(void);

/**
 * Limpia recursos del gestor de seguridad
 */
void security_cleanup(void);

/**
 * Obtiene el usuario actual
 * @param buffer: Buffer para el nombre de usuario
 * @param size: Tamaño del buffer
 * @return: SUCCESS o código de error
 */
int security_get_current_user(char *buffer, size_t size);

/**
 * Verifica si el proceso tiene privilegios de root
 * @return: 1 si es root, 0 si no
 */
int security_is_root(void);

/**
 * Convierte flags de atributos a string legible
 * @param flags: Flags de atributos
 * @param buffer: Buffer para el string
 * @param size: Tamaño del buffer
 */
void security_attrs_to_string(unsigned int flags, char *buffer, size_t size);

#endif /* SECURITY_MANAGER_H */

