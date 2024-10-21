#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

#include "icon_misc.h"
#include "main.h"



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
            printf("Device name truncated to fit buffer.\n");
        }
    } else {
        printf("No colon found in the provided path: %s\n", path);
        device_name[0] = '\0';  /* Set device_name to an empty string */
    }
}

void loadLeftOutIcons(const char *file_path) {
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
        printf("Error: Could not determine device name for path: %s\n", file_path);
        return;
    }
#ifdef DEBUG
    append_to_log("Getting any 'left out' icons for device: %s\n", device_name);
#endif

    /* Construct the full path to the .backdrop file (e.g., "DH0:.backdrop") */
    if (strlen(device_name) + 10 <= sizeof(backdrop_path)) {  /* 10 for ":.backdrop" + null terminator */
        sprintf(backdrop_path, "%s:.backdrop", device_name);
    } else {
        printf("Backdrop path too long.\n");
        return;
    }

    /* Open the .backdrop file */
    file = Open(backdrop_path, MODE_OLDFILE);
    if (!file) {
#ifdef DEBUG
        printf("Could not open %s\n", backdrop_path);
#endif
        return;
    }

    /* Initialize the left_out_icons array to empty strings */
    for (i = 0; i < MAX_LEFT_OUT_ICONS; i++) {
        left_out_icons[i][0] = '\0';
    }

    /* Read each line in the file and store it in the array */
    i = 0;
    while (FGets(file, buffer, sizeof(buffer))) {
        /* Remove the newline character */
        line = strchr(buffer, '\n');
        if (line) {
            *line = '\0';
        }

        /* Check if we have reached the maximum number of icons */
        if (i >= MAX_LEFT_OUT_ICONS) {
            break;
        }

        /* Construct the full icon path */
        /* The full icon path is device_name + ":" + buffer + ".info" */
        len = strlen(device_name) + 1 + strlen(buffer) + 5 + 1; /* device_name + ":" + buffer + ".info" + null terminator */

        if (len <= MAX_PATH_LENGTH) {
            sprintf(left_out_icons[i], "%s:%s.info", device_name, buffer);
#ifdef DEBUG
            append_to_log("Found left out icon: %s\n", left_out_icons[i]);
#endif
            i++;
        } else {
            printf("Icon path too long, skipping.\n");
        }
    }

    Close(file);
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
            return TRUE;  /* Icon found in the list */
        }
    }

    return FALSE;  /* Icon not found */
}
