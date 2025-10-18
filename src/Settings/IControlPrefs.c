// IControlPrefs.c

#include "IControlPrefs.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#if PLATFORM_AMIGA
// Function to initialize default IControlPrefs settings
static void initializeDefaultIControlPrefs(struct IControlPrefs *prefs)
{
    prefs->ic_Flags = 0x8000001e;
    prefs->ic_Version = IC_CURRENTVERSION;
    prefs->ic_GUIGeometry[0] = 0;
    prefs->ic_GUIGeometry[1] = 0;
    prefs->ic_GUIGeometry[2] = 0;
    prefs->ic_GUIGeometry[3] = 0;
    prefs->ic_Flags |= ICF_RATIO_9_7;
    prefs->ic_Flags &= ~(ICF_RATIO_9_8 | ICF_RATIO_1_1 | ICF_RATIO_8_9);
}

// Function to extract and save IControlPrefs details into the struct
static void saveIControlPrefsDetails(struct IControlPrefs *prefs, struct IControlPrefsDetails *details)
{
    uint32_t ratioMask;
    details->flags = prefs->ic_Flags;
    details->coerceColors = (prefs->ic_Flags & ICF_COERCE_COLORS) ? true : false;
    details->coerceLace = (prefs->ic_Flags & ICF_COERCE_LACE) ? true : false;
    details->strGadFilter = (prefs->ic_Flags & ICF_STRGAD_FILTER) ? true : false;
    details->menuSnap = (prefs->ic_Flags & ICF_MENUSNAP) ? true : false;
    details->modePromote = (prefs->ic_Flags & ICF_MODEPROMOTE) ? true : false;
    details->correctRatio = (prefs->ic_Flags & ICF_CORRECT_RATIO) ? true : false;
    details->offScrnWin = (prefs->ic_Flags & ICF_OFFSCRNWIN) ? true : false;
    details->moreSizeGadgets = (prefs->ic_Flags & ICF_MORESIZEGADGETS) ? true : false;
    details->versioned = (prefs->ic_Flags & ICF_VERSIONED) ? true : false;
    ratioMask = prefs->ic_Flags & ICF_RATIO_MASK;
    details->ratio_9_7 = (ratioMask == ICF_RATIO_9_7) ? true : false;
    details->ratio_9_8 = (ratioMask == ICF_RATIO_9_8) ? true : false;
    details->ratio_1_1 = (ratioMask == ICF_RATIO_1_1) ? true : false;
    details->ratio_8_9 = (ratioMask == ICF_RATIO_8_9) ? true : false;
    details->legacyLook = !(details->correctRatio) && (ratioMask == 0x1E);
    details->screenTitleBarExtraHeight = 0;
    details->windowTitleBarExtraHeight = 0;
    if (details->versioned && prefs->ic_Version >= IC_CURRENTVERSION)
    {
        details->screenTitleBarExtraHeight = prefs->ic_GUIGeometry[0];
        details->windowTitleBarExtraHeight = prefs->ic_GUIGeometry[1];
    }
    details->titleBar_50 = (prefs->ic_GUIGeometry[3] == 0x40);
    details->titleBar_67 = (prefs->ic_GUIGeometry[3] == 0x55);
    details->titleBar_75 = (prefs->ic_GUIGeometry[3] == 0x60);
    details->titleBar_100 = (prefs->ic_GUIGeometry[3] == 0x80);
    details->squareProportionalLook = details->titleBar_50 || details->titleBar_67 || details->titleBar_75 || details->titleBar_100;
    if (details->squareProportionalLook)
    {
        details->ratio_9_7 = false;
        details->ratio_9_8 = false;
        details->ratio_1_1 = false;
        details->ratio_8_9 = false;
    }

    if (details->legacyLook)
    {
        /* stick to default previously setup*/
    }
    else if (details->ratio_9_7 == TRUE)
    {
        details->currentBarHeight = 14;
    }
    else if (details->ratio_9_8 == TRUE)
    {
        details->currentBarHeight = 16;
    }
    else if (details->ratio_1_1 == TRUE)
    {
        details->currentBarHeight = 18;
    }
    else if (details->ratio_8_9 == TRUE)
    {
        /* stick to default previously setup- default legacy?*/
    }

    if (details->screenTitleBarExtraHeight > 0)
    {
        details->currentBarHeight = details->currentBarHeight + details->screenTitleBarExtraHeight;
    }
    if (details->windowTitleBarExtraHeight > 0)
    {
        details->currentWindowBarHeight = details->currentWindowBarHeight + details->windowTitleBarExtraHeight;
    }

    if (details->titleBar_75)
    {
        details->currentBarHeight = (UWORD)((details->currentWindowBarHeight * 3) / 4);
    }
    else if (details->titleBar_67)
    {
        details->currentBarHeight = (uint16_t)((details->currentWindowBarHeight * 67) / 100);
    }
    else if (details->titleBar_50)
    {
        details->currentBarHeight = (uint16_t)((details->currentWindowBarHeight * 1) / 2);
    }
    else if (details->titleBar_100)
    {
        details->currentBarHeight = details->currentWindowBarHeight;
    }

}
#endif

