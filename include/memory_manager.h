#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "common.h"

/* Información de memoria del sistema */
typedef struct {
    unsigned long long total_kb;       // Memoria total
    unsigned long long free_kb;        // Memoria libre
    unsigned long long available_kb;   // Memoria disponible (para procesos)
    unsigned long long cached_kb;      // Caché
    unsigned long long buffers_kb;     // Buffers
    unsigned long long swap_total_kb;  // Swap total
    unsigned long long swap_free_kb;   // Swap libre
    unsigned long long swap_used_kb;   // Swap usado
    float memory_pressure;             // Presión de memoria (0.0 a 1.0)
    float swap_usage_percent;          // % de uso de swap
} memory_info_t;

/* Información de swap individual */
typedef struct {
    char path[MAX_PATH];               // Ruta del swap
    char type[16];                     // "file" o "partition"
    unsigned long long size_kb;        // Tamaño
    unsigned long long used_kb;        // Usado
    int priority;                      // Prioridad
    int is_active;                     // 1 si está activo, 0 si no
} swap_info_t;

/* ========== Funciones de Swap ========== */

/**
 * Crea un archivo de swap
 * @param path: Ruta donde crear el archivo
 * @param size_mb: Tamaño en MB
 * @return: SUCCESS o código de error
 */
int swap_create_file(const char *path, unsigned long long size_mb);

/**
 * Prepara un dispositivo como swap
 * @param device: Dispositivo o archivo
 * @return: SUCCESS o código de error
 */
int swap_make(const char *device);

/**
 * Activa un swap
 * @param path: Ruta del swap
 * @param priority: Prioridad (-1 para default)
 * @return: SUCCESS o código de error
 */
int swap_enable(const char *path, int priority);

/**
 * Desactiva un swap
 * @param path: Ruta del swap
 * @return: SUCCESS o código de error
 */
int swap_disable(const char *path);

/**
 * Lista todos los swaps activos
 * @param swaps: Array de estructuras a llenar
 * @param max_swaps: Tamaño máximo del array
 * @param count: Número de swaps encontrados (salida)
 * @return: SUCCESS o código de error
 */
int swap_list(swap_info_t *swaps, int max_swaps, int *count);

/**
 * Remueve un archivo de swap (debe estar desactivado primero)
 * @param path: Ruta del archivo de swap
 * @return: SUCCESS o código de error
 */
int swap_remove_file(const char *path);

/* ========== Monitoreo de Memoria ========== */

/**
 * Obtiene información actual de memoria
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int memory_get_info(memory_info_t *info);

/**
 * Calcula la presión de memoria (0.0 = sin presión, 1.0 = crítico)
 * @param info: Estructura con información de memoria
 * @return: Valor de presión (0.0 a 1.0)
 */
float memory_calculate_pressure(const memory_info_t *info);

/**
 * Verifica si hay baja memoria
 * @param threshold: Umbral de memoria libre (MB)
 * @return: 1 si hay baja memoria, 0 si no, negativo en error
 */
int memory_check_low(unsigned long long threshold_mb);

/**
 * Parsea /proc/meminfo
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int memory_parse_meminfo(memory_info_t *info);

/**
 * Parsea /proc/swaps
 * @param swaps: Array de estructuras a llenar
 * @param max_swaps: Tamaño máximo del array
 * @param count: Número de swaps encontrados (salida)
 * @return: SUCCESS o código de error
 */
int memory_parse_swaps(swap_info_t *swaps, int max_swaps, int *count);

/* ========== Gestión Automática ========== */

/**
 * Crea swap automáticamente si se detecta baja memoria
 * @param threshold_mb: Umbral de memoria libre para activar
 * @param swap_size_mb: Tamaño del swap a crear
 * @param swap_path: Ruta donde crear el swap
 * @return: SUCCESS si se creó swap, 0 si no fue necesario, negativo en error
 */
int memory_auto_swap(unsigned long long threshold_mb, 
                     unsigned long long swap_size_mb,
                     const char *swap_path);

/**
 * Monitorea memoria continuamente y alerta si hay problemas
 * @param check_interval_sec: Intervalo de verificación en segundos
 * @param callback: Función a llamar cuando hay alerta (puede ser NULL)
 * @return: SUCCESS o código de error
 */
int memory_monitor_continuous(int check_interval_sec, 
                               void (*callback)(const memory_info_t *info));

/* ========== Utilidades ========== */

/**
 * Convierte KB a formato legible (KB, MB, GB)
 * @param kb: Kilobytes
 * @param buffer: Buffer de salida
 * @param size: Tamaño del buffer
 */
void memory_format_size(unsigned long long kb, char *buffer, size_t size);

/**
 * Imprime información de memoria formateada
 * @param info: Estructura con información
 */
void memory_print_info(const memory_info_t *info);

/**
 * Imprime información de swap formateada
 * @param swap: Estructura con información
 */
void memory_print_swap(const swap_info_t *swap);

#endif /* MEMORY_MANAGER_H */
