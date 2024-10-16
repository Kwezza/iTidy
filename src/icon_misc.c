#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

#include "icon_misc.h"
#include "main.h"



// Custom strdup implementation
char *StrDup(const char *src) {
    char *dup = (char *)AllocVec(strlen(src) + 1, MEMF_CLEAR | MEMF_ANY);
    if (dup) {
        strcpy(dup, src);
    }
    return dup;
}

// Manually reallocates memory by freeing and reallocating
char **ReallocVec(char **oldArray, int newSize) {
    char **newArray = (char **)AllocVec(newSize * sizeof(char *), MEMF_CLEAR | MEMF_ANY);
    if (newArray && oldArray) {
        // Copy old array contents to new array
        memcpy(newArray, oldArray, newSize * sizeof(char *));
        FreeVec(oldArray);  // Free the old memory
    }
    return newArray;
}


// Function to dynamically resize the global array if needed
void resizeLeftOutIconsArray() {
    left_out_size *= 2;
    left_out_icons = ReallocVec(left_out_icons, left_out_size);
    if (!left_out_icons) {
        fprintf(stderr, "Error reallocating memory.\n");
        exit(1);
    }
}

// Function to get the device name from a given path
// This function scans the path for the colon ':' and returns the text before it
char *getDeviceNameFromPath(const char *path) {
    char *device_name = NULL;
    char *colon_pos = strchr(path, ':');  // Find the position of the colon

    if (colon_pos) {
        // Calculate the length of the device name (up to the colon)
        int device_name_len = colon_pos - path;

        // Allocate memory for the device name (plus one for null terminator)
        device_name = (char *)AllocVec(device_name_len + 1, MEMF_CLEAR | MEMF_ANY);
        if (device_name) {
            // Copy the device name up to the colon
            strncpy(device_name, path, device_name_len);
            device_name[device_name_len] = '\0';  // Null-terminate the string
        } else {
            printf("Failed to allocate memory for device name.\n");
        }
    } else {
        printf("No colon found in the provided path: %s\n", path);
    }

    return device_name;
}

void loadLeftOutIcons(const char *file_path) {
    BPTR file;
    char *device_name;
    char backdrop_path[256];
    char buffer[256];
    char *line;
    int full_path_len;
    char *full_icon_path;

    // Get the device name from the given file path
    device_name = getDeviceNameFromPath(file_path);
    if (!device_name) {
        printf("Error: Could not determine device name for path: %s\n", file_path);
        return;
    }
    #ifdef DEBUG
        append_to_log("Getting any 'left out' icons for device: %s\n", device_name);
    #endif
    // Construct the full path to the .backdrop file (e.g., "DH0:.backdrop")
    sprintf(backdrop_path, "%s:.backdrop", device_name);

    // Open the .backdrop file
    file = Open(backdrop_path, MODE_OLDFILE);
    if (!file) {
        #ifdef DEBUG
            printf("Could not open %s\n", backdrop_path);
        #endif
        FreeVec(device_name);  // Free the device_name here after you're done using it
        return;
    }

    // Allocate initial memory for the global icon list if not already allocated
    if (!left_out_icons) {
        left_out_icons = (char **)AllocVec(left_out_size * sizeof(char *), MEMF_CLEAR | MEMF_ANY);
        if (!left_out_icons) {
            fprintf(stderr, "Error allocating memory for left_out_icons.\n");
            FreeVec(device_name);  // Free the device_name here after you're done using it
            Close(file);
            return;
        }
    }

    // Read each line in the file and store it in the global array
    while (FGets(file, buffer, sizeof(buffer))) {
        // Remove the newline character
        line = strchr(buffer, '\n');
        if (line) {
            *line = '\0';
        }

        // Resize the array if necessary
        if (left_out_count >= left_out_size) {
            resizeLeftOutIconsArray();
        }

        // Calculate the full path length and allocate memory
        full_path_len = strlen(device_name) + 1 + strlen(buffer) + 1; // Device name + ":" + buffer + null terminator
        full_icon_path = (char *)AllocVec(full_path_len, MEMF_CLEAR | MEMF_ANY);
        if (full_icon_path) {
            sprintf(full_icon_path, "%s%s.info", device_name, buffer);
            left_out_icons[left_out_count] = full_icon_path;
            #ifdef DEBUG
                append_to_log("Found left out icon: %s\n", left_out_icons[left_out_count]);
            #endif
            left_out_count++;
        } else {
            printf("Failed to allocate memory for icon path.\n");
        }
    }

    // Free the allocated memory for device_name as it's no longer needed
    FreeVec(device_name);

    Close(file);
}

int isIconLeftOut(const char *icon_path) {
    int i;

    // Iterate through the left_out_icons array
    for (i = 0; i < left_out_count; i++) {
        if (strcmp(left_out_icons[i], icon_path) == 0) {
            return TRUE;  // Icon found in the list
        }
    }

    return FALSE;  // Icon not found
}