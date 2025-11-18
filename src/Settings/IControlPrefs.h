// IControlPrefs.h

#ifndef ICONTROLPREFS_H
#define ICONTROLPREFS_H

#include <exec/types.h>  // Include necessary AmigaOS headers
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
/* Note: prefs/icontrol.h triggers compiler warnings from Amiga SDK system headers:
 * - "bitfield type non-portable" (lines 49-50): UBYTE bitfields, works correctly on Amiga
 * - "array of size <=0" (line 94): Flexible array member, VBCC handles this automatically
 * These warnings are from Commodore's original headers and cannot be changed. */
#include <prefs/icontrol.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <math.h>

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
    
    /* System font information (for ListViews, etc.) */
    char systemFontName[256];           /* Font name from fontPrefs (icon font) */
    UWORD systemFontSize;               /* Font size (ta_YSize) */
    UWORD systemFontCharWidth;          /* Character width (tf_XSize) from opened font */
    
    /* Workbench icon text font (from workbench.prefs) */
    char iconTextFontName[256];         /* Icon text font name */
    UWORD iconTextFontSize;             /* Icon text font size (ta_YSize) */
    UWORD iconTextFontCharWidth;        /* Icon text character width (tf_XSize) */
    
    /* Screen text font (from workbench.prefs) */
    char screenTextFontName[256];       /* Screen text font name */
    UWORD screenTextFontSize;           /* Screen text font size (ta_YSize) */
    UWORD screenTextFontCharWidth;      /* Screen text character width (tf_XSize) */
};

// Declare the fetchIControlSettings function
int fetchIControlSettings(struct IControlPrefsDetails *details);
void dumpIControlPrefs(const struct IControlPrefsDetails *prefs);

/* Global IControlPrefs instance (populated at startup) */
extern struct IControlPrefsDetails prefsIControl;

#endif // ICONTROLPREFS_H