int fetchIControlSettings(struct IControlPrefsDetails *details)
{
#if PLATFORM_AMIGA
    struct IFFHandle *iffhandle;
    struct IControlPrefs prefs;
    struct ContextNode *cnode;
    struct StoredProperty *sp;
    int32_t ifferror;
    int32_t rc = RETURN_OK;
    struct Library *IFFParseBase = OpenLibrary("iffparse.library", 0);

    /*  set some defaults for workbench 3.2 at least */
    details->currentBarWidth = 18;
    details->currentBarHeight = 10;
    details->currentCGaugeWidth = 19;
    details->currentTitleBarHeight = 16;
    details->currentWindowBarHeight = 16;
    details->currentLeftBarWidth = 4;

    initializeDefaultIControlPrefs(&prefs);
    if (IFFParseBase != NULL)
    {
        iffhandle = AllocIFF();
        if (iffhandle != NULL)
        {
            iffhandle->iff_Stream = (ULONG)Open("ENV:sys/icontrol.prefs", MODE_OLDFILE);
            if (iffhandle->iff_Stream != 0)
            {
                InitIFFasDOS(iffhandle);
                if ((ifferror = OpenIFF(iffhandle, IFFF_READ)) == 0)
                {
                    PropChunk(iffhandle, ID_PREF, ID_ICTL);
                    for (;;)
                    {
                        ifferror = ParseIFF(iffhandle, IFFPARSE_STEP);
                        if (ifferror == IFFERR_EOC)
                            continue;
                        else if (ifferror)
                            break;
                        cnode = CurrentChunk(iffhandle);
                        if (cnode && cnode->cn_Type == ID_PREF && cnode->cn_ID == ID_ICTL)
                        {
                            sp = FindProp(iffhandle, ID_PREF, ID_ICTL);
                            if (sp)
                            {
                                memcpy(&prefs, sp->sp_Data, sizeof(struct IControlPrefs));
                                saveIControlPrefsDetails(&prefs, details);
                            }
                        }
                    }
                    CloseIFF(iffhandle);
                }
                else
                {
                    printf("Error opening IFF file: %ld\n", ifferror);
                    rc = RETURN_FAIL;
                }
                Close((BPTR)iffhandle->iff_Stream);
            }
            else
            {
                saveIControlPrefsDetails(&prefs, details);
            }
            FreeIFF(iffhandle);
        }
        else
        {
            printf("Unable to allocate IFF handle\n");
            rc = RETURN_FAIL;
        }
        CloseLibrary(IFFParseBase);
    }
    else
    {
        printf("Unable to open IFFParse library\n");
        rc = RETURN_FAIL;
    }
    return rc;
#else
    /* Host stub - set default values */
    details->currentBarWidth = 18;
    details->currentBarHeight = 10;
    details->currentCGaugeWidth = 19;
    details->currentTitleBarHeight = 16;
    details->currentWindowBarHeight = 16;
    details->currentLeftBarWidth = 4;
    details->flags = 0x8000001e;
    details->coerceColors = false;
    details->coerceLace = false;
    details->strGadFilter = false;
    details->menuSnap = false;
    details->modePromote = false;
    details->correctRatio = false;
    details->offScrnWin = false;
    details->moreSizeGadgets = false;
    details->versioned = false;
    details->legacyLook = false;
    details->ratio_9_7 = true;
    details->ratio_9_8 = false;
    details->ratio_1_1 = false;
    details->ratio_8_9 = false;
    details->screenTitleBarExtraHeight = 0;
    details->windowTitleBarExtraHeight = 0;
    details->titleBar_50 = false;
    details->titleBar_67 = false;
    details->titleBar_75 = false;
    details->titleBar_100 = false;
    details->squareProportionalLook = false;
    return 0;
#endif
}

