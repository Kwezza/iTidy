#include "spinner.h"
#include <stdio.h>
#include <exec/memory.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <clib/timer_protos.h>
#include <clib/exec_protos.h>
#include <proto/dos.h>
#include "writeLog.h"
#include "icon_misc.h"

// Global variables
struct Device* TimerBase;
struct timerequest timereq;
ULONG lastUpdateTime = 0;
UBYTE currentCursor = 0;
const char cursorChars[] = {'/', '-', '\\', '|'};

ULONG timer(void) {
    struct timeval t;
    GetSysTime(&t);
    return t.tv_secs * 1000 + t.tv_micro / 1000;
    //return 0;
}

void updateCursor(void) {
    ULONG currentTime = timer();

#ifdef DEBUG
return;
#endif

    if ((currentTime - lastUpdateTime) >= 750) {
        // One second has passed, update the cursor
        lastUpdateTime = currentTime;
        currentCursor = (currentCursor + 1) % 4;

        // Print the current cursor character
        printf("\r  %c\r", cursorChars[currentCursor]);
        fflush(stdout);
    }
}

int setupTimer(void) {
    // Open the timer device
    if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)&timereq, 0) != 0) {
        printf("Failed to open timer.device\n");
        return 1;
    }
    TimerBase = timereq.tr_node.io_Device;

    // Initialize the timer
    lastUpdateTime = timer();
    return 0;
}

void disposeTimer(void) {
    // Close the timer device
    CloseDevice((struct IORequest *)&timereq);
}
