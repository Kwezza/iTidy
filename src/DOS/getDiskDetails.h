/* getDiskDetails.h */

#ifndef GETDISKDETAILS_H
#define GETDISKDETAILS_H

/* VBCC MIGRATION NOTE (Stage 2): Migrated to native AmigaDOS API
 * 
 * Changes:
 * - Removed custom bool typedef (use AmigaDOS BOOL instead)
 * - Added proper AmigaDOS includes
 * - Changed to use native Info() API instead of system() command
 * - Removed temporary file approach
 */

#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <string.h>
#include <stdio.h>

/* Structure to hold disk information */
typedef struct {
    char deviceName[64];
    char storageUnits[4];    // Changed from pointer to fixed array
    LONG size;               // In KB (AmigaDOS LONG type)
    LONG used;               // In KB (AmigaDOS LONG type)
    LONG free;               // In KB (AmigaDOS LONG type)
    LONG full;               // Full percentage (AmigaDOS LONG type)
    BOOL writeProtected;     // AmigaDOS BOOL (TRUE/FALSE)
} DeviceInfo;

/* Function to get device information using native AmigaDOS Info() API */
DeviceInfo GetDeviceInfo(const char *path);

#endif /* GETDISKDETAILS_H */
