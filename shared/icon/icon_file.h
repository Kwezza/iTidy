/*
 * icon_file.h - Bounds-checked raw Amiga .info envelope parser
 *
 * Walks serialized classic DiskObject structures field-by-field.
 * Does not decode NewIcons or ColorIcon pixels.
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_FILE_H
#define ITIDY_ICON_FILE_H

#include "icon_types.h"

/*========================================================================*/
/* Located blobs (offsets into the caller's buffer)                       */
/*========================================================================*/

typedef struct iTidy_IconImageLoc
{
    BOOL present;
    ULONG header_offset;
    ULONG data_offset;
    ULONG data_size;
    WORD width;
    WORD height;
    WORD depth;
    UBYTE plane_pick;
    UBYTE plane_onoff;
} iTidy_IconImageLoc;

typedef struct iTidy_IconTextLoc
{
    BOOL present;
    ULONG size_offset;   /* ULONG tx_Size */
    ULONG text_offset;   /* characters, including trailing NUL */
    ULONG text_size;     /* tx_Size, including trailing NUL */
} iTidy_IconTextLoc;

typedef struct iTidy_IconFile
{
    const UBYTE *data;
    ULONG size;

    UWORD magic;
    UWORD version;
    UBYTE type;
    WORD gadget_width;
    WORD gadget_height;
    UWORD gadget_flags;
    ULONG gadget_userdata;
    LONG current_x;
    LONG current_y;
    LONG stack_size;

    BOOL has_gadget_render;
    BOOL has_select_render;
    BOOL has_drawer_data;
    BOOL has_drawer_data2;

    ULONG drawer_data_offset;
    ULONG drawer_data2_offset;

    iTidy_IconImageLoc normal;
    iTidy_IconImageLoc selected;

    iTidy_IconTextLoc default_tool;
    iTidy_IconTextLoc tool_window;

    BOOL has_tooltypes;
    ULONG tooltypes_offset;      /* ULONG encoded count */
    ULONG tooltypes_count;       /* number of strings */

    ULONG classic_end;
    ULONG extension_offset;
    ULONG extension_size;
} iTidy_IconFile;

/*========================================================================*/
/* Safe readers                                                           */
/*========================================================================*/

BOOL icon_file_range_ok(ULONG size, ULONG offset, ULONG length);

BOOL icon_file_read_u8(const UBYTE *data, ULONG size, ULONG offset, UBYTE *out);
BOOL icon_file_read_u16(const UBYTE *data, ULONG size, ULONG offset, UWORD *out);
BOOL icon_file_read_s16(const UBYTE *data, ULONG size, ULONG offset, WORD *out);
BOOL icon_file_read_u32(const UBYTE *data, ULONG size, ULONG offset, ULONG *out);
BOOL icon_file_read_s32(const UBYTE *data, ULONG size, ULONG offset, LONG *out);

BOOL icon_file_planar_data_size(UWORD width, UWORD height, UWORD depth,
                                ULONG *out_size);

/*========================================================================*/
/* Parse                                                                  */
/*========================================================================*/

/**
 * Parse a serialized .info buffer. Does not allocate and does not copy
 * the buffer; `out->data` aliases `data`.
 *
 * Pointer fields in the DiskObject are treated as booleans only.
 */
iTidy_IconError icon_file_parse(const UBYTE *data, ULONG size,
                                iTidy_IconFile *out);

/**
 * Return ToolType string `index` (0-based) as a pointer into the buffer.
 * `text_len` is the length excluding the trailing NUL. Returns FALSE if
 * index is out of range or the stored string is truncated.
 */
BOOL icon_file_tooltype(const iTidy_IconFile *file, ULONG index,
                        const UBYTE **text, ULONG *text_len);

/**
 * Return DefaultTool as a pointer into the buffer (excluding NUL).
 */
BOOL icon_file_default_tool(const iTidy_IconFile *file,
                            const UBYTE **text, ULONG *text_len);

#endif /* ITIDY_ICON_FILE_H */
