/*
 * deficons_parser.c - DefIcons Type Hierarchy Parser
 * 
 * Parses ENV:Sys/deficons.prefs binary format and builds a hierarchical
 * type tree for use in the DefIcons type selection GUI.
 * 
 * Ported from: Tests/DefIcons/deficontree.c
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#include "deficons_parser.h"
#include "../writeLog.h"
#include "../platform/platform.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <string.h>
#include <stdio.h>

// Action codes from deficons.prefs format
#define ACT_MATCH            1
#define ACT_SEARCH           2
#define ACT_SEARCHSKIPSPACES 3
#define ACT_FILESIZE         4
#define ACT_NAMEPATTERN      5
#define ACT_PROTECTION       6
#define ACT_OR               7
#define ACT_ISASCII          8
#define ACT_MACROCLASS       20
#define ACT_END              0

// Type hierarchy codes
#define TYPE_DOWN_LEVEL      1
#define TYPE_UP_LEVEL        2
#define TYPE_END             0

// File header size
#define HEADER_SIZE 32

// Buffer for reading file
#define BUFFER_SIZE 4096

// Temporary node structure for building tree
typedef struct TempNode {
    char name[MAX_DEFICONS_TYPE_NAME];
    int parent_index;   // -1 for root nodes
    int self_index;     // Position in array
} TempNode;

// File reading state
typedef struct ParserState {
    BPTR file;
    UBYTE buffer[BUFFER_SIZE];
    ULONG buffer_pos;
    ULONG buffer_len;
    ULONG file_pos;
} ParserState;

/* Read a single byte from file */
static BOOL read_byte(ParserState *state, UBYTE *byte)
{
    if (state->buffer_pos >= state->buffer_len)
    {
        state->buffer_len = Read(state->file, state->buffer, BUFFER_SIZE);
        state->buffer_pos = 0;
        
        if (state->buffer_len <= 0)
        {
            return FALSE;
        }
    }
    
    *byte = state->buffer[state->buffer_pos++];
    state->file_pos++;
    return TRUE;
}

/* Read a null-terminated string */
static BOOL read_string(ParserState *state, char *str, int max_len)
{
    UBYTE byte;
    int i = 0;
    
    while (i < max_len - 1)
    {
        if (!read_byte(state, &byte))
        {
            return FALSE;
        }
        
        if (byte == 0)
        {
            str[i] = 0;
            return TRUE;
        }
        
        str[i++] = byte;
    }
    
    str[i] = 0;
    return TRUE;
}

/* Skip all action codes until ACT_END */
static BOOL skip_actions(ParserState *state)
{
    UBYTE action, arg1, arg2, arg3;
    int i;
    LONG len;
    
    while (TRUE)
    {
        if (!read_byte(state, &action))
        {
            return FALSE;
        }
        
        switch (action)
        {
            case ACT_MATCH:
                // Skip: offset(2), length(1), data(length)
                if (!read_byte(state, &arg1) || !read_byte(state, &arg2) || !read_byte(state, &arg3))
                    return FALSE;
                len = (LONG)(BYTE)arg3;
                if (len < 0) len = -len;
                for (i = 0; i < len; i++)
                {
                    if (!read_byte(state, &arg1))
                        return FALSE;
                }
                break;
            
            case ACT_SEARCH:
            case ACT_SEARCHSKIPSPACES:
                // Skip: length(1), data(length)
                if (!read_byte(state, &arg1))
                    return FALSE;
                len = (LONG)(BYTE)arg1;
                if (len < 0) len = -len;
                for (i = 0; i < len; i++)
                {
                    if (!read_byte(state, &arg2))
                        return FALSE;
                }
                break;
            
            case ACT_FILESIZE:
                // Skip: size(4)
                for (i = 0; i < 4; i++)
                {
                    if (!read_byte(state, &arg1))
                        return FALSE;
                }
                break;
            
            case ACT_NAMEPATTERN:
                // Skip: null-terminated string
                {
                    char dummy[256];
                    if (!read_string(state, dummy, sizeof(dummy)))
                        return FALSE;
                }
                break;
            
            case ACT_PROTECTION:
                // Skip: mask(4) + protbits(4)
                for (i = 0; i < 8; i++)
                {
                    if (!read_byte(state, &arg1))
                        return FALSE;
                }
                break;
            
            case ACT_OR:
            case ACT_ISASCII:
            case ACT_MACROCLASS:
                // No parameters
                break;
            
            case ACT_END:
                return TRUE;
            
            default:
                log_error(LOG_GUI, "Unknown action code %lu at position %lu\n", (ULONG)action, state->file_pos);
                return FALSE;
        }
    }
}

