/*
 * deficons_parser.h - DefIcons Type Hierarchy Parser
 * 
 * Parses ENV:Sys/deficons.prefs binary format and builds a hierarchical
 * type tree for use in the DefIcons type selection GUI.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_PARSER_H
#define DEFICONS_PARSER_H

#include <exec/types.h>

/* Maximum type name length */
#define MAX_DEFICONS_TYPE_NAME 64

/* DefIcons type tree node */
typedef struct DeficonTypeTreeNode {
    char type_name[MAX_DEFICONS_TYPE_NAME];  /* Type token (e.g., "music", "mod") */
    int generation;                          /* Tree depth: 0=root, 1=child, 2=grandchild */
    BOOL has_children;                       /* TRUE if branch node */
    BOOL enabled;                            /* User selection state (generation 0 only) */
    int parent_index;                        /* Index to parent node (-1 if root) */
} DeficonTypeTreeNode;

/* Parse deficons.prefs and build type tree
 * 
 * Reads ENV:Sys/deficons.prefs binary format and constructs a flat array
 * of tree nodes with parent/child relationships encoded via indices.
 * 
 * Parameters:
 *   tree_out  - Pointer to receive allocated tree array (caller must free)
 *   count_out - Pointer to receive node count
 * 
 * Returns:
 *   TRUE if parsing succeeded, FALSE on error
 * 
 * Memory:
 *   Allocates memory via whd_malloc() - caller must free with whd_free()
 */
BOOL parse_deficons_prefs(DeficonTypeTreeNode **tree_out, int *count_out);

/* Free type tree allocated by parse_deficons_prefs() */
void free_deficons_type_tree(DeficonTypeTreeNode *tree);

/* Get parent type name for a given type
 * 
 * Parameters:
 *   tree      - Tree array
 *   count     - Node count
 *   type_name - Type to look up
 * 
 * Returns:
 *   Parent type name, or NULL if not found or no parent
 */
const char* get_parent_type_name(const DeficonTypeTreeNode *tree, int count, const char *type_name);

/* Check if a type name exists in the tree
 * 
 * Parameters:
 *   tree      - Tree array
 *   count     - Node count
 *   type_name - Type to look up
 * 
 * Returns:
 *   TRUE if type exists, FALSE otherwise
 */
BOOL deficons_type_exists(const DeficonTypeTreeNode *tree, int count, const char *type_name);

#endif /* DEFICONS_PARSER_H */
