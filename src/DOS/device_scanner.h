/*
 * device_scanner.h - iTidy Device/Volume Scanner
 * Enumerates mounted volumes and probes media status
 * for Workbench Screen Manager (Backdrop Cleaner)
 */

#ifndef ITIDY_DEVICE_SCANNER_H
#define ITIDY_DEVICE_SCANNER_H

#include <exec/types.h>
#include <dos/dos.h>

/*------------------------------------------------------------------------*/
/* Constants                                                              */
/*------------------------------------------------------------------------*/
#define ITIDY_MAX_DEVICE_NAME   64
#define ITIDY_MAX_VOLUME_NAME   64
#define ITIDY_MAX_DEVICES       32

/*------------------------------------------------------------------------*/
/* Device Status Codes                                                    */
/*------------------------------------------------------------------------*/
typedef enum {
    ITIDY_DEV_OK = 0,           /* Device accessible, has media */
    ITIDY_DEV_NO_MEDIA,         /* Device exists but no media inserted */
    ITIDY_DEV_NOT_FILESYSTEM,   /* Not a filesystem device */
    ITIDY_DEV_ERROR             /* General error accessing device */
} iTidy_DeviceStatusCode;

/*------------------------------------------------------------------------*/
/* Device Information Structure                                           */
/*------------------------------------------------------------------------*/
typedef struct {
    char device_name[ITIDY_MAX_DEVICE_NAME];    /* e.g. "Workbench" */
    char volume_name[ITIDY_MAX_VOLUME_NAME];    /* Volume label if available */
    BOOL is_filesystem;                          /* TRUE if real filesystem */
    BOOL has_media;                              /* TRUE if media present */
    BOOL is_write_protected;                     /* TRUE if read-only */
    BOOL is_removable;                           /* TRUE if removable media */
    LONG total_kb;                               /* Total size in KB */
    LONG free_kb;                                /* Free space in KB */
    iTidy_DeviceStatusCode status;               /* Probe result */
} iTidy_DeviceInfo;

/*------------------------------------------------------------------------*/
/* Device List Structure                                                  */
/*------------------------------------------------------------------------*/
typedef struct {
    iTidy_DeviceInfo *devices;  /* Dynamic array (whd_malloc) */
    int count;                  /* Number of devices found */
    int capacity;               /* Allocated capacity */
} iTidy_DeviceList;

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Scan all mounted volumes on the system
 *
 * Uses LockDosList/NextDosEntry to enumerate all volumes.
 * Probes each device for media status, suppressing DOS requesters.
 *
 * @param out Device list to populate (caller provides, function fills)
 * @return BOOL TRUE if scan succeeded, FALSE on error
 */
BOOL itidy_scan_devices(iTidy_DeviceList *out);

/**
 * @brief Probe a single device for media and status
 *
 * Suppresses DOS requesters (pr_WindowPtr = -1) before Lock() attempt.
 * Safe to call on removable media without triggering insert-disk requesters.
 *
 * @param device_name Device name without colon (e.g. "DF0")
 * @param out Device info to populate
 * @return iTidy_DeviceStatusCode status result
 */
iTidy_DeviceStatusCode itidy_probe_device(const char *device_name,
                                           iTidy_DeviceInfo *out);

/**
 * @brief Check if a device is likely removable media
 *
 * Uses heuristics (DF0-DF3, CD0-CD9, etc.) to classify.
 *
 * @param device_name Device name without colon
 * @return BOOL TRUE if device appears to be removable
 */
BOOL itidy_is_removable_device(const char *device_name);

/**
 * @brief Free device list resources
 *
 * @param list Device list to free (does not free the struct itself)
 */
void itidy_free_device_list(iTidy_DeviceList *list);

/**
 * @brief Initialize an empty device list
 *
 * @param list Device list to initialize
 */
void itidy_init_device_list(iTidy_DeviceList *list);

#endif /* ITIDY_DEVICE_SCANNER_H */
