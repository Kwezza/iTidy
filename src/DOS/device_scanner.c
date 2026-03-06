/*
 * device_scanner.c - iTidy Device/Volume Scanner Implementation
 * Enumerates mounted volumes and probes media status
 * for Workbench Screen Manager (Backdrop Cleaner)
 *
 * Uses LockDosList/NextDosEntry for volume enumeration and
 * pr_WindowPtr suppression to avoid DOS requesters on
 * removable media without inserted disks.
 */

#include <platform/platform.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

/* Console output abstraction */
#include <console_output.h>

#include "device_scanner.h"
#include "writeLog.h"

/*------------------------------------------------------------------------*/
/* Internal Helpers                                                       */
/*------------------------------------------------------------------------*/

/**
 * Copy a BSTR (BCPL string) to a C string buffer.
 * BSTR format: first byte is length, followed by characters.
 * Access via BADDR(bstr) to get the real pointer.
 */
static void bstr_to_cstr(BSTR bstr, char *buffer, int max_len)
{
    UBYTE *bptr = (UBYTE *)BADDR(bstr);
    int len;

    if (!bptr || !buffer || max_len <= 0)
    {
        if (buffer && max_len > 0)
            buffer[0] = '\0';
        return;
    }

    len = bptr[0]; /* First byte is the length */
    if (len >= max_len)
        len = max_len - 1;

    memcpy(buffer, &bptr[1], len);
    buffer[len] = '\0';
}

/**
 * Grow device list capacity if needed.
 */
