/*
 * deficontree.c - DefIcons Type Hierarchy Tree Viewer
 * 
 * Displays the parent-child relationships of file types from deficons.prefs
 * Useful for finding the parent category of any given file type.
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>

// Action codes
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
static UBYTE buffer[BUFFER_SIZE];
static ULONG buffer_pos = 0;
static ULONG buffer_len = 0;
static BPTR file = 0;
static ULONG file_pos = 0;

// Tree node structure
#define MAX_CHILDREN 50
#define MAX_NAME_LEN 64

typedef struct TreeNode {
    char name[MAX_NAME_LEN];
    struct TreeNode *parent;
    struct TreeNode *children[MAX_CHILDREN];
    int child_count;
} TreeNode;

// Memory pool for tree nodes
#define MAX_NODES 500
static TreeNode node_pool[MAX_NODES];
static int node_pool_index = 0;

// Root node
static TreeNode *root = NULL;

// Read a single byte from file
static BOOL read_byte(UBYTE *byte)
{
    if (buffer_pos >= buffer_len)
    {
        buffer_len = Read(file, buffer, BUFFER_SIZE);
        buffer_pos = 0;
        
        if (buffer_len <= 0)
        {
            return FALSE;
        }
    }
    
    *byte = buffer[buffer_pos++];
    file_pos++;
    return TRUE;
}

// Read a null-terminated string
static BOOL read_string(char *str, int max_len)
{
    UBYTE byte;
    int i = 0;
    
    while (i < max_len - 1)
    {
        if (!read_byte(&byte))
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

// Allocate a new tree node
static TreeNode* alloc_node(const char *name, TreeNode *parent)
{
    TreeNode *node;
    
    if (node_pool_index >= MAX_NODES)
    {
        printf("ERROR: Too many nodes (max %d)\n", MAX_NODES);
        return NULL;
    }
    
    node = &node_pool[node_pool_index++];
    strncpy(node->name, name, MAX_NAME_LEN - 1);
    node->name[MAX_NAME_LEN - 1] = '\0';
    node->parent = parent;
    node->child_count = 0;
    
    return node;
}

// Add a child to a parent node
static BOOL add_child(TreeNode *parent, TreeNode *child)
{
    if (parent->child_count >= MAX_CHILDREN)
    {
        printf("ERROR: Too many children for '%s' (max %d)\n", parent->name, MAX_CHILDREN);
        return FALSE;
    }
    
    parent->children[parent->child_count++] = child;
    return TRUE;
}

// Skip all action codes until ACT_END
static BOOL skip_actions(void)
{
    UBYTE action, arg1, arg2, arg3;
    int i;
    LONG len;
    
    while (TRUE)
    {
        if (!read_byte(&action))
        {
            return FALSE;
        }
        
        switch (action)
        {
            case ACT_MATCH:
                // Skip: offset(2), length(1), data(length)
                if (!read_byte(&arg1) || !read_byte(&arg2) || !read_byte(&arg3))
                    return FALSE;
                len = (LONG)(BYTE)arg3;
                if (len < 0) len = -len;
                for (i = 0; i < len; i++)
                {
                    if (!read_byte(&arg1))
                        return FALSE;
                }
                break;
            
            case ACT_SEARCH:
            case ACT_SEARCHSKIPSPACES:
                // Skip: length(1), data(length)
                if (!read_byte(&arg1))
                    return FALSE;
                len = (LONG)(BYTE)arg1;
                if (len < 0) len = -len;
                for (i = 0; i < len; i++)
                {
                    if (!read_byte(&arg2))
                        return FALSE;
                }
                break;
            
            case ACT_FILESIZE:
                // Skip: size(4)
                for (i = 0; i < 4; i++)
                {
                    if (!read_byte(&arg1))
                        return FALSE;
                }
                break;
            
            case ACT_NAMEPATTERN:
                // Skip: null-terminated string
                {
                    char dummy[256];
                    if (!read_string(dummy, sizeof(dummy)))
                        return FALSE;
                }
                break;
            
            case ACT_PROTECTION:
                // Skip: mask(4) + protbits(4)
                for (i = 0; i < 8; i++)
                {
                    if (!read_byte(&arg1))
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
                printf("ERROR: Unknown action code %lu at position %lu\n", (ULONG)action, file_pos);
                return FALSE;
        }
    }
}

// Parse the database and build tree
static BOOL parse_database(void)
{
    char type_name[256];
    UBYTE type_code;
    TreeNode *current_parent = NULL;
    TreeNode *node;
    int level = 0;
    
    // Create root node
    root = alloc_node("ROOT", NULL);
    if (!root)
        return FALSE;
    
    current_parent = root;
    
    while (TRUE)
    {
        // Read type name
        if (!read_string(type_name, sizeof(type_name)))
        {
            printf("ERROR: Failed to read type name at position %lu\n", file_pos);
            return FALSE;
        }
        
        // Empty string means TYPE_END
        if (type_name[0] == 0)
        {
            break;
        }
        
        // Create node for this type
        node = alloc_node(type_name, current_parent);
        if (!node)
            return FALSE;
        
        // Add as child of current parent
        if (!add_child(current_parent, node))
            return FALSE;
        
        // Skip all action codes
        if (!skip_actions())
        {
            printf("ERROR: Failed to skip actions at position %lu\n", file_pos);
            return FALSE;
        }
        
        // Read hierarchy code
        if (!read_byte(&type_code))
        {
            // End of file
            break;
        }
        
        switch (type_code)
        {
            case TYPE_DOWN_LEVEL:
                // Next type is child of current type
                current_parent = node;
                level++;
                break;
            
            case TYPE_UP_LEVEL:
                // Next type is sibling of parent
                if (current_parent && current_parent->parent)
                {
                    current_parent = current_parent->parent;
                    level--;
                }
                break;
            
            case TYPE_END:
                return TRUE;
            
            default:
                // Must be start of next type name, put byte back
                buffer_pos--;
                file_pos--;
                break;
        }
    }
    
    return TRUE;
}

// Print tree recursively
static void print_tree(TreeNode *node, int indent)
{
    int i, j;
    
    // Print indentation
    for (i = 0; i < indent; i++)
    {
        printf("  ");
    }
    
    // Print node name
    printf("%s\n", node->name);
    
    // Print children
    for (i = 0; i < node->child_count; i++)
    {
        print_tree(node->children[i], indent + 1);
    }
}

// Find a node by name (recursive search)
static TreeNode* find_node(TreeNode *node, const char *name)
{
    TreeNode *result;
    int i;
    
    if (strcmp(node->name, name) == 0)
    {
        return node;
    }
    
    for (i = 0; i < node->child_count; i++)
    {
        result = find_node(node->children[i], name);
        if (result)
        {
            return result;
        }
    }
    
    return NULL;
}

// Print path from root to node
static void print_path(TreeNode *node)
{
    if (!node)
        return;
    
    if (node->parent)
    {
        print_path(node->parent);
        printf(" -> ");
    }
    
    printf("%s", node->name);
}

int main(int argc, char **argv)
{
    BOOL success = FALSE;
    TreeNode *found_node;
    
    printf("DefIcons Type Hierarchy Tree\n");
    printf("============================\n\n");
    
    // Open the deficons.prefs file
    file = Open("deficons.prefs", MODE_OLDFILE);
    if (file == 0)
    {
        printf("ERROR: Could not open deficons.prefs\n");
        return 20;
    }
    
    // Skip the file header
    {
        UBYTE header[HEADER_SIZE];
        LONG bytes_read = Read(file, header, HEADER_SIZE);
        if (bytes_read != HEADER_SIZE)
        {
            printf("ERROR: Could not read file header\n");
            Close(file);
            return 10;
        }
        file_pos = HEADER_SIZE;
    }
    
    // Parse database and build tree
    printf("Parsing database...\n");
    success = parse_database();
    
    if (!success)
    {
        printf("\nERROR: Parsing failed\n");
        Close(file);
        return 10;
    }
    
    printf("Parsed successfully! Total types: %d\n\n", node_pool_index);
    
    Close(file);
    
    // If a type name was provided as argument, find and display its path
    if (argc > 1)
    {
        printf("Searching for type: '%s'\n", argv[1]);
        found_node = find_node(root, argv[1]);
        
        if (found_node)
        {
            printf("\nPath: ");
            print_path(found_node);
            printf("\n");
            
            if (found_node->parent && strcmp(found_node->parent->name, "ROOT") != 0)
            {
                printf("Parent: %s\n", found_node->parent->name);
            }
            else
            {
                printf("Parent: (none - top level)\n");
            }
            
            if (found_node->child_count > 0)
            {
                int i;
                printf("Children: ");
                for (i = 0; i < found_node->child_count; i++)
                {
                    printf("%s", found_node->children[i]->name);
                    if (i < found_node->child_count - 1)
                        printf(", ");
                }
                printf("\n");
            }
        }
        else
        {
            printf("Type '%s' not found in database.\n", argv[1]);
        }
    }
    else
    {
        // No argument, print entire tree
        printf("Type Hierarchy Tree:\n");
        printf("====================\n\n");
        print_tree(root, 0);
    }
    
    return 0;
}
