/*
 * tool_cache_reports.c - Tool Cache Export/Report Implementation
 * 
 * Implements export functionality for the tool cache, generating formatted
 * text reports of validated tools and their associated files.
 */

#include "platform/platform.h"
#include "tool_cache_reports.h"
#include "../easy_request_helper.h"
#include "icon_types.h"
#include "writeLog.h"
#include "path_utilities.h"

#include <libraries/asl.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <string.h>
#include <stdio.h>

/* External global tool cache */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;

/*------------------------------------------------------------------------*/
/* Tool Export Structures and Functions                                  */
/*------------------------------------------------------------------------*/

/* Structure to hold tool entry for sorting */
typedef struct {
    char *toolName;
    char *versionString;
    BOOL exists;
    int iconCount;  /* Total number of icons using this tool */
} ToolEntry;

/* Comparison function for qsort - sorts by: exists (missing first), toolName */
static int compare_tool_entries(const void *a, const void *b)
{
    const ToolEntry *entryA = (const ToolEntry *)a;
    const ToolEntry *entryB = (const ToolEntry *)b;
    
    /* First sort by status - Missing (FALSE) before Valid (TRUE) */
    if (entryA->exists != entryB->exists)
    {
        return entryA->exists ? 1 : -1;  /* FALSE (0) comes before TRUE (1) */
    }
    
    /* Then sort by tool name */
    return strcmp(entryA->toolName, entryB->toolName);
}

/**
 * export_tool_list - Export unique tool list with status, icon count, and version
 * 
 * @param window Parent window for ASL requester and error dialogs
 * @param folder_path Current folder path being scanned (for report header)
 */
void export_tool_list(struct Window *window, const char *folder_path)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR file;
    int i, j;
    BOOL tool_already_listed;
    ToolEntry *tool_entries = NULL;
    int exported_count = 0;
    int max_tool_width;
    int valid_count;
    int missing_count;
    int tool_len;
    
    if (!window)
        return;
    
    /* Check if there's data to export */
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        ShowEasyRequest(window,
            "No Data to Export",
            "The tool cache is empty.\\nPlease scan a directory first.",
            "OK");
        return;
    }
    
    log_debug(LOG_GUI, "Menu: Export list of tools clicked\n");
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Export Tool List As...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "tools_list.txt",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\\n");
        ShowEasyRequest(window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "Exporting tool list to: %s\\n", full_path);
        
        /* Open file for writing */
        file = Open((STRPTR)full_path, MODE_NEWFILE);
        if (!file)
        {
            log_error(LOG_GUI, "Failed to create export file: %s\\n", full_path);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Export Failed",
                "Could not create export file.",
                "OK");
            return;
        }
        
        /* Allocate temporary array for unique tool entries */
        tool_entries = (ToolEntry *)whd_malloc(g_ToolCacheCount * sizeof(ToolEntry));
        if (!tool_entries)
        {
            Close(file);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Export Failed",
                "Memory allocation error.",
                "OK");
            return;
        }
        
        /* First pass: collect unique tools and find maximum tool name width */
        max_tool_width = 12; /* Minimum width for "Tool Name" header */
        valid_count = 0;
        missing_count = 0;
        
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            /* Check if this tool has already been seen */
            tool_already_listed = FALSE;
            for (j = 0; j < exported_count; j++)
            {
                if (strcmp(g_ToolCache[i].toolName, tool_entries[j].toolName) == 0)
                {
                    tool_already_listed = TRUE;
                    break;
                }
            }
            
            /* If not already listed, add it and check width */
            if (!tool_already_listed)
            {
                tool_entries[exported_count].toolName = g_ToolCache[i].toolName;
                tool_entries[exported_count].versionString = g_ToolCache[i].versionString;
                tool_entries[exported_count].exists = g_ToolCache[i].exists;
                tool_entries[exported_count].iconCount = g_ToolCache[i].fileCount;
                
                /* Track width */
                tool_len = strlen(g_ToolCache[i].toolName);
                if (tool_len > max_tool_width)
                {
                    max_tool_width = tool_len;
                }
                
                /* Track status counts */
                if (g_ToolCache[i].exists)
                {
                    valid_count++;
                }
                else
                {
                    missing_count++;
                }
                
                exported_count++;
            }
        }
        
        /* Sort entries by: Missing status, then tool name */
        qsort(tool_entries, exported_count, sizeof(ToolEntry), compare_tool_entries);
        
        /* Write header with summary */
        FPrintf(file, "Default Tool List Export\n");
        FPrintf(file, "Scanned Folder: %s\n", folder_path);
        FPrintf(file, "Total Unique Tools: %ld\n", (LONG)exported_count);
        FPrintf(file, "  Valid Tools: %ld\n", (LONG)valid_count);
        FPrintf(file, "  Missing Tools: %ld\n", (LONG)missing_count);
        FPrintf(file, "========================================\n");
        FPrintf(file, "\n");
        
        /* Write column headers */
        FPrintf(file, "Tool Name");
        for (i = strlen("Tool Name"); i < max_tool_width; i++) FPutC(file, ' ');
        FPrintf(file, "  Status      Icons  Version\n");
        
        /* Write separator line */
        for (i = 0; i < max_tool_width; i++) FPutC(file, '-');
        FPrintf(file, "  ");
        for (i = 0; i < 10; i++) FPutC(file, '-');
        FPrintf(file, "  ");
        for (i = 0; i < 5; i++) FPutC(file, '-');
        FPrintf(file, "  ");
        for (i = 0; i < 40; i++) FPutC(file, '-');
        FPrintf(file, "\n");
        
        /* Export sorted tools with status and version */
        for (i = 0; i < exported_count; i++)
        {
            /* Print tool name with proper width */
            FPrintf(file, "%s", tool_entries[i].toolName);
            for (tool_len = strlen(tool_entries[i].toolName); tool_len < max_tool_width; tool_len++)
            {
                FPutC(file, ' ');
            }
            FPrintf(file, "  ");
            
            /* Print status */
            if (tool_entries[i].exists)
            {
                FPrintf(file, "Found       ");
                
                /* Print icon count */
                FPrintf(file, "%-5ld  ", (LONG)tool_entries[i].iconCount);
                
                /* Print version if available */
                if (tool_entries[i].versionString && tool_entries[i].versionString[0] != '\0')
                {
                    FPrintf(file, "%s", tool_entries[i].versionString);
                }
                else
                {
                    FPrintf(file, "(no version)");
                }
            }
            else
            {
                FPrintf(file, "Missing     ");
                FPrintf(file, "%-5ld  ", (LONG)tool_entries[i].iconCount);
                FPrintf(file, "-");
            }
            
            FPrintf(file, "\n");
        }
        
        FPrintf(file, "\n");
        FPrintf(file, "========================================\n");
        FPrintf(file, "Total: %ld tools (%ld valid, %ld missing)\n", 
                (LONG)exported_count, (LONG)valid_count, (LONG)missing_count);
        
        /* Cleanup */
        whd_free(tool_entries);
        Close(file);
        FreeAslRequest(freq);
        
        ShowEasyRequest(window,
            "Export Successful",
            "Tool list exported successfully.",
            "OK");
        log_info(LOG_GUI, "Exported %ld unique tools to: %s\\n", (LONG)exported_count, full_path);
    }
    else
    {
        log_info(LOG_GUI, "User cancelled export operation\\n");
        FreeAslRequest(freq);
    }
}