static BOOL grow_device_list(iTidy_DeviceList *list)
{
    int new_capacity;
    iTidy_DeviceInfo *new_devices;

    if (list->count < list->capacity)
        return TRUE;

    new_capacity = list->capacity > 0 ? list->capacity * 2 : 8;
    new_devices = (iTidy_DeviceInfo *)whd_malloc(
        new_capacity * sizeof(iTidy_DeviceInfo));

    if (!new_devices)
    {
        log_error(LOG_GENERAL, "Failed to grow device list to %d entries\n",
                  new_capacity);
        return FALSE;
    }

    if (list->devices && list->count > 0)
    {
        memcpy(new_devices, list->devices,
               list->count * sizeof(iTidy_DeviceInfo));
        whd_free(list->devices);
    }

    list->devices = new_devices;
    list->capacity = new_capacity;
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

void itidy_init_device_list(iTidy_DeviceList *list)
{
    if (!list)
        return;

    list->devices = NULL;
    list->count = 0;
    list->capacity = 0;
}

void itidy_free_device_list(iTidy_DeviceList *list)
{
    if (!list)
        return;

    if (list->devices)
    {
        whd_free(list->devices);
        list->devices = NULL;
    }

    list->count = 0;
    list->capacity = 0;
}

BOOL itidy_is_removable_device(const char *device_name)
{
    /* Floppy drives: DF0 - DF3 */
    if (strncmp(device_name, "DF", 2) == 0 &&
        device_name[2] >= '0' && device_name[2] <= '3' &&
        device_name[3] == '\0')
    {
        return TRUE;
    }

    /* CD-ROM drives: CD0 - CD9 */
    if (strncmp(device_name, "CD", 2) == 0 &&
        device_name[2] >= '0' && device_name[2] <= '9' &&
        device_name[3] == '\0')
    {
        return TRUE;
    }

    /* PC card / PCMCIA: PC0 */
    if (strncmp(device_name, "PC", 2) == 0 &&
        device_name[2] >= '0' && device_name[2] <= '9' &&
        device_name[3] == '\0')
    {
        return TRUE;
    }

    return FALSE;
}

iTidy_DeviceStatusCode itidy_probe_device(const char *device_name,
                                           iTidy_DeviceInfo *out)
{
    struct Process *proc;
    APTR old_window_ptr;
    char device_path[ITIDY_MAX_DEVICE_NAME + 2]; /* name + ":" + null */
    BPTR lock;
    struct InfoData *info_data;

    if (!device_name || !out)
        return ITIDY_DEV_ERROR;

    /* Build "DeviceName:" path */
    snprintf(device_path, sizeof(device_path), "%s:", device_name);

    /* Suppress DOS requesters ("Please insert volume...") */
    proc = (struct Process *)FindTask(NULL);
    old_window_ptr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;

    /* Attempt to lock the device root */
    lock = Lock((STRPTR)device_path, ACCESS_READ);

    if (lock == 0)
    {
        /* Cannot access - likely no media */
        proc->pr_WindowPtr = old_window_ptr;
        out->has_media = FALSE;
        out->status = ITIDY_DEV_NO_MEDIA;
        log_info(LOG_GENERAL, "Device %s: no media or not accessible\n",
                 device_name);
        return ITIDY_DEV_NO_MEDIA;
    }

    /* Device is accessible - get info */
    out->has_media = TRUE;
    out->status = ITIDY_DEV_OK;

    info_data = (struct InfoData *)whd_malloc(sizeof(struct InfoData));
    if (info_data)
    {
        if (Info(lock, info_data))
        {
            LONG bytes_per_block = info_data->id_BytesPerBlock;
            LONG num_blocks = info_data->id_NumBlocks;
            LONG used_blocks = info_data->id_NumBlocksUsed;

            out->total_kb = (num_blocks * bytes_per_block) / 1024;
            out->free_kb = ((num_blocks - used_blocks) * bytes_per_block) / 1024;
            out->is_write_protected =
                (info_data->id_DiskState == ID_WRITE_PROTECTED) ? TRUE : FALSE;
        }
        else
        {
            out->total_kb = 0;
            out->free_kb = 0;
            out->is_write_protected = FALSE;
        }
        whd_free(info_data);
    }
    else
    {
        out->total_kb = 0;
        out->free_kb = 0;
        out->is_write_protected = FALSE;
    }

    UnLock(lock);

    /* Restore DOS requester window pointer */
    proc->pr_WindowPtr = old_window_ptr;

    return ITIDY_DEV_OK;
}

BOOL itidy_scan_devices(iTidy_DeviceList *out)
{
    struct DosList *dos_list;
    char vol_name[ITIDY_MAX_VOLUME_NAME];

    if (!out)
        return FALSE;

    itidy_init_device_list(out);

    log_info(LOG_GENERAL, "Scanning mounted volumes...\n");

    /* Lock the DOS device list for reading volumes */
    dos_list = LockDosList(LDF_VOLUMES | LDF_READ);
    if (!dos_list)
    {
        log_error(LOG_GENERAL, "Failed to lock DOS device list\n");
        return FALSE;
    }

    /* Iterate through all mounted volumes */
    while ((dos_list = NextDosEntry(dos_list, LDF_VOLUMES)) != NULL)
    {
        iTidy_DeviceInfo dev_info;

        /* Extract volume name from BSTR */
        bstr_to_cstr(dos_list->dol_Name, vol_name, sizeof(vol_name));

        if (vol_name[0] == '\0')
            continue;

        log_info(LOG_GENERAL, "  Found volume: %s\n", vol_name);

        /* Initialize device info */
        memset(&dev_info, 0, sizeof(dev_info));
        strncpy(dev_info.device_name, vol_name,
                sizeof(dev_info.device_name) - 1);
        strncpy(dev_info.volume_name, vol_name,
                sizeof(dev_info.volume_name) - 1);

        /* Check if this is a real filesystem */
        {
            char check_path[ITIDY_MAX_DEVICE_NAME + 2];
            snprintf(check_path, sizeof(check_path), "%s:", vol_name);
            dev_info.is_filesystem = IsFileSystem((STRPTR)check_path);
        }

        if (!dev_info.is_filesystem)
        {
            dev_info.status = ITIDY_DEV_NOT_FILESYSTEM;
            log_info(LOG_GENERAL, "    -> Not a filesystem, skipping\n");
            continue;
        }

        /* Check if removable */
        dev_info.is_removable = itidy_is_removable_device(vol_name);

        /* Probe device for media and disk info */
        itidy_probe_device(vol_name, &dev_info);

        /* Add to list */
        if (!grow_device_list(out))
        {
            log_error(LOG_GENERAL, "Failed to grow device list\n");
            break;
        }

        out->devices[out->count] = dev_info;
        out->count++;

        log_info(LOG_GENERAL,
                 "    -> Status: %s, Size: %ldKB, Free: %ldKB, "
                 "Write-protected: %s, Removable: %s\n",
                 dev_info.has_media ? "OK" : "No media",
                 (long)dev_info.total_kb, (long)dev_info.free_kb,
                 dev_info.is_write_protected ? "Yes" : "No",
                 dev_info.is_removable ? "Yes" : "No");
    }

    UnLockDosList(LDF_VOLUMES | LDF_READ);

    log_info(LOG_GENERAL, "Device scan complete: %d volumes found\n",
             out->count);
    return TRUE;
}
