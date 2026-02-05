/*
 * DefIcons Creator - Test Program
 * 
 * Purpose: Create .info files for iconless files using DefIcons templates
 * Queries DefIcons port, finds appropriate template, and copies to create .info file
 * 
 * Usage: deficons_creator <folder_path>
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/icon.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/rexxsyslib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* DefIcons preference file constants */
#define DEFICONS_PREFS_HEADER_SIZE 32
#define MAX_TYPE_NODES 500
#define MAX_TYPE_NAME 64
#define READ_BUFFER_SIZE 4096
#define COPY_BUFFER_SIZE 8192

/* Action codes from deficons.prefs */
#define ACT_END              0
#define ACT_MATCH            1
#define ACT_SEARCH           2
#define ACT_SEARCHSKIPSPACES 3
#define ACT_FILESIZE         4
#define ACT_NAMEPATTERN      5
#define ACT_PROTECTION       6
#define ACT_OR               7
#define ACT_ISASCII          8
#define ACT_MACROCLASS       20

/* Hierarchy codes */
#define TYPE_DOWN_LEVEL 1
#define TYPE_UP_LEVEL   2
#define TYPE_END        0

/* Type node structure */
typedef struct TypeNode {
    char name[MAX_TYPE_NAME];
    int parent_index;  /* Index of parent node, -1 if none */
    int level;         /* Depth in hierarchy */
} TypeNode;

/* Global state */
static TypeNode type_nodes[MAX_TYPE_NODES];
static int type_node_count = 0;

/* Library bases */
struct Library *IconBase = NULL;
struct RxsLib *RexxSysBase = NULL;
extern struct DosLibrary *DOSBase;  /* Provided by startup code */

/* DefIcons port handle */
static struct MsgPort *deficons_port = NULL;

/* Statistics */
static int icons_created = 0;
static int icons_skipped = 0;
static int icons_failed = 0;
static int icons_filtered = 0;

/* Stack size for this program */
#ifdef __VBCC__
long __stack = 80000L;  /* 80KB stack */
#endif

/* Forward declarations */
static BOOL parse_deficons_prefs(void);
static int find_type_index(const char *type_name);
static const char* get_parent_type(const char *type_name);
static BOOL query_deficons(const char *filepath, char *result, int result_size);
static BOOL find_icon_template(const char *type_token, char *template_path, int path_size, BOOL *is_exact);
static BOOL copy_icon_file(const char *source, const char *dest);
static BOOL should_create_icon_for_type(const char *type_token);
static BOOL identify_file_type(const char *filepath, char *type_token, int token_size);
static BOOL create_icon_with_type(const char *filepath, const char *type_token);
static void process_directory(const char *path);
static void print_usage(void);

/*
 * Setup: Open required libraries and check DefIcons port
 */
static BOOL setup(void)
{
    IconBase = OpenLibrary("icon.library", 0);
    if (!IconBase) {
        printf("ERROR: Cannot open icon.library\n");
        return FALSE;
    }
    
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (!RexxSysBase) {
        printf("ERROR: Cannot open rexxsyslib.library\n");
        return FALSE;
    }
    
    /* Check if DefIcons is running */
    Forbid();
    deficons_port = FindPort("DEFICONS");
    Permit();
    
    if (!deficons_port) {
        printf("ERROR: DefIcons port not found - DefIcons must be running!\n");
        return FALSE;
    }
    
    /* Parse deficons.prefs to build type hierarchy */
    if (!parse_deficons_prefs()) {
        printf("ERROR: Could not parse deficons.prefs - cannot proceed\n");
        return FALSE;
    }
    
    printf("Loaded %d file type definitions from deficons.prefs\n\n", type_node_count);
    
    return TRUE;
}

/*
 * Cleanup: Close libraries
 */
static void cleanup(void)
{
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    if (IconBase) CloseLibrary(IconBase);
}

/*
 * Parse deficons.prefs binary file and build type hierarchy
 * Returns TRUE on success, FALSE on error
 */