/* Calculate generation levels recursively */
static void calculate_generation_levels(DeficonTypeTreeNode *nodes, int count)
{
    int i, j;
    int changed = 1;
    
    // Initialize all to generation -1 (unprocessed)
    for (i = 0; i < count; i++)
    {
        if (nodes[i].parent_index == -1)
        {
            nodes[i].generation = 1;  // Root nodes (generations start at 1 per autodoc)
        }
        else
        {
            nodes[i].generation = -1;  // Unprocessed
        }
    }
    
    // Iterate until all nodes have generation set
    while (changed)
    {
        changed = 0;
        for (i = 0; i < count; i++)
        {
            if (nodes[i].generation == -1)
            {
                // Find parent
                int parent_idx = nodes[i].parent_index;
                if (parent_idx >= 0 && parent_idx < count)
                {
                    if (nodes[parent_idx].generation >= 0)
                    {
                        nodes[i].generation = nodes[parent_idx].generation + 1;
                        changed = 1;
                    }
                }
            }
        }
    }
}

/* Set has_children flags */
static void set_has_children_flags(DeficonTypeTreeNode *nodes, int count)
{
    int i, j;
    
    // Initialize all to FALSE
    for (i = 0; i < count; i++)
    {
        nodes[i].has_children = FALSE;
    }
    
    // Check if any node references this as parent
    for (i = 0; i < count; i++)
    {
        int parent_idx = nodes[i].parent_index;
        if (parent_idx >= 0 && parent_idx < count)
        {
            nodes[parent_idx].has_children = TRUE;
        }
    }
}

