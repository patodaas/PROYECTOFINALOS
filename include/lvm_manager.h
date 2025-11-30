#ifndef LVM_MANAGER_H
#define LVM_MANAGER_H

#include "common.h"

/* Estructuras de información LVM */

typedef struct {
    char pv_name[MAX_PATH];           // Nombre del PV (e.g., /dev/loop0)
    unsigned long long size_bytes;    // Tamaño total
    unsigned long long free_bytes;    // Espacio libre
    char vg_name[MAX_NAME];           // VG al que pertenece (si alguno)
    int is_allocated;                 // 1 si está en un VG, 0 si no
} pv_info_t;

typedef struct {
    char vg_name[MAX_NAME];           // Nombre del VG
    unsigned long long size_bytes;    // Tamaño total
    unsigned long long free_bytes;    // Espacio libre
    int pv_count;                     // Número de PVs
    int lv_count;                     // Número de LVs
    char pv_list[16][MAX_PATH];       // Lista de PVs en el VG
} vg_info_t;

typedef struct {
    char lv_name[MAX_NAME];           // Nombre del LV
    char vg_name[MAX_NAME];           // VG al que pertenece
    unsigned long long size_bytes;    // Tamaño
    char lv_path[MAX_PATH];           // Ruta completa (e.g., /dev/vg0/lv0)
    int is_snapshot;                  // 1 si es snapshot, 0 si no
    char origin[MAX_NAME];            // Si es snapshot, LV original
    int is_active;                    // 1 si está activo, 0 si no
} lv_info_t;

/* ========== Funciones de Physical Volume (PV) ========== */

/**
 * Crea un Physical Volume
 * @param device: Dispositivo a usar (e.g., /dev/loop0)
 * @return: SUCCESS o código de error
 */
int lvm_pv_create(const char *device);

/**
 * Lista todos los Physical Volumes
 * @param pvs: Array de estructuras a llenar
 * @param max_pvs: Tamaño máximo del array
 * @param count: Número de PVs encontrados (salida)
 * @return: SUCCESS o código de error
 */
int lvm_pv_list(pv_info_t *pvs, int max_pvs, int *count);

/**
 * Obtiene información de un PV específico
 * @param pv_name: Nombre del PV
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int lvm_pv_info(const char *pv_name, pv_info_t *info);

/**
 * Remueve un Physical Volume
 * @param pv_name: Nombre del PV
 * @return: SUCCESS o código de error
 */
int lvm_pv_remove(const char *pv_name);

/* ========== Funciones de Volume Group (VG) ========== */

/**
 * Crea un Volume Group
 * @param vg_name: Nombre del VG a crear
 * @param pvs: Array de nombres de PVs
 * @param pv_count: Número de PVs
 * @return: SUCCESS o código de error
 */
int lvm_vg_create(const char *vg_name, char **pvs, int pv_count);

/**
 * Extiende un Volume Group con más PVs
 * @param vg_name: Nombre del VG
 * @param pv: PV a añadir
 * @return: SUCCESS o código de error
 */
int lvm_vg_extend(const char *vg_name, const char *pv);

/**
 * Lista todos los Volume Groups
 * @param vgs: Array de estructuras a llenar
 * @param max_vgs: Tamaño máximo del array
 * @param count: Número de VGs encontrados (salida)
 * @return: SUCCESS o código de error
 */
int lvm_vg_list(vg_info_t *vgs, int max_vgs, int *count);

/**
 * Obtiene información de un VG específico
 * @param vg_name: Nombre del VG
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int lvm_vg_info(const char *vg_name, vg_info_t *info);

/**
 * Remueve un Volume Group
 * @param vg_name: Nombre del VG
 * @return: SUCCESS o código de error
 */
int lvm_vg_remove(const char *vg_name);

/* ========== Funciones de Logical Volume (LV) ========== */

/**
 * Crea un Logical Volume
 * @param vg_name: Nombre del VG
 * @param lv_name: Nombre del LV a crear
 * @param size_mb: Tamaño en MB
 * @return: SUCCESS o código de error
 */
int lvm_lv_create(const char *vg_name, const char *lv_name, 
                  unsigned long long size_mb);

/**
 * Extiende un Logical Volume
 * @param vg_name: Nombre del VG
 * @param lv_name: Nombre del LV
 * @param add_size_mb: Tamaño a añadir en MB
 * @return: SUCCESS o código de error
 */
int lvm_lv_extend(const char *vg_name, const char *lv_name, 
                  unsigned long long add_size_mb);

/**
 * Lista todos los Logical Volumes
 * @param lvs: Array de estructuras a llenar
 * @param max_lvs: Tamaño máximo del array
 * @param count: Número de LVs encontrados (salida)
 * @return: SUCCESS o código de error
 */
int lvm_lv_list(lv_info_t *lvs, int max_lvs, int *count);

/**
 * Obtiene información de un LV específico
 * @param vg_name: Nombre del VG
 * @param lv_name: Nombre del LV
 * @param info: Estructura a llenar
 * @return: SUCCESS o código de error
 */
int lvm_lv_info(const char *vg_name, const char *lv_name, lv_info_t *info);

/**
 * Remueve un Logical Volume
 * @param vg_name: Nombre del VG
 * @param lv_name: Nombre del LV
 * @return: SUCCESS o código de error
 */
int lvm_lv_remove(const char *vg_name, const char *lv_name);

/* ========== Funciones de Snapshot ========== */

/**
 * Crea un snapshot de un LV
 * @param vg_name: Nombre del VG
 * @param origin_lv: LV original
 * @param snapshot_name: Nombre del snapshot
 * @param size_mb: Tamaño del snapshot en MB
 * @return: SUCCESS o código de error
 */
int lvm_snapshot_create(const char *vg_name, const char *origin_lv,
                        const char *snapshot_name, unsigned long long size_mb);

/**
 * Merge un snapshot de vuelta al origen
 * @param vg_name: Nombre del VG
 * @param snapshot_name: Nombre del snapshot
 * @return: SUCCESS o código de error
 */
int lvm_snapshot_merge(const char *vg_name, const char *snapshot_name);

#endif /* LVM_MANAGER_H */
