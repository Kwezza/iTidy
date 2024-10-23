/* getDiskDetails.c */

#include "getDiskDetails.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define TEMP_FILE "RAM:info_output.txt"

/* Function to execute the "info" command and save the output to a temporary file */
static void GetInfoOutput(const char *deviceOrVolume) {
    char command[256];
    int len;

    /* Prepare the command string */
    strcpy(command, "info ");
    len = strlen(command);

    /* Ensure that we don't overflow the buffer */
    if (len + strlen(deviceOrVolume) + strlen(TEMP_FILE) + 4 < sizeof(command)) {
        /* Append the device or volume name and the redirection to the temporary file */
        strcat(command, deviceOrVolume);
        strcat(command, " > ");
        strcat(command, TEMP_FILE);

        /* Execute the command */
        system(command);
    } else {
        printf("Error: Command string too long\n");
    }
}

/* Function to parse the "info" output file and extract relevant information into the structure */
DeviceInfo GetDeviceInfo(const char *path) {
    FILE *file;
    char line[256];
    DeviceInfo deviceInfo = {0};
    char status[64]; /* For storing read/write status */
    int errors;
    char sizeUnit = '\0';  /* For storing whether the size is in K, M, or G */
    int usedSpace = 0, freeSpace = 0;
    char *statusPos;
    int numParsed;

    strcpy(deviceInfo.deviceName, "Unknown");

    /* Get info from the specified path */
    GetInfoOutput(path);

    /* Open the temporary file in RAM: */
    file = fopen(TEMP_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open temporary file %s\n", TEMP_FILE);
        return deviceInfo;
    }

    /* Read through the file and find the relevant line */
    while (fgets(line, sizeof(line), file)) {
        /* Locate the position of "Read Only" or "Read/Write" in the line */
        if ((statusPos = strstr(line, "Read Only")) != NULL) {
            strcpy(status, "Read Only");
            deviceInfo.writeProtected = true;
        } else if ((statusPos = strstr(line, "Read/Write")) != NULL) {
            strcpy(status, "Read/Write");
            deviceInfo.writeProtected = false;
        } else {
            continue;
        }

        /* Extract size, used, free, and full percentage */
        numParsed = sscanf(line, "%s %d%c %d %d %d%% %d", deviceInfo.deviceName,
                           &deviceInfo.size, &sizeUnit, &usedSpace, &freeSpace,
                           &deviceInfo.full, &errors);

        if (numParsed < 7) {
            fprintf(stderr, "Error: Failed to parse the info output.\n");
            fclose(file);
            remove(TEMP_FILE);
            return deviceInfo;
        }

        /* Adjust size based on unit (K, M, or G) */
        sizeUnit = toupper((unsigned char)sizeUnit);
        strcpy(sizeUnit,deviceInfo.storageUnits);
        /*
        if (sizeUnit == 'M') {
            deviceInfo.size *= 1024; 
        } else if (sizeUnit == 'G') {
            deviceInfo.size *= 1024 * 1024; 
        } else if (sizeUnit != 'K') {
            fprintf(stderr, "Error: Unknown size unit '%c'.\n", sizeUnit);
        }*/

        deviceInfo.used = usedSpace;
        deviceInfo.free = freeSpace;

        /* We have successfully extracted the data, so we can break out of the loop */
        break;
    }

    /* Close and delete the temporary file */
    fclose(file);
    remove(TEMP_FILE);

    return deviceInfo;
}
