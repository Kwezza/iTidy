// IControlPrefs.h

#ifndef ICONTROLPREFS_H
#define ICONTROLPREFS_H

#include <exec/types.h>  // Include necessary AmigaOS headers
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/icontrol.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <math.h>

#include "main.h"
#include "writeLog.h"

// Define the IControlPrefsDetails structure
struct IControlPrefsDetails {
    ULONG flags;
    BOOL coerceColors;
    BOOL coerceLace;
    BOOL strGadFilter;
    BOOL menuSnap;
    BOOL modePromote;
    BOOL correctRatio;
    BOOL offScrnWin;
    BOOL moreSizeGadgets;
    BOOL versioned;
    BOOL legacyLook; 
    BOOL ratio_9_7;
    BOOL ratio_9_8;
    BOOL ratio_1_1;
    BOOL ratio_8_9;
    UWORD screenTitleBarExtraHeight;
    UWORD windowTitleBarExtraHeight;
    BOOL titleBar_50;
    BOOL titleBar_67;
    BOOL titleBar_75;
    BOOL titleBar_100;
    BOOL squareProportionalLook; 
    UWORD currentLeftBarWidth;
    UWORD currentBarWidth;
    UWORD currentBarHeight;
    UWORD currentCGaugeWidth;
    UWORD currentTitleBarHeight;
    UWORD currentWindowBarHeight;
};

// Declare the fetchIControlSettings function
int fetchIControlSettings(struct IControlPrefsDetails *details);
void dumpIControlPrefs(const struct IControlPrefsDetails *prefs);

#endif // ICONTROLPREFS_H