static BOOL parse_deficons_prefs(void)
{
    BPTR file;
    UBYTE buffer[READ_BUFFER_SIZE];
    int bytes_read;
    int buffer_pos = 0;
    int current_level = 0;
    int parent_stack[32];  /* Stack of parent indices by level */
    int stack_ptr = 0;
    char type_name[MAX_TYPE_NAME];
    int name_len;
    UBYTE action, byte_val;
    int i;
    LONG len;
    
    type_node_count = 0;
    
    /* Try multiple locations - ENV: and ENVARC:, with/without Sys/, case variations */
    file = Open("ENV:Sys/deficons.prefs", MODE_OLDFILE);
    if (!file) file = Open("ENV:DefIcons.prefs", MODE_OLDFILE);
    if (!file) file = Open("ENV:deficons.prefs", MODE_OLDFILE);
    if (!file) file = Open("ENVARC:Sys/deficons.prefs", MODE_OLDFILE);
    if (!file) file = Open("ENVARC:DefIcons.prefs", MODE_OLDFILE);
    if (!file) file = Open("ENVARC:deficons.prefs", MODE_OLDFILE);
    
    if (!file) {
        return FALSE;
    }
    
    /* Skip 32-byte header */
    if (Seek(file, DEFICONS_PREFS_HEADER_SIZE, OFFSET_BEGINNING) < 0) {
        Close(file);
        return FALSE;
    }
    
    /* Read entire file into buffer */
    bytes_read = Read(file, buffer, READ_BUFFER_SIZE);
    Close(file);
    
    if (bytes_read <= 0) {
        return FALSE;
    }
    
    /* Initialize parent stack - starts with no parent (root level) */
    parent_stack[0] = -1;  /* Root level types have no parent */
    stack_ptr = 0;
    current_level = 0;
    
    /* Helper macro to read next byte */
    #define READ_BYTE() (buffer_pos < bytes_read ? buffer[buffer_pos++] : 0)
    
    /* Parse type entries */
    while (buffer_pos < bytes_read && type_node_count < MAX_TYPE_NODES) {
        /* Read type name (null-terminated string) */
        name_len = 0;
        while (buffer_pos < bytes_read && (byte_val = READ_BYTE()) != 0 && name_len < MAX_TYPE_NAME - 1) {
            type_name[name_len++] = byte_val;
        }
        type_name[name_len] = '\0';
        
        if (name_len == 0) {
            /* Empty name (null byte as first char) means TYPE_END - end of database */
            break;
        }
        
        /* Copy to node structure and set parent from current stack position */
        strncpy(type_nodes[type_node_count].name, type_name, MAX_TYPE_NAME - 1);
        type_nodes[type_node_count].name[MAX_TYPE_NAME - 1] = '\0';
        type_nodes[type_node_count].parent_index = parent_stack[stack_ptr];
        type_nodes[type_node_count].level = current_level;
        
        /* Skip action codes until ACT_END */
        while (buffer_pos < bytes_read) {
            action = READ_BYTE();
            
            if (action == ACT_END) {
                break;
            } else if (action == ACT_MATCH) {
                /* Skip: offset(2), length(1), data(length) */
                READ_BYTE(); /* offset high */
                READ_BYTE(); /* offset low */
                len = (LONG)(BYTE)READ_BYTE(); /* length (signed) */
                if (len < 0) len = -len;
                for (i = 0; i < len; i++) READ_BYTE();
            } else if (action == ACT_SEARCH || action == ACT_SEARCHSKIPSPACES) {
                /* Skip: length(1), data(length) */
                len = (LONG)(BYTE)READ_BYTE(); /* length (signed) */
                if (len < 0) len = -len;
                for (i = 0; i < len; i++) READ_BYTE();
            } else if (action == ACT_FILESIZE) {
                /* Skip: size(4 bytes) */
                for (i = 0; i < 4; i++) READ_BYTE();
            } else if (action == ACT_NAMEPATTERN) {
                /* Skip: null-terminated string */
                while (buffer_pos < bytes_read && READ_BYTE() != 0) ;
            } else if (action == ACT_PROTECTION) {
                /* Skip: mask(4) + protbits(4) = 8 bytes */
                for (i = 0; i < 8; i++) READ_BYTE();
            } else if (action == ACT_OR || action == ACT_ISASCII || action == ACT_MACROCLASS) {
                /* No parameters */
            }
        }
        
        /* Read hierarchy byte */
        if (buffer_pos >= bytes_read) break;
        UBYTE hierarchy = READ_BYTE();
        
        /* Increment type count BEFORE processing hierarchy */
        type_node_count++;
        
        /* Process hierarchy code */
        if (hierarchy == TYPE_DOWN_LEVEL) {
            /* Next type is child of the node we just added */
            if (stack_ptr < 31) {
                parent_stack[++stack_ptr] = type_node_count - 1;  /* Index of node just added */
                current_level++;
            }
        } else if (hierarchy == TYPE_UP_LEVEL) {
            /* Move up one level */
            if (stack_ptr > 0) {
                stack_ptr--;
                current_level--;
            }
        } else if (hierarchy == TYPE_END) {
            /* End of type definitions */
            break;
        } else {
            /* Not a recognized hierarchy code - must be start of next type name */
            /* Put byte back so it can be read as first char of type name */
            buffer_pos--;
        }
    }
    
    #undef READ_BYTE
    
    return (type_node_count > 0);
}