/* Parse deficons.prefs and build type tree */
BOOL parse_deficons_prefs(DeficonTypeTreeNode **tree_out, int *count_out)
{
    ParserState state;
    TempNode *temp_nodes = NULL;
    DeficonTypeTreeNode *tree = NULL;
    int temp_count = 0;
    int temp_capacity = 128;
    char type_name[256];
    UBYTE type_code;
    int current_parent_idx = -1;
    int level = 0;
    UBYTE header[HEADER_SIZE];
    LONG bytes_read;
    BOOL success = FALSE;
    int i;
    
    // Initialize output
    *tree_out = NULL;
    *count_out = 0;
    
    // Open deficons.prefs
    state.file = Open("ENV:deficons.prefs", MODE_OLDFILE);
    if (state.file == 0)
    {
        log_warning(LOG_GUI, "Could not open ENV:deficons.prefs\n");
        return FALSE;
    }
    
    // Initialize parser state
    state.buffer_pos = 0;
    state.buffer_len = 0;
    state.file_pos = 0;
    
    // Skip file header
    bytes_read = Read(state.file, header, HEADER_SIZE);
    if (bytes_read != HEADER_SIZE)
    {
        log_error(LOG_GUI, "Could not read deficons.prefs header\n");
        Close(state.file);
        return FALSE;
    }
    state.file_pos = HEADER_SIZE;
    
    // Allocate temporary node array
    temp_nodes = (TempNode *)whd_malloc(temp_capacity * sizeof(TempNode));
    if (temp_nodes == NULL)
    {
        log_error(LOG_GUI, "Out of memory allocating temp nodes\n");
        Close(state.file);
        return FALSE;
    }
    
    log_debug(LOG_GUI, "Parsing deficons.prefs...\n");
    
    // Parse database
    while (TRUE)
    {
        // Read type name
        if (!read_string(&state, type_name, sizeof(type_name)))
        {
            log_debug(LOG_GUI, "End of file reached\n");
            break;
        }
        
        // Empty string means end
        if (type_name[0] == 0)
        {
            break;
        }
        
        // Expand array if needed
        if (temp_count >= temp_capacity)
        {
            temp_capacity *= 2;
            TempNode *new_nodes = (TempNode *)whd_malloc(temp_capacity * sizeof(TempNode));
            if (new_nodes == NULL)
            {
                log_error(LOG_GUI, "Out of memory expanding temp nodes\n");
                goto cleanup;
            }
            memcpy(new_nodes, temp_nodes, temp_count * sizeof(TempNode));
            whd_free(temp_nodes);
            temp_nodes = new_nodes;
        }
        
        // Add node
        strncpy(temp_nodes[temp_count].name, type_name, MAX_DEFICONS_TYPE_NAME - 1);
        temp_nodes[temp_count].name[MAX_DEFICONS_TYPE_NAME - 1] = '\0';
        temp_nodes[temp_count].parent_index = current_parent_idx;
        temp_nodes[temp_count].self_index = temp_count;
        temp_count++;
        
        // Skip action codes
        if (!skip_actions(&state))
        {
            log_error(LOG_GUI, "Failed to skip actions\n");
            goto cleanup;
        }
        
        // Read hierarchy code
        if (!read_byte(&state, &type_code))
        {
            // End of file
            break;
        }
        
        switch (type_code)
        {
            case TYPE_DOWN_LEVEL:
                // Next type is child of current type
                current_parent_idx = temp_count - 1;
                level++;
                break;
            
            case TYPE_UP_LEVEL:
                // Next type is sibling of parent
                if (current_parent_idx >= 0)
                {
                    current_parent_idx = temp_nodes[current_parent_idx].parent_index;
                    level--;
                }
                break;
            
            case TYPE_END:
                goto parse_done;
            
            default:
                // Must be start of next type name, put byte back
                state.buffer_pos--;
                state.file_pos--;
                break;
        }
    }
    
parse_done:
    log_info(LOG_GUI, "Parsed %d DefIcons types\n", temp_count);
    
    // Convert to final tree structure
    tree = (DeficonTypeTreeNode *)whd_malloc(temp_count * sizeof(DeficonTypeTreeNode));
    if (tree == NULL)
    {
        log_error(LOG_GUI, "Out of memory allocating tree\n");
        goto cleanup;
    }
    
    for (i = 0; i < temp_count; i++)
    {
        strncpy(tree[i].type_name, temp_nodes[i].name, MAX_DEFICONS_TYPE_NAME - 1);
        tree[i].type_name[MAX_DEFICONS_TYPE_NAME - 1] = '\0';
        tree[i].parent_index = temp_nodes[i].parent_index;
        tree[i].has_children = FALSE;
        tree[i].enabled = TRUE;  // Default: all enabled
    }
    
    // Calculate generation levels and has_children flags
    calculate_generation_levels(tree, temp_count);
    set_has_children_flags(tree, temp_count);
    
    // Success
    *tree_out = tree;
    *count_out = temp_count;
    success = TRUE;
    
cleanup:
    if (temp_nodes)
    {
        whd_free(temp_nodes);
    }
    
    Close(state.file);
    
    return success;
}

/* Free type tree */
void free_deficons_type_tree(DeficonTypeTreeNode *tree)
{
    if (tree)
    {
        whd_free(tree);
    }
}

/* Get parent type name */
const char* get_parent_type_name(const DeficonTypeTreeNode *tree, int count, const char *type_name)
{
    int i;
    
    if (tree == NULL || type_name == NULL)
    {
        return NULL;
    }
    
    // Find the type
    for (i = 0; i < count; i++)
    {
        if (strcmp(tree[i].type_name, type_name) == 0)
        {
            // Found it, check for parent
            int parent_idx = tree[i].parent_index;
            if (parent_idx >= 0 && parent_idx < count)
            {
                return tree[parent_idx].type_name;
            }
            return NULL;  // No parent (root node)
        }
    }
    
    return NULL;  // Type not found
}

/* Check if type exists in tree */
BOOL deficons_type_exists(const DeficonTypeTreeNode *tree, int count, const char *type_name)
{
    int i;
    
    if (tree == NULL || type_name == NULL)
    {
        return FALSE;
    }
    
    for (i = 0; i < count; i++)
    {
        if (strcmp(tree[i].type_name, type_name) == 0)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}
