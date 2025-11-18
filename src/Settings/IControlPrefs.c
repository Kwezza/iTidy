// IControlPrefs.c

#include "IControlPrefs.h"
#include "get_fonts.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#if PLATFORM_AMIGA
#include <proto/diskfont.h>
#include <graphics/text.h>
#include <prefs/font.h>
#endif

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

/**
 * @brief Read specific font type from font.prefs
 * @param font_type FP_WBFONT (0), FP_SYSFONT (1), or FP_SCREENFONT (2)
 * @param name_out Buffer to receive font name (256 bytes)
 * @param size_out Pointer to receive font size
 * @param char_width_out Pointer to receive character width
 * @return TRUE if font was read successfully, FALSE otherwise
 */
static BOOL read_font_prefs(UWORD font_type, char *name_out, UWORD *size_out, UWORD *char_width_out)
{
    BPTR file;
    UBYTE buffer[8];
    ULONG chunk_id, chunk_size;
    struct FontPrefs font_prefs;
    BOOL found = FALSE;
    struct TextAttr font_attr;
    struct TextFont *opened_font;
    
    /* Try ENV: first, then ENVARC: */
    file = Open("ENV:sys/font.prefs", MODE_OLDFILE);
    if (!file) {
        file = Open("ENVARC:sys/font.prefs", MODE_OLDFILE);
    }
    if (!file) {
        return FALSE;
    }
    
    /* Skip FORM header (12 bytes) */
    if (Read(file, buffer, 12) != 12) {
        Close(file);
        return FALSE;
    }
    
    /* Read chunks until we find the font type we want */
    while (Read(file, buffer, 8) == 8) {
        chunk_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        chunk_size = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
        
        if (chunk_id == ID_FONT) {
            /* Read FontPrefs structure */
            if (Read(file, &font_prefs, sizeof(struct FontPrefs)) == sizeof(struct FontPrefs)) {
                if (font_prefs.fp_Type == font_type) {
                    /* Found the font we're looking for */
                    strncpy(name_out, font_prefs.fp_Name, 255);
                    name_out[255] = '\0';
                    *size_out = font_prefs.fp_TextAttr.ta_YSize;
                    
                    /* Open font to get actual character width */
                    font_attr.ta_Name = font_prefs.fp_Name;
                    font_attr.ta_YSize = font_prefs.fp_TextAttr.ta_YSize;
                    font_attr.ta_Style = FS_NORMAL;
                    font_attr.ta_Flags = 0;
                    
                    opened_font = OpenDiskFont(&font_attr);
                    if (opened_font) {
                        *char_width_out = opened_font->tf_XSize;
                        CloseFont(opened_font);
                    } else {
                        /* Estimate based on font size */
                        *char_width_out = *size_out;
                    }
                    
                    found = TRUE;
                    break;
                }
            }
            /* Skip any remaining chunk data */
            if (chunk_size > sizeof(struct FontPrefs)) {
                Seek(file, chunk_size - sizeof(struct FontPrefs), OFFSET_CURRENT);
            }
        } else {
            /* Skip this chunk */
            Seek(file, chunk_size + (chunk_size & 1), OFFSET_CURRENT);  /* Add padding byte if odd */
        }
    }
    
    Close(file);
    return found;
}