/*
 * Find type node index by name
 */
static int find_type_index(const char *type_name)
{
    int i;
    for (i = 0; i < type_node_count; i++) {
        if (strcmp(type_nodes[i].name, type_name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Get parent type name for a given type
 * Returns NULL if no parent or type not found
 */
static const char* get_parent_type(const char *type_name)
{
    int index = find_type_index(type_name);
    if (index < 0) return NULL;
    
    int parent_idx = type_nodes[index].parent_index;
    if (parent_idx < 0) return NULL;
    
    return type_nodes[parent_idx].name;
}

/*
 * Query DefIcons port to identify a file
 * Returns TRUE and fills result buffer with type token on success
 */
static BOOL query_deficons(const char *filepath, char *result, int result_size)
{
    struct MsgPort *reply_port = NULL;
    struct RexxMsg *rxmsg = NULL;
    struct RexxMsg *reply = NULL;
    char command[512];
    BOOL success = FALSE;
    
    if (!deficons_port || !RexxSysBase) {
        return FALSE;
    }
    
    /* Create reply port */
    reply_port = CreateMsgPort();
    if (!reply_port) {
        return FALSE;
    }
    
    /* Build Identify command */
    snprintf(command, sizeof(command), "Identify \"%s\"", filepath);
    
    /* Create REXX message */
    rxmsg = CreateRexxMsg(reply_port, NULL, NULL);
    if (rxmsg) {
        rxmsg->rm_Args[0] = CreateArgstring(command, strlen(command));
        if (rxmsg->rm_Args[0]) {
            rxmsg->rm_Action = RXFF_RESULT;
            
            /* Send message */
            PutMsg(deficons_port, (struct Message *)rxmsg);
            
            /* Wait for reply */
            WaitPort(reply_port);
            reply = (struct RexxMsg *)GetMsg(reply_port);
            
            if (reply && reply->rm_Result1 == 0 && reply->rm_Result2) {
                /* Success - copy result string */
                strncpy(result, (char *)reply->rm_Result2, result_size - 1);
                result[result_size - 1] = '\0';
                success = TRUE;
                
                /* Delete result string */
                DeleteArgstring((STRPTR)reply->rm_Result2);
            }
            
            DeleteArgstring(rxmsg->rm_Args[0]);
        }
        DeleteRexxMsg(rxmsg);
    }
    
    DeleteMsgPort(reply_port);
    return success;
}

/*
 * Find icon template for a given type token
 * Walks parent chain if exact match not found
 * Returns TRUE if template found, fills template_path and is_exact flag
 */
static BOOL find_icon_template(const char *type_token, char *template_path, int path_size, BOOL *is_exact)
{
    char test_path[512];
    BPTR lock;
    const char *current_type = type_token;
    int attempts = 0;
    const int max_attempts = 20;  /* Prevent infinite loops */
    
    *is_exact = TRUE;
    
    while (current_type && attempts < max_attempts) {
        /* Try multiple locations for def_<type>.info */
        /* ENV:Sys/ */
        snprintf(test_path, sizeof(test_path), "ENV:Sys/def_%s.info", current_type);
        lock = Lock(test_path, ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strncpy(template_path, test_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            return TRUE;
        }
        
        /* ENV: root */
        snprintf(test_path, sizeof(test_path), "ENV:def_%s.info", current_type);
        lock = Lock(test_path, ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strncpy(template_path, test_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            return TRUE;
        }
        
        /* ENVARC:Sys/ */
        snprintf(test_path, sizeof(test_path), "ENVARC:Sys/def_%s.info", current_type);
        lock = Lock(test_path, ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strncpy(template_path, test_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            return TRUE;
        }
        
        /* ENVARC: root */
        snprintf(test_path, sizeof(test_path), "ENVARC:def_%s.info", current_type);
        lock = Lock(test_path, ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strncpy(template_path, test_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            return TRUE;
        }
        
        /* Not found - try parent type */
        current_type = get_parent_type(current_type);
        *is_exact = FALSE;
        attempts++;
    }
    
    return FALSE;
}

/*
 * Copy icon file from source to destination
 * Returns TRUE on success, FALSE on error
 */
static BOOL copy_icon_file(const char *source, const char *dest)
{
    BPTR src_file, dest_file;
    UBYTE *buffer;
    LONG bytes_read, bytes_written;
    BOOL success = FALSE;
    
    /* Allocate copy buffer */
    buffer = (UBYTE *)AllocMem(COPY_BUFFER_SIZE, MEMF_PUBLIC);
    if (!buffer) {
        return FALSE;
    }
    
    /* Open source file */
    src_file = Open((STRPTR)source, MODE_OLDFILE);
    if (!src_file) {
        FreeMem(buffer, COPY_BUFFER_SIZE);
        return FALSE;
    }
    
    /* Open destination file */
    dest_file = Open((STRPTR)dest, MODE_NEWFILE);
    if (!dest_file) {
        Close(src_file);
        FreeMem(buffer, COPY_BUFFER_SIZE);
        return FALSE;
    }
    
    /* Copy data */
    success = TRUE;
    while ((bytes_read = Read(src_file, buffer, COPY_BUFFER_SIZE)) > 0) {
        bytes_written = Write(dest_file, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            success = FALSE;
            break;
        }
    }
    
    /* Cleanup */
    Close(dest_file);
    Close(src_file);
    FreeMem(buffer, COPY_BUFFER_SIZE);
    
    return success;
}

/*
 * Check if icon should be created for this file type
 * Returns TRUE if type is allowed by user preferences
 * 
 * TODO: In iTidy, this would read from user preferences structure
 * For now, implements simple blacklist for demonstration
 */
static BOOL should_create_icon_for_type(const char *type_token)
{
    /* Blacklist: Skip tool-related types (common in SYS:C) */
    if (strcmp(type_token, "tool") == 0) return FALSE;
    if (strcmp(type_token, "device") == 0) return FALSE;
    if (strcmp(type_token, "filesystem") == 0) return FALSE;
    if (strcmp(type_token, "handler") == 0) return FALSE;
    if (strcmp(type_token, "library") == 0) return FALSE;
    if (strcmp(type_token, "loadmodule") == 0) return FALSE;
    if (strcmp(type_token, "rexx") == 0) return FALSE;
    /* Whitelist: Allow everything else */
    return TRUE;
}

/*
 * Stage 1: Identify file type using DefIcons
 * Returns TRUE and fills type_token on success
 * This function queries DefIcons ONCE to get the file type
 */
static BOOL identify_file_type(const char *filepath, char *type_token, int token_size)
{
    return query_deficons(filepath, type_token, token_size);
}

/*
 * Stage 2: Create icon using pre-determined type token
 * Returns TRUE if icon created, FALSE if skipped or failed
 * This function does NOT query DefIcons - it uses the provided type
 */
static BOOL create_icon_with_type(const char *filepath, const char *type_token)
{
    char icon_path[512];
    char template_path[512];
    BOOL is_exact;
    BPTR lock;
    
    /* Build .info path */
    snprintf(icon_path, sizeof(icon_path), "%s.info", filepath);
    
    /* Check if .info already exists */
    lock = Lock(icon_path, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        icons_skipped++;
        return FALSE;  /* Already has icon */
    }
    
    /* Find icon template (uses type hierarchy, no DefIcons query) */
    if (!find_icon_template(type_token, template_path, sizeof(template_path), &is_exact)) {
        icons_failed++;
        printf("  SKIP: %s (no template for '%s')\n", filepath, type_token);
        return FALSE;  /* No template available */
    }
    
    /* Copy template to create icon */
    if (copy_icon_file(template_path, icon_path)) {
        icons_created++;
        printf("  CREATED: %s.info (from %s, type '%s')\n", 
               filepath, 
               is_exact ? "exact" : "parent", 
               type_token);
        return TRUE;
    } else {
        icons_failed++;
        printf("  FAILED: %s (copy error)\n", filepath);
        return FALSE;
    }
}

/*
 * Process directory and create icons for iconless files
 */
static void process_directory(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib = NULL;
    char fullpath[512];
    int files_processed = 0;
    
    printf("Processing: %s\n\n", path);
    
    /* Lock directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock) {
        printf("ERROR: Cannot access directory: %s\n", path);
        return;
    }
    
    /* Allocate FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        printf("ERROR: Cannot allocate FileInfoBlock\n");
        UnLock(lock);
        return;
    }
    
    /* Examine directory */
    if (!Examine(lock, fib)) {
        printf("ERROR: Cannot examine directory\n");
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return;
    }
    
    /* Process entries */
    while (ExNext(lock, fib)) {
        char type_token[128];
        
        /* Build full path */
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, fib->fib_FileName);
        
        /* Skip .info files */
        if (strlen(fib->fib_FileName) > 5 && 
            strcmp(fib->fib_FileName + strlen(fib->fib_FileName) - 5, ".info") == 0) {
            continue;
        }
        
        /* Skip directories for now */
        if (fib->fib_DirEntryType > 0) {
            continue;
        }
        
        /* OPTIMIZED FLOW: Query DefIcons only ONCE per file */
        
        /* Stage 1: Identify file type (DefIcons query happens here) */
        if (!identify_file_type(fullpath, type_token, sizeof(type_token))) {
            icons_failed++;
            files_processed++;
            continue;  /* Cannot identify type */
        }
        
        /* Stage 2: Check user preferences (cheap string comparison) */
        if (!should_create_icon_for_type(type_token)) {
            icons_filtered++;
            printf("  FILTERED: %s (type '%s' excluded by preferences)\n", 
                   fib->fib_FileName, type_token);
            files_processed++;
            continue;  /* User doesn't want icons for this type */
        }
        
        /* Stage 3: Create icon using pre-determined type (no DefIcons query) */
        create_icon_with_type(fullpath, type_token);
        files_processed++;
    }
    
    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    printf("\n================================================================================\n");
    printf("Files processed: %d\n", files_processed);
    printf("Icons created:   %d\n", icons_created);
    printf("Icons skipped:   %d (already exist)\n", icons_skipped);
    printf("Icons filtered:  %d (type excluded by preferences)\n", icons_filtered);
    printf("Icons failed:    %d\n", icons_failed);
    printf("================================================================================\n");
    printf("\nPerformance: Each file queried DefIcons only ONCE\n");
    printf("Type filtering done via cheap string comparison\n");
}

/*
 * Print usage information
 */
static void print_usage(void)
{
    printf("DefIcons Creator - Test Program\n");
    printf("Usage: deficons_creator <folder_path>\n\n");
    printf("Creates .info files for iconless files using DefIcons templates.\n");
    printf("Skips files that already have .info files.\n\n");
    printf("Examples:\n");
    printf("  deficons_creator Work:Documents\n");
    printf("  deficons_creator SYS:Utilities\n");
}

/*
 * Main entry point
 */
int main(int argc, char **argv)
{
    if (argc != 2) {
        print_usage();
        return 5;
    }
    
    printf("\n=== DefIcons Creator ===\n\n");
    
    if (!setup()) {
        cleanup();
        return 20;
    }
    
    process_directory(argv[1]);
    
    cleanup();
    return 0;
}
