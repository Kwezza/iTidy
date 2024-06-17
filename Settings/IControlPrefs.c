// IControlPrefs.c

#include "IControlPrefs.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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
    ULONG ratioMask;
    details->flags = prefs->ic_Flags;
    details->coerceColors = (prefs->ic_Flags & ICF_COERCE_COLORS) ? TRUE : FALSE;
    details->coerceLace = (prefs->ic_Flags & ICF_COERCE_LACE) ? TRUE : FALSE;
    details->strGadFilter = (prefs->ic_Flags & ICF_STRGAD_FILTER) ? TRUE : FALSE;
    details->menuSnap = (prefs->ic_Flags & ICF_MENUSNAP) ? TRUE : FALSE;
    details->modePromote = (prefs->ic_Flags & ICF_MODEPROMOTE) ? TRUE : FALSE;
    details->correctRatio = (prefs->ic_Flags & ICF_CORRECT_RATIO) ? TRUE : FALSE;
    details->offScrnWin = (prefs->ic_Flags & ICF_OFFSCRNWIN) ? TRUE : FALSE;
    details->moreSizeGadgets = (prefs->ic_Flags & ICF_MORESIZEGADGETS) ? TRUE : FALSE;
    details->versioned = (prefs->ic_Flags & ICF_VERSIONED) ? TRUE : FALSE;
    ratioMask = prefs->ic_Flags & ICF_RATIO_MASK;
    details->ratio_9_7 = (ratioMask == ICF_RATIO_9_7) ? TRUE : FALSE;
    details->ratio_9_8 = (ratioMask == ICF_RATIO_9_8) ? TRUE : FALSE;
    details->ratio_1_1 = (ratioMask == ICF_RATIO_1_1) ? TRUE : FALSE;
    details->ratio_8_9 = (ratioMask == ICF_RATIO_8_9) ? TRUE : FALSE;
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
        details->ratio_9_7 = FALSE;
        details->ratio_9_8 = FALSE;
        details->ratio_1_1 = FALSE;
        details->ratio_8_9 = FALSE;
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
        details->currentBarWidth = details->currentBarHeight;
    }
    else if (details->titleBar_67)
    {
        details->currentBarHeight = (UWORD)((details->currentWindowBarHeight * 67) / 100);
        details->currentBarWidth = details->currentBarHeight;
    }
    else if (details->titleBar_50)
    {
        details->currentBarHeight = (UWORD)((details->currentWindowBarHeight * 1) / 2);
        details->currentBarWidth = details->currentBarHeight;
    }
    else if (details->titleBar_100)
    {
        details->currentBarHeight = details->currentWindowBarHeight;
        details->currentBarWidth = details->currentBarHeight;
    }

}

int fetchIControlSettings(struct IControlPrefsDetails *details)
{
    struct IFFHandle *iffhandle;
    struct IControlPrefs prefs;
    struct ContextNode *cnode;
    struct StoredProperty *sp;
    LONG ifferror;
    LONG rc = RETURN_OK;
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
}
