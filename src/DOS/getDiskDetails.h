/* getDiskDetails.h */

#ifndef GETDISKDETAILS_H
#define GETDISKDETAILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Define a boolean type for C89 compliance */
typedef int bool;
#define true 1
#define false 0

/* Structure to hold disk information */
typedef struct {
    char deviceName[64];
    char *storageUnits;
    int size;             /* In KB */
    int used;             /* In KB */
    int free;             /* In KB */
    int full;             /* Full percentage */
    bool writeProtected;  /* True if write-protected */
} DeviceInfo;

/* Function to get device information */
DeviceInfo GetDeviceInfo(const char *path);

#endif /* GETDISKDETAILS_H */