/*------------------------------------------------------------------------*/
/* Files and Tools Export Structures and Functions                       */
/*------------------------------------------------------------------------*/

/* Structure to hold file entry for sorting */
typedef struct {
    char *toolName;
    char *filePath;
    BOOL exists;
} FileToolEntry;

/* Comparison function for qsort - sorts by: exists (missing first), toolName, filePath */
static int compare_file_tool_entries(const void *a, const void *b)
{
    const FileToolEntry *entryA = (const FileToolEntry *)a;
    const FileToolEntry *entryB = (const FileToolEntry *)b;
    int result;
    
    /* First sort by status - Missing (FALSE) before Valid (TRUE) */
    if (entryA->exists != entryB->exists)
    {
        return entryA->exists ? 1 : -1;  /* FALSE (0) comes before TRUE (1) */
    }
    
    /* Then sort by tool name */
    result = strcmp(entryA->toolName, entryB->toolName);
    if (result != 0)
    {
        return result;
    }
    
    /* Finally sort by file path */
    return strcmp(entryA->filePath, entryB->filePath);
}

/**
 * export_files_and_tools_list - Export files and their associated default tools
 * 
 * @param window Parent window for ASL requester and error dialogs
 * @param folder_path Current folder path being scanned (for report header)
 */
void export_files_and_tools_list(struct Window *window, const char *folder_path)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR file;
    int i, j;
    int exported_count;
    int total_file_count;
    int valid_count;
    int missing_count;
    int max_tool_width;
    int max_path_width;
    int tool_len, path_len;
    FileToolEntry *entries = NULL;
    
    if (!window)
        return;
    
    /* Check if there's data to export */
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        ShowEasyRequest(window,
            "No Data to Export",
            "The tool cache is empty.\nPlease scan a directory first.",
            "OK");
        return;
    }
    
    log_debug(LOG_GUI, "Menu: Export list of files and tools clicked\n");
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Export Files and Tools As...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "files_and_tools.txt",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\\n");
        ShowEasyRequest(window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "Exporting files and tools to: %s\\n", full_path);
        
        /* Open file for writing */
        file = Open((STRPTR)full_path, MODE_NEWFILE);
        if (!file)
        {
            log_error(LOG_GUI, "Failed to create export file: %s\\n", full_path);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Export Failed",
                "Could not create export file.",
                "OK");
            return;
        }
        
        /* Count total files first */
        total_file_count = 0;
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            total_file_count += g_ToolCache[i].fileCount;
        }
        
        if (total_file_count == 0)
        {
            Close(file);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "No Files to Export",
                "No file references found in cache.",
                "OK");
            return;
        }
        
        /* Allocate array for all file entries */
        entries = (FileToolEntry *)whd_malloc(total_file_count * sizeof(FileToolEntry));
        if (!entries)
        {
            Close(file);
            FreeAslRequest(freq);
            ShowEasyRequest(window,
                "Export Failed",
                "Memory allocation error.",
                "OK");
            return;
        }
        
        /* Collect all file entries and find maximum widths */
        exported_count = 0;
        max_tool_width = strlen("Default Tool");  /* Minimum for header */
        max_path_width = strlen("File Path");     /* Minimum for header */
        valid_count = 0;
        missing_count = 0;
        
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            for (j = 0; j < g_ToolCache[i].fileCount; j++)
            {
                entries[exported_count].toolName = g_ToolCache[i].toolName;
                entries[exported_count].filePath = g_ToolCache[i].referencingFiles[j];
                entries[exported_count].exists = g_ToolCache[i].exists;
                
                /* Track maximum widths */
                tool_len = strlen(g_ToolCache[i].toolName);
                if (tool_len > max_tool_width)
                {
                    max_tool_width = tool_len;
                }
                
                path_len = strlen(g_ToolCache[i].referencingFiles[j]);
                if (path_len > max_path_width)
                {
                    max_path_width = path_len;
                }
                
                /* Track status counts */
                if (g_ToolCache[i].exists)
                {
                    valid_count++;
                }
                else
                {
                    missing_count++;
                }
                
                exported_count++;
            }
        }
        
        /* Sort entries by: Missing status, tool name, file path */
        qsort(entries, exported_count, sizeof(FileToolEntry), compare_file_tool_entries);
        
        /* Write header with summary */
        FPrintf(file, "Files and Default Tools Export\n");
        FPrintf(file, "Scanned Folder: %s\n", folder_path);
        FPrintf(file, "Total Files: %ld\n", (LONG)exported_count);
        FPrintf(file, "  Valid Tools: %ld\n", (LONG)valid_count);
        FPrintf(file, "  Missing Tools: %ld\n", (LONG)missing_count);
        FPrintf(file, "========================================\n");
        FPrintf(file, "\n");
        
        /* Write column headers */
        FPrintf(file, "Default Tool");
        for (i = strlen("Default Tool"); i < max_tool_width; i++) FPutC(file, ' ');
        FPrintf(file, "  Status      File Path\n");
        
        /* Write separator line */
        for (i = 0; i < max_tool_width; i++) FPutC(file, '-');
        FPrintf(file, "  ");
        for (i = 0; i < 10; i++) FPutC(file, '-');
        FPrintf(file, "  ");
        for (i = 0; i < max_path_width; i++) FPutC(file, '-');
        FPrintf(file, "\n");
        
        /* Export sorted entries */
        for (i = 0; i < exported_count; i++)
        {
            /* Print tool name with padding */
            FPrintf(file, "%s", entries[i].toolName);
            for (tool_len = strlen(entries[i].toolName); tool_len < max_tool_width; tool_len++)
            {
                FPutC(file, ' ');
            }
            FPrintf(file, "  ");
            
            /* Print status */
            if (entries[i].exists)
            {
                FPrintf(file, "Valid       ");
            }
            else
            {
                FPrintf(file, "Missing     ");
            }
            
            /* Print file path */
            FPrintf(file, "%s\n", entries[i].filePath);
        }
        
        FPrintf(file, "\n");
        FPrintf(file, "========================================\n");
        FPrintf(file, "Total: %ld files (%ld valid, %ld missing)\n", 
                (LONG)exported_count, (LONG)valid_count, (LONG)missing_count);
        
        /* Cleanup */
        whd_free(entries);
        Close(file);
        FreeAslRequest(freq);
        
        ShowEasyRequest(window,
            "Export Successful",
            "Files and tools list exported successfully.",
            "OK");
        log_info(LOG_GUI, "Exported %ld file entries to: %s\\n", (LONG)exported_count, full_path);
    }
    else
    {
        log_info(LOG_GUI, "User cancelled export operation\\n");
        FreeAslRequest(freq);
    }
}
