
#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/wb.h>
#include <proto/diskfont.h> // Include diskfont library
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <exec/types.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>

#include <ctype.h>

#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"

// Define global variables
struct Screen *screen = NULL;
struct Window *window = NULL;
struct RastPort *rastPort = NULL;
struct TextFont *font = NULL;
struct WorkbenchSettings prefsWorkbench;
struct IControlPrefsDetails prefsIControl;
int screenHight = 0;
int screenWidth = 0;

int main(int argc, char **argv)
{
    char filePath[256];
    int i;
    BOOL iterateDIRs = FALSE;
    user_dontResize=FALSE;
    user_folderViewMode=DDVM_BYICON;
    user_folderFlags=DDFLAGS_SHOWICONS;

    if (argc < 2)
    {
        Printf("Usage: %s <directory> -iterateDIRs -dontResize -folderViewShowAll -folderViewDefault\n", argv[0]);
        return RETURN_FAIL;
    }
    strcpy(filePath, argv[1]);
    sanitizeAmigaPath(filePath);
    if (does_file_or_folder_exist(filePath,0) == FALSE)
    {
        Printf("The directory %s does not exist\n", filePath);
        return RETURN_FAIL;
    }

    for (i = 0; i < argc; i++)
    {
        if (strncasecmp_custom(argv[i], "-iterateDIRs", strlen(argv[i])) == 0)
            iterateDIRs = TRUE;
        if (strncasecmp_custom(argv[i], "-dontResize", strlen(argv[i])) == 0)
            user_dontResize = TRUE;
                if (strncasecmp_custom(argv[i], "-folderViewShowAll", strlen(argv[i])) == 0)
            user_folderViewMode=DDFLAGS_SHOWALL;
                if (strncasecmp_custom(argv[i], "-folderViewDefault", strlen(argv[i])) == 0)
            user_folderViewMode=DDFLAGS_SHOWDEFAULT;
    }

    fetchWorkbenchSettings(&prefsWorkbench);
    fetchIControlSettings(&prefsIControl);

#ifdef DEBUG
    printf("IControl prefs:  border width: %d, border height: %d\n", prefsIControl.currentBarWidth, prefsIControl.currentBarHeight);
    printf("IControl prefs:  title height: %d, Window height: %d\n", prefsIControl.currentTitleBarHeight, prefsIControl.currentWindowBarHeight);
    printf("IControl prefs:  volume guage width: %d\n", prefsIControl.currentCGaugeWidth);
#endif

    InitializeWindow();
    ProcessDirectory(argv[1], FALSE);
    CleanupWindow();
    return RETURN_OK;
}





