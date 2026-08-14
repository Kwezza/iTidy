/*
 * icon_types.h - Neutral types and error codes for shared icon file parsing
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 *
 * On-disk .info layout is documented in support files/IconFormats.txt.
 * Serialized pointer fields are NOT memory pointers and must never be
 * dereferenced; a non-zero value only means "the corresponding blob follows".
 */

#ifndef ITIDY_ICON_TYPES_H
#define ITIDY_ICON_TYPES_H

#include <exec/types.h>

/*========================================================================*/
/* DiskObject constants (on-disk, big-endian)                             */
/*========================================================================*/

#define ITIDY_ICON_MAGIC            0xE310U
#define ITIDY_ICON_VERSION          1

#define ITIDY_ICON_DISKOBJECT_SIZE  78UL    /* 0x4E */
#define ITIDY_ICON_DRAWERDATA_SIZE  56UL    /* 0x38 */
#define ITIDY_ICON_DRAWERDATA2_SIZE 6UL
#define ITIDY_ICON_IMAGE_HEADER_SIZE 20UL   /* 0x14 */

/* do_Type / ic_Type */
#define ITIDY_ICON_WB_DISK          1
#define ITIDY_ICON_WB_DRAWER        2
#define ITIDY_ICON_WB_TOOL          3
#define ITIDY_ICON_WB_PROJECT       4
#define ITIDY_ICON_WB_GARBAGE       5
#define ITIDY_ICON_WB_DEVICE        6
#define ITIDY_ICON_WB_KICK          7
#define ITIDY_ICON_WB_APPICON       8

/* Conservative sanity limits for low-memory classic systems */
#define ITIDY_ICON_MAX_WIDTH        1024U
#define ITIDY_ICON_MAX_HEIGHT       1024U
#define ITIDY_ICON_MAX_DEPTH        8U
#define ITIDY_ICON_MAX_TOOLTYPES    512UL
#define ITIDY_ICON_MAX_TEXT_SIZE    65536UL

/*========================================================================*/
/* Errors                                                                 */
/*========================================================================*/

typedef int iTidy_IconError;

#define ITIDY_ICON_OK                 0
#define ITIDY_ICON_ERR_NULL           1
#define ITIDY_ICON_ERR_TOO_SMALL      2
#define ITIDY_ICON_ERR_BAD_MAGIC      3
#define ITIDY_ICON_ERR_BAD_VERSION    4
#define ITIDY_ICON_ERR_TRUNCATED      5
#define ITIDY_ICON_ERR_OVERFLOW       6
#define ITIDY_ICON_ERR_BAD_DIMENSION  7
#define ITIDY_ICON_ERR_BAD_COUNT      8
#define ITIDY_ICON_ERR_BAD_TEXT       9

const char *icon_error_string(iTidy_IconError err);

#endif /* ITIDY_ICON_TYPES_H */
