#ifndef RAID_MANAGER_H
#define RAID_MANAGER_H

#include "common.h"

/* Tipos de RAID soportados */
#define RAID_LEVEL_0  0   // Striping
#define RAID_LEVEL_1  1   // Mirroring
#define RAID_LEVEL_5  5   // Striping con paridad
#define RAID_LEVEL_10 10  // Mirroring + Striping

/* Estados de RAID */
#define RAID_STATUS_ACTIVE   "active"
#define RAID_STATUS_DEGRADED "degraded"
#define RAID_STATUS_FAILED   "failed"
#define RAID_STATUS_UNKNOWN  "unknown"

/* Estructura de información de array RAID */
typedef struct {
    char name[MAX_NAME];           // e.g., "/dev/md0"
    int raid_level;                // 0, 1, 5, 10
    int num_devices;               // Número de dispositivos
    char devices[16][MAX_PATH];    // Array de rutas de dispositivos
    char status[32];               // Estado actual
    int num_failed;                // Dispositivos fallidos
    int num_active;                // Dispositivos activos
    unsigned long long size_kb;    // Tamaño total en KB
} raid_array_t;

/* Funciones principales de RAID */

/**
 * Crea un nuevo array RAID
 * @param array_name: Nombre del dispositivo RAID (e.g., /dev/md0)
 * @param level: Nivel de RAID (0, 1, 5, 10)
 * @param devices: Array de dispositivos a usar
 * @param count: Número de dispositivos
 * @return: SUCCESS o código de error
 */
int raid_create(const char *array_name, int level, char **devices, int count);

/**
 * Monitorea el estado de un array RAID
 * @param array: Estructura a llenar con información
 * @return: SUCCESS o código de error
 */
int raid_monitor(raid_array_t *array);

/**
 * Obtiene el estado de un array RAID específico
 * @param array_name: Nombre del array
 * @param array: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int raid_get_status(const char *array_name, raid_array_t *array);

/**
 * Añade un disco al array RAID
 * @param array_name: Nombre del array
 * @param device: Dispositivo a añadir
 * @return: SUCCESS o código de error
 */
int raid_add_disk(const char *array_name, const char *device);

/**
 * Marca un disco como fallido
 * @param array_name: Nombre del array
 * @param device: Dispositivo a marcar como fallido
 * @return: SUCCESS o código de error
 */
int raid_fail_disk(const char *array_name, const char *device);

/**
 * Remueve un disco del array
 * @param array_name: Nombre del array
 * @param device: Dispositivo a remover
 * @return: SUCCESS o código de error
 */
int raid_remove_disk(const char *array_name, const char *device);

/**
 * Detiene un array RAID
 * @param array_name: Nombre del array
 * @return: SUCCESS o código de error
 */
int raid_stop(const char *array_name);

/**
 * Lista todos los arrays RAID activos
 * @param arrays: Array de estructuras a llenar
 * @param max_arrays: Tamaño máximo del array
 * @param count: Número de arrays encontrados (salida)
 * @return: SUCCESS o código de error
 */
int raid_list_all(raid_array_t *arrays, int max_arrays, int *count);

/**
 * Parsea /proc/mdstat para obtener información
 * @return: SUCCESS o código de error
 */
int raid_parse_mdstat(void);

#endif /* RAID_MANAGER_H */
