/* getDiskDetails.c */

/* VBCC MIGRATION NOTE (Stage 2): Complete rewrite using native AmigaDOS API
 * 
 * Changes:
 * - Removed system() command approach entirely
 * - Implemented GetDeviceInfo() using Info() and Lock() directly
 * - Removed temporary file creation (RAM:info_output.txt)
 * - Removed text parsing with sscanf()
 * - Used struct InfoData for disk information
 * - Added proper error handling with IoErr()
 * - Used AmigaDOS types: BPTR, LONG, BOOL
 * - Used AllocVec/FreeVec for memory management
 * - Used C99 features: inline, //, mixed declarations
 */

#include "platform/platform.h"
#include "getDiskDetails.h"
#include <ctype.h>
#include <proto/exec.h>

// Helper function to determine appropriate storage unit (K, M, G) and scale value
static inline void FormatStorageSize(LONG sizeInKB, LONG *scaledSize, char *unit)
{
    if (sizeInKB >= 1024 * 1024) {
        // Gigabytes
        *scaledSize = sizeInKB / (1024 * 1024);
        *unit = 'G';
    } else if (sizeInKB >= 1024) {
        // Megabytes
        *scaledSize = sizeInKB / 1024;
        *unit = 'M';
    } else {
        // Kilobytes
        *scaledSize = sizeInKB;
        *unit = 'K';
    }
}

// Helper function to extract device name from path
static inline void ExtractDeviceName(const char *path, char *deviceName, int maxLen)
{
    int i = 0;
    
    // Copy until we hit a colon or slash, or reach max length
    while (path[i] != '\0' && path[i] != '/' && i < maxLen - 1) {
        deviceName[i] = path[i];
        i++;
    }
    
    // Add colon if the path had one
    if (path[i] == ':' && i < maxLen - 1) {
        deviceName[i] = ':';
        i++;
    }
    
    deviceName[i] = '\0';
}

/* Function to get device information using native AmigaDOS Info() API */
DeviceInfo GetDeviceInfo(const char *path)
{
    DeviceInfo deviceInfo = {0};
    struct InfoData *infoData;
    BPTR lock;
    LONG totalBytes, usedBytes, freeBytes;
    LONG totalKB, usedKB, freeKB;
    LONG scaledSize;
    char unit;
    
    // Initialize with defaults
    strncpy(deviceInfo.deviceName, "Unknown", sizeof(deviceInfo.deviceName) - 1);
    deviceInfo.deviceName[sizeof(deviceInfo.deviceName) - 1] = '\0';
    strncpy(deviceInfo.storageUnits, "K", sizeof(deviceInfo.storageUnits) - 1);
    deviceInfo.storageUnits[sizeof(deviceInfo.storageUnits) - 1] = '\0';
    
    // Allocate InfoData structure
    infoData = (struct InfoData *)whd_malloc(sizeof(struct InfoData));
    if (!infoData) {
#ifdef DEBUG
        Printf("Error: Unable to allocate InfoData structure\n");
#endif
        return deviceInfo;
    }
    memset(infoData, 0, sizeof(struct InfoData));
    
    // Lock the device/volume with shared lock
    lock = Lock(path, SHARED_LOCK);
    if (!lock) {
        LONG error = IoErr();
#ifdef DEBUG
        Printf("Error: Unable to lock path '%s' (error: %ld)\n", path, error);
#endif
        FreeVec(infoData);
        return deviceInfo;
    }
    
    // Get disk information using Info()
    if (Info(lock, infoData)) {
        // Calculate sizes in bytes
        totalBytes = infoData->id_NumBlocks * infoData->id_BytesPerBlock;
        usedBytes = infoData->id_NumBlocksUsed * infoData->id_BytesPerBlock;
        freeBytes = totalBytes - usedBytes;
        
        // Convert to kilobytes
        totalKB = totalBytes / 1024;
        usedKB = usedBytes / 1024;
        freeKB = freeBytes / 1024;
        
        // Store sizes in KB
        deviceInfo.used = usedKB;
        deviceInfo.free = freeKB;
        
        // Format size with appropriate unit (K, M, or G)
        FormatStorageSize(totalKB, &scaledSize, &unit);
        deviceInfo.size = scaledSize;
        
        // Store the unit
        deviceInfo.storageUnits[0] = unit;
        deviceInfo.storageUnits[1] = '\0';
        
        // Calculate percentage full
        if (totalBytes > 0) {
            deviceInfo.full = (LONG)((usedBytes * 100) / totalBytes);
        } else {
            deviceInfo.full = 0;
        }
        
        // Check write protection
        deviceInfo.writeProtected = (infoData->id_DiskState == ID_WRITE_PROTECTED) ? TRUE : FALSE;
        
        // Extract device name from path
        ExtractDeviceName(path, deviceInfo.deviceName, sizeof(deviceInfo.deviceName));
        
#ifdef DEBUG
        Printf("Device: %s, Size: %ld%s, Used: %ldK, Free: %ldK, Full: %ld%%, WP: %s\n",
               deviceInfo.deviceName, deviceInfo.size, deviceInfo.storageUnits,
               deviceInfo.used, deviceInfo.free, deviceInfo.full,
               deviceInfo.writeProtected ? "Yes" : "No");
#endif
    } else {
        LONG error = IoErr();
#ifdef DEBUG
        Printf("Error: Info() failed for path '%s' (error: %ld)\n", path, error);
#endif
    }
    
    // Cleanup
    UnLock(lock);
    FreeVec(infoData);
    
    return deviceInfo;
}
