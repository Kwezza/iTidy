#include <platform/platform.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>

#include "icon_misc.h"
#include "itidy_types.h"
#include "dos/getDiskDetails.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>


char left_out_icons[MAX_LEFT_OUT_ICONS][MAX_PATH_LENGTH];

void getDeviceNameFromPath(const char *path, char *device_name, int max_length) {
    const char *colon_pos;
    int device_name_len;

    /* Find the position of the colon */
    colon_pos = strchr(path, ':');

    if (colon_pos) {
        /* Calculate the length of the device name (up to the colon) */
        device_name_len = colon_pos - path;

        /* Ensure device name fits within the provided buffer */
        if (device_name_len < max_length) {
            strncpy(device_name, path, device_name_len);
            device_name[device_name_len] = '\0';  /* Null-terminate the string */
        } else {
            /* Truncate if necessary */
            strncpy(device_name, path, max_length - 1);
            device_name[max_length - 1] = '\0';
            CONSOLE_WARNING("Device name truncated to fit buffer.\n");
        }
    } else {
        CONSOLE_ERROR("No colon found in the provided path: %s\n", path);
        device_name[0] = '\0';  /* Set device_name to an empty string */
    }
}

void loadLeftOutIcons(const char *file_path) {
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for file handle */
    BPTR file;
    char device_name[MAX_DEVICE_NAME_LENGTH];
    char backdrop_path[256];
    char buffer[256];
    char *line;
    int i;
    int len;

    /* Get the device name from the given file path */
    getDeviceNameFromPath(file_path, device_name, MAX_DEVICE_NAME_LENGTH);
    if (device_name[0] == '\0') {
        CONSOLE_ERROR("Could not determine device name for path: %s\n", file_path);
        return;
    }

    /* Construct the full path to the .backdrop file at the device root
     * The .backdrop file is always at the root of the device (e.g., "Workbench:.backdrop")
     * It contains paths to all left-out icons from anywhere on that device */
    if (strlen(device_name) + 11 <= sizeof(backdrop_path)) {  /* 11 for ":.backdrop" + null terminator */
        sprintf(backdrop_path, "%s:.backdrop", device_name);
    } else {
        CONSOLE_ERROR("Backdrop path too long.\n");
        return;
    }

    append_to_log("========================================\n");
    append_to_log("Loading 'left out' icons for device: %s\n", device_name);
    append_to_log("Looking for .backdrop file: %s\n", backdrop_path);
    
    /* Open the .backdrop file */
    file = Open(backdrop_path, MODE_OLDFILE);
    if (!file) {
        append_to_log("  --> .backdrop file not found or could not be opened\n");
        append_to_log("  --> No left-out icons to protect on this device\n");
        append_to_log("========================================\n");
        return;
    }
    
    append_to_log("  --> .backdrop file opened successfully\n");

    /* Initialize the left_out_icons array to empty strings */
    for (i = 0; i < MAX_LEFT_OUT_ICONS; i++) {
        left_out_icons[i][0] = '\0';
    }

    append_to_log("  --> Reading .backdrop file contents:\n");
    
    /* Read each line in the file and store it in the array */
    i = 0;
#if PLATFORM_AMIGA
    while (FGets(file, buffer, sizeof(buffer))) {
#else
    while (fgets(buffer, sizeof(buffer), (FILE*)file)) {
#endif
        /* Remove the newline character */
        line = strchr(buffer, '\n');
        if (line) {
            *line = '\0';
        }

        /* Check if we have reached the maximum number of icons */
        if (i >= MAX_LEFT_OUT_ICONS) {
            break;
        }

        /* Construct the full icon path from the .backdrop entry
         * .backdrop entries are in the format ":Path/To/Icon" (relative to device root)
         * We need to replace the leading ":" with "DeviceName:" to get the full path
         * Example: ":Prefs/ScreenMode" becomes "Workbench:Prefs/ScreenMode"
         */
        if (buffer[0] == ':') {
            /* Entry starts with ':', replace it with device name */
            /* Full path is device_name + buffer (where buffer starts with ':') + ".info" */
            len = strlen(device_name) + strlen(buffer) + 5 + 1; /* device_name + buffer + ".info" + null terminator */
            if (len <= MAX_PATH_LENGTH) {
                sprintf(left_out_icons[i], "%s%s.info", device_name, buffer);
                append_to_log("      [%d] %s (from .backdrop entry: '%s')\n", i, left_out_icons[i], buffer);
                i++;
            } else {
                append_to_log("      Icon path too long, skipping: %s\n", buffer);
            }
        } else {
            /* Entry doesn't start with ':', this is unexpected but handle it */
            append_to_log("      WARNING: Unexpected .backdrop entry format (missing leading ':'): '%s'\n", buffer);
        }
    }

    Close(file);
    
    append_to_log("  --> Total left-out icons loaded: %d\n", i);
    append_to_log("========================================\n");
}

int isIconLeftOut(const char *icon_path) {
    int i;

    /* Iterate through the left_out_icons array */
    for (i = 0; i < MAX_LEFT_OUT_ICONS; i++) {
        /* Stop if we reach an empty string */
        if (left_out_icons[i][0] == '\0') {
            break;
        }
        if (strcmp(left_out_icons[i], icon_path) == 0) {
            append_to_log("*** PROTECTED: Icon '%s' is left out on desktop - SKIPPING ***\n", icon_path);
            return TRUE;  /* Icon found in the list */
        }
    }

    return FALSE;  /* Icon not found */
}

void dumpLeftOutIcons(void) {

#ifdef DEBUG_MAX
int i;
    for (i = 0; i < MAX_LEFT_OUT_ICONS; i++) {
        if (left_out_icons[i][0] != '\0') {
            append_to_log("Left out icon: %s\n", left_out_icons[i]);
        }
    }
    #endif
}

int countLeftOutIcons(void) {
    int i;
    int count = 0;

    for (i = 0; i < MAX_LEFT_OUT_ICONS; i++) {
        if (left_out_icons[i][0] != '\0') {
            count++;
        }
    }

    return count;
}