// Function to extract and save IControlPrefs details into the struct
static void saveIControlPrefsDetails(struct IControlPrefs *prefs, struct IControlPrefsDetails *details)
{
    uint32_t ratioMask;
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
    
    /* Read all font types from font.prefs */
    
    /* 1. System font (FP_SYSFONT) - used for ListViews */
    if (!read_font_prefs(FP_SYSFONT, details->systemFontName, 
                        &details->systemFontSize, &details->systemFontCharWidth)) {
        /* Fallback to fontPrefs global if available */
        if (fontPrefs && fontPrefs->name[0]) {
            struct TextAttr font_attr;
            struct TextFont *opened_font;
            
            strncpy(details->systemFontName, fontPrefs->name, sizeof(details->systemFontName) - 1);
            details->systemFontName[sizeof(details->systemFontName) - 1] = '\0';
            details->systemFontSize = fontPrefs->size;
            
            font_attr.ta_Name = fontPrefs->name;
            font_attr.ta_YSize = fontPrefs->size;
            font_attr.ta_Style = FS_NORMAL;
            font_attr.ta_Flags = 0;
            
            opened_font = OpenDiskFont(&font_attr);
            if (opened_font) {
                details->systemFontCharWidth = opened_font->tf_XSize;
                CloseFont(opened_font);
            } else {
                details->systemFontCharWidth = fontPrefs->size;
            }
        } else {
            /* Ultimate fallback to Topaz 8 */
            strcpy(details->systemFontName, "topaz.font");
            details->systemFontSize = 8;
            details->systemFontCharWidth = 8;
        }
    }
    
    /* 2. Workbench icon text font (FP_WBFONT) */
    if (!read_font_prefs(FP_WBFONT, details->iconTextFontName,
                        &details->iconTextFontSize, &details->iconTextFontCharWidth)) {
        /* Fallback to system font */
        strcpy(details->iconTextFontName, details->systemFontName);
        details->iconTextFontSize = details->systemFontSize;
        details->iconTextFontCharWidth = details->systemFontCharWidth;
    }
    
    /* 3. Screen text font (FP_SCREENFONT) */
    if (!read_font_prefs(FP_SCREENFONT, details->screenTextFontName,
                        &details->screenTextFontSize, &details->screenTextFontCharWidth)) {
        /* Fallback to system font */
        strcpy(details->screenTextFontName, details->systemFontName);
        details->screenTextFontSize = details->systemFontSize;
        details->screenTextFontCharWidth = details->systemFontCharWidth;
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
    details->coerceColors = FALSE;
    details->coerceLace = FALSE;
    details->strGadFilter = FALSE;
    details->menuSnap = FALSE;
    details->modePromote = FALSE;
    details->correctRatio = FALSE;
    details->offScrnWin = FALSE;
    details->moreSizeGadgets = FALSE;
    details->versioned = FALSE;
    details->legacyLook = FALSE;
    details->ratio_9_7 = true;
    details->ratio_9_8 = FALSE;
    details->ratio_1_1 = FALSE;
    details->ratio_8_9 = FALSE;
    details->screenTitleBarExtraHeight = 0;
    details->windowTitleBarExtraHeight = 0;
    details->titleBar_50 = FALSE;
    details->titleBar_67 = FALSE;
    details->titleBar_75 = FALSE;
    details->titleBar_100 = FALSE;
    details->squareProportionalLook = FALSE;
    
    /* Host stub - default fonts */
    strcpy(details->systemFontName, "topaz.font");
    details->systemFontSize = 8;
    details->systemFontCharWidth = 8;
    
    strcpy(details->iconTextFontName, "topaz.font");
    details->iconTextFontSize = 8;
    details->iconTextFontCharWidth = 8;
    
    strcpy(details->screenTextFontName, "topaz.font");
    details->screenTextFontSize = 8;
    details->screenTextFontCharWidth = 8;
    
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

    append_to_log("Font Settings:\n");
    append_to_log("  System Font (FP_SYSFONT):\n");
    append_to_log("    Name: %s, Size: %d, Char Width: %d\n", 
                  prefs->systemFontName, prefs->systemFontSize, prefs->systemFontCharWidth);
    append_to_log("  Icon Text Font (FP_WBFONT):\n");
    append_to_log("    Name: %s, Size: %d, Char Width: %d\n",
                  prefs->iconTextFontName, prefs->iconTextFontSize, prefs->iconTextFontCharWidth);
    append_to_log("  Screen Text Font (FP_SCREENFONT):\n");
    append_to_log("    Name: %s, Size: %d, Char Width: %d\n",
                  prefs->screenTextFontName, prefs->screenTextFontSize, prefs->screenTextFontCharWidth);

    append_to_log("--------------------------\n");
}