void dumpIControlPrefs(const struct IControlPrefsDetails *prefs) {
    append_to_log("IControl Preferences Dump:\n");
    append_to_log("--------------------------\n");
    append_to_log("Flags: 0x%08X\n", prefs->flags);

    append_to_log("Boolean Settings:\n");
    append_to_log("  Coerce Colors: %s\n", prefs->coerceColors ? "Yes" : "No");
    append_to_log("  Coerce Lace: %s\n", prefs->coerceLace ? "Yes" : "No");
    append_to_log("  String Gadget Filter: %s\n", prefs->strGadFilter ? "Yes" : "No");
    append_to_log("  Menu Snap: %s\n", prefs->menuSnap ? "Yes" : "No");
    append_to_log("  Mode Promote: %s\n", prefs->modePromote ? "Yes" : "No");
    append_to_log("  Correct Ratio: %s\n", prefs->correctRatio ? "Yes" : "No");
    append_to_log("  Off-Screen Windows: %s\n", prefs->offScrnWin ? "Yes" : "No");
    append_to_log("  More Size Gadgets: %s\n", prefs->moreSizeGadgets ? "Yes" : "No");
    append_to_log("  Versioned: %s\n", prefs->versioned ? "Yes" : "No");
    append_to_log("  Legacy Look: %s\n", prefs->legacyLook ? "Yes" : "No");

    append_to_log("Aspect Ratios Enabled:\n");
    if (prefs->ratio_9_7) append_to_log("  9:7\n");
    if (prefs->ratio_9_8) append_to_log("  9:8\n");
    if (prefs->ratio_1_1) append_to_log("  1:1\n");
    if (prefs->ratio_8_9) append_to_log("  8:9\n");

    append_to_log("Title Bar Heights:\n");
    append_to_log("  Screen Extra Height: %d\n", prefs->screenTitleBarExtraHeight);
    append_to_log("  Window Extra Height: %d\n", prefs->windowTitleBarExtraHeight);

    append_to_log("Title Bar Settings:\n");
    append_to_log("  Title Bar 50: %s\n", prefs->titleBar_50 ? "Yes" : "No");
    append_to_log("  Title Bar 67: %s\n", prefs->titleBar_67 ? "Yes" : "No");
    append_to_log("  Title Bar 75: %s\n", prefs->titleBar_75 ? "Yes" : "No");
    append_to_log("  Title Bar 100: %s\n", prefs->titleBar_100 ? "Yes" : "No");

    append_to_log("Square Proportional Look: %s\n", prefs->squareProportionalLook ? "Yes" : "No");

    append_to_log("UI Element Dimensions:\n");
    append_to_log("  Left Bar Width: %d\n", prefs->currentLeftBarWidth);
    append_to_log("  Border Width: %d, Border Height: %d\n", prefs->currentBarWidth, prefs->currentBarHeight);
    append_to_log("  Title Height: %d, Window Height: %d\n", prefs->currentTitleBarHeight, prefs->currentWindowBarHeight);
    append_to_log("  Volume Gauge Width: %d\n", prefs->currentCGaugeWidth);

    append_to_log("--------------------------\n");
}

