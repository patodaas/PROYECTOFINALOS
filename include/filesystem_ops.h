#ifndef FILESYSTEM_OPS_H
#define FILESYSTEM_OPS_H

#include "common.h"

/* Tipos de filesystem soportados */
typedef enum {
    FS_TYPE_EXT4,
    FS_TYPE_XFS,
    FS_TYPE_BTRFS,
    FS_TYPE_UNKNOWN
} fs_type_t;

/* Información de filesystem */
typedef struct {
    char device[MAX_PATH];              // Dispositivo (e.g., /dev/vg0/lv0)
    char mount_point[MAX_PATH];         // Punto de montaje
    fs_type_t type;                     // Tipo de filesystem
    char type_str[16];                  // Tipo como string
    char options[256];                  // Opciones de montaje
    unsigned long long total_bytes;     // Espacio total
    unsigned long long used_bytes;      // Espacio usado
    unsigned long long available_bytes; // Espacio disponible
    int is_mounted;                     // 1 si está montado, 0 si no
} fs_info_t;

/* ========== Creación de Filesystems ========== */

/**
 * Crea un filesystem en un dispositivo
 * @param device: Dispositivo donde crear el FS
 * @param type: Tipo de filesystem
 * @param label: Etiqueta del filesystem (puede ser NULL)
 * @return: SUCCESS o código de error
 */
int fs_create(const char *device, fs_type_t type, const char *label);

/**
 * Crea filesystem ext4 con opciones personalizadas
 * @param device: Dispositivo
 * @param block_size: Tamaño de bloque (0 para default)
 * @param inode_ratio: Ratio de inodos (0 para default)
 * @param label: Etiqueta
 * @return: SUCCESS o código de error
 */
int fs_create_ext4_advanced(const char *device, int block_size, 
                             int inode_ratio, const char *label);

/**
 * Crea filesystem xfs con opciones personalizadas
 * @param device: Dispositivo
 * @param block_size: Tamaño de bloque (0 para default)
 * @param label: Etiqueta
 * @return: SUCCESS o código de error
 */
int fs_create_xfs_advanced(const char *device, int block_size, const char *label);

/* ========== Montaje y Desmontaje ========== */

/**
 * Monta un filesystem
 * @param device: Dispositivo a montar
 * @param mount_point: Punto de montaje
 * @param type: Tipo de filesystem
 * @param options: Opciones de montaje (puede ser NULL para defaults)
 * @return: SUCCESS o código de error
 */
int fs_mount(const char *device, const char *mount_point, 
             fs_type_t type, const char *options);

/**
 * Desmonta un filesystem
 * @param mount_point: Punto de montaje
 * @param force: 1 para forzar desmontaje, 0 para normal
 * @return: SUCCESS o código de error
 */
int fs_unmount(const char *mount_point, int force);

/**
 * Verifica si un filesystem está montado
 * @param device_or_mount: Dispositivo o punto de montaje
 * @return: 1 si está montado, 0 si no, negativo en error
 */
int fs_is_mounted(const char *device_or_mount);

/* ========== Verificación y Reparación ========== */

/**
 * Verifica la integridad de un filesystem
 * @param device: Dispositivo a verificar
 * @param type: Tipo de filesystem
 * @return: SUCCESS si está OK, código de error si hay problemas
 */
int fs_check(const char *device, fs_type_t type);

/**
 * Repara un filesystem
 * @param device: Dispositivo a reparar
 * @param type: Tipo de filesystem
 * @param auto_repair: 1 para reparación automática, 0 para interactiva
 * @return: SUCCESS o código de error
 */
int fs_repair(const char *device, fs_type_t type, int auto_repair);

/* ========== Redimensionamiento ========== */

/**
 * Redimensiona un filesystem
 * @param device: Dispositivo
 * @param type: Tipo de filesystem
 * @param new_size_mb: Nuevo tamaño en MB (0 para llenar dispositivo)
 * @return: SUCCESS o código de error
 */
int fs_resize(const char *device, fs_type_t type, 
              unsigned long long new_size_mb);

/**
 * Extiende un filesystem ext4 (debe estar montado)
 * @param device: Dispositivo
 * @return: SUCCESS o código de error
 */
int fs_resize_ext4_online(const char *device);

/**
 * Extiende un filesystem xfs (debe estar montado)
 * @param mount_point: Punto de montaje
 * @return: SUCCESS o código de error
 */
int fs_resize_xfs_online(const char *mount_point);

/* ========== Información y Estadísticas ========== */

/**
 * Obtiene información de un filesystem
 * @param device_or_mount: Dispositivo o punto de montaje
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int fs_get_info(const char *device_or_mount, fs_info_t *info);

/**
 * Lista todos los filesystems montados
 * @param fs_list: Array de estructuras a llenar
 * @param max_fs: Tamaño máximo del array
 * @param count: Número de FS encontrados (salida)
 * @return: SUCCESS o código de error
 */
int fs_list_mounted(fs_info_t *fs_list, int max_fs, int *count);

/**
 * Obtiene el uso de espacio de un filesystem
 * @param mount_point: Punto de montaje
 * @param total: Total en bytes (salida)
 * @param used: Usado en bytes (salida)
 * @param available: Disponible en bytes (salida)
 * @return: SUCCESS o código de error
 */
int fs_get_usage(const char *mount_point, unsigned long long *total,
                 unsigned long long *used, unsigned long long *available);

/* ========== Utilidades ========== */

/**
 * Convierte string a fs_type_t
 * @param type_str: String del tipo (e.g., "ext4", "xfs")
 * @return: fs_type_t correspondiente
 */
fs_type_t fs_string_to_type(const char *type_str);

/**
 * Convierte fs_type_t a string
 * @param type: Tipo de filesystem
 * @return: String del tipo
 */
const char* fs_type_to_string(fs_type_t type);

/**
 * Crea un directorio de montaje si no existe
 * @param path: Ruta del directorio
 * @return: SUCCESS o código de error
 */
int fs_create_mount_point(const char *path);

#endif /* FILESYSTEM_OPS_H */
