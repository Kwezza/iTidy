/*
**	$VER: types.h 47.6 (2.8.2020)
**
**	Data typing. Must be included before any other Amiga include.
**
**	Copyright (C) 2020 Hyperion Entertainment CVBA.
**	    Developed under license.
*/


/* Version of the include files in use. Do not use this label for
 * OpenLibrary() calls!
 */
/*
 * The following definitions are intended to provide for backwards
 * compatibility with Amiga 'C' code written through the years 1985
 * and the decades which followed it. Compatibility with the state
 * of the 'C' language prior to ISO/IEC 9899:1990 (C90) is needed
 * for the sake of being able to build historic code. However, no
 * attempt is made to allow for 'C' code written to conform to
 * ANSI 'C'/C89/C90, etc. to build on older compilers which fail
 * to implement the required 'C' language features.
 */


/* Scalar types must be a specific size for Amiga data structures
 * and this requires knowledge of how the compiler defines them.
 * This information is available in standard 'C' header files
 * starting with ISO/IEC 9899:1999 (C99).
 */
/* WARNING: APTR was redefined for the V36 Includes in 1989! APTR is a
 * 32-Bit Absolute Memory Pointer. 'C' pointer math will not operate on
 * APTR -- use "BYTE *" instead.
 */
typedef void * APTR; /* 32-bit untyped pointer */
typedef const void * CONST_APTR; /* 32-bit untyped pointer to constant data */
/* By default, the Kickstart/Workbench 1.x (1985) types will be used. */
typedef long		LONG;		/* signed 32-bit quantity */
typedef unsigned long	ULONG;		/* unsigned 32-bit quantity */
typedef unsigned long	LONGBITS;	/* 32 bits manipulated individually */
typedef short		WORD;		/* signed 16-bit quantity */
typedef unsigned short	UWORD;		/* unsigned 16-bit quantity */
typedef unsigned short	WORDBITS;	/* 16 bits manipulated individually */
typedef signed char	BYTE;		/* signed 8-bit quantity */
typedef unsigned char	UBYTE;		/* unsigned 8-bit quantity */
typedef unsigned char	BYTEBITS;	/* 8 bits manipulated individually */
typedef unsigned short	RPTR;		/* unsigned relative pointer */

/* For compatibility with Kickstart/Workbench 1.x (1985) only!
 * Do not use these in newer code!
 */
typedef short		SHORT;		/* signed 16-bit quantity (use WORD) */
typedef unsigned short	USHORT;		/* unsigned 16-bit quantity (use UWORD) */
typedef short		COUNT;
typedef unsigned short	UCOUNT;
typedef ULONG		CPTR;

typedef unsigned char *	STRPTR;		/* string pointer (NUL-terminated) */
/* Constant string pointer (NUL-terminated), which is most useful
 * in function prototypes and data structure definitions.
 */
typedef const unsigned char *	CONST_STRPTR;
/* Types with specific semantics */
typedef float		FLOAT;
typedef double		DOUBLE;

typedef unsigned char	TEXT;
/* Please note that the BOOL type defined below is intended to
 * be used within data structure declarations and must be of a
 * specific size, i.e. sizeof(BOOL) == 2. It is not intended
 * to be a substitute for the 'C' <stdbool.h> header file
 * definitions or the C++ bool type.
 */
typedef short	BOOL;
/* The '#define LIBRARY_VERSION' has been obsolete since 1989. Please use
 * LIBRARY_MINIMUM or code the specific minimum library version you require.
 * Version 33 stands for Kickstart/Workbench 1.2 (1986).
 */
/* Remove the local preprocessor symbol which indicated
 * that C99 integer types were to be used.
 */
/* Compiler-specific data and function attributes are used
 * widely in both operating system header files as well as
 * operating system code itself. For convenience, these are
 * included through <exec/types.h>.
 */
/*
**	$VER: compiler-specific.h 47.5 (24.11.2021)
**
**	Compiler-specific data and function attributes. Must
**	be included with operating system library and device
**	header files.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* Some structure definitions include prototypes for embedded function
 * pointers. This may not work with 'C' compilers that do not comply to
 * the ANSI standard, which we will have to work around.
 */
/*
 * Some functions have special calling conventions that may be communicated
 * to the compiler through compiler-specific keywords. For the Lattice and SAS/C
 * compilers the #defines are given here. Otherwise, they should be defined
 * before including the OS functions.
 */

/*
 * Assembler call with arguments placed in registers. __ASM__ defines such a
 * function, with __REG__(r, p) defining the order of the arguments.
 */
/*
 * The __REG__(r, p) macro uses two parameters, which cover the register
 * name and the parameter declaration itself. This is necessary because
 * the respective compiler syntax may require that the order in which the
 * register and the parameter are used is changed. This is the case for
 * the GNU 'C' compiler.
 */
/*
 * Stack based calling conventions
 */
/*
 * Small data model using A4-relative addressing needs to establish the
 * initial A4 register value.
 */
/*
 * Data or functions marked for absolute addressing rather than PC or
 * register-relative addressing.
 */
/*
 * Request that upon exit from a function the CPU condition codes should be
 * updated based upon whether or not the value of register D0 is non-zero.
 */
/*
 * Request that the object is placed into chip memory (see <exec/memory.h>).
 */
/*
 * Request that the object is placed into fast memory (see <exec/memory.h>).
 */
/*
**	$VER: dos.h 47.1 (29.7.2019)
**
**	Standard C header for AmigaDOS 
**	Obsolete - see dos/dos.h
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: dos.h 47.2 (16.16.2021)
**
**	Standard C header for AmigaDOS
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* Predefined Amiga DOS global constants */

/* Mode parameter to Open() */
/* Relative position to Seek() */
/* Passed as type to Lock() */
struct DateStamp {
   LONG	 ds_Days;	      /* Number of days since Jan. 1, 1978 */
   LONG	 ds_Minute;	      /* Number of minutes past midnight */
   LONG	 ds_Tick;	      /* Number of ticks past minute */
}; /* DateStamp */

/* Returned by Examine() and ExNext(), must be on a 4 byte boundary */
struct FileInfoBlock {
   LONG	  fib_DiskKey;
   LONG	  fib_DirEntryType;  /* Type of Directory. If < 0, then a plain file.
			      * If > 0 a directory */
   TEXT	  fib_FileName[108]; /* Null terminated. Max 30 chars used for now */
   LONG	  fib_Protection;    /* bit mask of protection, rwxd are 3-0.	   */
   LONG	  fib_EntryType;
   LONG	  fib_Size;	     /* Number of bytes in file */
   LONG	  fib_NumBlocks;     /* Number of blocks in file */
   struct DateStamp fib_Date;/* Date file last changed */
   TEXT	  fib_Comment[80];  /* Null terminated comment associated with file */

   /* Note: the following fields are not supported by all filesystems.	*/
   /* They should be initialized to 0 sending an ACTION_EXAMINE packet.	*/
   /* When Examine() is called, these are set to 0 for you.		*/
   /* AllocDosObject() also initializes them to 0.			*/
   UWORD  fib_OwnerUID;		/* owner's UID */
   UWORD  fib_OwnerGID;		/* owner's GID */

   UBYTE  fib_Reserved[32];
}; /* FileInfoBlock */

/* FIB stands for FileInfoBlock */

/* FIBB are bit definitions, FIBF are field definitions */
/* Regular RWED bits are 0 == allowed. */
/* NOTE: GRP and OTR RWED permissions are 0 == not allowed! */
/* Group and Other permissions are not directly handled by the filesystem */
/* Standard maximum length for an error string from fault.  However, most */
/* error strings should be kept under 60 characters if possible.  Don't   */
/* forget space for the header you pass in. */
/* All BCPL data must be long word aligned.  BCPL pointers are the long word
 *  address (i.e byte address divided by 4 (>>2)) */
typedef long BPTR;		    /* Long word pointer */
typedef long BSTR;		    /* Long word pointer to BCPL string	 */

/* Convert BPTR to typical C pointer */
/* This one has no problems with CASTing */
/* Convert address into a BPTR */
/* BCPL strings have a length in the first byte and then the characters.
 * For example:	 s[0]=3 s[1]=S s[2]=Y s[3]=S				 */

/* returned by Info(), must be on a 4 byte boundary */
struct InfoData {
   LONG	  id_NumSoftErrors;	/* number of soft errors on disk */
   LONG	  id_UnitNumber;	/* Which unit disk is (was) mounted on */
   LONG	  id_DiskState;		/* See defines below */
   LONG	  id_NumBlocks;		/* Number of blocks on disk */
   LONG	  id_NumBlocksUsed;	/* Number of block in use */
   LONG	  id_BytesPerBlock;
   LONG	  id_DiskType;		/* Disk Type code */
   BPTR	  id_VolumeNode;	/* BCPL pointer to volume node (see DosList) */
   LONG	  id_InUse;		/* Flag, zero if not in use */
}; /* InfoData */

/* ID stands for InfoData */
	/* Disk states */
	/* Disk types */
/* ID_INTER_* use international case comparison routines for hashing */
/* Any other new filesystems should also, if possible. */
/* Errors from IoErr(), etc. */
/* added for 1.4 */
/* error codes 303-305 are defined in dosasl.h */

/* These are the return codes used by convention by AmigaDOS commands */
/* See FAILAT and IF for relvance to EXECUTE files		      */
/* Bit numbers that signal you that a user has issued a break */
/* Bit fields that signal you that a user has issued a break */
/* for example:	 if (SetSignal(0,0) & SIGBREAKF_CTRL_C) cleanup_and_exit(); */
/* Values returned by SameLock() */
/* LOCK_SAME_HANDLER was a misleading name, def kept for src compatibility */

/* types for ChangeMode() */
/* Values for MakeLink() */
/* values returned by ReadItem */
/* types for AllocDosObject/FreeDosObject */
/*
**	$VER: workbench.h 47.5 (27.10.2021)
**
**	workbench.library general definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: nodes.h 47.1 (28.6.2019)
**
**	Nodes & Node type identifiers.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/*
 *  List Node Structure.  Each member in a list starts with a Node
 */

struct Node {
    struct  Node *ln_Succ;	/* Pointer to next (successor) */
    struct  Node *ln_Pred;	/* Pointer to previous (predecessor) */
    UBYTE   ln_Type;
    BYTE    ln_Pri;		/* Priority, for sorting */
    char    *ln_Name;		/* ID string, null terminated */
};	/* Note: word aligned */

/* minimal node -- no type checking possible */
struct MinNode {
    struct MinNode *mln_Succ;
    struct MinNode *mln_Pred;
};


/*
** Note: Newly initialized IORequests, and software interrupt structures
** used with Cause(), should have type NT_UNKNOWN.  The OS will assign a type
** when they are first used.
*/
/*----- Node Types for LN_TYPE -----*/
/*
**	$VER: lists.h 47.1 (28.6.2019)
**
**	Definitions and macros for use with Exec lists
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/*
 *  Full featured list header.
 */
struct List {
   struct  Node *lh_Head;
   struct  Node *lh_Tail;
   struct  Node *lh_TailPred;
   UBYTE   lh_Type;
   UBYTE   l_pad;
};	/* word aligned */

/*
 * Minimal List Header - no type checking
 */
struct MinList {
   struct  MinNode *mlh_Head;
   struct  MinNode *mlh_Tail;
   struct  MinNode *mlh_TailPred;
};	/* longword aligned */


/*
 *	Check for the presence of any nodes on the given list.	These
 *	macros are even safe to use on lists that are modified by other
 *	tasks.	However; if something is simultaneously changing the
 *	list, the result of the test is unpredictable.
 *
 *	Unless you first arbitrated for ownership of the list, you can't
 *	_depend_ on the contents of the list.  Nodes might have been added
 *	or removed during or after the macro executes.
 *
 *		if( IsListEmpty(list) )		printf("List is empty\n");
 */
/*
**	$VER: tasks.h 47.1 (28.6.2019)
**
**	Task Control Block, Signals, and Task flags.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/* Please use Exec functions to modify task structure fields, where available.
 */
struct Task {
    struct  Node tc_Node;
    UBYTE   tc_Flags;
    UBYTE   tc_State;
    BYTE    tc_IDNestCnt;	    /* intr disabled nesting*/
    BYTE    tc_TDNestCnt;	    /* task disabled nesting*/
    ULONG   tc_SigAlloc;	    /* sigs allocated */
    ULONG   tc_SigWait;	    	    /* sigs we are waiting for */
    ULONG   tc_SigRecvd;	    /* sigs we have received */
    ULONG   tc_SigExcept;	    /* sigs we will take excepts for */
    UWORD   tc_TrapAlloc;	    /* traps allocated */
    UWORD   tc_TrapAble;	    /* traps enabled */
    APTR    tc_ExceptData;	    /* points to except data */
    APTR    tc_ExceptCode;	    /* points to except code */
    APTR    tc_TrapData;	    /* points to trap data */
    APTR    tc_TrapCode;	    /* points to trap code */
    APTR    tc_SPReg;		    /* stack pointer	    */
    APTR    tc_SPLower;	    /* stack lower bound    */
    APTR    tc_SPUpper;	    /* stack upper bound + 2*/
    void    (*tc_Switch)();	    /* task losing CPU	  */
    void    (*tc_Launch)();	    /* task getting CPU  */
    struct  List tc_MemEntry;	    /* Allocated memory. Freed by RemTask() */
    APTR    tc_UserData;	    /* For use by the task; no restrictions! */
};

/*
 * Stack swap structure as passed to StackSwap()
 */
struct	StackSwapStruct {
	APTR	stk_Lower;	/* Lowest byte of stack */
	ULONG	stk_Upper;	/* Upper end of stack (size + Lowest) */
	APTR	stk_Pointer;	/* Stack pointer at switch point */
};

/*----- Flag Bits ------------------------------------------*/
/*----- Task States ----------------------------------------*/
/*----- Predefined Signals -------------------------------------*/
/*
**	$VER: intuition.h 47.7 (26.12.2021)
**
**	Interface definitions for Intuition applications.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: gfx.h 47.3 (13.11.2021)
**
**	Low level data structures, types, flags, Tags and macros used
**	by graphics.library for display and rendering operations
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/*
**	$VER: tagitem.h 47.1 (3.8.2019)
**
**	Extended specification mechanism
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*****************************************************************************/


/* Tags are a general mechanism of extensible data arrays for parameter
 * specification and property inquiry. In practice, tags are used in arrays,
 * or chain of arrays.
 *
 */

typedef ULONG Tag;

struct TagItem
{
    Tag	  ti_Tag;	/* identifies the type of data */
    ULONG ti_Data;	/* type-specific data	       */
};

/* constants for Tag.ti_Tag, control tag values */
/* differentiates user tags from control tags */
/* If the TAG_USER bit is set in a tag number, it tells utility.library that
 * the tag is not a control tag (like TAG_DONE, TAG_IGNORE, TAG_MORE) and is
 * instead an application tag. "USER" means a client of utility.library in
 * general, including system code like Intuition or ASL, it has nothing to do
 * with user code.
 */


/*****************************************************************************/


/* Tag filter logic specifiers for use with FilterTagItems() */
/*****************************************************************************/


/* Mapping types for use with MapTags() */
/*****************************************************************************/


/* For use with the Custom.dmacon and Custom.intena registers. */
/* The TOBB() macro serves no real purpose on modern Amiga systems,
 * post the development work which took place in 1984-1985.
 *
 * It can be used to cast addresses into 32 bit integers, to be
 * written into custom chip registers. You may find these in
 * legacy source code only.
 */
struct Rectangle
{
	WORD MinX,MinY;
	WORD MaxX,MaxY;
};

struct Rect32
{
	LONG MinX,MinY;
	LONG MaxX,MaxY;
};

typedef struct tPoint
{
	WORD x,y;
} Point;


typedef UBYTE * PLANEPTR;

struct BitMap
{
	UWORD    BytesPerRow;
	UWORD    Rows;
	UBYTE    Flags;
	UBYTE    Depth;
	UWORD    pad;
	PLANEPTR Planes[8];
};


/* This macro is obsolete as of V39 and beyond if used for the purpose
 * of allocating memory for BitMap planes. AllocBitMap() should be used
 * for allocating BitMap data instead, since it is aware of the
 * machine's particular alignment requirements.
 */
/* Flags for AllocBitMap(), V39 and beyond */
/* Additional flags for AllocBitMap(), V45 and beyond */
/* These bit combinations and the BITMAPFLAGS_ARE_EXTENDED() macro
 * below must be used to detect if AllocBitMap() has been invoked
 * with a 'struct TagItem *' argument in place of the 'struct BitMap *'
 * friend BitMap parameter.
 */
/* The following attributes are used with GetBitMapAttr() */
/* Tags for use with AllocBitMap(), for V45 and beyond only. */

/* Specify a friend BitMap by tags. Default is not to use a friend BitMap */
/* Depth of the BitMap, which translates into the maximum number
 * of colors which may be used, e.g. depth=8 for 256 colors.
 * Default is the depth parameter of AllocBitMap().
 */
/* Low level BitMap data format (see enPixelFormat below) */
/* Clear the BitMap? Default is the BMF_CLEAR flag specified value. */
/* BitMap must be directly usable by display hardware?
 * Default is to follow whether the BMF_DISPLAYABLE flag is set or not.
 */
/* Do not provide memory for the BitMap, just allocate the structure
 * itself. Defaults to FALSE, i.e. memory will be provided.
 */
/* Disallow generation of a sprite (for the mouse pointer).
 * Defaults to FALSE, i.e. the sprite is enabled.
 */
/* Width of the display mode in pixels. Defaults to the width of the
 * DisplayID stored in the monitor database.
 */
/* Height of the display mode in pixels. Default is the height of the
 * DisplayID stored in the monitor database.
 */
/* Specify additional alignment requirements for the BitMap rows. This
 * must be a power of two (number of bytes). If this Tag is used then
 * bit plane rows are aligned to this boundary. Otherwise, the native
 * alignment restriction remains in effect.
 */
/* Set along with the BMATags_Alignment Tag to enforce alignment
 * for displayable screens
 */
/* User BitMap which will never be stored in video memory */
/* A DisplayID from the monitor data base. The system will attempt to
 * extract all the necessary information to build a suitable BitMap.
 * This Tag ID is intentionally identical to intuition.library/SA_DisplayID
 */
/* If set to TRUE, the BitMap is not allocated on the graphics
 * board directly, but may remain in an off-hardware location
 * if the screen is not visible. Defaults to FALSE, i.e. the
 * BitMap is allocated from the graphics board memory.
 * This Tag ID is intentionally identical to intuition.library/SA_Behind.
 */
/* Set the initial screen palette colors. ti_Data is an array of
 * "struct ColorSpec" entries, terminated by ColorSpec.ColorIndex = -1.
 * This Tag ID is intentionally identical to intuition.library/SA_Colors.
 */
/* Set the BitMaps's initial palette colors at 32 bits-per-gun. ti_Data is a
 * pointer to a table to be passed to the graphics.library/LoadRGB32()
 * function. This format supports both runs of color registers and sparse
 * registers. See the AutoDoc entry for that function for full details.
 * Any color set here has precedence over the same register set by
 * ABMA_BitmapColors.
 * This Tag is intentionally identical to intuition.library/SA_Colors32.
 */
/* Record an error in case AllocBitMap() fails to allocate a non-native
 * BitMap. The ti_Data value is a pointer to a ULONG in which the RTG
 * software will store the error code.
 * This Tag is intentionally identical to intuition.library/SA_ErrorCode.
 */
/* internal use only! */
/* Private. Do not use in new code. */
/* Low level pixel formats as used by AllocBitMap() and
 * the BMATags_PixelFormat Tag.
 */
/*
**	$VER: clip.h 47.1 (30.7.2019)
**
**	Layer and cliprect definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: semaphores.h 47.1 (28.6.2019)
**
**	Definitions for locking functions.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/*
**	$VER: ports.h 47.1 (28.6.2019)
**
**	Message ports and Messages.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****** MsgPort *****************************************************/

struct MsgPort {
    struct  Node mp_Node;
    UBYTE   mp_Flags;
    UBYTE   mp_SigBit;		/* signal bit number	*/
    void   *mp_SigTask;		/* object to be signalled */
    struct  List mp_MsgList;	/* message linked list	*/
};

/* mp_Flags: Port arrival actions (PutMsg) */
/****** Message *****************************************************/

struct Message {
    struct  Node mn_Node;
    struct  MsgPort *mn_ReplyPort;  /* message reply port */
    UWORD   mn_Length;		    /* total message length, in bytes */
				    /* (include the size of the Message */
				    /* structure in the length) */
};

/****** SignalSemaphore *********************************************/

/* Private structure used by ObtainSemaphore() */
struct SemaphoreRequest
{
	struct MinNode	sr_Link;
	struct Task	*sr_Waiter;
};

/* Signal Semaphore data structure */
struct SignalSemaphore
{
	struct Node		ss_Link;
	WORD			ss_NestCount;
	struct MinList		ss_WaitQueue;
	struct SemaphoreRequest	ss_MultipleLink;
	struct Task		*ss_Owner;
	WORD			ss_QueueCount;
};

/****** Semaphore procure message (for use in V39 Procure/Vacate) ****/
struct SemaphoreMessage
{
	struct Message		ssm_Message;
	struct SignalSemaphore	*ssm_Semaphore;
};

/****** Semaphore (Old Procure/Vacate type, not reliable) ***********/

struct Semaphore	/* Do not use these semaphores! */
{
	struct MsgPort	sm_MsgPort;
	WORD		sm_Bids;
};

/*
**	$VER: hooks.h 47.1 (3.8.2019)
**
**	Callback hooks
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*****************************************************************************/


struct Hook
{
    struct MinNode h_MinNode;
    ULONG	   (*h_Entry)();	/* assembler entry point */
    ULONG	   (*h_SubEntry)();	/* often HLL entry point */
    APTR	   h_Data;		/* owner specific	 */
};

/* Useful definition for casting function pointers:
 * hook.h_SubEntry = (HOOKFUNC)AFunction
 */
typedef unsigned long (*HOOKFUNC)();

/* Hook calling conventions.
 *
 * The function pointed to by Hook.h_Entry is called with the following
 * parameters:
 *
 *	A0 - pointer to hook data structure itself
 *	A1 - pointer to parameter structure ("message")
 *	A2 - Hook specific address data ("object")
 *
 * Control will be passed to the routine h_Entry.  For many
 * High-Level Languages (HLL), this will be an assembly language
 * stub which pushes registers on the stack, does other setup,
 * and then calls the function at h_SubEntry.
 *
 * The standard C receiving code is:
 *
 *    HookFunc(struct Hook *hook, APTR object, APTR message)
 *
 * Note that register natural order differs from this convention for C
 * parameter order, which is A0,A2,A1.
 *
 * The assembly language stub for "vanilla" C parameter conventions
 * could be:
 *
 * _hookEntry:
 *	move.l	a1,-(sp)		; push message packet pointer
 *	move.l	a2,-(sp)		; push object pointer
 *	move.l	a0,-(sp)		; push hook pointer
 *	move.l	h_SubEntry(a0),a0	; fetch C entry point ...
 *	jsr	(a0)			; ... and call it
 *	lea	12(sp),sp		; fix stack
 *	rts
 *
 * With this function as your interface stub, you can write a Hook setup
 * function as:
 *
 * InitHook(struct Hook *hook, ULONG (*c_function)(), APTR userdata)
 * {
 * ULONG (*hookEntry)();
 *
 *     hook->h_Entry	= hookEntry;
 *     hook->h_SubEntry = c_function;
 *     hook->h_Data	= userdata;
 * }
 *
 * With a compiler capable of registerized parameters, such as SAS C, you
 * can put the C function in the h_Entry field directly. For example, for
 * SAS C:
 *
 *   ULONG __saveds __asm HookFunc(register __a0 struct Hook *hook,
 *				   register __a2 APTR	      object,
 *				   register __a1 APTR	      message);
 *
 */


/*****************************************************************************/


/*
 * Thor says: Keep hands off this structure. Layers builds it for you,
 * and keeps and adminstrates it for you. What's documented here is
 * only of interest for graphics and intuition. Especially, you may
 * only look at the front/back pointer of this layer while keeping the
 * layers list locked, or at the (Super)ClipRect singly (!) linked 
 * list while keeping this layer locked. Bounds are the bounds of this 
 * layer, rp its rastport whose layer is, surprise, surprise, this here.
 * Lock is the access lock you please lock by LockLayer() or 
 * LockLayerRom().
 * Everything else is completely off-limits and for private use only.
 */

struct Layer
{
    struct  Layer       *front,*back;
    struct  ClipRect    *ClipRect;      /* singly linked list of active cliprects */
    struct  RastPort    *rp;            /* rastport to draw into. Its layer is me */
    struct  Rectangle   bounds;         /* screen bounds of this layer */
    struct  Layer       *nlink;         /* new in V45:
                                         * next back layer for display
                                         * reorganization
                                         */
    UWORD   priority;                   /* internal use: on layer front/back move,
                                         * relative priority of the layers.
                                         * Topmost layer has lowest priority.
                                         */
    UWORD   Flags;                      /* see <graphics/layers.h> */
    struct  BitMap      *SuperBitMap;   /* if non-NULL, superbitmap layer */
    struct  ClipRect    *SuperClipRect; /* super bitmap cliprects if VBitMap != 0*/
                                        /* else damage cliprect list for refresh */
    APTR    Window;                     /* Intuition keeps its window here */
    WORD    Scroll_X,Scroll_Y;          /* layer displacement */
    struct  ClipRect *OnScreen;         /* temporary list of on-screen cliprects */
    struct  ClipRect *OffScreen;        /* temporary list of off-screen cliprects */
    struct  ClipRect *Backup;           /* temporary copy of un-damaged clips */
    struct  ClipRect *SuperSaveClipRects; /* five preallocated super cr's */
    struct  ClipRect *Undamaged;        /* undamaged, un-user clipped version
                                         * of the cliprect, restored on EndUpdate()
                                         */
    struct  Layer_Info  *LayerInfo;     /* points to head of the list */
    struct  SignalSemaphore Lock;       /* access to this layer */
    struct  Hook *BackFill;             /* backfill hook */
    ULONG   reserved1;
    struct  Region *ClipRegion;         /* user InstallClipRegion()'d region */
    struct  ClipRect *clipped;          /* clipped away by damage list or
                                         * user clip rect
                                         */
    WORD    Width,Height;               /* system use */
    UBYTE   reserved2[18];              /* more reserved fields */
    struct  Region  *DamageList;        /* list of rectangles to refresh */
};

/*
 * Describes one graphic rectangle of this layer, may it
 * be drawable or not. 
 * The meaning of some fields in here changed in the past,
 * and will remain changing. Chaining is done by Next, and
 * if obscured is non-NULL, BitMap *may* point to a backing
 * store bitmap aligned to multiples of 16 pixels. 
 * Everything else is private, and may change.
 * Especially, note that this structure grew in v33(!!!)
 * and has now been documented to be of this size in
 * v44. NEWCLIPRECTS_1_1 is really, really obsolete.
 * Do *never* allocate this yourself as layers handles them
 * internally more efficiently than AllocMem() could.
 */

struct ClipRect
{
    struct  ClipRect   *Next;           /* roms used to find next ClipRect */
    struct  ClipRect   *reservedlink;   /* Currently unused */
    LONG                obscured;       /* If non-zero, this is a backing store
                                         * cliprect that is currently obscured.
                                         * NEW: In V45, this is no longer a
                                         * valid pointer since a cliprect
                                         * can be obscured by more than one
                                         * layer. Just test for zero or non-
                                         * zero, do *NOT* dereference.
                                         */
	struct  BitMap     *BitMap;         /* backing store bitmap if obscured != NULL */
    struct  Rectangle   bounds;         /* bounds of this cliprect */
    struct  ClipRect   *vlink;          /* Layers private use!!! */
    struct  Layer_Info *home;           /* where this cliprect belongs to.
                                         * If you *MUST* hack in your private
                                         * cliprects, ensure that you set this
                                         * field to NULL. If you don't, layers
                                         * will pool your cliprect and will
                                         * release it when it "feels like".
                                         * For NULL, V40 behaivour is
                                         * re-established.
                                         */
    APTR                reserved;
};

/* internal cliprect flags */
/* defines for code values for getcode 
 * this really belongs to graphics, and is of no
 * use for layers. It's here only for traditional
 * reasons.
 */
/*
**	$VER: view.h 47.1 (31.7.2019)
**
**	graphics view/viewport definintions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: copper.h 1.1 (7.8.2001)
**
**	graphics copper list intstruction definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct CopIns
{
    WORD   OpCode; /* 0 = move, 1 = wait */
    union
    {
	    struct CopList *nxtlist;
	    struct 
    	{
			union
			{
				WORD VWaitPos;        /* vertical beam wait */
				WORD DestAddr;        /* destination address of copper move */
			} u1;
			union
			{
				WORD HWaitPos;        /* horizontal beam wait position */
				WORD DestData;        /* destination immediate data to send */
			} u2;
		} u4;
    } u3;
};

/* shorthand for above */
/* structure of cprlist that points to list that hardware actually executes */
struct cprlist
{
    struct cprlist *Next;
    UWORD   *start;         /* start of copper list */
    WORD   MaxCount;       /* number of long instructions */
};

struct CopList
{
    struct  CopList *Next;  /* next block for this copper list */
    struct  CopList *_CopList;  /* system use */
    struct  ViewPort *_ViewPort;    /* system use */
    struct  CopIns *CopIns; /* start of this block */
    struct  CopIns *CopPtr; /* intermediate ptr */
    UWORD   *CopLStart;     /* mrgcop fills this in for Long Frame*/
    UWORD   *CopSStart;     /* mrgcop fills this in for Short Frame*/
    WORD   Count;          /* intermediate counter */
    WORD   MaxCount;       /* max # of copins for this block */
    WORD   DyOffset;       /* offset this copper list vertical waits */
    UWORD  SLRepeat;
    UWORD  Flags;
};

/* These CopList->Flags are private */
struct UCopList
{
    struct UCopList *Next;
    struct CopList  *FirstCopList; /* head node of this copper list */
    struct CopList  *CopList;      /* node in use */
};

/* Private graphics data structure. This structure has changed in the past,
 * and will continue to change in the future. Do Not Touch!
 */

struct copinit
{
    UWORD vsync_hblank[2];
    UWORD diagstrt[12];      /* copper list for first bitplane */
    UWORD fm0[2];
    UWORD diwstart[10];
    UWORD bplcon2[2];
	UWORD sprfix[2*8];
    UWORD sprstrtup[(2*8*2)];
    UWORD wait14[2];
    UWORD norm_hblank[2];
    UWORD jump[2];
    UWORD wait_forever[6];
    UWORD   sprstop[8];
};

/*
**	$VER: gfxnodes.h 47.1 (31.7.2019)
**
**	graphics extended node definintions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct  ExtendedNode    {
  struct  Node   *xln_Succ;
  struct  Node   *xln_Pred;
  UBYTE           xln_Type;
  BYTE            xln_Pri;
  char           *xln_Name;
  UBYTE           xln_Subsystem;
  UBYTE           xln_Subtype;
  struct GfxBase *xln_Library;
  /*
  ** The following function shall only be used internally, the
  ** calling convention is with registers populated below.
  */
  LONG (* __asm xln_Init) (register __a0 struct ExtendedNode *,register __d0 UWORD);
};

/* Forward declaration for <graphics/monitor.h> to use. */
struct View;

/*
**	$VER: monitor.h 47.1 (31.7.2019)
**
**	graphics monitorspec definintions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
** The functions in the MonitorSpec shall only be used internally, the
** calling convention is with registers populated below.
*/

struct	MonitorSpec
{
    struct	ExtendedNode	ms_Node;
    UWORD	ms_Flags;
    LONG	ratioh;
    LONG	ratiov;
    UWORD	total_rows;
    UWORD	total_colorclocks;
    UWORD	DeniseMaxDisplayColumn;
    UWORD	BeamCon0;
    UWORD	min_row;
    struct	SpecialMonitor	*ms_Special;
    WORD	ms_OpenCount;
    LONG (* __asm ms_transform)
(register __a0 struct MonitorSpec *, register __a1 Point *, register __d0 UWORD, register __a2 Point *);
    LONG (* __asm ms_translate)
(register __a0 struct MonitorSpec *, register __a1 Point *, register __d0 UWORD, register __a2 Point *);
    LONG (* __asm ms_scale)    
(register __a0 struct MonitorSpec *, register __a1 Point *, register __d0 UWORD, register __a2 Point *);
    UWORD	ms_xoffset;
    UWORD	ms_yoffset;
    struct	Rectangle	ms_LegalView;
    LONG (* __asm ms_maxoscan) 
(register __a0 struct MonitorSpec *, register __a1 struct Rectangle *, register __d0 ULONG);/* maximum legal overscan */
    LONG (* __asm ms_videoscan)
(register __a0 struct MonitorSpec *, register __a1 struct Rectangle *, register __d0 ULONG);/* video display overscan */
    UWORD	DeniseMinDisplayColumn;
    ULONG	DisplayCompatible;
    struct  	List DisplayInfoDataBase;
    struct	SignalSemaphore DisplayInfoDataBaseSemaphore;
    LONG (* __stdargs ms_MrgCop)   (struct View *);
    LONG (* __stdargs ms_LoadView) (struct View *);
    LONG (* __asm ms_KillView)     (register __a0 struct MonitorSpec *);
};

/* obsolete, v37 compatible definitions follow */
/* NOTE: VGA70 definitions are obsolete - a VGA70 monitor has never been
 * implemented.
 */
struct	AnalogSignalInterval
{
    UWORD	asi_Start;
    UWORD	asi_Stop;
};

struct	SpecialMonitor
{
    struct	ExtendedNode	spm_Node;
    UWORD	spm_Flags;
    LONG	(* __stdargs do_monitor) (struct MonitorSpec *mspc);
    LONG	(* __stdargs reserved1)();
    LONG	(* __stdargs reserved2)();
    LONG	(* __stdargs reserved3)();
    struct	AnalogSignalInterval	hblank;
    struct	AnalogSignalInterval	vblank;
    struct	AnalogSignalInterval	hsync;
    struct	AnalogSignalInterval	vsync;
};

/*
**	$VER: displayinfo.h 47.1 (30.7.2019)
**
**	include define file for displayinfo database
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/*
**	$VER: modeid.h 47.1 (31.7.2019)
**
**	include define file for graphics display mode IDs.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* With all the new modes that are available under V38 and V39, it is highly
 * recommended that you use either the asl.library screenmode requester,
 * and/or the V39 graphics.library function BestModeIDA().
 *
 * DO NOT interpret the any of the bits in the ModeID for its meaning. For
 * example, do not interpret bit 3 (0x4) as meaning the ModeID is interlaced.
 * Instead, use GetDisplayInfoData() with DTAG_DISP, and examine the DIPF_...
 * flags to determine a ModeID's characteristics. The only exception to
 * this rule is that bit 7 (0x80) will always mean the ModeID is
 * ExtraHalfBright, and bit 11 (0x800) will always mean the ModeID is HAM.
 */

/* normal identifiers */

/* the following 22 composite keys are for Modes on the default Monitor.
 * NTSC & PAL "flavors" of these particular keys may be made by or'ing
 * the NTSC or PAL MONITOR_ID with the desired MODE_KEY...
 *
 * For example, to specifically open a PAL HAM interlaced ViewPort
 * (or intuition screen), you would use the modeid of
 * (PAL_MONITOR_ID | HAMLACE_KEY)
 */

/* New for AA ChipSet (V39) */
/* Added for V40 - may be useful modes for some games or animations. */
/* VGA identifiers */

/* New for AA ChipSet (V39) */
/* These ModeIDs are the scandoubled equivalents of the above, with the
 * exception of the DualPlayfield modes, as AA does not allow for scandoubling
 * dualplayfield.
 */
/* a2024 identifiers */

/* prototype identifiers (private) */

/* These monitors and modes were added for the V38 release. */

/* New AA modes (V39) */
/* These ModeIDs are the scandoubled equivalents of the above, with the
 * exception of the DualPlayfield modes, as AA does not allow for scandoubling
 * dualplayfield.
 */
/* Euro36 modeids can be ORed with the default modeids a la NTSC and PAL.
 * For example, Euro36 SuperHires is
 * (EURO36_MONITOR_ID | SUPER_KEY)
 */

/* Super72 modeids can be ORed with the default modeids a la NTSC and PAL.
 * For example, Super72 SuperHiresLace (800x600) is
 * (SUPER72_MONITOR_ID | SUPERLACE_KEY).
 * The following scandoubled Modes are the exception:
 */
/* These monitors and modes were added for the V39 release. */

/* Use these tags for passing to BestModeID() (V39) */

				/* Default - NULL */
				/* Default - SPECIAL_FLAGS */
				/* Default - NULL */
				/* Default - SourceID NominalDimensionInfo,
				 * or vp->DWidth/Height, or (640 * 200),
				 * in that preferred order.
				 */
				/* Default - same as Nominal */
				/* Default - vp->RasInfo->BitMap->Depth or 1 */
				/* Default - use best monitor available */
				/* Default - VPModeID(vp) if BIDTAG_ViewPort is
				 * specified, else leave the DIPFMustHave and
				 * DIPFMustNotHave values untouched.
				 */
				/* Default - 4 */
/* the "public" handle to a DisplayInfoRecord */

typedef APTR DisplayInfoHandle;

/* datachunk type identifiers */

struct QueryHeader
{
	ULONG	StructID;	/* datachunk type identifier */
	ULONG	DisplayID;	/* copy of display record key	*/
	ULONG	SkipID;		/* TAG_SKIP -- see tagitems.h */
	ULONG	Length;		/* length of local data in double-longwords */
};

struct DisplayInfo
{
	struct	QueryHeader Header;
	UWORD	NotAvailable;	/* if NULL available, else see defines */
	ULONG	PropertyFlags;	/* Properties of this mode see defines */
	Point	Resolution;	/* ticks-per-pixel X/Y		       */
	UWORD	PixelSpeed;	/* aproximation in nanoseconds	       */
	UWORD	NumStdSprites;	/* number of standard amiga sprites    */
	UWORD	PaletteRange;	/* OBSOLETE - use Red/Green/Blue bits instead */
	Point	SpriteResolution; /* std sprite ticks-per-pixel X/Y    */
	UBYTE	pad[4];		/* used internally */
	UBYTE	RedBits;	/* number of Red bits this display supports (V39) */
	UBYTE	GreenBits;	/* number of Green bits this display supports (V39) */
	UBYTE	BlueBits;	/* number of Blue bits this display supports (V39) */
	UBYTE	pad2[5];	/* find some use for this. */
	ULONG	reserved[2];	/* terminator */
};

/* availability */

/* mode properties */

/* The following DIPF_IS_... flags are new for V39 */
											/* can change the sprite base colour */
											/* can change the sprite priority
											** with respect to the playfield(s).
											*/
struct DimensionInfo
{
	struct	QueryHeader Header;
	UWORD	MaxDepth;	      /* log2( max number of colors ) */
	UWORD	MinRasterWidth;       /* minimum width in pixels      */
	UWORD	MinRasterHeight;      /* minimum height in pixels     */
	UWORD	MaxRasterWidth;       /* maximum width in pixels      */
	UWORD	MaxRasterHeight;      /* maximum height in pixels     */
	struct	Rectangle   Nominal;  /* "standard" dimensions	      */
	struct	Rectangle   MaxOScan; /* fixed, hardware dependent    */
	struct	Rectangle VideoOScan; /* fixed, hardware dependent    */
	struct	Rectangle   TxtOScan; /* editable via preferences     */
	struct	Rectangle   StdOScan; /* editable via preferences     */
	UBYTE	pad[14];
	ULONG	reserved[2];	      /* terminator */
};

struct MonitorInfo
{
	struct	QueryHeader Header;
	struct	MonitorSpec  *Mspc;   /* pointer to monitor specification  */
	Point	ViewPosition;	      /* editable via preferences	   */
	Point	ViewResolution;       /* standard monitor ticks-per-pixel  */
	struct	Rectangle ViewPositionRange;  /* fixed, hardware dependent */
	UWORD	TotalRows;	      /* display height in scanlines	   */
	UWORD	TotalColorClocks;     /* scanline width in 280 ns units    */
	UWORD	MinRow;	      /* absolute minimum active scanline  */
	WORD	Compatibility;	      /* how this coexists with others	   */
	UBYTE	pad[32];
	Point	MouseTicks;
	Point	DefaultViewPosition;  /* original, never changes */
	ULONG	PreferredModeID;      /* for Preferences */
	ULONG	reserved[2];	      /* terminator */
};

/* monitor compatibility */

struct NameInfo
{
	struct	QueryHeader Header;
	UBYTE	Name[32];
	ULONG	reserved[2];	      /* terminator */
};

/******************************************************************************/

/* The following VecInfo structure is PRIVATE, for our use only
 * Touch these, and burn! (V39)
 */

struct VecInfo
{
	struct	QueryHeader   Header;
	APTR	Vec;
	APTR	Data;
	UWORD	Type;
	UWORD	pad[3];
	ULONG	reserved[2];
};

/*
**	$VER: custom.h 47.1 (1.8.2019)
**
**      Offsets of Amiga custom chip registers
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
 * do this to get base of custom registers:
 * extern struct Custom custom;
 */


struct Custom {
    UWORD   bltddat;
    UWORD   dmaconr;
    UWORD   vposr;
    UWORD   vhposr;
    UWORD   dskdatr;
    UWORD   joy0dat;
    UWORD   joy1dat;
    UWORD   clxdat;
    UWORD   adkconr;
    UWORD   pot0dat;
    UWORD   pot1dat;
    UWORD   potinp;
    UWORD   serdatr;
    UWORD   dskbytr;
    UWORD   intenar;
    UWORD   intreqr;
    APTR    dskpt;
    UWORD   dsklen;
    UWORD   dskdat;
    UWORD   refptr;
    UWORD   vposw;
    UWORD   vhposw;
    UWORD   copcon;
    UWORD   serdat;
    UWORD   serper;
    UWORD   potgo;
    UWORD   joytest;
    UWORD   strequ;
    UWORD   strvbl;
    UWORD   strhor;
    UWORD   strlong;
    UWORD   bltcon0;
    UWORD   bltcon1;
    UWORD   bltafwm;
    UWORD   bltalwm;
    APTR    bltcpt;
    APTR    bltbpt;
    APTR    bltapt;
    APTR    bltdpt;
    UWORD   bltsize;
    UBYTE   pad2d;
    UBYTE   bltcon0l;	/* low 8 bits of bltcon0, write only */
    UWORD   bltsizv;
    UWORD   bltsizh;	/* 5e */
    UWORD   bltcmod;
    UWORD   bltbmod;
    UWORD   bltamod;
    UWORD   bltdmod;
    UWORD   pad34[4];
    UWORD   bltcdat;
    UWORD   bltbdat;
    UWORD   bltadat;
    UWORD   pad3b[3];
    UWORD   deniseid;	/* 7c */
    UWORD   dsksync;
    ULONG   cop1lc;
    ULONG   cop2lc;
    UWORD   copjmp1;
    UWORD   copjmp2;
    UWORD   copins;
    UWORD   diwstrt;
    UWORD   diwstop;
    UWORD   ddfstrt;
    UWORD   ddfstop;
    UWORD   dmacon;
    UWORD   clxcon;
    UWORD   intena;
    UWORD   intreq;
    UWORD   adkcon;
    struct  AudChannel {
      UWORD *ac_ptr; /* ptr to start of waveform data */
      UWORD ac_len;	/* length of waveform in words */
      UWORD ac_per;	/* sample period */
      UWORD ac_vol;	/* volume */
      UWORD ac_dat;	/* sample pair */
      UWORD ac_pad[2];	/* unused */
    } aud[4];
    APTR    bplpt[8];
    UWORD   bplcon0;
    UWORD   bplcon1;
    UWORD   bplcon2;
    UWORD   bplcon3;
    UWORD   bpl1mod;
    UWORD   bpl2mod;
    UWORD   bplcon4;
    UWORD   clxcon2;
    UWORD   bpldat[8];
    APTR    sprpt[8];
    struct  SpriteDef {
      UWORD pos;
      UWORD ctl;
      UWORD dataa;
      UWORD datab;
    } spr[8];
    UWORD   color[32];
    UWORD htotal;
    UWORD hsstop;
    UWORD hbstrt;
    UWORD hbstop;
    UWORD vtotal;
    UWORD vsstop;
    UWORD vbstrt;
    UWORD vbstop;
    UWORD sprhstrt;
    UWORD sprhstop;
    UWORD bplhstrt;
    UWORD bplhstop;
    UWORD hhposw;
    UWORD hhposr;
    UWORD beamcon0;
    UWORD hsstrt;
    UWORD vsstrt;
    UWORD hcenter;
    UWORD diwhigh;	/* 1e4 */
    UWORD padf3[11];
    UWORD fmode;
};

/* defines for beamcon register */
/* new defines for bplcon0 */
/* new defines for bplcon2 */
/* defines for bplcon3 register */
struct ViewPort
{
	struct	ViewPort *Next;
	struct	ColorMap *ColorMap;	/* table of colors for this viewport */
					/* if this is nil, MakeVPort assumes default values */
	struct	CopList  *DspIns;	/* used by MakeVPort() */
	struct	CopList  *SprIns;	/* used by sprite stuff */
	struct	CopList  *ClrIns;	/* used by sprite stuff */
	struct	UCopList *UCopIns;	/* User copper list */
	WORD	DWidth,DHeight;
	WORD	DxOffset,DyOffset;
	UWORD	Modes;
	UBYTE	SpritePriorities;
	UBYTE	ExtendedModes;
	struct	RasInfo *RasInfo;
};

struct View
{
	struct	ViewPort *ViewPort;
	struct	cprlist *LOFCprList;   /* used for interlaced and noninterlaced */
	struct	cprlist *SHFCprList;   /* only used during interlace */
	WORD	DyOffset,DxOffset;   /* for complete View positioning */
				   /* offsets are +- adjustments to standard #s */
	UWORD	Modes;		   /* such as INTERLACE, GENLOC */
};

/* these structures are obtained via GfxNew */
/* and disposed by GfxFree */
struct ViewExtra
{
	struct ExtendedNode n;
	struct View *View;		/* backwards link */
	struct MonitorSpec *Monitor;	/* monitors for this view */
	UWORD TopLine;
};

/* this structure is obtained via GfxNew */
/* and disposed by GfxFree */
struct ViewPortExtra
{
	struct ExtendedNode n;
	struct ViewPort *ViewPort;	/* backwards link */
	struct Rectangle DisplayClip;	/* MakeVPort display clipping information */
	/* These are added for V39 */
	APTR   VecTable;		/* Private */
	APTR   DriverData[2];
	UWORD  Flags;
	Point  Origin[2];		/* First visible point relative to the DClip.
					 * One for each possible playfield.
					 */
	ULONG cop1ptr;			/* private */
	ULONG cop2ptr;			/* private */
};

/* All these VPXF_ flags are private */
/* defines used for Modes in IVPargs */

struct RasInfo	/* used by callers to and InitDspC() */
{
   struct   RasInfo *Next;	    /* used for dualpf */
   struct   BitMap *BitMap;
   WORD    RxOffset,RyOffset;	   /* scroll offsets in this BitMap */
};

struct ColorMap
{
	UBYTE	Flags;
	UBYTE	Type;
	UWORD	Count;
	APTR	ColorTable;
	struct	ViewPortExtra *cm_vpe;
	APTR	LowColorBits;
	UBYTE	TransparencyPlane;
	UBYTE	SpriteResolution;
	UBYTE	SpriteResDefault;	/* what resolution you get when you have set SPRITERESN_DEFAULT */
	UBYTE	AuxFlags;
	struct	ViewPort *cm_vp;
	APTR	NormalDisplayInfo;
	APTR	CoerceDisplayInfo;
	struct	TagItem *cm_batch_items;
	ULONG	VPModeID;
	struct	PaletteExtra *PalExtra;
	UWORD	SpriteBase_Even;
	UWORD	SpriteBase_Odd;
	UWORD	Bp_0_base;
	UWORD	Bp_1_base;

};

/* if Type == 0 then ColorMap is V1.2/V1.3 compatible */
/* if Type != 0 then ColorMap is V38	   compatible */
/* the system will never create other than V39 type colormaps when running V39 */

/* Flags variable */
/* ^140ns, except in 35ns viewport, where it is 70ns. */
/* AuxFlags : */
struct PaletteExtra				/* structure may be extended so watch out! */
{
	struct SignalSemaphore pe_Semaphore;		/* shared semaphore for arbitration	*/
	UWORD	pe_FirstFree;				/* *private*				*/
	UWORD	pe_NFree;				/* number of free colors		*/
	UWORD	pe_FirstShared;				/* *private*				*/
	UWORD	pe_NShared;				/* *private*				*/
	UBYTE	*pe_RefCnt;				/* *private*				*/
	UBYTE	*pe_AllocList;				/* *private*				*/
	struct ViewPort *pe_ViewPort;			/* back pointer to viewport		*/
	UWORD	pe_SharableColors;			/* the number of sharable colors.	*/
};

/* flags values for ObtainPen */

/* obsolete names for PENF_xxx flags: */

/* precision values for ObtainBestPen : */

/* tags for ObtainBestPen: */
/* From V39, MakeVPort() will return an error if there is not enough memory,
 * or the requested mode cannot be opened with the requested depth with the
 * given bitmap (for higher bandwidth alignments).
 */

/* From V39, MrgCop() will return an error if there is not enough memory,
 * or for some reason MrgCop() did not need to make any copper lists.
 */

struct DBufInfo {
	APTR	dbi_Link1;
	ULONG	dbi_Count1;
	struct Message dbi_SafeMessage;		/* replied to when safe to write to old bitmap */
	APTR dbi_UserData1;			/* first user data */

	APTR	dbi_Link2;
	ULONG	dbi_Count2;
	struct Message dbi_DispMessage;	/* replied to when new bitmap has been displayed at least
							once */
	APTR	dbi_UserData2;			/* second user data */
	ULONG	dbi_MatchLong;
	APTR	dbi_CopPtr1;
	APTR	dbi_CopPtr2;
	APTR	dbi_CopPtr3;
	UWORD	dbi_BeamPos1;
	UWORD	dbi_BeamPos2;
};

/*
**	$VER: rastport.h 47.1 (31.7.2019)
**
**	graphics RastPort and related structures
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct AreaInfo
{
    WORD   *VctrTbl;	     /* ptr to start of vector table */
    WORD   *VctrPtr;	     /* ptr to current vertex */
    BYTE    *FlagTbl;	      /* ptr to start of vector flag table */
    BYTE    *FlagPtr;	      /* ptrs to areafill flags */
    WORD   Count;	     /* number of vertices in list */
    WORD   MaxCount;	     /* AreaMove/Draw will not allow Count>MaxCount*/
    WORD   FirstX,FirstY;    /* first point for this polygon */
};

struct TmpRas
{
    BYTE *RasPtr;
    LONG Size;
};

/* unoptimized for 32bit alignment of pointers */
struct GelsInfo
{
    BYTE sprRsrvd;	      /* flag of which sprites to reserve from
				 vsprite system */
    UBYTE Flags;	      /* system use */
    struct VSprite *gelHead, *gelTail; /* dummy vSprites for list management*/
    /* pointer to array of 8 WORDS for sprite available lines */
    WORD *nextLine;
    /* pointer to array of 8 pointers for color-last-assigned to vSprites */
    WORD **lastColor;
    struct collTable *collHandler;     /* addresses of collision routines */
    WORD leftmost, rightmost, topmost, bottommost;
   APTR firstBlissObj,lastBlissObj;    /* system use only */
};

struct RastPort
{
    struct  Layer *Layer;
    struct  BitMap   *BitMap;
    UWORD  *AreaPtrn;	     /* ptr to areafill pattern */
    struct  TmpRas *TmpRas;
    struct  AreaInfo *AreaInfo;
    struct  GelsInfo *GelsInfo;
    UBYTE   Mask;	      /* write mask for this raster */
    BYTE    FgPen;	      /* foreground pen for this raster */
    BYTE    BgPen;	      /* background pen  */
    BYTE    AOlPen;	      /* areafill outline pen */
    BYTE    DrawMode;	      /* drawing mode for fill, lines, and text */
    BYTE    AreaPtSz;	      /* 2^n words for areafill pattern */
    BYTE    linpatcnt;	      /* current line drawing pattern preshift */
    BYTE    dummy;
    UWORD  Flags;	     /* miscellaneous control bits */
    UWORD  LinePtrn;	     /* 16 bits for textured lines */
    WORD   cp_x, cp_y;	     /* current pen position */
    UBYTE   minterms[8];
    WORD   PenWidth;
    WORD   PenHeight;
    struct  TextFont *Font;   /* current font address */
    UBYTE   AlgoStyle;	      /* the algorithmically generated style */
    UBYTE   TxFlags;	      /* text specific flags */
    UWORD   TxHeight;	      /* text height */
    UWORD   TxWidth;	      /* text nominal width */
    UWORD   TxBaseline;       /* text baseline */
    WORD    TxSpacing;	      /* text spacing (per character) */
    APTR    *RP_User;
    ULONG   longreserved[2];
    UWORD   wordreserved[7];  /* used to be a node */
    UBYTE   reserved[8];      /* for future use */
};

/* drawing modes */
/* these are the flag bits for RastPort flags */
	     /* only used for bobs */

/* there is only one style of clipping: raster clipping */
/* this preserves the continuity of jaggies regardless of clip window */
/* When drawing into a RastPort, if the ptr to ClipRect is nil then there */
/* is no clipping done, this is dangerous but useful for speed */

/*
**	$VER: layers.h 47.1 (31.7.2019)
**
**	Layer flags, and Layer_Info
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
 * layer status flags. These really belong to
 * graphics/clip.h but are here for traditional reason
 * (and to confuse you).
 */

                                        /* or during layerop */
                                        /* this happens if out of memory */
/*
 * Thor says: Keep hands off this Layer_Info. There's really nothing in
 * here to play with. The only thing you possibly may be interested in
 * is the top_layer that points to the topmost layer of this layer_info,
 * and the Lock which locks this structure. Even that is quite private,
 * but everything else is really really private. Leave all this to
 * layers.library as some fields are likely to change their meaning
 * in the near future.
 */

struct Layer_Info
{
struct  Layer           *top_layer;             /* Frontmost layer */
        void            *resPtr1;               /* V45 spare */
        void            *resPtr2;               /* Another V45 spare */
struct  ClipRect        *FreeClipRects;         /* Implements a backing store
                                                 * of cliprects to avoid
                                                 * frequent re-allocation
                                                 * of cliprects. Private.
                                                 */
struct  Rectangle       bounds;                 /* clipping bounds of
                                                 * this layer info. All layers
                                                 * are clipped against this
                                                 */
struct  SignalSemaphore Lock;                   /* Layer_Info lock */
struct  MinList         gs_Head;                /* linked list of all semaphores of all
                                                 * layers within this layer info
                                                 */
        WORD            PrivateReserve3;        /* !! Private !! */
        void           *PrivateReserve4;        /* !! Private !! */
        UWORD           Flags;
        BYTE            res_count;              /* V45 spare, no longer used */
        BYTE            LockLayersCount;        /* Counts # times LockLayers
                                                 * has been called
                                                 */
        BYTE           PrivateReserve5;         /* !! Private !! */
        BYTE           UserClipRectsCount;      /* Also private */
        struct Hook    *BlankHook;              /* LayerInfo backfill hook */
        void           *resPtr5;                /* !! Private !! */
};

/*
 * Special backfill hook values you may want to install here.
 *
 * LAYERS_NOBACKFILL is the value needed to get no backfill hook
 * LAYERS_BACKFILL is the value needed to get the default backfill hook
 */
/*
 * Special codes for ShowLayer():
 * Give this as target layer where
 * to move your layer to.
 */
/*
**	$VER: text.h 47.1 (31.7.2019)
**
**	graphics library text structures
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*------ Font Styles ------------------------------------------------*/
/*------ Font Flags -------------------------------------------------*/
				/* note: if you do not set this bit in your */
				/* textattr, then a font may be constructed */
				/* for you by scaling an existing rom or disk */
				/* font (under V36 and above). */
    /* bit 7 is always clear for fonts on the graphics font list */
/****** TextAttr node, matches text attributes in RastPort **********/
struct TextAttr {
    STRPTR  ta_Name;		/* name of the font */
    UWORD   ta_YSize;		/* height of the font */
    UBYTE   ta_Style;		/* intrinsic font style */
    UBYTE   ta_Flags;		/* font preferences and flags */
};

struct TTextAttr {
    STRPTR  tta_Name;		/* name of the font */
    UWORD   tta_YSize;		/* height of the font */
    UBYTE   tta_Style;		/* intrinsic font style */
    UBYTE   tta_Flags;		/* font preferences and flags */
    struct TagItem *tta_Tags;	/* extended attributes */
};


/****** Text Tags ***************************************************/
					/* Hi word XDPI, Lo word YDPI */

/****** TextFonts node **********************************************/
struct TextFont {
    struct Message tf_Message;	/* reply message for font removal */
				/* font name in LN	  \    used in this */
    UWORD   tf_YSize;		/* font height		  |    order to best */
    UBYTE   tf_Style;		/* font style		  |    match a font */
    UBYTE   tf_Flags;		/* preferences and flags  /    request. */
    UWORD   tf_XSize;		/* nominal font width */
    UWORD   tf_Baseline;	/* distance from the top of char to baseline */
    UWORD   tf_BoldSmear;	/* smear to affect a bold enhancement */

    UWORD   tf_Accessors;	/* access count */

    UBYTE   tf_LoChar;		/* the first character described here */
    UBYTE   tf_HiChar;		/* the last character described here */
    APTR    tf_CharData;	/* the bit character data */

    UWORD   tf_Modulo;		/* the row modulo for the strike font data */
    APTR    tf_CharLoc;		/* ptr to location data for the strike font */
				/*   2 words: bit offset then size */
    APTR    tf_CharSpace;	/* ptr to words of proportional spacing data */
    APTR    tf_CharKern;	/* ptr to words of kerning data */
};

/* unfortunately, this needs to be explicitly typed */
/*-----	tfe_Flags0 (partial definition) ----------------------------*/
struct TextFontExtension {	/* this structure is read-only */
    UWORD   tfe_MatchWord;		/* a magic cookie for the extension */
    UBYTE   tfe_Flags0;			/* (system private flags) */
    UBYTE   tfe_Flags1;			/* (system private flags) */
    struct TextFont *tfe_BackPtr;	/* validation of compilation */
    struct MsgPort *tfe_OrigReplyPort;	/* original value in tf_Extension */
    struct TagItem *tfe_Tags;		/* Text Tags for the font */
    UWORD  *tfe_OFontPatchS;		/* (system private use) */
    UWORD  *tfe_OFontPatchK;		/* (system private use) */
    /* this space is reserved for future expansion */
};

/******	ColorTextFont node ******************************************/
/*-----	ctf_Flags --------------------------------------------------*/
				/* brightnesses from low to high */
/*----- ColorFontColors --------------------------------------------*/
struct ColorFontColors {
    UWORD   cfc_Reserved;	/* *must* be zero */
    UWORD   cfc_Count;		/* number of entries in cfc_ColorTable */
    UWORD  *cfc_ColorTable;	/* 4 bit per component color map packed xRGB */
};

/*-----	ColorTextFont ----------------------------------------------*/
struct ColorTextFont {
    struct TextFont ctf_TF;
    UWORD   ctf_Flags;		/* extended flags */
    UBYTE   ctf_Depth;		/* number of bit planes */
    UBYTE   ctf_FgColor;	/* color that is remapped to FgPen */
    UBYTE   ctf_Low;		/* lowest color represented here */
    UBYTE   ctf_High;		/* highest color represented here */
    UBYTE   ctf_PlanePick;	/* PlanePick ala Images */
    UBYTE   ctf_PlaneOnOff;	/* PlaneOnOff ala Images */
    struct ColorFontColors *ctf_ColorFontColors; /* colors for font */
    APTR    ctf_CharData[8];	/*pointers to bit planes ala tf_CharData */
};

/****** TextExtent node *********************************************/
struct TextExtent {
    UWORD   te_Width;		/* same as TextLength */
    UWORD   te_Height;		/* same as tf_YSize */
    struct Rectangle te_Extent;	/* relative to CP */
};

/*
**	$VER: inputevent.h 47.1 (28.6.2019)
**
**	input event definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: timer.h 47.2 (24.8.2019)
**
**	Timer device name and useful definitions.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: io.h 47.1 (28.6.2019)
**
**	Message structures used for device communication
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct IORequest {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;	    /* unit (driver private)*/
    UWORD   io_Command;	    /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;		    /* error or warning num */
};

struct IOStdReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;	    /* unit (driver private)*/
    UWORD   io_Command;	    /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;		    /* error or warning num */
    ULONG   io_Actual;		    /* actual number of bytes transferred */
    ULONG   io_Length;		    /* requested number bytes transferred*/
    APTR    io_Data;		    /* points to data area */
    ULONG   io_Offset;		    /* offset for block structured devices */
};

/* library vector offsets for device reserved vectors */
/* io_Flags defined bits */
/* unit definitions */
/* These are the default definitions for now. If you need a different
   definition of the timeval and timerequest data structures, see below. */

struct TimeVal {
	ULONG tv_secs;
	ULONG tv_micro;
};

struct TimeRequest {
	struct IORequest tr_node;
	struct TimeVal   tr_time;
};

/****************************************************************************/

/* The 'struct timeval' definition used by AmigaOS since its introduction in
 * the year 1985 was similar to a data structure of the same name as used in
 * the Unix domain. Similar, but not identical. This had consequences when
 * porting software to the Amiga which came from a Unix/POSIX system, as it
 * clashed with the Amiga definition of the 'struct timeval'.
 *
 * Rather than use compatibility workarounds, the 'struct timeval' and
 * 'struct timerequest' data structures were replaced with 'struct TimeVal'
 * and 'struct TimeRequest', respectively. The respective structure members
 * (tr_node and tr_time for the 'struct timerequest' and 'struct TimeRequest'
 * alike and tv_secs and tv_micro for the 'struct timeval' and also the
 * 'struct TimeVal') remain unchanged. Please note that macros which
 * redefine tr_node, tr_time, tv_secs or tv_micro are not used so as to avoid
 * side-effects when rebuilding existing code.
 *
 * New types will be defined as "TimeVal_Type" and "TimeRequest_Type" so that
 * you may use these for casts with function parameters and in data structures.
 *
 * To check if these new types are defined, check if the preprocessor
 * definition __TIME_TYPES_DEFINED__ has been defined. If so, check if
 * __USE_NEW_TIMEVAL__ is defined as well, which means that 'struct TimeVal'
 * and 'struct TimeRequest' are being used.
 *
 * When building code which defines its own (e.g. POSIX) 'timeval' please
 * use "#define __USE_NEW_TIMEVAL__" prior to including the operating system
 * header files.
 */

struct timeval {
	ULONG tv_secs;
	ULONG tv_micro;
};

struct timerequest {
	struct IORequest tr_node;
	struct timeval   tr_time;
};

typedef struct timeval     TimeVal_Type;
typedef struct timerequest TimeRequest_Type;

/* Take note that these two types are now defined
 * and may be used.
 */
/* This is really a 64 bit integer value split into two 32 bit integers. */
struct EClockVal {
	ULONG ev_hi;
	ULONG ev_lo;
};


/* I/O command to use for adding a timer */
/* I/O commands for obtaining the current system time
 * and for setting it, respectively.
 */
/*----- constants --------------------------------------------------*/

/*  --- InputEvent.ie_Class --- */
/* A NOP input event */
/* A raw keycode from the keyboard device */
/* The raw mouse report from the game port device */
/* A private console event */
/* A Pointer Position report */
/* A timer event */
/* select button pressed down over a Gadget (address in ie_EventAddress) */
/* select button released over the same Gadget (address in ie_EventAddress) */
/* some Requester activity has taken place.  See Codes REQCLEAR and REQSET */
/* this is a Menu Number transmission (Menu number is in ie_Code) */
/* User has selected the active Window's Close Gadget */
/* this Window has a new size */
/* the Window pointed to by ie_EventAddress needs to be refreshed */
/* new preferences are available */
/* the disk has been removed */
/* the disk has been inserted */
/* the window is about to be been made active */
/* the window is about to be made inactive */
/* extended-function pointer position report (V36) */
/* Help key report during Menu session (V36) */
/* the Window has been modified with move, size, zoom, or change (V36) */
/* the last class */
/*  --- InputEvent.ie_SubClass --- */
/*  IECLASS_NEWPOINTERPOS */
/*	like IECLASS_POINTERPOS */
/*	ie_EventAddress points to struct IEPointerPixel */
/*	ie_EventAddress points to struct IEPointerTablet */
/*	ie_EventAddress points to struct IENewTablet */
/* pointed to by ie_EventAddress for IECLASS_NEWPOINTERPOS,
 * and IESUBCLASS_PIXEL.
 *
 * You specify a screen and pixel coordinates in that screen
 * at which you'd like the mouse to be positioned.
 * Intuition will try to oblige, but there will be restrictions
 * to positioning the pointer over offscreen pixels.
 *
 * IEQUALIFIER_RELATIVEMOUSE is supported for IESUBCLASS_PIXEL.
 */

struct IEPointerPixel	{
    struct Screen	*iepp_Screen;	/* pointer to an open screen */
    struct {				/* pixel coordinates in iepp_Screen */
	WORD	X;
	WORD	Y;
    }			iepp_Position;
};

/* pointed to by ie_EventAddress for IECLASS_NEWPOINTERPOS,
 * and IESUBCLASS_TABLET.
 *
 * You specify a range of values and a value within the range
 * independently for each of X and Y (the minimum value of
 * the ranges is always normalized to 0).
 *
 * Intuition will position the mouse proportionally within its
 * natural mouse position rectangle limits.
 *
 * IEQUALIFIER_RELATIVEMOUSE is not supported for IESUBCLASS_TABLET.
 */
struct IEPointerTablet	{
    struct {
	UWORD	X;
	UWORD	Y;
    }			iept_Range;	/* 0 is min, these are max	*/
    struct {
	UWORD	X;
	UWORD	Y;
    }			iept_Value;	/* between 0 and iept_Range	*/

    WORD		iept_Pressure;	/* -128 to 127 (unused, set to 0)  */
};


/* The ie_EventAddress of an IECLASS_NEWPOINTERPOS event of subclass
 * IESUBCLASS_NEWTABLET points at an IENewTablet structure.
 *
 *
 * IEQUALIFIER_RELATIVEMOUSE is not supported for IESUBCLASS_NEWTABLET.
 */

struct IENewTablet
{
    /* Pointer to a hook you wish to be called back through, in
     * order to handle scaling.  You will be provided with the
     * width and height you are expected to scale your tablet
     * to, perhaps based on some user preferences.
     * If NULL, the tablet's specified range will be mapped directly
     * to that width and height for you, and you will not be
     * called back.
     */
    struct Hook *ient_CallBack;

    /* Post-scaling coordinates and fractional coordinates.
     * DO NOT FILL THESE IN AT THE TIME THE EVENT IS WRITTEN!
     * Your driver will be called back and provided information
     * about the width and height of the area to scale the
     * tablet into.  It should scale the tablet coordinates
     * (perhaps based on some preferences controlling aspect
     * ratio, etc.) and place the scaled result into these
     * fields.	The ient_ScaledX and ient_ScaledY fields are
     * in screen-pixel resolution, but the origin ( [0,0]-point )
     * is not defined.	The ient_ScaledXFraction and
     * ient_ScaledYFraction fields represent sub-pixel position
     * information, and should be scaled to fill a UWORD fraction.
     */
    UWORD ient_ScaledX, ient_ScaledY;
    UWORD ient_ScaledXFraction, ient_ScaledYFraction;

    /* Current tablet coordinates along each axis: */
    ULONG ient_TabletX, ient_TabletY;

    /* Tablet range along each axis.  For example, if ient_TabletX
     * can take values 0-999, ient_RangeX should be 1000.
     */
    ULONG ient_RangeX, ient_RangeY;

    /* Pointer to tag-list of additional tablet attributes.
     * See <intuition/intuition.h> for the tag values.
     */
    struct TagItem *ient_TagList;
};


/*  --- InputEvent.ie_Code --- */
/*  IECLASS_RAWKEY */
/*  IECLASS_ANSI */
/*  IECLASS_RAWMOUSE */
/*  IECLASS_EVENT (V36) */
/*  IECLASS_REQUESTER */
/*	broadcast when the first Requester (not subsequent ones) opens up in */
/*	the Window */
/*	broadcast when the last Requester clears out of the Window */
/*  --- InputEvent.ie_Qualifier --- */
/*----- InputEvent -------------------------------------------------*/

struct InputEvent {
    struct  InputEvent *ie_NextEvent;	/* the chronologically next event */
    UBYTE   ie_Class;			/* the input event class */
    UBYTE   ie_SubClass;		/* optional subclass of the class */
    UWORD   ie_Code;			/* the input event code */
    UWORD   ie_Qualifier;		/* qualifiers in effect for the event*/
    union {
	struct {
	    WORD    ie_x;		/* the pointer position for the event*/
	    WORD    ie_y;
	} ie_xy;
	APTR	ie_addr;		/* the event address */
	struct {
	    UBYTE   ie_prev1DownCode;	/* previous down keys for dead */
	    UBYTE   ie_prev1DownQual;	/*   key translation: the ie_Code */
	    UBYTE   ie_prev2DownCode;	/*   & low byte of ie_Qualifier for */
	    UBYTE   ie_prev2DownQual;	/*   last & second last down keys */
	} ie_dead;
    } ie_position;
    TimeVal_Type ie_TimeStamp;	/* the system tick at the event */
};

/*
 * NOTE:  intuition/iobsolete.h is included at the END of this file!
 */

/* ======================================================================== */
/* === Menu =============================================================== */
/* ======================================================================== */
struct Menu
{
    struct Menu *NextMenu;	/* same level */
    WORD LeftEdge, TopEdge;	/* position of the select box */
    WORD Width, Height; 	/* dimensions of the select box */
    UWORD Flags;  		/* see flag definitions below */
    CONST_STRPTR MenuName;	/* text for this Menu Header */
    struct MenuItem *FirstItem; /* pointer to first in chain */

    /* these mysteriously-named variables are for internal use only */
    WORD JazzX, JazzY, BeatX, BeatY;
};


/* FLAGS SET BY BOTH THE APPLIPROG AND INTUITION */
/* FLAGS SET BY INTUITION */
/* ======================================================================== */
/* === MenuItem =========================================================== */
/* ======================================================================== */
struct MenuItem
{
    struct MenuItem *NextItem;	/* pointer to next in chained list */
    WORD LeftEdge, TopEdge;	/* position of the select box */
    WORD Width, Height;		/* dimensions of the select box */
    UWORD Flags;		/* see the defines below */

    LONG MutualExclude;		/* set bits mean this item excludes that */

    APTR ItemFill;		/* points to Image, IntuiText, or NULL */

    /* when this item is pointed to by the cursor and the items highlight
     *	mode HIGHIMAGE is selected, this alternate image will be displayed
     */
    APTR SelectFill;		/* points to Image, IntuiText, or NULL */

    BYTE Command;		/* only if appliprog sets the COMMSEQ flag */

    struct MenuItem *SubItem;	/* if non-zero, points to MenuItem for submenu */

    /* The NextSelect field represents the menu number of next selected
     *  item (when user has drag-selected several items)
     */
    UWORD NextSelect;
};


/* FLAGS SET BY THE APPLIPROG */
/* these are the SPECIAL HIGHLIGHT FLAG state meanings */
/* FLAGS SET BY BOTH APPLIPROG AND INTUITION */
/* FLAGS SET BY INTUITION */
/* ======================================================================== */
/* === Requester ========================================================== */
/* ======================================================================== */
struct Requester
{
    struct Requester *OlderRequest;
    WORD LeftEdge, TopEdge;		/* dimensions of the entire box */
    WORD Width, Height;			/* dimensions of the entire box */
    WORD RelLeft, RelTop;		/* for Pointer relativity offsets */

    struct Gadget *ReqGadget;		/* pointer to a list of Gadgets */
    struct Border *ReqBorder;		/* the box's border */
    struct IntuiText *ReqText;		/* the box's text */
    UWORD Flags;			/* see definitions below */

    /* pen number for back-plane fill before draws */
    UBYTE BackFill;
    /* Layer in place of clip rect	*/
    struct Layer *ReqLayer;

    UBYTE ReqPad1[32];

    /* If the BitMap plane pointers are non-zero, this tells the system
     * that the image comes pre-drawn (if the appliprog wants to define
     * its own box, in any shape or size it wants!);  this is OK by
     * Intuition as long as there's a good correspondence between
     * the image and the specified Gadgets
     */
    struct BitMap *ImageBMap;	/* points to the BitMap of PREDRAWN imagery */
    struct Window *RWindow;	/* added.  points back to Window */

    struct Image  *ReqImage;	/* new for V36: drawn if USEREQIMAGE set */

    UBYTE ReqPad2[32];
};


/* FLAGS SET BY THE APPLIPROG */
			  /* if POINTREL set, TopLeft is relative to pointer
			   * for DMRequester, relative to window center
			   * for Request().
			   */
	/* set if Requester.ImageBMap points to predrawn Requester imagery */
	/* if you don't want requester to filter input	   */
	/* to use SIMPLEREFRESH layer (recommended)	*/

/* New for V36		*/
	/*  render linked list ReqImage after BackFill
	 * but before gadgets and text
	 */
	/* don't bother filling requester with Requester.BackFill pen	*/


/* FLAGS SET BY INTUITION */
/* ======================================================================== */
/* === Gadget ============================================================= */
/* ======================================================================== */
struct Gadget
{
    struct Gadget *NextGadget;  /* next gadget in the list */

    WORD LeftEdge, TopEdge;	/* "hit box" of gadget */
    WORD Width, Height;		/* "hit box" of gadget */

    UWORD Flags; 		/* see below for list of defines */

    UWORD Activation;		/* see below for list of defines */

    UWORD GadgetType;		/* see below for defines */

    /* appliprog can specify that the Gadget be rendered as either as Border
     * or an Image.  This variable points to which (or equals NULL if there's
     * nothing to be rendered about this Gadget)
     */
    APTR GadgetRender;

    /* appliprog can specify "highlighted" imagery rather than algorithmic
     * this can point to either Border or Image data
     */
    APTR SelectRender;

    struct IntuiText *GadgetText;   /* text for this gadget */

    /* MutualExclude, never implemented, is now declared obsolete.
     * There are published examples of implementing a more general
     * and practical exclusion in your applications.
     *
     * Starting with V36, this field is used to point to a hook
     * for a custom gadget.
     *
     * Programs using this field for their own processing will
     * continue to work, as long as they don't try the
     * trick with custom gadgets.
     */
    LONG MutualExclude;  /* obsolete */

    /* pointer to a structure of special data required by Proportional,
     * String and Integer Gadgets
     */
    APTR SpecialInfo;

    UWORD GadgetID;	/* user-definable ID field */
    APTR UserData;	/* ptr to general purpose User data (ignored by In) */
};


struct ExtGadget
{
    /* The first fields match struct Gadget exactly */
    struct ExtGadget *NextGadget; /* Matches struct Gadget */
    WORD LeftEdge, TopEdge;	  /* Matches struct Gadget */
    WORD Width, Height;		  /* Matches struct Gadget */
    UWORD Flags;		  /* Matches struct Gadget */
    UWORD Activation;		  /* Matches struct Gadget */
    UWORD GadgetType;		  /* Matches struct Gadget */
    APTR GadgetRender;		  /* Matches struct Gadget */
    APTR SelectRender;		  /* Matches struct Gadget */
    struct IntuiText *GadgetText; /* Matches struct Gadget */
    LONG MutualExclude;		  /* Matches struct Gadget */
    APTR SpecialInfo;		  /* Matches struct Gadget */
    UWORD GadgetID;		  /* Matches struct Gadget */
    APTR UserData;		  /* Matches struct Gadget */

    /* These fields only exist under V39 and only if GFLG_EXTENDED is set */
    ULONG MoreFlags;		/* see GMORE_ flags below */
    WORD BoundsLeftEdge;	/* Bounding extent for gadget, valid   */
    WORD BoundsTopEdge;		/* only if GMORE_BOUNDS is set.  The   */
    WORD BoundsWidth;		/* GFLG_RELxxx flags affect these      */
    WORD BoundsHeight;		/* coordinates as well.                */
};


/* --- Gadget.Flags values	--- */
/* combinations in these bits describe the highlight technique to be used */
/* combinations in these next two bits specify to which corner the gadget's
 *  Left & Top coordinates are relative.  If relative to Top/Left,
 *  these are "normal" coordinates (everything is relative to something in
 *  this universe).
 *
 * Gadget positions and dimensions are relative to the window or
 * requester which contains the gadget
 */
/* New for V39: GFLG_RELSPECIAL allows custom gadget implementors to
 * make gadgets whose position and size depend in an arbitrary way
 * on their window's dimensions.  The GM_LAYOUT method will be invoked
 * for such a gadget (or any other GREL_xxx gadget) at suitable times,
 * such as when the window opens or the window's size changes.
 */
/* the GFLG_DISABLED flag is initialized by you and later set by Intuition
 * according to your calls to On/OffGadget().  It specifies whether or not
 * this Gadget is currently disabled from being selected
 */
/* These flags specify the type of text field that Gadget.GadgetText
 * points to.  In all normal (pre-V36) gadgets which you initialize
 * this field should always be zero.  Some types of gadget objects
 * created from classes will use these fields to keep track of
 * types of labels/contents that different from IntuiText, but are
 * stashed in GadgetText.
 */

/* New for V37: GFLG_TABCYCLE */
/* New for V37: GFLG_STRINGEXTEND.  We discovered that V34 doesn't properly
 * ignore the value we had chosen for the Gadget->Activation flag
 * GACT_STRINGEXTEND.  NEVER SET THAT FLAG WHEN RUNNING UNDER V34.
 * The Gadget->Flags bit GFLG_STRINGEXTEND is provided as a synonym which is
 * safe under V34, and equivalent to GACT_STRINGEXTEND under V37.
 * (Note that the two flags are not numerically equal)
 */
/* New for V39: GFLG_IMAGEDISABLE.  This flag is automatically set if
 * the custom image of this gadget knows how to do disabled rendering
 * (more specifically, if its IA_SupportsDisable attribute is TRUE).
 * Intuition uses this to defer the ghosting to the image-class,
 * instead of doing it itself (the old compatible way).
 * Do not set this flag yourself - Intuition will do it for you.
 */

/* New for V39:  If set, this bit means that the Gadget is actually
 * a struct ExtGadget, with new fields and flags.  All V39 boopsi
 * gadgets are ExtGadgets.  Never ever attempt to read the extended
 * fields of a gadget if this flag is not set.
 */
/* ---	Gadget.Activation flag values	--- */
/* Set GACT_RELVERIFY if you want to verify that the pointer was still over
 * the gadget when the select button was released.  Will cause
 * an IDCMP_GADGETUP message to be sent if so.
 */
/* the flag GACT_IMMEDIATE, when set, informs the caller that the gadget
 *  was activated when it was activated.  This flag works in conjunction with
 *  the GACT_RELVERIFY flag
 */
/* the flag GACT_ENDGADGET, when set, tells the system that this gadget,
 * when selected, causes the Requester to be ended.  Requesters
 * that are ended are erased and unlinked from the system.
 */
/* the GACT_FOLLOWMOUSE flag, when set, specifies that you want to receive
 * reports on mouse movements while this gadget is active.
 * You probably want to set the GACT_IMMEDIATE flag when using
 * GACT_FOLLOWMOUSE, since that's the only reasonable way you have of
 * learning why Intuition is suddenly sending you a stream of mouse
 * movement events.  If you don't set GACT_RELVERIFY, you'll get at
 * least one Mouse Position event.
 * Note: boolean FOLLOWMOUSE gadgets require GACT_RELVERIFY to get
 * _any_ mouse movement events (this unusual behavior is a compatibility
 * hold-over from the old days).
 */
/* if any of the BORDER flags are set in a Gadget that's included in the
 * Gadget list when a Window is opened, the corresponding Border will
 * be adjusted to make room for the Gadget
 */
/* should properly be in StringInfo, but aren't	*/
				  /* NOTE: NEVER SET GACT_STRINGEXTEND IF YOU
				   * ARE RUNNING ON LESS THAN V36!  SEE
				   * GFLG_STRINGEXTEND (ABOVE) INSTEAD
				   */

/* note 0x8000 is used above (GACT_BORDERSNIFF);
 * all Activation flags defined */

/* --- GADGET TYPES ------------------------------------------------------- */
/* These are the Gadget Type definitions for the variable GadgetType
 * gadget number type MUST start from one.  NO TYPES OF ZERO ALLOWED.
 * first comes the mask for Gadget flags reserved for Gadget typing
 */
/* GTYP_SYSGADGET means that Intuition ALLOCATED the gadget.
 * GTYP_SYSTYPEMASK is the mask you can apply to tell what type of
 * system-gadget it is.  The possible types follow.
 */
/* These definitions describe system gadgets in V36 and higher: */
/* These definitions describe system gadgets prior to V36: */
/* GTYP_GTYPEMASK is a mask you can apply to tell what class
 * of gadget this is.  The possible classes follow.
 */
/* This bit in GadgetType is reserved for undocumented internal use
 * by the Gadget Toolkit, and cannot be used nor relied on by
 * applications:	0x0100
 */

/* New for V39.  Gadgets which have the GFLG_EXTENDED flag set are
 * actually ExtGadgets, which have more flags.  The GMORE_xxx
 * identifiers describe those flags.  For GMORE_SCROLLRASTER, see
 * important information in the ScrollWindowRaster() autodoc.
 * NB: GMORE_SCROLLRASTER must be set before the gadget is
 * added to a window.
 */
/* ======================================================================== */
/* === BoolInfo======================================================= */
/* ======================================================================== */
/* This is the special data needed by an Extended Boolean Gadget
 * Typically this structure will be pointed to by the Gadget field SpecialInfo
 */
struct BoolInfo
{
    UWORD  Flags;	/* defined below */
    UWORD  *Mask;	/* bit mask for highlighting and selecting
			 * mask must follow the same rules as an Image
			 * plane.  Its width and height are determined
			 * by the width and height of the gadget's
			 * select box. (i.e. Gadget.Width and .Height).
			 */
    ULONG  Reserved;	/* set to 0	*/
};

/* set BoolInfo.Flags to this flag bit.
 * in the future, additional bits might mean more stuff hanging
 * off of BoolInfo.Reserved.
 */
/* ======================================================================== */
/* === PropInfo =========================================================== */
/* ======================================================================== */
/* this is the special data required by the proportional Gadget
 * typically, this data will be pointed to by the Gadget variable SpecialInfo
 */
struct PropInfo
{
    UWORD Flags;	/* general purpose flag bits (see defines below) */

    /* You initialize the Pot variables before the Gadget is added to
     * the system.  Then you can look here for the current settings
     * any time, even while User is playing with this Gadget.  To
     * adjust these after the Gadget is added to the System, use
     * ModifyProp();  The Pots are the actual proportional settings,
     * where a value of zero means zero and a value of MAXPOT means
     * that the Gadget is set to its maximum setting.
     */
    UWORD HorizPot;	/* 16-bit FixedPoint horizontal quantity percentage */
    UWORD VertPot;	/* 16-bit FixedPoint vertical quantity percentage */

    /* the 16-bit FixedPoint Body variables describe what percentage of
     * the entire body of stuff referred to by this Gadget is actually
     * shown at one time.  This is used with the AUTOKNOB routines,
     * to adjust the size of the AUTOKNOB according to how much of
     * the data can be seen.  This is also used to decide how far
     * to advance the Pots when User hits the Container of the Gadget.
     * For instance, if you were controlling the display of a 5-line
     * Window of text with this Gadget, and there was a total of 15
     * lines that could be displayed, you would set the VertBody value to
     *     (MAXBODY / (TotalLines / DisplayLines)) = MAXBODY / 3.
     * Therefore, the AUTOKNOB would fill 1/3 of the container, and
     * if User hits the Cotainer outside of the knob, the pot would
     * advance 1/3 (plus or minus) If there's no body to show, or
     * the total amount of displayable info is less than the display area,
     * set the Body variables to the MAX.  To adjust these after the
     * Gadget is added to the System, use ModifyProp();
     */
    UWORD HorizBody;		/* horizontal Body */
    UWORD VertBody;		/* vertical Body */

    /* these are the variables that Intuition sets and maintains */
    UWORD CWidth;	/* Container width (with any relativity absoluted) */
    UWORD CHeight;	/* Container height (with any relativity absoluted) */
    UWORD HPotRes, VPotRes;	/* pot increments */
    UWORD LeftBorder;		/* Container borders */
    UWORD TopBorder;		/* Container borders */
};


/* --- FLAG BITS ---------------------------------------------------------- */
/* NOTE: if you do not use an AUTOKNOB for a proportional gadget,
 * you are currently limited to using a single Image of your own
 * design: Intuition won't handle a linked list of images as
 * a proportional gadget knob.
 */

/* ======================================================================== */
/* === StringInfo ========================================================= */
/* ======================================================================== */
/* this is the special data required by the string Gadget
 * typically, this data will be pointed to by the Gadget variable SpecialInfo
 */
struct StringInfo
{
    /* you initialize these variables, and then Intuition maintains them */
    STRPTR Buffer;	/* the buffer containing the start and final string */
    STRPTR UndoBuffer;	/* optional buffer for undoing current entry */
    WORD BufferPos;	/* character position in Buffer */
    WORD MaxChars;	/* max number of chars in Buffer (including NULL) */
    WORD DispPos;	/* Buffer position of first displayed character */

    /* Intuition initializes and maintains these variables for you */
    WORD UndoPos;	/* character position in the undo buffer */
    WORD NumChars;	/* number of characters currently in Buffer */
    WORD DispCount;	/* number of whole characters visible in Container */
    WORD CLeft, CTop;	/* topleft offset of the container */

    /* This unused field is changed to allow extended specification
     * of string gadget parameters.  It is ignored unless the flag
     * GACT_STRINGEXTEND is set in the Gadget's Activation field
     * or the GFLG_STRINGEXTEND flag is set in the Gadget Flags field.
     * (See GFLG_STRINGEXTEND for an important note)
     */
    /* struct Layer *LayerPtr;	--- obsolete --- */
    struct StringExtend *Extension;

    /* you can initialize this variable before the gadget is submitted to
     * Intuition, and then examine it later to discover what integer
     * the user has entered (if the user never plays with the gadget,
     * the value will be unchanged from your initial setting)
     */
    LONG LongInt;

    /* If you want this Gadget to use your own Console keymapping, you
     * set the GACT_ALTKEYMAP bit in the Activation flags of the Gadget,
     * and then set this variable to point to your keymap.  If you don't
     * set the GACT_ALTKEYMAP, you'll get the standard ASCII keymapping.
     */
    struct KeyMap *AltKeyMap;
};

/* ======================================================================== */
/* === IntuiText ========================================================== */
/* ======================================================================== */
/* IntuiText is a series of strings that start with a location
 *  (always relative to the upper-left corner of something) and then the
 *  text of the string.  The text is null-terminated.
 */
struct IntuiText
{
    UBYTE FrontPen, BackPen;	/* the pen numbers for the rendering */
    UBYTE DrawMode;		/* the mode for rendering the text */
    WORD LeftEdge;		/* relative start location for the text */
    WORD TopEdge;		/* relative start location for the text */
    const struct TextAttr *ITextFont;	/* if NULL, you accept the default */
    STRPTR IText;		/* pointer to null-terminated text */
    struct IntuiText *NextText; /* pointer to another IntuiText to render */
};






/* ======================================================================== */
/* === Border ============================================================= */
/* ======================================================================== */
/* Data type Border, used for drawing a series of lines which is intended for
 *  use as a border drawing, but which may, in fact, be used to render any
 *  arbitrary vector shape.
 *  The routine DrawBorder sets up the RastPort with the appropriate
 *  variables, then does a Move to the first coordinate, then does Draws
 *  to the subsequent coordinates.
 *  After all the Draws are done, if NextBorder is non-zero we call DrawBorder
 *  on NextBorder
 */
struct Border
{
    WORD LeftEdge, TopEdge;	/* initial offsets from the origin */
    UBYTE FrontPen, BackPen;	/* pens numbers for rendering */
    UBYTE DrawMode;		/* mode for rendering */
    BYTE Count;			/* number of XY pairs */
    WORD *XY;			/* vector coordinate pairs rel to LeftTop */
    struct Border *NextBorder;	/* pointer to any other Border too */
};






/* ======================================================================== */
/* === Image ============================================================== */
/* ======================================================================== */
/* This is a brief image structure for very simple transfers of
 * image data to a RastPort
 */
struct Image
{
    WORD LeftEdge;		/* starting offset relative to some origin */
    WORD TopEdge;		/* starting offsets relative to some origin */
    WORD Width;			/* pixel size (though data is word-aligned) */
    WORD Height;
    WORD Depth;			/* >= 0, for images you create		*/
    UWORD *ImageData;		/* pointer to the actual word-aligned bits */

    /* the PlanePick and PlaneOnOff variables work much the same way as the
     * equivalent GELS Bob variables.  It's a space-saving
     * mechanism for image data.  Rather than defining the image data
     * for every plane of the RastPort, you need define data only
     * for the planes that are not entirely zero or one.  As you
     * define your Imagery, you will often find that most of the planes
     * ARE just as color selectors.  For instance, if you're designing
     * a two-color Gadget to use colors one and three, and the Gadget
     * will reside in a five-plane display, bit plane zero of your
     * imagery would be all ones, bit plane one would have data that
     * describes the imagery, and bit planes two through four would be
     * all zeroes.  Using these flags avoids wasting all
     * that memory in this way:  first, you specify which planes you
     * want your data to appear in using the PlanePick variable.  For
     * each bit set in the variable, the next "plane" of your image
     * data is blitted to the display.  For each bit clear in this
     * variable, the corresponding bit in PlaneOnOff is examined.
     * If that bit is clear, a "plane" of zeroes will be used.
     * If the bit is set, ones will go out instead.  So, for our example:
     *   Gadget.PlanePick = 0x02;
     *   Gadget.PlaneOnOff = 0x01;
     * Note that this also allows for generic Gadgets, like the
     * System Gadgets, which will work in any number of bit planes.
     * Note also that if you want an Image that is only a filled
     * rectangle, you can get this by setting PlanePick to zero
     * (pick no planes of data) and set PlaneOnOff to describe the pen
     * color of the rectangle.
     *
     * NOTE:  Intuition relies on PlanePick to know how many planes
     * of data are found in ImageData.  There should be no more
     * '1'-bits in PlanePick than there are planes in ImageData.
     */
    UBYTE PlanePick, PlaneOnOff;

    /* if the NextImage variable is not NULL, Intuition presumes that
     * it points to another Image structure with another Image to be
     * rendered
     */
    struct Image *NextImage;
};






/* ======================================================================== */
/* === IntuiMessage ======================================================= */
/* ======================================================================== */
struct IntuiMessage
{
    struct Message ExecMessage;

    /* the Class bits correspond directly with the IDCMP Flags, except for the
     * special bit IDCMP_LONELYMESSAGE (defined below)
     */
    ULONG Class;

    /* the Code field is for special values like MENU number */
    UWORD Code;

    /* the Qualifier field is a copy of the current InputEvent's Qualifier */
    UWORD Qualifier;

    /* IAddress contains particular addresses for Intuition functions, like
     * the pointer to the Gadget or the Screen
     */
    APTR IAddress;

    /* when getting mouse movement reports, any event you get will have the
     * the mouse coordinates in these variables.  the coordinates are relative
     * to the upper-left corner of your Window (WFLG_GIMMEZEROZERO
     * notwithstanding).  If IDCMP_DELTAMOVE is set, these values will
     * be deltas from the last reported position.
     */
    WORD MouseX, MouseY;

    /* the time values are copies of the current system clock time.  Micros
     * are in units of microseconds, Seconds in seconds.
     */
    ULONG Seconds, Micros;

    /* the IDCMPWindow variable will always have the address of the Window of
     * this IDCMP
     */
    struct Window *IDCMPWindow;

    /* system-use variable */
    struct IntuiMessage *SpecialLink;
};

/* New for V39:
 * All IntuiMessages are now slightly extended.  The ExtIntuiMessage
 * structure has an additional field for tablet data, which is usually
 * NULL.  If a tablet driver which is sending IESUBCLASS_NEWTABLET
 * events is installed in the system, windows with the WA_TabletMessages
 * property set will find that eim_TabletData points to the TabletData
 * structure.  Applications must first check that this field is non-NULL;
 * it will be NULL for certain kinds of message, including mouse activity
 * generated from other than the tablet (i.e. the keyboard equivalents
 * or the mouse itself).
 *
 * NEVER EVER examine any extended fields when running under pre-V39!
 *
 * NOTE: This structure is subject to grow in the future.  Making
 * assumptions about its size is A BAD IDEA.
 */

struct ExtIntuiMessage
{
    struct IntuiMessage eim_IntuiMessage;
    struct TabletData *eim_TabletData;
};

/* New for V47 & V51:
 * The IAddress field of IDCMP_EXTENDEDMOUSE messages points to the
 * following structure. Always check the Code field of the IntuiMessage
 * against IMSGCODE_INTUIWHEELDATA, future versions of Intuition may introduce
 * additional structures!
 */

struct IntuiWheelData
{
    UWORD Version;  /* version of this structure (see below) */
    UWORD Reserved; /* always 0, reserved for future use     */
    WORD  WheelX;   /* horizontal wheel movement delta       */
    WORD  WheelY;   /* vertical wheel movement delta         */
    struct Gadget *HoveredGadget; /* Version 2: pointer to gadget that is hovered */
};

enum
{
    IMSGCODE_INTUIWHEELDATA  = (1<<15),
    IMSGCODE_INTUIRAWKEYDATA = (1<<14), /* reserved by V51 */
    IMSGCODE_INTUIWHEELDATAREJECT = (1<<13) /* reserved for V47 for internal use */
};

/* --- IDCMP Classes ------------------------------------------------------ */
/* Please refer to the Autodoc for OpenWindow() and to the Rom Kernel
 * Manual for full details on the IDCMP classes.
 */
/*  for notifications from "boopsi" gadgets	*/
/* for getting help key report during menu session	*/
/* for notification of any move/size/zoom/change window		*/
/* NOTEZ-BIEN: 			0x80000000 is reserved for internal use   */

/* the IDCMP Flags do not use this special bit, which is cleared when
 * Intuition sends its special message to the Task, and set when Intuition
 * gets its Message back from the Task.  Therefore, I can check here to
 * find out fast whether or not this Message is available for me to send
 */
/* --- IDCMP Codes -------------------------------------------------------- */
/* This group of codes is for the IDCMP_CHANGEWINDOW message */
/* This group of codes is for the IDCMP_MENUVERIFY message */
/* These are internal tokens to represent state of verification attempts
 * shown here as a clue.
 */
/* This group of codes is for the IDCMP_WBENCHMESSAGE messages */
/* A data structure common in V36 Intuition processing	*/
struct IBox
{
    WORD Left;
    WORD Top;
    WORD Width;
    WORD Height;
};



/* ======================================================================== */
/* === Window ============================================================= */
/* ======================================================================== */
struct Window
{
    struct Window *NextWindow;		/* for the linked list in a screen */

    WORD LeftEdge, TopEdge;		/* screen dimensions of window */
    WORD Width, Height;			/* screen dimensions of window */

    WORD MouseY, MouseX;		/* relative to upper-left of window */

    WORD MinWidth, MinHeight;		/* minimum sizes */
    UWORD MaxWidth, MaxHeight;		/* maximum sizes */

    ULONG Flags;  			/* see below for defines */

    struct Menu *MenuStrip;		/* the strip of Menu headers */

    STRPTR Title;			/* the title text for this window */

    struct Requester *FirstRequest;	/* all active Requesters */

    struct Requester *DMRequest;	/* double-click Requester */

    WORD ReqCount;			/* count of reqs blocking Window */

    struct Screen *WScreen;		/* this Window's Screen */
    struct RastPort *RPort;		/* this Window's very own RastPort */

    /* the border variables describe the window border.  If you specify
     * WFLG_GIMMEZEROZERO when you open the window, then the upper-left of
     * the ClipRect for this window will be upper-left of the BitMap (with
     * correct offsets when in SuperBitMap mode; you MUST select
     * WFLG_GIMMEZEROZERO when using SuperBitMap).  If you don't specify
     * ZeroZero, then you save memory (no allocation of RastPort, Layer,
     * ClipRect and associated Bitmaps), but you also must offset all your
     * writes by BorderTop, BorderLeft and do your own mini-clipping to
     * prevent writing over the system gadgets
     */
    BYTE BorderLeft, BorderTop, BorderRight, BorderBottom;
    struct RastPort *BorderRPort;


    /* You supply a linked-list of Gadgets for your Window.
     * This list DOES NOT include system gadgets.  You get the standard
     * window system gadgets by setting flag-bits in the variable Flags (see
     * the bit definitions below)
     */
    struct Gadget *FirstGadget;

    /* these are for opening/closing the windows */
    struct Window *Parent, *Descendant;

    /* sprite data information for your own Pointer
     * set these AFTER you Open the Window by calling SetPointer()
     */
    UWORD *Pointer;	/* sprite data */
    BYTE PtrHeight;	/* sprite height (not including sprite padding) */
    BYTE PtrWidth;	/* sprite width (must be less than or equal to 16) */
    BYTE XOffset, YOffset;	/* sprite offsets */

    /* the IDCMP Flags and User's and Intuition's Message Ports */
    ULONG IDCMPFlags;	/* User-selected flags */
    struct MsgPort *UserPort, *WindowPort;
    struct IntuiMessage *MessageKey;

    UBYTE DetailPen, BlockPen;	/* for bar/border/gadget rendering */

    /* the CheckMark is a pointer to the imagery that will be used when
     * rendering MenuItems of this Window that want to be checkmarked
     * if this is equal to NULL, you'll get the default imagery
     */
    struct Image *CheckMark;

    STRPTR ScreenTitle;	/* if non-null, Screen title when Window is active */

    /* These variables have the mouse coordinates relative to the
     * inner-Window of WFLG_GIMMEZEROZERO Windows.  This is compared with the
     * MouseX and MouseY variables, which contain the mouse coordinates
     * relative to the upper-left corner of the Window, WFLG_GIMMEZEROZERO
     * notwithstanding
     */
    WORD GZZMouseX;
    WORD GZZMouseY;
    /* these variables contain the width and height of the inner-Window of
     * WFLG_GIMMEZEROZERO Windows
     */
    WORD GZZWidth;
    WORD GZZHeight;

    UBYTE *ExtData;

    BYTE *UserData;	/* general-purpose pointer to User data extension */

    /** 11/18/85: this pointer keeps a duplicate of what
     * Window.RPort->Layer is _supposed_ to be pointing at
     */
    struct Layer *WLayer;

    /* NEW 1.2: need to keep track of the font that
     * OpenWindow opened, in case user SetFont's into RastPort
     */
    struct TextFont *IFont;

    /* (V36) another flag word (the Flags field is used up).
     * At present, all flag values are system private.
     * Until further notice, you may not change nor use this field.
     */
    ULONG	MoreFlags;

    /**** Data beyond this point are Intuition Private.  DO NOT USE ****/
};


/* --- Flags requested at OpenWindow() time by the application --------- */
/* --- refresh modes ------------------------------------------------------ */
/* combinations of the WFLG_REFRESHBITS select the refresh type */
/* --- Other User Flags --------------------------------------------------- */
/* - V36 new Flags which the programmer may specify in NewWindow.Flags  */
					/* see struct ExtNewWindow	*/

/* - V39 new Flags which the programmer may specify in NewWindow.Flags  */
/* These flags are set only by Intuition.  YOU MAY NOT SET THEM YOURSELF! */
/* V36 and higher flags to be set only by Intuition: */
/* --- Other Window Values ---------------------------------------------- */
/* --- see struct IntuiMessage for the IDCMP Flag definitions ------------- */


/* ======================================================================== */
/* === NewWindow ========================================================== */
/* ======================================================================== */
/*
 * Note that the new extension fields have been removed.  Use ExtNewWindow
 * structure below to make use of these fields
 */
struct NewWindow
{
    WORD LeftEdge, TopEdge;		/* screen dimensions of window */
    WORD Width, Height;			/* screen dimensions of window */

    UBYTE DetailPen, BlockPen;		/* for bar/border/gadget rendering */

    ULONG IDCMPFlags;			/* User-selected IDCMP flags */

    ULONG Flags;  			/* see Window struct for defines */

    /* You supply a linked-list of Gadgets for your Window.
     *  This list DOES NOT include system Gadgets.  You get the standard
     *  system Window Gadgets by setting flag-bits in the variable Flags (see
     *  the bit definitions under the Window structure definition)
     */
    struct Gadget *FirstGadget;

    /* the CheckMark is a pointer to the imagery that will be used when
     * rendering MenuItems of this Window that want to be checkmarked
     * if this is equal to NULL, you'll get the default imagery
     */
    struct Image *CheckMark;

    STRPTR Title;			  /* the title text for this window */

    /* the Screen pointer is used only if you've defined a CUSTOMSCREEN and
     * want this Window to open in it.  If so, you pass the address of the
     * Custom Screen structure in this variable.  Otherwise, this variable
     * is ignored and doesn't have to be initialized.
     */
    struct Screen *Screen;

    /* WFLG_SUPER_BITMAP Window?  If so, put the address of your BitMap
     * structure in this variable.  If not, this variable is ignored and
     * doesn't have to be initialized
     */
    struct BitMap *BitMap;

    /* the values describe the minimum and maximum sizes of your Windows.
     * these matter only if you've chosen the WFLG_SIZEGADGET option,
     * which means that you want to let the User to change the size of
     * this Window.  You describe the minimum and maximum sizes that the
     * Window can grow by setting these variables.  You can initialize
     * any one these to zero, which will mean that you want to duplicate
     * the setting for that dimension (if MinWidth == 0, MinWidth will be
     * set to the opening Width of the Window).
     * You can change these settings later using SetWindowLimits().
     * If you haven't asked for a SIZING Gadget, you don't have to
     * initialize any of these variables.
     */
    WORD MinWidth, MinHeight;       /* minimums */
    UWORD MaxWidth, MaxHeight;       /* maximums */

    /* the type variable describes the Screen in which you want this Window to
     * open.  The type value can either be CUSTOMSCREEN or one of the
     * system standard Screen Types such as WBENCHSCREEN.  See the
     * type definitions under the Screen structure.
     */
    UWORD Type;

};

/* The following structure is the future NewWindow.  Compatibility
 * issues require that the size of NewWindow not change.
 * Data in the common part (NewWindow) indicates the the extension
 * fields are being used.
 * NOTE WELL: This structure may be subject to future extension.
 * Writing code depending on its size is not allowed.
 */
struct ExtNewWindow
{
    WORD LeftEdge, TopEdge;
    WORD Width, Height;

    UBYTE DetailPen, BlockPen;
    ULONG IDCMPFlags;
    ULONG Flags;
    struct Gadget *FirstGadget;

    struct Image *CheckMark;

    STRPTR Title;
    struct Screen *Screen;
    struct BitMap *BitMap;

    WORD MinWidth, MinHeight;
    UWORD MaxWidth, MaxHeight;

    /* the type variable describes the Screen in which you want this Window to
     * open.  The type value can either be CUSTOMSCREEN or one of the
     * system standard Screen Types such as WBENCHSCREEN.  See the
     * type definitions under the Screen structure.
     * A new possible value for this field is PUBLICSCREEN, which
     * defines the window as a 'visitor' window.  See below for
     * additional information provided.
     */
    UWORD Type;

    /* ------------------------------------------------------- *
     * extensions for V36
     * if the NewWindow Flag value WFLG_NW_EXTENDED is set, then
     * this field is assumed to point to an array ( or chain of arrays)
     * of TagItem structures.  See also ExtNewScreen for another
     * use of TagItems to pass optional data.
     *
     * see below for tag values and the corresponding data.
     */
    struct TagItem	*Extension;
};

/*
 * The TagItem ID's (ti_Tag values) for OpenWindowTagList() follow.
 * They are values in a TagItem array passed as extension/replacement
 * values for the data in NewWindow.  OpenWindowTagList() can actually
 * work well with a NULL NewWindow pointer.
 */

/* these tags simply override NewWindow parameters */
			/* If neither WA_Left nor WA_Top is given,
			 * and WA_AutoAdjust is TRUE, then intuition
			 * will pick a location for your window
			 * itself. The current algorithm is to place
			 * the title bar centered under the pointer.
			 * (added in V47).
			 */
			/* "bulk" initialization of NewWindow.Flags */
			/* means you don't have to call SetWindowTitles
			 * after you open your window
			 */
			/* also implies WFLG_SUPER_BITMAP property	*/
/* The following are specifications for new features	*/

			/* You can specify the dimensions of the interior
			 * region of your window, independent of what
			 * the border widths will be.  You probably want
			 * to also specify WA_AutoAdjust to allow
			 * Intuition to move your window or even
			 * shrink it so that it is completely on screen.
			 */

			/* declares that you want the window to open as
			 * a visitor on the public screen whose name is
			 * pointed to by (STRPTR) ti_Data
			 */
			/* open as a visitor window on the public screen
			 * whose address is in (struct Screen *) ti_Data.
			 * To ensure that this screen remains open, you
			 * should either be the screen's owner, have a
			 * window open on the screen, or use LockPubScreen().
			 */
			/* A Boolean, specifies whether a visitor window
			 * should "fall back" to the default public screen
			 * (or Workbench) if the named public screen isn't
			 * available
			 */
			/* not implemented	*/
			/* a ColorSpec array for colors to be set
			 * when this window is active.  This is not
			 * implemented, and may not be, since the default
			 * values to restore would be hard to track.
			 * We'd like to at least support per-window colors
			 * for the mouse pointer sprite.
			 */
			/* ti_Data points to an array of four WORD's,
			 * the initial Left/Top/Width/Height values of
			 * the "alternate" zoom position/dimensions.
			 * It also specifies that you want a Zoom gadget
			 * for your window, whether or not you have a
			 * sizing gadget.
			 */
			/* ti_Data contains initial value for the mouse
			 * message backlog limit for this window.
			 */
			/* provides a "backfill hook" for your window's Layer.
			 * See layers.library/CreateUpfrontHookLayer().
			 */
			/* initial value of repeat key backlog limit	*/

    /* These Boolean tag items are alternatives to the NewWindow.Flags
     * boolean flags with similar names.
     */
			/* only specify if TRUE	*/
			/* only specify if TRUE	*/
    /* New Boolean properties	*/
			/* shift or squeeze the window's position and
			 * dimensions to fit it on screen.
			 */

			/* equiv. to NewWindow.Flags WFLG_GIMMEZEROZERO	*/

/* New for V37: WA_MenuHelp (ignored by V36) */
			/* Enables IDCMP_MENUHELP:  Pressing HELP during menus
			 * will return IDCMP_MENUHELP message.
			 */

/* New for V39:  (ignored by V37 and earlier) */
			/* Set to TRUE if you want NewLook menus */
			/* Pointer to image for Amiga-key equiv in menus */
			/* Requests IDCMP_CHANGEWINDOW message when
			 * window is depth arranged
			 * (imsg->Code = CWCODE_DEPTH)
			 */

/* WA_Dummy + 0x33 is obsolete */

			/* Allows you to specify a custom pointer
			 * for your window.  ti_Data points to a
			 * pointer object you obtained via
			 * "pointerclass". NULL signifies the
			 * default pointer.
			 * This tag may be passed to OpenWindowTags()
			 * or SetWindowPointer().
			 */

			/* ti_Data is boolean.  Set to TRUE to
			 * request the standard busy pointer.
			 * This tag may be passed to OpenWindowTags()
			 * or SetWindowPointer().
			 */

			/* ti_Data is boolean.  Set to TRUE to
			 * request that the changing of the
			 * pointer be slightly delayed.  The change
			 * will be called off if you call NewSetPointer()
			 * before the delay expires.  This allows
			 * you to post a busy-pointer even if you think
			 * the busy-time may be very short, without
			 * fear of a flashing pointer.
			 * This tag may be passed to OpenWindowTags()
			 * or SetWindowPointer().
			 */

			/* ti_Data is a boolean.  Set to TRUE to
			 * request that tablet information be included
			 * in IntuiMessages sent to your window.
			 * Requires that something (i.e. a tablet driver)
			 * feed IESUBCLASS_NEWTABLET InputEvents into
			 * the system.  For a pointer to the TabletData,
			 * examine the ExtIntuiMessage->eim_TabletData
			 * field.  It is UNSAFE to check this field
			 * when running on pre-V39 systems.  It's always
			 * safe to check this field under V39 and up,
			 * though it may be NULL.
			 */

			/* When the active window has gadget help enabled,
			 * other windows of the same HelpGroup number
			 * will also get GadgetHelp.  This allows GadgetHelp
			 * to work for multi-windowed applications.
			 * Use GetGroupID() to get an ID number.  Pass
			 * this number as ti_Data to all your windows.
			 * See also the HelpControl() function.
			 */

			/* When the active window has gadget help enabled,
			 * other windows of the same HelpGroup will also get
			 * GadgetHelp.  This allows GadgetHelp to work
			 * for multi-windowed applications.  As an alternative
			 * to WA_HelpGroup, you can pass a pointer to any
			 * other window of the same group to join its help
			 * group.  Defaults to NULL, which has no effect.
			 * See also the HelpControl() function.
			 */

			/* If TRUE, the window will open in hidden state. V47.
			 */

            /* Allows you to set one of Intuition's built-in pointers
             * for your window. Zero signifies the default pointer.
             * This tag may be passed to OpenWindowTags()
             * or SetWindowPointer(). V47 and V53.
             */

			/* New for V46: If this tag is present, intuition builds
			 * an iconification gadget for this window and places
			 * it into the top window border. The application will
			 * receive a CLOSEWINDOW event with ie_Code set to 1
			 * as soon as this gadget is pressed.
			 */

/* HelpControl() flags:
 *
 * HC_GADGETHELP - Set this flag to enable Gadget-Help for one or more
 * windows.
 */

/* IntuitionControlA() tags:
 */

/* No public tags defined so far */

/*
 * Special codes for ShowWindow() and WA_InFrontOf:
 * Give this as target window where to move your window to.
 */

/*
**	$VER: screens.h 47.1 (1.8.2019)
**
**	The Screen and NewScreen structures and attributes
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
 * NOTE:  intuition/iobsolete.h is included at the END of this file!
 */

/* ======================================================================== */
/* === DrawInfo ========================================================= */
/* ======================================================================== */

/* This is a packet of information for graphics rendering.  It originates
 * with a Screen, and is gotten using GetScreenDrawInfo( screen );
 */

/* You can use the Intuition version number to tell which fields are
 * present in this structure.
 *
 * DRI_VERSION of 1 corresponds to V37 release.
 * DRI_VERSION of 2 corresponds to V39, and includes three new pens
 *	and the dri_CheckMark and dri_AmigaKey fields.
 * DRI_VERSION of 3 corresponds to V47, and includes one new pen and
 *	the dri_Screen field.
 *
 * Note that sometimes applications need to create their own DrawInfo
 * structures, in which case the DRI_VERSION won't correspond exactly
 * to the OS version!!!
 */
struct DrawInfo
{
    UWORD		 dri_Version;	  /* will be  DRI_VERSION		*/
    UWORD		 dri_NumPens;	  /* guaranteed to be >= 9		*/
    UWORD		*dri_Pens;	  /* pointer to pen array		*/

    struct TextFont	*dri_Font;	  /* screen default font		*/
    UWORD		 dri_Depth;	  /* (initial) depth of screen bitmap	*/

    struct {		/* from DisplayInfo database for initial display mode	*/
	UWORD	X;
	UWORD	Y;
    }			 dri_Resolution;

    ULONG		 dri_Flags;	  /* defined below			*/
/* New for V39: dri_CheckMark, dri_AmigaKey. */
    struct Image	*dri_CheckMark;	  /* pointer to scaled checkmark image
					   * Will be NULL if DRI_VERSION < 2
					   */
    struct Image	*dri_AmigaKey;	  /* pointer to scaled Amiga-key image
					   * Will be NULL if DRI_VERSION < 2
					   */
/* New for V47: dri_Screen. */
    struct Screen	*dri_Screen;	  /* pointer to associated screen
					   * Will be NULL if DRI_VERSION < 3
					   */
    ULONG		 dri_Reserved[4]; /* avoid recompilation ;^)		*/
};

/* rendering pen number indexes into DrawInfo.dri_Pens[]	*/
/* New for V39, only present if DRI_VERSION >= 2: */
/* New for V47, only present if DRI_VERSION >= 3: */
/* New for V39:  It is sometimes useful to specify that a pen value
 * is to be the complement of color zero to three.  The "magic" numbers
 * serve that purpose:
 */
/* ======================================================================== */
/* === Screen ============================================================= */
/* ======================================================================== */

/* VERY IMPORTANT NOTE ABOUT Screen->BitMap.  In the future, bitmaps
 * will need to grow.  The embedded instance of a bitmap in the screen
 * will no longer be large enough to hold the whole description of
 * the bitmap.
 *
 * YOU ARE STRONGLY URGED to use Screen->RastPort.BitMap in place of
 * &Screen->BitMap whenever and whereever possible.
 */

struct Screen
{
    struct Screen *NextScreen;		/* linked list of screens */
    struct Window *FirstWindow;		/* linked list Screen's Windows */

    WORD LeftEdge, TopEdge;		/* parameters of the screen */
    WORD Width, Height;			/* parameters of the screen */

    WORD MouseY, MouseX;		/* position relative to upper-left */

    UWORD Flags;				/* see definitions below */

    STRPTR Title;				/* null-terminated Title text */
    STRPTR DefaultTitle;		/* for Windows without ScreenTitle */

    /* Bar sizes for this Screen and all Window's in this Screen */
    /* Note that BarHeight is one less than the actual menu bar
     * height.	We're going to keep this in V36 for compatibility,
     * although V36 artwork might use that extra pixel
     *
     * Also, the title bar height of a window is calculated from the
     * screen's WBorTop field, plus the font height, plus one.
     */
    BYTE BarHeight, BarVBorder, BarHBorder, MenuVBorder, MenuHBorder;
    BYTE WBorTop, WBorLeft, WBorRight, WBorBottom;

    struct TextAttr *Font;		/* this screen's default font	   */

    /* the display data structures for this Screen */
    struct ViewPort ViewPort;		/* describing the Screen's display */
    struct RastPort RastPort;		/* describing Screen rendering	   */
    struct BitMap BitMap;		/* SEE WARNING ABOVE!		   */
    struct Layer_Info LayerInfo;	/* each screen gets a LayerInfo    */

    /* Only system gadgets may be attached to a screen.
     *	You get the standard system Screen Gadgets automatically
     */
    struct Gadget *FirstGadget;

    UBYTE DetailPen, BlockPen;		/* for bar/border/gadget rendering */

    /* the following variable(s) are maintained by Intuition to support the
     * DisplayBeep() color flashing technique
     */
    UWORD SaveColor0;

    /* This layer is for the Screen and Menu bars */
    struct Layer *BarLayer;

    UBYTE *ExtData;

    UBYTE *UserData;	/* general-purpose pointer to User data extension */

    /**** Data below this point are SYSTEM PRIVATE ****/
};


/* --- FLAGS SET BY INTUITION --------------------------------------------- */
/* The SCREENTYPE bits are reserved for describing various Screen types
 * available under Intuition.
 */
/* --- the definitions for the Screen Type ------------------------------- */
/* V36 applications can use OpenScreenTagList() instead of NS_EXTENDED	*/

/* New for V39: */
/*
 * Screen attribute tag ID's.  These are used in the ti_Tag field of
 * TagItem arrays passed to OpenScreenTagList() (or in the
 * ExtNewScreen.Extension field).
 */

/* Screen attribute tags.  Please use these versions, not those in
 * iobsolete.h.
 */

/*
 * these items specify items equivalent to fields in NewScreen
 */
			/* traditional screen positions	and dimensions	*/
			/* screen bitmap depth				*/
			/* serves as default for windows, too		*/
			/* default screen title				*/
			/* ti_Data is an array of struct ColorSpec,
			 * terminated by ColorIndex = -1.  Specifies
			 * initial screen palette colors.
			 * Also see SA_Colors32 for use under V39.
			 */
			/* ti_Data points to LONG error code (values below)*/
			/* equiv. to NewScreen.Font			*/
			/* Selects one of the preferences system fonts:
			 *	0 - old DefaultFont, fixed-width
			 *	1 - WB Screen preferred font
			 */
			/* ti_Data is PUBLICSCREEN or CUSTOMSCREEN.  For other
			 * fields of NewScreen.Type, see individual tags,
			 * eg. SA_Behind, SA_Quiet.
			 */
			/* ti_Data is pointer to custom BitMap.  This
			 * implies type of CUSTOMBITMAP
			 */
			/* presence of this tag means that the screen
			 * is to be a public screen.  Please specify
			 * BEFORE the two tags below
			 */
			/* Task ID and signal for being notified that
			 * the last window has closed on a public screen.
			 */
			/* ti_Data is new extended display ID from
			 * <graphics/displayinfo.h> (V37) or from
			 * <graphics/modeid.h> (V39 and up)
			 */
			/* ti_Data points to a rectangle which defines
			 * screen display clip region
			 */
			/* Set to one of the OSCAN_
			 * specifiers below to get a system standard
			 * overscan region for your display clip,
			 * screen dimensions (unless otherwise specified),
			 * and automatically centered position (partial
			 * support only so far).
			 * If you use this, you shouldn't specify
			 * SA_DClip.  SA_Overscan is for "standard"
			 * overscan dimensions, SA_DClip is for
			 * your custom numeric specifications.
			 */
			/* obsolete S_MONITORNAME			*/

/** booleans **/
			/* boolean equivalent to flag SHOWTITLE		*/
			/* boolean equivalent to flag SCREENBEHIND	*/
			/* boolean equivalent to flag SCREENQUIET	*/
			/* boolean equivalent to flag AUTOSCROLL	*/
			/* pointer to ~0 terminated UWORD array, as
			 * found in struct DrawInfo
			 */
			/* boolean: initialize color table to entire
			 *  preferences palette (32 for V36), rather
			 * than compatible pens 0-3, 17-19, with
			 * remaining palette as returned by GetColorMap()
			 */

			/* New for V39:
			 * Allows you to override the number of entries
			 * in the ColorMap for your screen.  Intuition
			 * normally allocates (1<<depth) or 32, whichever
			 * is more, but you may require even more if you
			 * use certain V39 graphics.library features
			 * (eg. palette-banking).
			 */

			/* New for V39:
			 * ti_Data is a pointer to a "parent" screen to
			 * attach this one to.	Attached screens slide
			 * and depth-arrange together.
			 */

			/* New for V39:
			 * Boolean tag allowing non-draggable screens.
			 * Do not use without good reason!
			 * (Defaults to TRUE).
			 */

			/* New for V39:
			 * Boolean tag allowing screens that won't share
			 * the display.  Use sparingly!  Starting with 3.01,
			 * attached screens may be SA_Exclusive.  Setting
			 * SA_Exclusive for each screen will produce an
			 * exclusive family.   (Defaults to FALSE).
			 */

			/* New for V39:
			 * For those pens in the screen's DrawInfo->dri_Pens,
			 * Intuition obtains them in shared mode (see
			 * graphics.library/ObtainPen()).  For compatibility,
			 * Intuition obtains the other pens of a public
			 * screen as PEN_EXCLUSIVE.  Screens that wish to
			 * manage the pens themselves should generally set
			 * this tag to TRUE.  This instructs Intuition to
			 * leave the other pens unallocated.
			 */

			/* New for V39:
			 * provides a "backfill hook" for your screen's
			 * Layer_Info.
			 * See layers.library/InstallLayerInfoHook()
			 */

			/* New for V39:
			 * Boolean tag requesting that the bitmap
			 * allocated for you be interleaved.
			 * (Defaults to FALSE).
			 */

			/* New for V39:
			 * Tag to set the screen's initial palette colors
			 * at 32 bits-per-gun.	ti_Data is a pointer
			 * to a table to be passed to the
			 * graphics.library/LoadRGB32() function.
			 * This format supports both runs of color
			 * registers and sparse registers.  See the
			 * autodoc for that function for full details.
			 * Any color set here has precedence over
			 * the same register set by SA_Colors.
			 */

			/* New for V39:
			 * ti_Data is a pointer to a taglist that Intuition
			 * will pass to graphics.library/VideoControl(),
			 * upon opening the screen.
			 */

			/* New for V39:
			 * ti_Data is a pointer to an already open screen
			 * that is to be the child of the screen being
			 * opened.  The child screen will be moved to the
			 * front of its family.
			 */

			/* New for V39:
			 * ti_Data is a pointer to an already open screen
			 * that is to be the child of the screen being
			 * opened.  The child screen will be moved to the
			 * back of its family.
			 */

			/* New for V39:
			 * Set ti_Data to 1 to request a screen which
			 * is just like the Workbench.	This gives
			 * you the same screen mode, depth, size,
			 * colors, etc., as the Workbench screen.
			 */

			/* Reserved for private Intuition use */

			/* New for V40:
			 * For compatibility, Intuition always ensures
			 * that the inter-screen gap is at least three
			 * non-interlaced lines.  If your application
			 * would look best with the smallest possible
			 * inter-screen gap, set ti_Data to TRUE.
			 * If you use the new graphics VideoControl()
			 * VC_NoColorPaletteLoad tag for your screen's
			 * ViewPort, you should also set this tag.
			 */
                        /* New for V45:
			 * If this tag is set, then windows can be moved
			 * partially out of the screen if layers is
			 * new enough. Defaults to TRUE for the workbench
			 * screen and FALSE to all others.
			 */
  
/* this is an obsolete tag included only for compatibility with V35
 * interim release for the A2024 and Viking monitors
 */
/* OpenScreen error codes, which are returned in the (optional) LONG
 * pointed to by ti_Data for the SA_ErrorCode tag item
 */
/* ======================================================================== */
/* === NewScreen ========================================================== */
/* ======================================================================== */
/* note: to use the Extended field, you must use the
 * new ExtNewScreen structure, below
 */
struct NewScreen
{
    WORD LeftEdge, TopEdge, Width, Height, Depth;  /* screen dimensions */

    UBYTE DetailPen, BlockPen;	/* for bar/border/gadget rendering	*/

    UWORD ViewModes;		/* the Modes for the ViewPort (and View) */

    UWORD Type;			/* the Screen type (see defines above)	*/

    struct TextAttr *Font;	/* this Screen's default text attributes */

    STRPTR DefaultTitle;	/* the default title for this Screen	*/

    struct Gadget *Gadgets;	/* UNUSED:  Leave this NULL		*/

    /* if you are opening a CUSTOMSCREEN and already have a BitMap
     * that you want used for your Screen, you set the flags CUSTOMBITMAP in
     * the Type field and you set this variable to point to your BitMap
     * structure.  The structure will be copied into your Screen structure,
     * after which you may discard your own BitMap if you want
     */
    struct BitMap *CustomBitMap;
};

/*
 * For compatibility reasons, we need a new structure for extending
 * NewScreen.  Use this structure is you need to use the new Extension
 * field.
 *
 * NOTE: V36-specific applications should use the
 * OpenScreenTagList( newscreen, tags ) version of OpenScreen().
 * Applications that want to be V34-compatible as well may safely use the
 * ExtNewScreen structure.  Its tags will be ignored by V34 Intuition.
 *
 */
struct ExtNewScreen
{
    WORD LeftEdge, TopEdge, Width, Height, Depth;
    UBYTE DetailPen, BlockPen;
    UWORD ViewModes;
    UWORD Type;
    struct TextAttr *Font;
    STRPTR DefaultTitle;
    struct Gadget *Gadgets;
    struct BitMap *CustomBitMap;

    struct TagItem	*Extension;
				/* more specification data, scanned if
				 * NS_EXTENDED is set in NewScreen.Type
				 */
};

/* === Overscan Types ===	*/
/* === Public Shared Screen Node ===	*/

/* This is the representative of a public shared screen.
 * This is an internal data structure, but some functions may
 * present a copy of it to the calling application.  In that case,
 * be aware that the screen pointer of the structure can NOT be
 * used safely, since there is no guarantee that the referenced
 * screen will remain open and a valid data structure.
 *
 * Never change one of these.
 */

struct PubScreenNode	{
    struct Node		psn_Node;	/* ln_Name is screen name */
    struct Screen	*psn_Screen;
    UWORD		psn_Flags;	/* below		*/
    WORD		psn_Size;	/* includes name buffer	*/
    WORD		psn_VisitorCount; /* how many visitor windows */
    struct Task		*psn_SigTask;	/* who to signal when visitors gone */
    UBYTE		psn_SigBit;	/* which signal	*/
};

/* NOTE: Due to a bug in NextPubScreen(), make sure your buffer
 * actually has MAXPUBSCREENNAME+1 characters in it!
 */
/* pub screen modes	*/
/* New for V39:  Intuition has new screen depth-arrangement and movement
 * functions called ScreenDepth() and ScreenPosition() respectively.
 * These functions permit the old behavior of ScreenToFront(),
 * ScreenToBack(), and MoveScreen().  ScreenDepth() also allows
 * independent depth control of attached screens.  ScreenPosition()
 * optionally allows positioning screens even though they were opened
 * {SA_Draggable,FALSE}.
 */

/* For ScreenDepth(), specify one of SDEPTH_TOFRONT or SDEPTH_TOBACK,
 * and optionally also SDEPTH_INFAMILY.
 *
 * NOTE: ONLY THE OWNER OF THE SCREEN should ever specify
 * SDEPTH_INFAMILY.  Commodities, "input helper" programs,
 * or any other program that did not open a screen should never
 * use that flag.  (Note that this is a style-behavior
 * requirement;  there is no technical requirement that the
 * task calling this function need be the task which opened
 * the screen).
 */

/* Here's an obsolete name equivalent to SDEPTH_INFAMILY: */
/* For ScreenPosition(), specify one of SPOS_RELATIVE, SPOS_ABSOLUTE,
 * or SPOS_MAKEVISIBLE to describe the kind of screen positioning you
 * wish to perform:
 *
 * SPOS_RELATIVE: The x1 and y1 parameters to ScreenPosition() describe
 *	the offset in coordinates you wish to move the screen by.
 * SPOS_ABSOLUTE: The x1 and y1 parameters to ScreenPosition() describe
 *	the absolute coordinates you wish to move the screen to.
 * SPOS_MAKEVISIBLE: (x1,y1)-(x2,y2) describes a rectangle on the
 *	screen which you would like autoscrolled into view.
 *
 * You may additionally set SPOS_FORCEDRAG along with any of the
 * above.  Set this if you wish to reposition an {SA_Draggable,FALSE}
 * screen that you opened.
 *
 * NOTE: ONLY THE OWNER OF THE SCREEN should ever specify
 * SPOS_FORCEDRAG.  Commodities, "input helper" programs,
 * or any other program that did not open a screen should never
 * use that flag.
 */

/* New for V39: Intuition supports double-buffering in screens,
 * with friendly interaction with menus and certain gadgets.
 * For each buffer, you need to get one of these structures
 * from the AllocScreenBuffer() call.  Never allocate your
 * own ScreenBuffer structures!
 *
 * The sb_DBufInfo field is for your use.  See the graphics.library
 * AllocDBufInfo() autodoc for details.
 */
struct ScreenBuffer
{
    struct BitMap *sb_BitMap;		/* BitMap of this buffer */
    struct DBufInfo *sb_DBufInfo;	/* DBufInfo for this buffer */
};

/* These are the flags that may be passed to AllocScreenBuffer().
 */
/* Include obsolete identifiers: */
/*
**	$VER: iobsolete.h 47.1 (1.8.2019)
**
**	Obsolete identifiers for Intuition.  Use the new ones instead!
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/


/* This file contains:
 *
 * 1.  The traditional identifiers for gadget Flags, Activation, and Type,
 * and for window Flags and IDCMP classes.  They are defined in terms
 * of their new versions, which serve to prevent confusion between
 * similar-sounding but different identifiers (like IDCMP_WINDOWACTIVE
 * and WFLG_ACTIVATE).
 *
 * 2.  Some tag names and constants whose labels were adjusted after V36.
 *
 * By default, 1 and 2 are enabled.
 *
 * #define INTUI_V36_NAMES_ONLY to exclude the traditional identifiers and
 * the original V36 names of some identifiers.
 *
 */


/* #define INTUI_V36_NAMES_ONLY to remove these older names */

/* V34-style Gadget->Flags names: */

/* V34-style Gadget->Activation flag names: */

/* V34-style Gadget->Type names: */

/* V34-style IDCMP class names: */

/* V34-style Window->Flags names: */

/* These are the obsolete tag names for general gadgets, proportional gadgets,
 * and string gadgets.  Use the mixed-case equivalents from gadgetclass.h
 * instead.
 */

/* These are the obsolete tag names for image attributes.
 * Use the mixed-case equivalents from imageclass.h instead.
 */

/* These are the obsolete identifiers for the various DrawInfo pens.
 * Use the uppercase versions in screens.h instead.
 */

/*
**	$VER: preferences.h 47.2 (26.12.2019)
**
**	Structure definition for old-style preferences
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/* ======================================================================== */
/* === Preferences ======================================================== */
/* ======================================================================== */

/* these are the definitions for the printer configurations */
/* These defines are for the default font size.  These actually describe the
 * height of the defaults fonts.  The default font type is the topaz
 * font, which is a fixed width font that can be used in either
 * eighty-column or sixty-column mode.	The Preferences structure reflects
 * which is currently selected by the value found in the variable FontSize,
 * which may have either of the values defined below.  These values actually
 * are used to select the height of the default font.  By changing the
 * height, the resolution of the font changes as well.
 */
/* Note:  Starting with V36, and continuing with each new version of
 * Intuition, an increasing number of fields of struct Preferences
 * are ignored by SetPrefs().  (Some fields are obeyed only at the
 * initial SetPrefs(), which comes from the devs:system-configuration
 * file).  Elements are generally superseded as new hardware or software
 * features demand more information than fits in struct Preferences.
 * Parts of struct Preferences must be ignored so that applications
 * calling GetPrefs(), modifying some other part of struct Preferences,
 * then calling SetPrefs(), don't end up truncating the extended
 * data.
 *
 * Consult the autodocs for SetPrefs() for further information as
 * to which fields are not always respected.
 */

struct Preferences
{
    /* the default font height */
    BYTE FontHeight;			/* height for system default font  */

    /* constant describing what's hooked up to the port */
    UBYTE PrinterPort;			/* printer port connection	   */

    /* the baud rate of the port */
    UWORD BaudRate;			/* baud rate for the serial port   */

    /* various timing rates */
    TimeVal_Type KeyRptSpeed;		/* repeat speed for keyboard	   */
    TimeVal_Type KeyRptDelay;		/* Delay before keys repeat	   */
    TimeVal_Type DoubleClick;		/* Interval allowed between clicks */

    /* Intuition Pointer data */
    UWORD PointerMatrix[((1 + 16 + 1) * 2)];	/* Definition of pointer sprite    */
    BYTE XOffset;			/* X-Offset for active 'bit'	   */
    BYTE YOffset;			/* Y-Offset for active 'bit'	   */
    UWORD color17;			/***********************************/
    UWORD color18;			/* Colours for sprite pointer	   */
    UWORD color19;			/***********************************/
    UWORD PointerTicks;			/* Sensitivity of the pointer	   */

    /* Workbench Screen colors */
    UWORD color0;			/***********************************/
    UWORD color1;			/*  Standard default colours	   */
    UWORD color2;			/*   Used in the Workbench	   */
    UWORD color3;			/***********************************/

    /* positioning data for the Intuition View */
    BYTE ViewXOffset;			/* Offset for top lefthand corner  */
    BYTE ViewYOffset;			/* X and Y dimensions		   */
    WORD ViewInitX, ViewInitY;		/* View initial offset values	   */

    BOOL EnableCLI;			/* CLI availability switch */

    /* printer configurations */
    UWORD PrinterType;			/* printer type		   */
    TEXT PrinterFilename[30];/* file for printer	   */

    /* print format and quality configurations */
    UWORD PrintPitch;			/* print pitch			   */
    UWORD PrintQuality;			/* print quality		   */
    UWORD PrintSpacing;			/* number of lines per inch	   */
    UWORD PrintLeftMargin;		/* left margin in characters	   */
    UWORD PrintRightMargin;		/* right margin in characters	   */
    UWORD PrintImage;			/* positive or negative		   */
    UWORD PrintAspect;			/* horizontal or vertical	   */
    UWORD PrintShade;			/* b&w, half-tone, or color	   */
    WORD PrintThreshold;		/* darkness ctrl for b/w dumps	   */

    /* print paper descriptors */
    UWORD PaperSize;			/* paper size			   */
    UWORD PaperLength;			/* paper length in number of lines */
    UWORD PaperType;			/* continuous or single sheet	   */

    /* Serial device settings: These are six nibble-fields in three bytes */
    /* (these look a little strange so the defaults will map out to zero) */
    UBYTE   SerRWBits;	 /* upper nibble = (8-number of read bits)	*/
			 /* lower nibble = (8-number of write bits)	*/
    UBYTE   SerStopBuf;  /* upper nibble = (number of stop bits - 1)	*/
			 /* lower nibble = (table value for BufSize)	*/
    UBYTE   SerParShk;	 /* upper nibble = (value for Parity setting)	*/
			 /* lower nibble = (value for Handshake mode)	*/
    UBYTE   LaceWB;	 /* if workbench is to be interlaced		*/

    UBYTE   Pad[ 12 ];
    TEXT   PrtDevName[16];	/* device used by printer.device
					 * (omit the ".device")
					 */
    UBYTE   DefaultPrtUnit;	/* default unit opened by printer.device */
    UBYTE   DefaultSerUnit;	/* default serial unit */

    BYTE    RowSizeChange;	/* affect NormalDisplayRows/Columns	*/
    BYTE    ColumnSizeChange;

    UWORD    PrintFlags;	/* user preference flags */
    UWORD    PrintMaxWidth;	/* max width of printed picture in 10ths/in */
    UWORD    PrintMaxHeight;	/* max height of printed picture in 10ths/in */
    UBYTE    PrintDensity;	/* print density */
    UBYTE    PrintXOffset;	/* offset of printed picture in 10ths/inch */

    UWORD    wb_Width;		/* override default workbench width  */
    UWORD    wb_Height;		/* override default workbench height */
    UBYTE    wb_Depth;		/* override default workbench depth  */

    UBYTE    ext_size;		/* extension information -- do not touch! */
			    /* extension size in blocks of 64 bytes */
};


/* Workbench Interlace (use one bit) */
/* Enable_CLI	*/
/* PrinterPort */
/* BaudRate */
/* PaperType */
/* PrintPitch */
/* PrintQuality */
/* PrintSpacing */
/* Print Image */
/* PrintAspect */
/* PrintShade */
/* PaperSize (all paper sizes have a zero in the lowest nybble) */
/* New PaperSizes for V36: */
/* PrinterType */
/* new printer entries, 3 October 1985 */
/* Serial Input Buffer Sizes */
/* Serial Bit Masks */
/* Serial Parity (upper nibble, after being shifted by
 * macro SPARNUM() )
 */
/* New parity definitions for V36: */
/* Serial Handshake Mode (lower nibble, after masking using
 * macro SHANKNUM() )
 */
/* new defines for PrintFlags */

/* masks used for checking bits */

/* ======================================================================== */
/* === Remember =========================================================== */
/* ======================================================================== */
/* this structure is used for remembering what memory has been allocated to
 * date by a given routine, so that a premature abort or systematic exit
 * can deallocate memory cleanly, easily, and completely
 */
struct Remember
{
    struct Remember *NextRemember;
    ULONG RememberSize;
    UBYTE *Memory;
};


/* === Color Spec ====================================================== */
/* How to tell Intuition about RGB values for a color table entry.
 * NOTE:  The way the structure was defined, the color value was
 * right-justified within each UWORD.  This poses problems for
 * extensibility to more bits-per-gun.  The SA_Colors32 tag to
 * OpenScreenTags() provides an alternate way to specify colors
 * with greater precision.
 */
struct ColorSpec
{
    WORD	ColorIndex;	/* -1 terminates an array of ColorSpec	*/
    UWORD	Red;	/* only the _bottom_ 4 bits recognized */
    UWORD	Green;	/* only the _bottom_ 4 bits recognized */
    UWORD	Blue;	/* only the _bottom_ 4 bits recognized */
};

/* === Easy Requester Specification ======================================= */
/* see also autodocs for EasyRequest and BuildEasyRequest	*/
/* NOTE: This structure may grow in size in the future		*/
struct EasyStruct {
    ULONG        es_StructSize;	/* should be sizeof (struct EasyStruct )*/
    ULONG        es_Flags;	/* should be 0 for now			*/
    CONST_STRPTR es_Title;	/* title of requester window		*/
    CONST_STRPTR es_TextFormat;	/* 'printf' style formatting string	*/
    CONST_STRPTR es_GadgetFormat; /* 'printf' style formatting string	*/
};



/* ======================================================================== */
/* === Miscellaneous ====================================================== */
/* ======================================================================== */

/* = MACROS ============================================================== */
/* = MENU STUFF =========================================================== */
/* = =RJ='s peculiarities ================================================= */
/* these defines are for the COMMSEQ and CHECKIT menu stuff.  If CHECKIT,
 * I'll use a generic Width (for all resolutions) for the CheckMark.
 * If COMMSEQ, likewise I'll use this generic stuff
 */
/* these are the AlertNumber defines.  if you are calling DisplayAlert()
 * the AlertNumber you supply must have the ALERT_TYPE bits set to one
 * of these patterns
 */
/* When you're defining IntuiText for the Positive and Negative Gadgets
 * created by a call to AutoRequest(), these defines will get you
 * reasonable-looking text.  The only field without a define is the IText
 * field; you decide what text goes with the Gadget
 */
/* --- RAWMOUSE Codes and Qualifiers (Console OR IDCMP) ------------------- */
/* New for V39, Intuition supports the IESUBCLASS_NEWTABLET subclass
 * of the IECLASS_NEWPOINTERPOS event.  The ie_EventAddress of such
 * an event points to a TabletData structure (see below).
 *
 * The TabletData structure contains certain elements including a taglist.
 * The taglist can be used for special tablet parameters.  A tablet driver
 * should include only those tag-items the tablet supports.  An application
 * can listen for any tag-items that interest it.  Note: an application
 * must set the WA_TabletMessages attribute to TRUE to receive this
 * extended information in its IntuiMessages.
 *
 * The definitions given here MUST be followed.  Pay careful attention
 * to normalization and the interpretation of signs.
 *
 * TABLETA_TabletZ:  the current value of the tablet in the Z direction.
 * This unsigned value should typically be in the natural units of the
 * tablet.  You should also provide TABLETA_RangeZ.
 *
 * TABLETA_RangeZ:  the maximum value of the tablet in the Z direction.
 * Normally specified along with TABLETA_TabletZ, this allows the
 * application to scale the actual Z value across its range.
 *
 * TABLETA_AngleX:  the angle of rotation or tilt about the X-axis.  This
 * number should be normalized to fill a signed long integer.  Positive
 * values imply a clockwise rotation about the X-axis when viewing
 * from +X towards the origin.
 *
 * TABLETA_AngleY:  the angle of rotation or tilt about the Y-axis.  This
 * number should be normalized to fill a signed long integer.  Positive
 * values imply a clockwise rotation about the Y-axis when viewing
 * from +Y towards the origin.
 *
 * TABLETA_AngleZ:  the angle of rotation or tilt about the Z axis.  This
 * number should be normalized to fill a signed long integer.  Positive
 * values imply a clockwise rotation about the Z-axis when viewing
 * from +Z towards the origin.
 *
 *	Note: a stylus that supports tilt should use the TABLETA_AngleX
 *	and TABLETA_AngleY attributes.  Tilting the stylus so the tip
 *	points towards increasing or decreasing X is actually a rotation
 *	around the Y-axis.  Thus, if the stylus tip points towards
 *	positive X, then that tilt is represented as a negative
 *	TABLETA_AngleY.  Likewise, if the stylus tip points towards
 *	positive Y, that tilt is represented by positive TABLETA_AngleX.
 *
 * TABLETA_Pressure:  the pressure reading of the stylus.  The pressure
 * should be normalized to fill a signed long integer.  Typical devices
 * won't generate negative pressure, but the possibility is not precluded.
 * The pressure threshold which is considered to cause a button-click is
 * expected to be set in a Preferences program supplied by the tablet
 * vendor.  The tablet driver would send IECODE_LBUTTON-type events as
 * the pressure crossed that threshold.
 *
 * TABLETA_ButtonBits:  ti_Data is a long integer whose bits are to
 * be interpreted at the state of the first 32 buttons of the tablet.
 *
 * TABLETA_InProximity:  ti_Data is a boolean.  For tablets that support
 * proximity, they should send the {TABLETA_InProximity,FALSE} tag item
 * when the stylus is out of proximity.  One possible use we can forsee
 * is a mouse-blanking commodity which keys off this to blank the
 * mouse.  When this tag is absent, the stylus is assumed to be
 * in proximity.
 *
 * TABLETA_ResolutionX:  ti_Data is an unsigned long integer which
 * is the x-axis resolution in dots per inch.
 *
 * TABLETA_ResolutionY:  ti_Data is an unsigned long integer which
 * is the y-axis resolution in dots per inch.
 */

/* If your window sets WA_TabletMessages to TRUE, then it will receive
 * extended IntuiMessages (struct ExtIntuiMessage) whose eim_TabletData
 * field points at a TabletData structure.  This structure contains
 * additional information about the input event.
 */

struct TabletData
{
    /* Sub-pixel position of tablet, in screen coordinates,
     * scaled to fill a UWORD fraction:
     */
    UWORD td_XFraction, td_YFraction;

    /* Current tablet coordinates along each axis: */
    ULONG td_TabletX, td_TabletY;

    /* Tablet range along each axis.  For example, if td_TabletX
     * can take values 0-999, td_RangeX should be 1000.
     */
    ULONG td_RangeX, td_RangeY;

    /* Pointer to tag-list of additional tablet attributes.
     * See <intuition/intuition.h> for the tag values.
     */
    struct TagItem *td_TagList;
};

/* If a tablet driver supplies a hook for ient_CallBack, it will be
 * invoked in the standard hook manner.  A0 will point to the Hook
 * itself, A2 will point to the InputEvent that was sent, and
 * A1 will point to a TabletHookData structure.  The InputEvent's
 * ie_EventAddress field points at the IENewTablet structure that
 * the driver supplied.
 *
 * Based on the thd_Screen, thd_Width, and thd_Height fields, the driver
 * should scale the ient_TabletX and ient_TabletY fields and store the
 * result in ient_ScaledX, ient_ScaledY, ient_ScaledXFraction, and
 * ient_ScaledYFraction.
 *
 * The tablet hook must currently return NULL.  This is the only
 * acceptable return-value under V39.
 */

struct TabletHookData
{
    /* Pointer to the active screen:
     * Note: if there are no open screens, thd_Screen will be NULL.
     * thd_Width and thd_Height will then describe an NTSC 640x400
     * screen.  Please scale accordingly.
     */
    struct Screen *thd_Screen;

    /* The width and height (measured in pixels of the active screen)
     * that your are to scale to:
     */
    ULONG thd_Width;
    ULONG thd_Height;

    /* Non-zero if the screen or something about the screen
     * changed since the last time you were invoked:
     */
    LONG thd_ScreenChanged;
};

/* Include obsolete identifiers: */
struct OldDrawerData { /* pre V36 definition */
    struct NewWindow	dd_NewWindow;	/* args to open window */
    LONG		dd_CurrentX;	/* current x coordinate of origin */
    LONG		dd_CurrentY;	/* current y coordinate of origin */
};
/* the amount of DrawerData actually written to disk */
struct DrawerData {
    struct NewWindow	dd_NewWindow;	/* args to open window */
    LONG		dd_CurrentX;	/* current x coordinate of origin */
    LONG		dd_CurrentY;	/* current y coordinate of origin */
    ULONG		dd_Flags;	/* flags for drawer */
    UWORD		dd_ViewModes;	/* view mode for drawer */
};
/* the amount of DrawerData actually written to disk */
/* definitions for dd_ViewModes */
/* definitions for dd_Flags */
struct DiskObject {
    UWORD		do_Magic; /* a magic number at the start of the file */
    UWORD		do_Version; /* a version number, so we can change it */
    struct Gadget 	do_Gadget;	/* a copy of in core gadget */
    UBYTE		do_Type;
    STRPTR		do_DefaultTool;
    STRPTR *		do_ToolTypes;
    LONG		do_CurrentX;
    LONG		do_CurrentY;
    struct DrawerData *	do_DrawerData;
    STRPTR		do_ToolWindow;	/* only applies to tools */
    LONG		do_StackSize;	/* only applies to tools */

};

/* I only use the lower 8 bits of Gadget.UserData for the revision # */
struct FreeList {
    WORD		fl_NumFree;
    struct List		fl_MemList;
};

/* workbench does different complement modes for its gadgets.
** It supports separate images, complement mode, and backfill mode.
** The first two are identical to intuitions GFLG_GADGIMAGE and GFLG_GADGHCOMP.
** backfill is similar to GFLG_GADGHCOMP, but the region outside of the
** image (which normally would be color three when complemented)
** is flood-filled to color zero.
*/
/* if an icon does not really live anywhere, set its current position
** to here
*/
/* workbench now is a library.  this is it's name */
/****************************************************************************/

/* If you find am_Version >= AM_VERSION, you know this structure has
 * at least the fields defined in this version of the include file
 */
struct AppMessage {
    struct Message am_Message;	/* standard message structure */
    UWORD am_Type;		/* message type */
    ULONG am_UserData;		/* application specific */
    ULONG am_ID;		/* application definable ID */
    LONG am_NumArgs;		/* # of elements in arglist */
    struct WBArg *am_ArgList;	/* the arguments themselves */
    UWORD am_Version;		/* will be >= AM_VERSION */
    UWORD am_Class;		/* message class */
    WORD am_MouseX;		/* mouse x position of event */
    WORD am_MouseY;		/* mouse y position of event */
    ULONG am_Seconds;		/* current system clock time */
    ULONG am_Micros;		/* current system clock time */
    ULONG am_Reserved[8];	/* avoid recompilation */
};

/* types of app messages */
/* Classes of AppIcon messages (V44) */
/*
 * The following structures are private.  These are just stub
 * structures for code compatibility...
 */
struct AppWindow		{ void * aw_PRIVATE;   };
struct AppWindowDropZone	{ void * awdz_PRIVATE; };
struct AppIcon			{ void * ai_PRIVATE;   };
struct AppMenuItem		{ void * ami_PRIVATE;  };
struct AppMenu			{ void * am_PRIVATE;   };

/****************************************************************************/

/****************************************************************************/

/* Tags for use with AddAppIconA() */

/* AppIcon responds to the "Open" menu item (BOOL). */
/* AppIcon responds to the "Copy" menu item (BOOL). */
/* AppIcon responds to the "Rename" menu item (BOOL). */
/* AppIcon responds to the "Information" menu item (BOOL). */
/* AppIcon responds to the "Snapshot" menu item (BOOL). */
/* AppIcon responds to the "UnSnapshot" menu item (BOOL). */
/* AppIcon responds to the "LeaveOut" menu item (BOOL). */
/* AppIcon responds to the "PutAway" menu item (BOOL). */
/* AppIcon responds to the "Delete" menu item (BOOL). */
/* AppIcon responds to the "FormatDisk" menu item (BOOL). */
/* AppIcon responds to the "EjectDisk" menu item (BOOL). */
/* AppIcon responds to the "EmptyTrash" menu item (BOOL). */
/* AppIcon position should be propagated back to original DiskObject (BOOL). */
/* Callback hook to be invoked when rendering this icon (struct Hook *). */
/* AppIcon wants to be notified when its select state changes (BOOL). */
/****************************************************************************/

/* Tags for use with AddAppMenuA() */

/* Command key string for this AppMenu (STRPTR). */
/* Item to be added should get sub menu items attached to; make room for it,
 * then return the key to use later for attaching the items (ULONG *).
 */
/* This item should be attached to a sub menu; the key provided refers to
 * the sub menu it should be attached to (ULONG).
 */
/* Item to be added is in fact a new menu title; make room for it, then
 * return the key to use later for attaching the items (ULONG *).
 */
/****************************************************************************/

/* Tags for use with OpenWorkbenchObjectA() */

/* Corresponds to the wa_Lock member of a struct WBArg */
/* Corresponds to the wa_Name member of a struct WBArg */
/* When opening a drawer, show all files or only icons?
 * This must be one out of DDFLAGS_SHOWICONS,
 * or DDFLAGS_SHOWALL; (UBYTE); (V45)
 */
/* When opening a drawer, view the contents by icon, name,
 * date, size or type? This must be one out of DDVM_BYICON,
 * DDVM_BYNAME, DDVM_BYDATE, DDVM_BYSIZE or DDVM_BYTYPE;
 * (UBYTE); (V45)
 */
/****************************************************************************/

/* Tags for use with WorkbenchControlA() */

/* Check if the named drawer is currently open (LONG *). */
/* Create a duplicate of the Workbench private search path list (BPTR *). */
/* Free the duplicated search path list (BPTR). */
/* Get the default stack size for launching programs with (ULONG *). */
/* Set the default stack size for launching programs with (ULONG). */
/* Cause an AppIcon to be redrawn (struct AppIcon *). */
/* Get a list of currently running Workbench programs (struct List **). */
/* Release the list of currently running Workbench programs (struct List *). */
/* Get a list of currently selected icons (struct List **). */
/* Release the list of currently selected icons (struct List *). */
/* Get a list of currently open drawers (struct List **). */
/* Release the list of currently open icons (struct List *). */
/* Get the list of hidden devices (struct List **). */
/* Release the list of hidden devices (struct List *). */
/* Add the name of a device which Workbench should never try to
 * read a disk icon from (STRPTR).
 */
/* Remove a name from list of hidden devices (STRPTR). */
/* Get the number of seconds that have to pass before typing
 * the next character in a drawer window will restart
 * with a new file name (ULONG *).
 */
/* Set the number of seconds that have to pass before typing
 * the next character in a drawer window will restart
 * with a new file name (ULONG).
 */
/* Obtain the hook that will be invoked when Workbench starts
 * to copy files and data (struct Hook **); (V45)
 */
/* Install the hook that will be invoked when Workbench starts
 * to copy files and data (struct Hook *); (V45)
 */
/* Obtain the hook that will be invoked when Workbench discards
 * files and drawers or empties the trashcan (struct Hook **);
 * (V45).
 */
/* Install the hook that will be invoked when Workbench discards
 * files and drawers or empties the trashcan (struct Hook *);
 * (V45).
 */
/* Obtain the hook that will be invoked when Workbench requests
 * that the user enters text, such as when a file is to be renamed
 * or a new drawer is to be created (struct Hook **); (V45)
 */
/* Install the hook that will be invoked when Workbench requests
 * that the user enters text, such as when a file is to be renamed
 * or a new drawer is to be created (struct Hook *); (V45)
 */
/* Add a hook that will be invoked when Workbench is about
 * to shut down (cleanup), and when Workbench has returned
 * to operational state (setup) (struct Hook *); (V45)
 */
/* Remove a hook that has been installed with the
 * WBCTRLA_AddSetupCleanupHook tag (struct Hook *); (V45)
 */
/* Set assorted Workbench options. See below for the various
 * public flags (ULONG); (V47)
 */
/* Get assorted Workbench options. See below for the various
 * public flags (ULONG); (V47)
 */
/* Obtain the hook that will be invoked when Workbench detects
 * an unreadable drive, such as a MS-Dos formatted disk in
 * a trackdisk.device driven handler. (struct Hook **); (V47)
 */
/* Add a hook that will be invoked when Workbench detects an
 * unreadable disk, and which is called with the object
 * as struct InfoData *, and message as struct MsgPort *.
 * The result code is the new disk state to be used by
 * the workbench. (struct Hook *) (V47)
 */
/****************************************************************************/

/* The message your setup/cleanup hook gets invoked with. */
struct SetupCleanupHookMsg
{
	ULONG	schm_Length;	/* Size of this data structure (in bytes). */
	LONG	schm_State;	/* See below for definitions. */
};

/****************************************************************************/

/* Flags for WBCTRLA_SetGlobalFlags/WBCTRLA_GetGlobalFlags. */

/****************************************************************************/

/* Tags for use with AddAppWindowDropZoneA() */

/* Zone left edge (WORD) */
/* Zone left edge, if relative to the right edge of the window (WORD) */
/* Zone top edge (WORD) */
/* Zone top edge, if relative to the bottom edge of the window (WORD) */
/* Zone width (WORD) */
/* Zone width, if relative to the window width (WORD) */
/* Zone height (WORD) */
/* Zone height, if relative to the window height (WORD) */
/* Zone position and size (struct IBox *). */
/* Hook to invoke when the mouse enters or leave a drop zone (struct Hook *). */
/****************************************************************************/

/* Tags for use with WhichWorkbenchObjectA() (V47) */

/* Type of icon: one of the WB#? definitions from above (ULONG *). */
/* Left offset of the icon box relative to its parent window (LONG *). */
/* Top offset of the icon box relative to its parent window (LONG *). */
/* Width of the icon box, including border (ULONG *). */
/* Height of the icon box, including border (ULONG *). */
/* Current state of the icon: IDS_NORMAL, IDS_SELECTED... (ULONG *). */
/* Is the icon a fake one (i.e. without a real .info file)? (ULONG *). */
/* Name of the icon as displayed in the Workbench window (STRPTR). */
/* Size of the buffer provided with WBOBJA_Name; default is 64 (ULONG). */
/* Full path (if applicable) of the object the icon represents (STRPTR). */
/* Size of the buffer provided with WBOBJA_FullPath; default is 512 (ULONG). */
/* Does the icon represent a link, rather than a real file? (ULONG *). */
/* Path of the drawer whose window the given coordinates fall into (STRPTR). */
/* Size of the buffer provided with WBOBJA_DrawerPath; default is 64 (ULONG). */
/* Current flags of the drawer found at the given coordinates (ULONG *). */
/* Current viewmodes of the drawer found at the given coordinates (ULONG *). */
/****************************************************************************/

/* Possible results from WhichWorkbenchObjectA() (V47) */

/****************************************************************************/

/* Reserved tags; don't use! */
/****************************************************************************/

/****************************************************************************/

/* The message your AppIcon rendering hook gets invoked with. */
struct AppIconRenderMsg
{
	struct RastPort *	arm_RastPort;	/* RastPort to render into */
	struct DiskObject *	arm_Icon;	/* The icon to be rendered */
	STRPTR			arm_Label;	/* The icon label txt */
	struct TagItem *	arm_Tags;	/* Further tags to be passed on
						 * to DrawIconStateA().
						 */

	WORD			arm_Left;	/* \ Rendering origin, not taking the */
	WORD			arm_Top;	/* / button border into account. */

	WORD			arm_Width;	/* \ Limit your rendering to */
	WORD			arm_Height;	/* / this area. */

	ULONG			arm_State;	/* IDS_SELECTED, IDS_NORMAL, etc. */
};

/****************************************************************************/

/* The message your drop zone hook gets invoked with. */
struct AppWindowDropZoneMsg
{
	struct RastPort *	adzm_RastPort;		/* RastPort to render into. */
	struct IBox		adzm_DropZoneBox;	/* Limit your rendering to this area. */
	ULONG			adzm_ID;		/* \ These come from straight */
	ULONG			adzm_UserData;		/* / from AddAppWindowDropZoneA(). */
	LONG			adzm_Action;		/* See below for a list of actions. */
};

/****************************************************************************/

/* The message your icon selection change hook is invoked with. */
struct IconSelectMsg
{
	/* Size of this data structure (in bytes). */
	ULONG			ism_Length;

	/* Lock on the drawer this object resides in,
	 * NULL for Workbench backdrop (devices).
	 */
	BPTR			ism_Drawer;

	/* Name of the object in question. */
	STRPTR			ism_Name;

	/* One of WBDISK, WBDRAWER, WBTOOL, WBPROJECT,
	 * WBGARBAGE, WBDEVICE, WBKICK or WBAPPICON.
	 */
	UWORD			ism_Type;

	/* TRUE if currently selected, FALSE otherwise. */
	BOOL			ism_Selected;

	/* Pointer to the list of tag items passed to
	 * ChangeWorkbenchSelectionA().
	 */
	struct TagItem *	ism_Tags;

	/* Pointer to the window attached to this icon,
	 * if the icon is a drawer-like object.
	 */
	struct Window *		ism_DrawerWindow;

	/* Pointer to the window the icon resides in. */
	struct Window *		ism_ParentWindow;

	/* Position and size of the icon; note that the
	 * icon may not entirely reside within the visible
	 * bounds of the parent window.
	 */
	WORD			ism_Left;
	WORD			ism_Top;
	WORD			ism_Width;
	WORD			ism_Height;
};

/* These are the values your hook code can return. */
/****************************************************************************/

/* The messages your copy hook is invoked with. */
struct CopyBeginMsg
{
	ULONG	cbm_Length;		/* Size of this data structure in bytes. */
	LONG	cbm_Action;		/* Will be set to CPACTION_Begin (see below). */
	BPTR	cbm_SourceDrawer;	/* A lock on the source drawer. */
	BPTR	cbm_DestinationDrawer;	/* A lock on the destination drawer. */
};

struct CopyDataMsg
{
	ULONG	cdm_Length;		/* Size of this data structure in bytes. */
	LONG	cdm_Action;		/* Will be set to CPACTION_Copy (see below). */

	BPTR	cdm_SourceLock;		/* A lock on the parent directory of the
					 * source file/drawer.
					 */
	STRPTR	cdm_SourceName;		/* The name of the source file or drawer. */

	BPTR	cdm_DestinationLock;	/* A lock on the parent directory of the
					 * destination file/drawer.
					 */
	STRPTR	cdm_DestinationName;	/* The name of the destination file/drawer.
					 * This may or may not match the name of
					 * the source file/drawer in case the
					 * data is to be copied under a different
					 * name. For example, this is the case
					 * with the Workbench "Copy" command which
					 * creates duplicates of file/drawers by
					 * prefixing the duplicate's name with
					 * "Copy_XXX_of".
					 */
	LONG	cdm_DestinationX;	/* When the icon corresponding to the
					 * destination is written to disk, this
					 * is the position (put into its
					 * DiskObject->do_CurrentX/DiskObject->do_CurrentY
					 * fields) it should be placed at.
					 */
	LONG	cdm_DestinationY;
};

struct CopyEndMsg
{
	ULONG	cem_Length;		/* Size of this data structure in bytes. */
	LONG	cem_Action;		/* Will be set to CPACTION_End (see below). */
};

/****************************************************************************/

/* The messages your delete hook is invoked with. */
struct DeleteBeginMsg
{
	ULONG	dbm_Length;		/* Size of this data structure in bytes. */
	LONG	dbm_Action;		/* Will be set to either DLACTION_BeginDiscard
					 * or DLACTION_BeginEmptyTrash (see below).
					 */
};

struct DeleteDataMsg
{
	ULONG	ddm_Length;		/* Size of this data structure in bytes. */
	LONG	ddm_Action;		/* Will be set to either DLACTION_DeleteContents
					 * or DLACTION_DeleteObject (see below).
					 */
	BPTR	ddm_Lock;		/* A Lock on the parent directory of the object
					 * whose contents or which itself should be
					 * deleted.
					 */
	STRPTR	ddm_Name;		/* The name of the object whose contents or
					 * which itself should be deleted.
					 */
};

struct DeleteEndMsg
{
	ULONG	dem_Length;		/* Size of this data structure in bytes. */
	LONG	dem_Action;		/* Will be set to DLACTION_End (see below). */
};

/****************************************************************************/

/* The messages your text input hook is invoked with. */
struct TextInputMsg
{
	ULONG	tim_Length;			/* Size of this data structure
						 * in bytes.
						 */
	LONG	tim_Action;			/* One of the TIACTION_...
						 * values listed below.
						 */
	STRPTR	tim_Prompt;			/* The Workbench suggested
						 * result, depending on what
						 * kind of input is requested
						 * (as indicated by the
						 * tim_Action member).
						 */
};

/****************************************************************************/

/* Parameters for the UpdateWorkbench() function. */

/****************************************************************************/

/*
**	$VER: icon.h 47.4 (16.7.2020)
**
**	External declarations for icon.library
**
**	Copyright (C) 2020 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: imageclass.h 47.4 (1.1.2021)
**
**	Definitions for the image classes
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/******************************************************/

/*
 * NOTE:  <intuition/iobsolete.h> is included at the END of this file!
 */

/* if image.Depth is this, it's a new Image class object */

/* some convenient macros and casts */
/******************************************************/
		    /* IA_FGPen also means "PlanePick"	*/
		    /* IA_BGPen also means "PlaneOnOff"	*/
		    /* bitplanes, for classic image,
		     * other image classes may use it for other things
		     */
		    /* pointer to UWORD pens[],
		     * ala DrawInfo.Pens, MUST be
		     * terminated by ~0.  Some classes can
		     * choose to have this, or SYSIA_DrawInfo,
		     * or both.
		     */
		    /* packed uwords for x/y resolution into a longword
		     * ala DrawInfo.Resolution
		     */

/**** see class documentation to learn which	*****/
/**** classes recognize these			*****/
/**** "sysiclass" attributes			*****/
		    /* #define's below		*/
		    /* this is unused by Intuition.  SYSIA_DrawInfo
		     * is used instead for V36
		     */
		    /* see #define's below	*/
		    /* pass to sysiclass, please */

/*****	obsolete: don't use these, use IA_Pens	*****/
/* New for V39: */
		    /* Font to use as reference for scaling
		     * certain sysiclass images
		     */
		    /* By default, Intuition ghosts gadgets itself,
		     * instead of relying on IDS_DISABLED or
		     * IDS_SELECTEDDISABLED.  An imageclass that
		     * supports these states should return this attribute
		     * as TRUE.  You cannot set or clear this attribute,
		     * however.
		     */

		    /* Starting with V39, FrameIClass recognizes
		     * several standard types of frame.  Use one
		     * of the FRAME_ specifiers below.	Defaults
		     * to FRAME_DEFAULT.
		     */

		    /* V44, Indicate underscore keyboard shortcut for image labels.
		     * (UBYTE) Defaults to '_'
		     */

		    /* V44, Attribute indicates this image is allowed
			 * to/can scale its rendering.
		     * (BOOL) Defaults to FALSE.
		     */

		    /* V44, Used to get an underscored label shortcut.
		     * Useful for labels attached to string gadgets.
		     * (UBYTE) Defaults to NULL.
		     */

		    /* V44 Screen pointer, may be useful/required by certain classes.
		     * (struct Screen *)
		     */

		    /* V44 Precision value, typically pen precision but may be
		     * used for similar custom purposes.
		     * (ULONG)
		     */

/* New for V47: */
		    /* Defines orientation, for images needing this kind
		     * of information, such as the PROPKNOB frameiclass
		     * type. As of V47, the values can be 0 (horizontal)
		     * or 1 (vertical). (UBYTE) Defaults to 0. (V47)
		     */

		    /* Pointer to a string to be used as the image's text
		     * label, if it supports one. (STRPTR) Defaults to
		     * NULL. (V47)
		     */

		    /* Erase the background before rendering the image?
		     * (BOOL) Typically defaults to TRUE for images having
		     * a non-rectangular shape, FALSE otherwise. (V47)
		     */

		    /* Color of the image's text label, if it supports one.
		     * (UWORD) The default depends on the class. (V47)
		     */

/** next attribute: (IA_Dummy + 0x39)	**/
/*************************************************/

/* data values for SYSIA_Size	*/
/*
 * SYSIA_Which tag data values:
 * Specifies which system gadget you want an image for.
 * Some numbers correspond to internal Intuition #defines
 */
/* New for V39: */
/* New for V47: */
/* Data values for IA_FrameType (recognized by FrameIClass)
 *
 * FRAME_DEFAULT:  The standard V37-type frame, which has
 *	thin edges.
 * FRAME_BUTTON:  Standard button gadget frames, having thicker
 *	sides and nicely edged corners.
 * FRAME_RIDGE:  A ridge such as used by standard string gadgets.
 *	You can recess the ridge to get a groove image.
 * FRAME_ICONDROPBOX: A broad ridge which is the standard imagery
 *	for areas in AppWindows where icons may be dropped.
 * FRAME_PROPBORDER: A frame suitable for use as border of a
 *	proportional gadget container. (V47)
 * FRAME_PROPKNOB: A frame suitable for use as knob of a
 *	proportional gadget. (V47)
 * FRAME_DISPLAY: A recessed frame for display elements, such as
 *	read-only text or number gadgets. (V47)
 * FRAME_CONTEXT: A frame that is used to indicate contexts
 *	in GUIs that has a thin black/white border, with
 *	the black frame on top and the white frame offset in
 *	lower-right direction drawn underneath. (V47)
 */

/* New for V47: */
/* image message id's	*/
/* image draw states or styles, for IM_DRAW */
/* Note that they have no bitwise meanings (unfortunately) */
/* oops, please forgive spelling error by jimm */
/* IM_FRAMEBOX	*/
struct impFrameBox {
    ULONG		MethodID;
    struct IBox	*imp_ContentsBox;	/* input: relative box of contents */
    struct IBox	*imp_FrameBox;		/* output: rel. box of encl frame  */
    struct DrawInfo	*imp_DrInfo;	/* NB: May be NULL */
    ULONG	imp_FrameFlags;
};

/* New for V47: */
/* IM_DRAW, IM_DRAWFRAME	*/
struct impDraw
{
    ULONG		MethodID;
    struct RastPort	*imp_RPort;
    struct
    {
	WORD	X;
	WORD	Y;
    }			imp_Offset;

    ULONG		imp_State;
    struct DrawInfo	*imp_DrInfo;	/* NB: May be NULL */

    /* these parameters only valid for IM_DRAWFRAME */
    struct
    {
	WORD	Width;
	WORD	Height;
    }			imp_Dimensions;
};

/* IM_ERASE, IM_ERASEFRAME	*/
/* NOTE: This is a subset of impDraw	*/
struct impErase
{
    ULONG		MethodID;
    struct RastPort	*imp_RPort;
    struct
    {
	WORD	X;
	WORD	Y;
    }			imp_Offset;

    /* these parameters only valid for IM_ERASEFRAME */
    struct
    {
	WORD	Width;
	WORD	Height;
    }			imp_Dimensions;
};

/* IM_HITTEST, IM_HITFRAME	*/
struct impHitTest
{
    ULONG		MethodID;
    struct
    {
	WORD	X;
	WORD	Y;
    }			imp_Point;

    /* these parameters only valid for IM_HITFRAME */
    struct
    {
	WORD	Width;
	WORD	Height;
    }			imp_Dimensions;
};


/* The IM_DOMAINFRAME method is used to obtain the sizing
 * requirements of an image object within a layout group.
 */

/* IM_DOMAINFRAME */
struct impDomainFrame
{
    ULONG		 MethodID;
    struct DrawInfo	*imp_DrInfo;	/* DrawInfo */
    struct RastPort	*imp_RPort;	/* RastPort to layout for */
    LONG	 	 imp_Which;	/* what size - min/nominal/max */
    struct IBox		 imp_Domain;	/* Resulting domain */
    struct TagItem	*imp_Attrs;	/* Additional attributes */
};

/* Accepted vales for imp_Which.
 */
/* Include obsolete identifiers: */
/*
**	$VER: pictureclass.h 47.4 (14.12.2021)
**
**  Interface definitions for DataType picture objects.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: datatypesclass.h 47.2 (16.11.2021)
**
**	Interface definitions for DataType objects.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: datatypes.h 47.1 (28.6.2019)
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/

/*
**	$VER: libraries.h 47.1 (28.6.2019)
**
**	Definitions for use when creating or using Exec libraries
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*------ Special Constants ---------------------------------------*/
/*------ Standard Functions --------------------------------------*/
/*------ Library Base Structure ----------------------------------*/
/* Also used for Devices and some Resources */
struct Library {
    struct  Node lib_Node;
    UBYTE   lib_Flags;
    UBYTE   lib_pad;
    UWORD   lib_NegSize;	    /* number of bytes before library */
    UWORD   lib_PosSize;	    /* number of bytes after library */
    UWORD   lib_Version;	    /* major */
    UWORD   lib_Revision;	    /* minor */
    APTR    lib_IdString;	    /* ASCII identification */
    ULONG   lib_Sum;		    /* the checksum itself */
    UWORD   lib_OpenCnt;	    /* number of current opens */
};	/* Warning: size is not a longword multiple! */

/* lib_Flags bit definitions (all others are system reserved) */
/* Temporary Compatibility */
/*
**	$VER: iffparse.h 47.2 (26.12.2021)
**
**      iffparse.library structures and constants
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*
**	$VER: clipboard.h 47.1 (28.6.2019)
**
**	clipboard.device structure definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct ClipboardUnitPartial {
    struct  Node cu_Node;	/* list of units */
    ULONG   cu_UnitNum;		/* unit number for this unit */
    /* the remaining unit data is private to the device */
};


struct IOClipReq {
    struct Message io_Message;
    struct Device *io_Device;	/* device node pointer	*/
    struct ClipboardUnitPartial *io_Unit; /* unit node pointer */
    UWORD   io_Command;		/* device command */
    UBYTE   io_Flags;		/* including QUICK and SATISFY */
    BYTE    io_Error;		/* error or warning num */
    ULONG   io_Actual;		/* number of bytes transferred */
    ULONG   io_Length;		/* number of bytes requested */
    STRPTR  io_Data;		/* either clip stream or post port */
    ULONG   io_Offset;		/* offset in clip stream */
    LONG    io_ClipID;		/* ordinal clip identifier */
};

struct SatisfyMsg {
    struct Message sm_Msg;	/* the length will be 6 */
    UWORD   sm_Unit;		/* which clip unit this is */
    LONG    sm_ClipID;		/* the clip identifier of the post */
};

struct ClipHookMsg {
    ULONG   chm_Type;		/* zero for this structure format */
    LONG    chm_ChangeCmd;	/* command that caused this hook invocation: */
				/*   either CMD_UPDATE or CBD_POST */
    LONG    chm_ClipID;		/* the clip identifier of the new data */
};

/*****************************************************************************/


/* Structure associated with an active IFF stream.
 * "iff_Stream" is a value used by the client's read/write/seek functions -
 * it will not be accessed by the library itself and can have any value
 * (could even be a pointer or a BPTR).
 *
 * This structure can only be allocated by iffparse.library
 */
struct IFFHandle
{
    ULONG iff_Stream;
    ULONG iff_Flags;
    LONG  iff_Depth;    /*  Depth of context stack */
};

/* bit masks for "iff_Flags" field */
/*****************************************************************************/


/* When the library calls your stream handler, you'll be passed a pointer
 * to this structure as the "message packet".
 */
struct IFFStreamCmd
{
    LONG sc_Command;    /* Operation to be performed (IFFCMD_) */
    APTR sc_Buf;        /* Pointer to data buffer              */
    LONG sc_NBytes;     /* Number of bytes to be affected      */
};


/*****************************************************************************/


/* A node associated with a context on the iff_Stack. Each node
 * represents a chunk, the stack representing the current nesting
 * of chunks in the open IFF file. Each context node has associated
 * local context items in the (private) LocalItems list.  The ID, type,
 * size and scan values describe the chunk associated with this node.
 *
 * This structure can only be allocated by iffparse.library
 */
struct ContextNode
{
    struct MinNode cn_Node;
    LONG           cn_ID;
    LONG           cn_Type;
    LONG           cn_Size;     /*  Size of this chunk             */
    LONG           cn_Scan;     /*  # of bytes read/written so far */
};


/*****************************************************************************/


/* Local context items live in the ContextNode's.  Each class is identified
 * by its lci_Ident code and has a (private) purge vector for when the
 * parent context node is popped.
 *
 * This structure can only be allocated by iffparse.library
 */
struct LocalContextItem
{
    struct MinNode lci_Node;
    ULONG          lci_ID;
    ULONG          lci_Type;
    ULONG          lci_Ident;
};


/*****************************************************************************/


/* StoredProperty: a local context item containing the data stored
 * from a previously encountered property chunk.
 */
struct StoredProperty
{
    LONG sp_Size;
    APTR sp_Data;
};


/*****************************************************************************/


/* Collection Item: the actual node in the collection list at which
 * client will look. The next pointers cross context boundaries so
 * that the complete list is accessable.
 */
struct CollectionItem
{
    struct CollectionItem *ci_Next;
    LONG                   ci_Size;
    APTR                   ci_Data;
};


/*****************************************************************************/


/* Structure returned by OpenClipboard(). You may do CMD_POSTs and such
 * using this structure. However, once you call OpenIFF(), you may not
 * do any more of your own I/O to the clipboard until you call CloseIFF().
 */
struct ClipboardHandle
{
    struct IOClipReq cbh_Req;
    struct MsgPort   cbh_CBport;
    struct MsgPort   cbh_SatisfyPort;
};


/*****************************************************************************/


/* IFF return codes. Most functions return either zero for success or
 * one of these codes. The exceptions are the read/write functions which
 * return positive values for number of bytes or records read or written,
 * or a negative error code. Some of these codes are not errors per sae,
 * but valid conditions such as EOF or EOC (End of Chunk).
 */
/*****************************************************************************/


/* Universal IFF identifiers */
/* Identifier codes for universally recognized local context items */
/*****************************************************************************/


/* Control modes for ParseIFF() function */
/*****************************************************************************/


/* Control modes for StoreLocalItem() function */
/*****************************************************************************/


/* Magic value for writing functions. If you pass this value in as a size
 * to PushChunk() when writing a file, the parser will figure out the
 * size of the chunk for you. If you know the size, is it better to
 * provide as it makes things faster.
 */
/*****************************************************************************/


/* Possible call-back command values */
/*****************************************************************************/


/* Obsolete IFFParse definitions, here for source code compatibility only.
 * Please do NOT use in new code.
 *
 * #define IFFPARSE_V37_NAMES_ONLY to remove these older names
 */
/*****************************************************************************/


/*****************************************************************************/

/*****************************************************************************/

struct DataTypeHeader
{
    STRPTR	 dth_Name;				/* Descriptive name of the data type */
    STRPTR	 dth_BaseName;				/* Base name of the data type */
    STRPTR	 dth_Pattern;				/* Match pattern for file name. */
    WORD	*dth_Mask;				/* Comparision mask */
    ULONG	 dth_GroupID;				/* Group that the DataType is in */
    ULONG	 dth_ID;				/* ID for DataType (same as IFF FORM type) */
    WORD	 dth_MaskLen;				/* Length of comparision mask */
    WORD	 dth_Pad;				/* Unused at present (must be 0) */
    UWORD	 dth_Flags;				/* Flags */
    UWORD	 dth_Priority;				/* Priority */
};

/*****************************************************************************/

/* Basic type */
/* Set if case is important */
/* Reserved for system use */
/*****************************************************************************
 *
 * GROUP ID and ID
 *
 * This is used for filtering out objects that you don't want.  For
 * example, you could make a filter for the ASL file requester so
 * that it only showed the files that were pictures, or even to
 * narrow it down to only show files that were ILBM pictures.
 *
 * Note that the Group ID's are in lower case, and always the first
 * four characters of the word.
 *
 * For ID's; If it is an IFF file, then the ID is the same as the
 * FORM type.  If it isn't an IFF file, then the ID would be the
 * first four characters of name for the file type.
 *
 *****************************************************************************/

/* System file, such as; directory, executable, library, device, font, etc. */
/* Formatted or unformatted text */
/* Formatted text with graphics or other DataTypes */
/* Sound */
/* Musical instruments used for musical scores */
/* Musical score */
/* Still picture */
/* Animated picture */
/* Animation with audio track */
/*****************************************************************************/

/* A code chunk contains an embedded executable that can be loaded
 * with InternalLoadSeg. */
/* DataTypes comparision hook context (Read-Only).  This is the
 * argument that is passed to a custom comparision routine. */
struct DTHookContext
{
    /* Libraries that are already opened for your use */
    struct Library		*dthc_SysBase;
    struct Library		*dthc_DOSBase;
    struct Library		*dthc_IFFParseBase;
    struct Library		*dthc_UtilityBase;

    /* File context */
    BPTR			 dthc_Lock;		/* Lock on the file */
    struct FileInfoBlock	*dthc_FIB;		/* Pointer to a FileInfoBlock */
    BPTR			 dthc_FileHandle;	/* Pointer to the file handle (may be NULL) */
    struct IFFHandle		*dthc_IFF;		/* Pointer to an IFFHandle (may be NULL) */
    STRPTR			 dthc_Buffer;		/* Buffer */
    ULONG			 dthc_BufferLength;	/* Length of the buffer */
};

/*****************************************************************************/

struct Tool
{
    UWORD	 tn_Which;				/* Which tool is this */
    UWORD	 tn_Flags;				/* Flags */
    STRPTR	 tn_Program;				/* Application to use */
};

/* defines for tn_Which */
/* defines for tn_Flags */
/*****************************************************************************/

/*****************************************************************************/

struct DataType
{
    struct Node	 		 dtn_Node1;		/* Reserved for system use */
    struct Node			 dtn_Node2;		/* Reserved for system use */
    struct DataTypeHeader	*dtn_Header;		/* Pointer to the DataTypeHeader */
    struct List			 dtn_ToolList;		/* List of tool nodes */
    STRPTR			 dtn_FunctionName;	/* Name of comparision routine */
    struct TagItem		*dtn_AttrList;		/* Object creation tags */
    ULONG			 dtn_Length;		/* Length of the memory block */
};
/*****************************************************************************/

struct ToolNode
{
    struct Node	 tn_Node;				/* Embedded node */
    struct Tool  tn_Tool;				/* Embedded tool */
    ULONG	 tn_Length;				/* Length of the memory block */
};

/*****************************************************************************/

/*****************************************************************************/

/* text ID's */
/* new for V40 */
/* New for V44 */
/* Offset for types */
/*****************************************************************************/

/*
**	$VER: printer.h 47.1 (28.6.2019)
**
**	printer.device structure definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* printer command definitions */

/*
	Suggested typefaces are:

	 0 - default typeface.
	 1 - Line Printer or equiv.
	 2 - Pica or equiv.
	 3 - Elite or equiv.
	 4 - Helvetica or equiv.
	 5 - Times Roman or equiv.
	 6 - Gothic or equiv.
	 7 - Script or equiv.
	 8 - Prestige or equiv.
	 9 - Caslon or equiv.
	10 - Orator or equiv.
*/

struct IOPrtCmdReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    UWORD   io_PrtCommand;          /* printer command */
    UBYTE   io_Parm0;               /* first command parameter */
    UBYTE   io_Parm1;               /* second command parameter */
    UBYTE   io_Parm2;               /* third command parameter */
    UBYTE   io_Parm3;               /* fourth command parameter */
};

struct IODRPReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    struct  RastPort *io_RastPort;  /* raster port */
    struct  ColorMap *io_ColorMap;  /* color map */
    ULONG   io_Modes;               /* graphics viewport modes */
    UWORD   io_SrcX;                /* source x origin */
    UWORD   io_SrcY;                /* source y origin */
    UWORD   io_SrcWidth;            /* source x width */
    UWORD   io_SrcHeight;           /* source x height */
    LONG    io_DestCols;            /* destination x width */
    LONG    io_DestRows;            /* destination y height */
    UWORD   io_Special;             /* option flags */
};

struct IODRPTagsReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    struct  RastPort *io_RastPort;  /* raster port */
    struct  ColorMap *io_ColorMap;  /* color map */
    ULONG   io_Modes;               /* graphics viewport modes */
    UWORD   io_SrcX;                /* source x origin */
    UWORD   io_SrcY;                /* source y origin */
    UWORD   io_SrcWidth;            /* source x width */
    UWORD   io_SrcHeight;           /* source x height */
    LONG    io_DestCols;            /* destination x width */
    LONG    io_DestRows;            /* destination y height */
    UWORD   io_Special;             /* option flags */
    struct  TagItem *io_TagList;    /* tag list with additional info */
};

/*
	Compute print size, set 'io_DestCols' and 'io_DestRows' in the calling
	program's 'IODRPReq' structure and exit, DON'T PRINT.  This allows the
	calling program to see what the final print size would be in printer
	pixels.  Note that it modifies the 'io_DestCols' and 'io_DestRows'
	fields of your 'IODRPReq' structure.  Also, set the print density and
	update the 'MaxXDots', 'MaxYDots', 'XDotsInch', and 'YDotsInch' fields
	of the 'PrinterExtendedData' structure.
*/
/*
	Note : this is an internal error that can be returned from the render
	function to the printer device.  It is NEVER returned to the user.
	If the printer device sees this error it converts it 'PDERR_NOERR'
	and exits gracefully.  Refer to the document on
	'How to Write a Graphics Printer Driver' for more info.
*/
/*
	Note: all error codes < 32 are reserved for printer.device.
	All error codes >= 32 and < 127 are reserved for driver specific
	errors. Negative errors are reserved for system use (standard I/O
	errors) and error code 127 is reserved for future expansion.

*/
/* internal use */
/* The following 3 tags are not implemented but reserved for future use.
*/
/* If the following tag is used io_RastPort and io_ColorMap are
   ignored.
*/
/* If these tags are used io_Modes is ignored for aspect ratio
*/
/* The source hook (DRPA_SourceHook) is called with object NULL and
   message is a pointer to the following struct.
*/
struct DRPSourceMsg {
	LONG x;
	LONG y;
	LONG width;
	LONG height;
	ULONG *buf; /* fill this buffer with 0x00RRGGBB pixels */
};

/* Request to edit prefs
*/

struct IOPrtPrefsReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    struct  TagItem *io_TagList;    /* requester tag list */
};

/* Request to set error hook
*/

struct IOPrtErrReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    struct  Hook *io_Hook;
};

/*
	The error hook is called with the IORequest that caused the error as
	object (2nd Parameter) and a pointer to struct PrtErrMsg as message
	(3thd Parameter).
*/
struct PrtErrMsg {
	ULONG pe_Version; /* Version of this struct */
	ULONG pe_ErrorLevel; /* RETURN_WARN, RETURN_ERROR, RETURN_FAIL */
	struct Window *pe_Window; /* window for EasyRequest() */
	struct EasyStruct *pe_ES;
	ULONG *pe_IDCMP;
	APTR pe_ArgList;
};

/* PRIVATE: Request to change prefs temporary. DO NOT USE!
*/

struct IOPrefsReq {
    struct  Message io_Message;
    struct  Device  *io_Device;     /* device node pointer  */
    struct  Unit    *io_Unit;       /* unit (driver private)*/
    UWORD   io_Command;             /* device command */
    UBYTE   io_Flags;
    BYTE    io_Error;               /* error or warning num */
    struct PrinterTxtPrefs *io_TxtPrefs;
    struct PrinterUnitPrefs *io_UnitPrefs;
    struct PrinterDeviceUnitPrefs *io_DevUnitPrefs;
    struct PrinterGfxPrefs *io_GfxPrefs;
};

/*
**	$VER: prtbase.h 47.2 (16.11.2021)
**
**	printer.device base structure definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: parallel.h 47.1 (28.6.2019)
**
**	parallel.device I/O request structure information
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

 struct  IOPArray {
	ULONG PTermArray0;
	ULONG PTermArray1;
};

/******************************************************************/
/* CAUTION !!  IF YOU ACCESS the parallel.device, you MUST (!!!!) use
   an IOExtPar-sized structure or you may overlay innocent memory !! */
/******************************************************************/

 struct   IOExtPar {
	struct	 IOStdReq IOPar;

/*     STRUCT	MsgNode
*   0	APTR	 Succ
*   4	APTR	 Pred
*   8	UBYTE	 Type
*   9	UBYTE	 Pri
*   A	APTR	 Name
*   E	APTR	 ReplyPort
*  12	UWORD	 MNLength
*     STRUCT   IOExt
*  14	APTR	 io_Device
*  18	APTR	 io_Unit
*  1C	UWORD	 io_Command
*  1E	UBYTE	 io_Flags
*  1F	UBYTE	 io_Error
*     STRUCT   IOStdExt
*  20	ULONG	 io_Actual
*  24	ULONG	 io_Length
*  28	APTR	 io_Data
*  2C	ULONG	 io_Offset
*  30 */
	ULONG	io_PExtFlags;	 /* (not used) flag extension area */
	UBYTE	io_Status;	 /* status of parallel port and registers */
	UBYTE	io_ParFlags;	 /* see PARFLAGS bit definitions below */
	struct	IOPArray io_PTermArray; /* termination character array */
};

/* Note: previous versions of this include files had bits 0 and 2 swapped */

/*
**	$VER: serial.h 47.1 (28.6.2019)
**
**	external declarations for the serial device
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

		   /* array of termination char's */
		   /* to use,see serial.doc setparams */

 struct  IOTArray {
	ULONG TermArray0;
	ULONG TermArray1;
};


/* You may change these via SETPARAMS.	At this time, parity is not
   calculated for xON/xOFF characters.	You must supply them with the
   desired parity. */

/******************************************************************/
/* CAUTION !!  IF YOU ACCESS the serial.device, you MUST (!!!!) use an
   IOExtSer-sized structure or you may overlay innocent memory !! */
/******************************************************************/

 struct  IOExtSer {
	struct	 IOStdReq IOSer;

/*     STRUCT	MsgNode
*   0	APTR	 Succ
*   4	APTR	 Pred
*   8	UBYTE	 Type
*   9	UBYTE	 Pri
*   A	APTR	 Name
*   E	APTR	 ReplyPort
*  12	UWORD	 MNLength
*     STRUCT   IOExt
*  14	APTR	 io_Device
*  18	APTR	 io_Unit
*  1C	UWORD	 io_Command
*  1E	UBYTE	 io_Flags
*  1F	BYTE	 io_Error
*     STRUCT   IOStdExt
*  20	ULONG	 io_Actual
*  24	ULONG	 io_Length
*  28	APTR	 io_Data
*  2C	ULONG	 io_Offset
*
*  30 */
   ULONG   io_CtlChar;	  /* control char's (order = xON,xOFF,INQ,ACK) */
   ULONG   io_RBufLen;	  /* length in bytes of serial port's read buffer */
   ULONG   io_ExtFlags;   /* additional serial flags (see bitdefs below) */
   ULONG   io_Baud;	  /* baud rate requested (true baud) */
   ULONG   io_BrkTime;	  /* duration of break signal in MICROseconds */
   struct  IOTArray io_TermArray; /* termination character array */
   UBYTE   io_ReadLen;	  /* bits per read character (# of bits) */
   UBYTE   io_WriteLen;   /* bits per write character (# of bits) */
   UBYTE   io_StopBits;   /* stopbits for read (# of bits) */
   UBYTE   io_SerFlags;   /* see SerFlags bit definitions below  */
   UWORD   io_Status;
};
   /* status of serial port, as follows:
*		   BIT	ACTIVE	FUNCTION
*		    0	 ---	reserved
*		    1	 ---	reserved
*		    2	 high	Connected to parallel "select" on the A1000.
*				Connected to both the parallel "select" and
*				serial "ring indicator" pins on the A500
*				& A2000.  Take care when making cables.
*		    3	 low	Data Set Ready
*		    4	 low	Clear To Send
*		    5	 low	Carrier Detect
*		    6	 low	Ready To Send
*		    7	 low	Data Terminal Ready
*		    8	 high	read overrun
*		    9	 high	break sent
*		   10	 high	break received
*		   11	 high	transmit x-OFFed
*		   12	 high	receive x-OFFed
*		13-15		reserved
*/

/* These now refect the actual bit positions in the io_Status UWORD */
				/*	    instead of odd-even. */
/*
**	$VER: dosextens.h 47.1 (29.7.2019)
**
**	DOS structures not needed for the casual AmigaDOS user 
**	Obsolete - see dos/dosextens.h
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: dosextens.h 47.2 (16.11.2021)
**
**	DOS structures not needed for the casual AmigaDOS user
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* All DOS processes have this structure */
/* Create and Device Proc returns pointer to the MsgPort in this structure */
/* dev_proc = (struct Process *) (DeviceProc(..) - sizeof(struct Task)); */

struct Process {
    struct  Task    pr_Task;
    struct  MsgPort pr_MsgPort; /* This is BPTR address from DOS functions  */
    WORD    pr_Pad;		/* Remaining variables on 4 byte boundaries */
    BPTR    pr_SegList;		/* Array of seg lists used by this process  */
    LONG    pr_StackSize;	/* Size of process stack in bytes	    */
    APTR    pr_GlobVec;		/* Global vector for this process (BCPL)    */
    LONG    pr_TaskNum;		/* CLI task number of zero if not a CLI	    */
    BPTR    pr_StackBase;	/* Ptr to high memory end of process stack  */
    LONG    pr_Result2;		/* Value of secondary result from last call */
    BPTR    pr_CurrentDir;	/* Lock associated with current directory   */
    BPTR    pr_CIS;		/* Current CLI Input Stream		    */
    BPTR    pr_COS;		/* Current CLI Output Stream		    */
    APTR    pr_ConsoleTask;	/* Console handler process for current window*/
    APTR    pr_FileSystemTask;	/* File handler process for current drive   */
    BPTR    pr_CLI;		/* pointer to CommandLineInterface	    */
    APTR    pr_ReturnAddr;	/* pointer to previous stack frame	    */
    APTR    pr_PktWait;		/* Function to be called when awaiting msg  */
    APTR    pr_WindowPtr;	/* Window for error printing		    */

    /* following definitions are new with 2.0 */
    BPTR    pr_HomeDir;		/* Home directory of executing program	    */
    LONG    pr_Flags;		/* flags telling dos about process	    */
    void    (*pr_ExitCode)();	/* code to call on exit of program or NULL  */
    LONG    pr_ExitData;	/* Passed as an argument to pr_ExitCode.    */
    STRPTR  pr_Arguments;	/* Arguments passed to the process at start */
    struct MinList pr_LocalVars; /* Local environment variables		    */
    ULONG   pr_ShellPrivate;	/* for the use of the current shell	    */
    BPTR    pr_CES;		/* Error stream - if NULL, use pr_COS	    */
};  /* Process */

/*
 * Flags for pr_Flags
 */
/* The long word address (BPTR) of this structure is returned by
 * Open() and other routines that return a file.  You need only worry
 * about this struct to do async io's via PutMsg() instead of
 * standard file system calls */

struct FileHandle {
   struct Message *fh_Link;	 /* private use, not really a message   */
   struct MsgPort *fh_Port;	 /* Misnamed by tradition, is
				  * Boolean; true if interactive handle */
   struct MsgPort *fh_Type;	 /* Port to do PutMsg() to
				  * or NULL for NIL: */
   BPTR fh_Buf;                  /* BPTR to I/O buffer if present */
   LONG fh_Pos;
   LONG fh_End;
   LONG fh_Funcs;
   LONG fh_Func2;
   LONG fh_Func3;
   LONG fh_Args;
   LONG fh_Arg2;
}; /* FileHandle */

/* This is the extension to EXEC Messages used by DOS */

struct DosPacket {
   struct Message *dp_Link;	 /* EXEC message	      */
   struct MsgPort *dp_Port;	 /* Reply port for the packet */
				 /* Must be filled in each send. */
   LONG dp_Type;		 /* See ACTION_... below and
				  * 'R' means Read, 'W' means Write to the
				  * file system */
   LONG dp_Res1;		 /* For file system calls this is the result
				  * that would have been returned by the
				  * function, e.g. Write ('W') returns actual
				  * length written */
   LONG dp_Res2;		 /* For file system calls this is what would
				  * have been returned by IoErr() */
/*  Device packets common equivalents */
   LONG dp_Arg1;
   LONG dp_Arg2;
   LONG dp_Arg3;
   LONG dp_Arg4;
   LONG dp_Arg5;
   LONG dp_Arg6;
   LONG dp_Arg7;
}; /* DosPacket */

/* A Packet does not require the Message to be before it in memory, but
 * for convenience it is useful to associate the two.
 * Also see the function init_std_pkt for initializing this structure */

struct StandardPacket {
   struct Message   sp_Msg;
   struct DosPacket sp_Pkt;
}; /* StandardPacket */

/* Packet types */
/* new 2.0 packets */
/**/
/**/
/**/
/* Added in V39: */
/* Tell a file system to serialize the current volume. This is typically
 * done by changing the creation date of the disk. This packet does not take
 * any arguments.  NOTE: be prepared to handle failure of this packet for
 * V37 ROM filesystems.
 */
/*
 * A structure for holding error messages - stored as array with error == 0
 * for the last entry.
 */
struct ErrorString {
	LONG * estr_Nums;
	STRPTR estr_Strings;
};

/* DOS library node structure.
 * This is the data at positive offsets from the library node.
 * Negative offsets from the node is the jump table to DOS functions
 * node = (struct DosLibrary *) OpenLibrary( "dos.library" .. )	     */

struct DosLibrary {
    struct Library dl_lib;
    struct RootNode *dl_Root; /* Pointer to RootNode, described below */
    APTR    dl_GV;	      /* Pointer to BCPL global vector	      */
    LONG    dl_A2;	      /* BCPL standard register values	      */
    LONG    dl_A5;
    LONG    dl_A6;
    struct ErrorString *dl_Errors;	  /* PRIVATE pointer to array of error msgs */
    struct timerequest *dl_TimeReq;	  /* PRIVATE pointer to timer request */
    struct Library     *dl_UtilityBase;   /* PRIVATE ptr to utility library */
    struct Library     *dl_IntuitionBase; /* PRIVATE ptr to intuition library */
};  /*	DosLibrary */

/*			       */

struct RootNode {
    BPTR    rn_TaskArray;	     /* [0] is max number of CLI's
				      * [1] is APTR to process id of CLI 1
				      * [n] is APTR to process id of CLI n */
    BPTR    rn_ConsoleSegment; /* SegList for the CLI			   */
    struct  DateStamp rn_Time; /* Current time				   */
    LONG    rn_RestartSeg;     /* SegList for the disk validator process   */
    BPTR    rn_Info;	       /* Pointer to the Info structure		   */
    BPTR    rn_FileHandlerSegment; /* segment for a file handler	   */
    struct MinList rn_CliList; /* new list of all CLI processes */
			       /* the first cpl_Array is also rn_TaskArray */
    struct MsgPort *rn_BootProc; /* private ptr to msgport of boot fs      */
    BPTR    rn_ShellSegment;   /* seglist for Shell (for NewShell)	   */
    LONG    rn_Flags;          /* dos flags */
};  /* RootNode */

/* ONLY to be allocated by DOS! */
struct CliProcList {
	struct MinNode cpl_Node;
	LONG cpl_First;	     /* number of first entry in array */
	struct MsgPort **cpl_Array;
			     /* [0] is max number of CLI's in this entry (n)
			      * [1] is CPTR to process id of CLI cpl_First
			      * [n] is CPTR to process id of CLI cpl_First+n-1
			      */
};

struct DosInfo {
    BPTR    di_McName;	       /* PRIVATE: system resident module list	    */
    BPTR    di_DevInfo;	       /* Device List				    */
    BPTR    di_Devices;	       /* Currently zero			    */
    BPTR    di_Handlers;       /* Currently zero			    */
    APTR    di_NetHand;	       /* Network handler processid; currently zero */
    struct  SignalSemaphore di_DevLock;	   /* do NOT access directly! */
    struct  SignalSemaphore di_EntryLock;  /* do NOT access directly! */
    struct  SignalSemaphore di_DeleteLock; /* do NOT access directly! */
};  /* DosInfo */

/* structure for the Dos resident list.  Do NOT allocate these, use	  */
/* AddSegment(), and heed the warnings in the autodocs!			  */

struct Segment {
	BPTR seg_Next;
	LONG seg_UC;
	BPTR seg_Seg;
	UBYTE seg_Name[4];	/* actually the first 4 chars of BSTR name */
};

/* DOS Processes started from the CLI via RUN or NEWCLI have this additional
 * set to data associated with them */

struct CommandLineInterface {
    LONG   cli_Result2;	       /* Value of IoErr from last command	  */
    BSTR   cli_SetName;	       /* Name of current directory		  */
    BPTR   cli_CommandDir;     /* Head of the path locklist		  */
    LONG   cli_ReturnCode;     /* Return code from last command		  */
    BSTR   cli_CommandName;    /* Name of current command		  */
    LONG   cli_FailLevel;      /* Fail level (set by FAILAT)		  */
    BSTR   cli_Prompt;	       /* Current prompt (set by PROMPT)	  */
    BPTR   cli_StandardInput;  /* Default (terminal) CLI input		  */
    BPTR   cli_CurrentInput;   /* Current CLI input			  */
    BSTR   cli_CommandFile;    /* Name of EXECUTE command file		  */
    LONG   cli_Interactive;    /* Boolean; True if prompts required	  */
    LONG   cli_Background;     /* Boolean; True if CLI created by RUN	  */
    BPTR   cli_CurrentOutput;  /* Current CLI output			  */
    LONG   cli_DefaultStack;   /* Stack size to be obtained in long words */
    BPTR   cli_StandardOutput; /* Default (terminal) CLI output		  */
    BPTR   cli_Module;	       /* SegList of currently loaded command	  */
};  /* CommandLineInterface */

/* This structure can take on different values depending on whether it is
 * a device, an assigned directory, or a volume.  Below is the structure
 * reflecting volumes only.  Following that is the structure representing
 * only devices. Following that is the unioned structure representing all
 * the values
 */

/* structure representing a volume */

struct DeviceList {
    BPTR		dl_Next;	/* bptr to next device list */
    LONG		dl_Type;	/* see DLT below */
    struct MsgPort *	dl_Task;	/* ptr to handler task */
    BPTR		dl_Lock;	/* not for volumes */
    struct DateStamp	dl_VolumeDate;	/* creation date */
    BPTR		dl_LockList;	/* outstanding locks */
    LONG		dl_DiskType;	/* 'DOS', etc */
    LONG		dl_unused;
    BSTR 		dl_Name;	/* bptr to bcpl name */
};

/* device structure (same as the DeviceNode structure in filehandler.h) */

struct	      DevInfo {
    BPTR  dvi_Next;
    LONG  dvi_Type;
    APTR  dvi_Task;
    BPTR  dvi_Lock;
    BSTR  dvi_Handler;
    LONG  dvi_StackSize;
    LONG  dvi_Priority;
    LONG  dvi_Startup;
    BPTR  dvi_SegList;
    BPTR  dvi_GlobVec;
    BSTR  dvi_Name;
};

/* combined structure for devices, assigned directories, volumes */

struct DosList {
    BPTR		dol_Next;	 /* bptr to next device on list */
    LONG		dol_Type;	 /* see DLT below */
    struct MsgPort     *dol_Task;	 /* ptr to handler task */
    BPTR		dol_Lock;
    union {
	struct {
	BSTR	dol_Handler;	/* file name to load if seglist is null */
	LONG	dol_StackSize;	/* stacksize to use when starting process */
	LONG	dol_Priority;	/* task priority when starting process */
	ULONG	dol_Startup;	/* startup msg: FileSysStartupMsg for disks */
	BPTR	dol_SegList;	/* already loaded code for new task */
	BPTR	dol_GlobVec;	/* BCPL global vector to use when starting
				 * a process. -1 indicates a C/Assembler
				 * program. */
	} dol_handler;

	struct {
	struct DateStamp	dol_VolumeDate;	 /* creation date */
	BPTR			dol_LockList;	 /* outstanding locks */
	LONG			dol_DiskType;	 /* 'DOS', etc */
	} dol_volume;

	struct {
	STRPTR dol_AssignName;     /* name for non-or-late-binding assign */
	struct AssignList *dol_List; /* for multi-directory assigns (regular) */
	} dol_assign;

    } dol_misc;

    BSTR		dol_Name;	 /* bptr to bcpl name */
    };

/* structure used for multi-directory assigns. AllocVec()ed. */

struct AssignList {
	struct AssignList *al_Next;
	BPTR		   al_Lock;
};

/* definitions for dl_Type */
/* structure return by GetDeviceProc() */
struct DevProc {
	struct MsgPort *dvp_Port;
	BPTR		dvp_Lock;
	ULONG		dvp_Flags;
	struct DosList *dvp_DevNode;	/* DON'T TOUCH OR USE! */
};

/* definitions for dvp_Flags */
/* Flags to be passed to LockDosList(), etc */
/* you MUST specify one of LDF_READ or LDF_WRITE */
/* actually all but LDF_ENTRY (which is used for internal locking) */
/* a lock structure, as returned by Lock() or DupLock() */
struct FileLock {
    BPTR		fl_Link;	/* bcpl pointer to next lock */
    LONG		fl_Key;		/* disk block number */
    LONG		fl_Access;	/* exclusive or shared */
    struct MsgPort *	fl_Task;	/* handler task's port */
    BPTR		fl_Volume;	/* bptr to DLT_VOLUME DosList entry */
};

/* error report types for ErrorReport() */
/* Special error codes for ErrorReport() */
/* types for initial packets to shells from run/newcli/execute/system. */
/* For shell-writers only */
/* Types for fib_DirEntryType.  NOTE that both USERDIR and ROOT are	 */
/* directories, and that directory/file checks should use <0 and >=0.	 */
/* This is not necessarily exhaustive!  Some handlers may use other      */
/* values as needed, though <0 and >=0 should remain as supported as	 */
/* possible.								 */
struct DeviceData {
    struct Library dd_Device; /* standard library node */
    APTR dd_Segment;          /* A0 when initialized */
    APTR dd_ExecBase;         /* A6 for exec */
    APTR dd_CmdVectors;       /* command table for device commands */
    APTR dd_CmdBytes;         /* bytes describing which command queue */
    UWORD   dd_NumCommands;   /* the number of commands supported */
};

/* IO Flags */
/* pd_Flags */
/* du_Flags (actually placed in pd_Unit.mp_Node.ln_Pri) */
/* Forward declaration for a combined printer I/O request
 * which is suitable for all commands, e.g.:
 *
 *     union printerIO
 *     {
 *         struct IOStdReq ios;
 *         struct IODRPReq iodrp;
 *         struct IOPrtCmdReq iopc;
 *     };
 */

union printerIO;

/* Forward declaration for <devices/printer.h>. */
/*
	"struct PrinterData" was a very bad concept in the old V1.0 days
	because it is both: the device and the unit.

	Starting with V44 PrinterData may be duplicated for many Units. But all
	new fields that are specific to the Unit  are now part of the new
	"struct PrinterUnit". Don't touch the private fields!

	A note on the function pointers in these data structure definitions:
	unless otherwise specified, all functions expect that their parameters
	are passed on the *stack* rather than in CPU registers. Every parameter
	must be passed a 32 bit long word, i.e. an "UWORD" will use the same
	stack space as an "ULONG".
*/

struct   PrinterData {
	struct DeviceData pd_Device;
        /* PRIVATE & OBSOLETE: the one and only unit */
	struct MsgPort pd_Unit;	/* the one and only unit */
	BPTR pd_PrinterSegment;	/* the printer specific segment */
	UWORD pd_PrinterType;	/* the segment printer type */
				/* the segment data structure */
	struct PrinterSegment *pd_SegmentData;
	UBYTE *pd_PrintBuf;	/* the raster print buffer */
	LONG (* __stdargs pd_PWrite)(APTR buffer,LONG length);	/* the write function */
	LONG (* __stdargs pd_PBothReady)(void);                 /* write function's done */
	union {			/* port I/O request 0 */
		struct IOExtPar pd_p0;
		struct IOExtSer pd_s0;
	} pd_ior0;

	union {			/*   and 1 for double buffering */
		struct IOExtPar pd_p1;
		struct IOExtSer pd_s1;
	} pd_ior1;

	struct timerequest pd_TIOR;	/* timer I/O request */
	struct MsgPort pd_IORPort;	/* and message reply port */
	struct Task pd_TC;		/* write task */
        /* PRIVATE: and stack space (OBSOLETE) */
	UBYTE pd_OldStk[0x0800];	/* and stack space (OBSOLETE) */
	UBYTE pd_Flags;			/* device flags */
	UBYTE pd_pad;			/* padding */
	struct Preferences pd_Preferences;	/* the latest preferences */
	UBYTE pd_PWaitEnabled;		/* wait function switch */
	/* new fields for V2.0 */
	UBYTE pd_Flags1;		/* padding */
	UBYTE pd_Stk[0x1000];	/* stack space */

        /**************************************************************
	 *
	 *  New fields for V3.5 (V44):
	 *
	 *************************************************************/

	/* PRIVATE: the Unit. pd_Unit is obsolete */
	struct PrinterUnit * pd_PUnit;

	/* the read function:
	 *
	 *	LONG pd_PRead(APTR buffer,
	 *	              LONG * length,
	 *	              TimeVal_Type * tv);
	 */
	LONG (* __stdargs pd_PRead)(APTR buffer, LONG *length, TimeVal_Type *tv);

	/* call application's error hook:
	 *
	 *	LONG pd_CallErrorHook(struct Hook * hook,
	 *	                      union printerIO * ior,
	 *	                      struct PrtErrMsg * pem);
	 */
        LONG (* __stdargs pd_CallErrHook)(struct Hook * hook,
					  union printerIO * ior,
					  struct PrtErrMsg * pem);

	/* unit number */
	ULONG pd_UnitNumber;

	/* name of loaded driver */
	STRPTR pd_DriverName;

	/* the query function:
	 *
	 *	LONG pd_PQuery(LONG * numofchars);
	 */
	LONG (* __stdargs pd_PQuery)(LONG * numofchars);
};

/* Printer Class */
/*
	Some printer drivers (PrinterPS) do not support
	strip printing. An application has to print a page
	using a single print request or through clever use
	of the PRD_DUMPRPORTTAGS printing callback hook.
*/
/* Color Class */
/*
	The picture must be scanned once for each color component, as the
	printer can only define one color at a time.  ie. If 'PCC_YMC' then
	first pass sends all 'Y' info to printer, second pass sends all 'M'
	info, and third pass sends all C info to printer.  The CalComp
	PlotMaster is an example of this type of printer.
*/
struct PrinterExtendedData {
	STRPTR	ped_PrinterName;    /* printer name, null terminated */
	void    (* __stdargs ped_Init)(struct PrinterData * pd);     /* called after LoadSeg */
	void    (* __stdargs ped_Expunge)(void);                     /* called before UnLoadSeg */
	int     (* __stdargs ped_Open)(struct IORequest *ior);       /* called at OpenDevice */
	void    (* __stdargs ped_Close)(struct IORequest *ior);      /* called at CloseDevice */
	UBYTE   ped_PrinterClass;    /* printer class */
	UBYTE   ped_ColorClass;      /* color class */
	UBYTE   ped_MaxColumns;      /* number of print columns available */
	UBYTE   ped_NumCharSets;     /* number of character sets */
	UWORD   ped_NumRows;         /* number of 'pins' in print head */
	ULONG   ped_MaxXDots;        /* number of dots max in a raster dump */
	ULONG   ped_MaxYDots;        /* number of dots max in a raster dump */
	UWORD   ped_XDotsInch;       /* horizontal dot density */
	UWORD   ped_YDotsInch;       /* vertical dot density */
	STRPTR	**ped_Commands;     /* printer text command table */
	LONG    (* __stdargs ped_DoSpecial)(UWORD * command, UBYTE output_buffer[],
					    BYTE * current_line_position, BYTE * current_line_spacing,
					    BYTE * crlf_flag, STRPTR params);  /* special command handler */
	LONG    (* __stdargs ped_Render)(LONG ct, LONG x, LONG y, LONG status,...);     /* raster render function */
	LONG    ped_TimeoutSecs;     /* good write timeout */
	/* the following only exists if the segment version is >= 33 */
	STRPTR	*ped_8BitChars;     /* conv. strings for the extended font */
	LONG	ped_PrintMode;       /* set if text printed, otherwise 0 */
	/* the following only exists if the segment version is >= 34 */
	/* ptr to conversion function for all chars */
	LONG	(* __stdargs ped_ConvFunc)(STRPTR buf, TEXT c, LONG crlf_flag);
        /**************************************************************
	 *
	 * The following only exists if the segment version is >= 44
	 * AND PPCB_EXTENDED is set in ped_PrinterClass:
	 *
	 *************************************************************/

	/* Attributes and features */
	struct TagItem * ped_TagList;

	/* driver specific preferences:
	 *
	 *	LONG ped_DoPreferences(union printerIO * ior,
	 *	                       LONG command);
	 */
	LONG (* __stdargs ped_DoPreferences)(union printerIO * ior,LONG command);

	/* custom error handling:
	 *
	 *	VOID ped_CallErrHook(union printerIO * ior,
	 *	                     struct Hook * hook);
	 */
	void (* __stdargs ped_CallErrHook)(union printerIO * ior,struct Hook * hook);
};

/****************************************************************************/

/* The following tags are used to define more printer driver features */

/****************************************************************************/

/* V44 features */
/* User interface */
/* Hardware page borders */
/* Driver Preferences */
/****************************************************************************/

struct PrinterSegment {
    ULONG   ps_NextSegment;      /* (actually a BPTR) */
    ULONG   ps_runAlert;         /* MOVEQ #0,D0 : RTS */
    UWORD   ps_Version;          /* segment version */
    UWORD   ps_Revision;         /* segment revision */
    struct  PrinterExtendedData ps_PED;   /* printer extended data */
};
/*****************************************************************************/

/*****************************************************************************/

/*****************************************************************************/

/* Generic attributes */


/* (struct TextAttr *) Pointer to the default TextAttr to use for
 * the text within the object.
 */
/* (LONG) Current top vertical unit */
/* (LONG) Number of visible vertical units */
/* (LONG) Total number of vertical units */
/* (LONG) Number of pixels per vertical unit */
/* (LONG) Current top horizontal unit */
/* (LONG)  Number of visible horizontal units */
/* (LONG) Total number of horizontal units */
/* (LONG) Number of pixels per horizontal unit */
/* (STRPTR) Name of the current element within the object. */
/* (STRPTR) Title of the object. */
/* (struct DTMethod *) Pointer to a NULL terminated array of
 * supported trigger methods.
 */
/* (APTR) Object specific data. */
/* (struct TextFont *) Default font to use for text within the
 * object.
 */
/* (ULONG *) Pointer to a ~0 terminated array of supported
 * methods.
 */
/* (LONG) Printer error message.  Error numbers are defined in
 * <devices/printer.h>
 */
/* PRIVATE (struct Process *) Pointer to the print process. */
/* PRIVATE (struct Process *) Pointer to the layout process. */
/* Used to turn the applications' busy pointer off and on */
/* Used to indicate that new information has been loaded into
 * an object.  This is for models that cache the DTA_TopVert-
 * like tags
 */
/* The base name of the class */
/* Group that the object must belong in */
/* Error level */
/* datatypes.library error number */
/* Argument for datatypes.library error */
/* New for V40. (STRPTR) specifies the name of the
 * realtime.library conductor.	Defaults to "Main".
 */
/* New for V40. (BOOL) Indicate whether a control panel should be
 * embedded within the object (in the animation datatype, for
 * example).  Defaults to TRUE.
 */
/* New for V40. (BOOL) Indicate whether the object should
 * immediately begin playing.  Defaults to FALSE.
 */
/* New for V40. (BOOL) Indicate that the object should repeat
 * playing.  Defaults to FALSE.
 */
/* New for V44. Address of a DTST_MEMORY source type
 * object (APTR).
 */
/* New for V44. Size of a DTST_MEMORY source type
 * object (ULONG).
 */
/* Reserved tag; DO NOT USE (V44) */
/* New for V45.3. (struct IClass *) Use this class when creating
 * new objects using NewDTObjectA. Useful when creating private
 * subclasses inside applications (e.g. when a subclass is needed
 * to superset some methods (see "DTConvert" example)).
 */
/* DTObject attributes */
/* DON'T USE THE FOLLOWING FOUR TAGS.  USE THE CORRESPONDING TAGS IN
 * <intuition/gadgetclass.h> */
/* DON'T USE THE FOLLOWING FOUR TAGS.  USE THE CORRESPONDING TAGS IN
 * <intuition/gadgetclass.h> */
/* Printing attributes */

/* (LONG) Destination X width */
/* (LONG) Destination Y height */
/* (UWORD) Option flags */
/* (struct RastPort *) RastPort to use when printing. (V40) */
/* (STRPTR) Pointer to base name for ARexx port (V40) */
/*****************************************************************************/

/*****************************************************************************/

/* Attached to the Gadget.SpecialInfo field of the gadget.  Don't access directly,
 * use the Get/Set calls instead.
 */
struct DTSpecialInfo
{
    struct SignalSemaphore	 si_Lock;	/* Locked while in DoAsyncLayout() */
    ULONG			 si_Flags;

    LONG			 si_TopVert;	/* Top row (in units) */
    LONG			 si_VisVert;	/* Number of visible rows (in units) */
    LONG			 si_TotVert;	/* Total number of rows (in units) */
    LONG			 si_OTopVert;	/* Previous top (in units) */
    LONG			 si_VertUnit;	/* Number of pixels in vertical unit */

    LONG			 si_TopHoriz;	/* Top column (in units) */
    LONG			 si_VisHoriz;	/* Number of visible columns (in units) */
    LONG			 si_TotHoriz;	/* Total number of columns (in units) */
    LONG			 si_OTopHoriz;	/* Previous top (in units) */
    LONG			 si_HorizUnit;	/* Number of pixels in horizontal unit */
};


/* Object is in layout processing */
/* Object needs to be layed out */
/* Object is being printed */
/* Object is in layout process */
/*****************************************************************************/

struct DTMethod
{
    STRPTR	 dtm_Label;
    STRPTR	 dtm_Command;
    ULONG	 dtm_Method;
};

/*****************************************************************************/

/* Inquire what environment an object requires */
/* Same as GM_LAYOUT except guaranteed to be on a process already */
/* Layout that is occurring on a process */
/* When a RemoveDTObject() is called */
/* Used to ask the object about itself */
struct FrameInfo
{
    ULONG		 fri_PropertyFlags;		/* DisplayInfo (graphics/displayinfo.h) */
    Point		 fri_Resolution;		/* DisplayInfo */

    UBYTE		 fri_RedBits;
    UBYTE		 fri_GreenBits;
    UBYTE		 fri_BlueBits;

    struct
    {
	ULONG Width;
	ULONG Height;
	ULONG Depth;

    } fri_Dimensions;

    struct Screen	*fri_Screen;
    struct ColorMap	*fri_ColorMap;

    ULONG		 fri_Flags;
};

/* DTM_REMOVEDTOBJECT, DTM_CLEARSELECTED, DTM_COPY, DTM_ABORTPRINT */
struct dtGeneral
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtg_GInfo;
};

/* DTM_SELECT */
struct dtSelect
{
    ULONG		 MethodID;
    struct GadgetInfo	*dts_GInfo;
    struct Rectangle	 dts_Select;
};

/* DTM_FRAMEBOX */
struct dtFrameBox
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtf_GInfo;
    struct FrameInfo	*dtf_ContentsInfo;	/* Input */
    struct FrameInfo	*dtf_FrameInfo;		/* Output */
    ULONG		 dtf_SizeFrameInfo;
    ULONG		 dtf_FrameFlags;
};

/* DTM_GOTO */
struct dtGoto
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtg_GInfo;
    STRPTR		 dtg_NodeName;		/* Node to goto */
    struct TagItem	*dtg_AttrList;		/* Additional attributes */
};

/* DTM_TRIGGER */
struct dtTrigger
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtt_GInfo;
    ULONG		 dtt_Function;
    APTR		 dtt_Data;
};

/* New for V40 */
/* New for V45 */
/* New for V47 */
/* New for V45: Custom trigger methods starts here; but must be within
 * STMF_METHOD_MASKs value
 */
/* Printer IO request */
union printerIO
{
    struct IOStdReq ios;
    struct IODRPReq iodrp;
    struct IOPrtCmdReq iopc;
};

/* DTM_PRINT */
struct dtPrint
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtp_GInfo;		/* Gadget information */
    union printerIO	*dtp_PIO;		/* Printer IO request */
    struct TagItem	*dtp_AttrList;		/* Additional attributes */
};

/* DTM_DRAW */
struct dtDraw
{
    ULONG		 MethodID;
    struct RastPort	*dtd_RPort;
    LONG		 dtd_Left;
    LONG		 dtd_Top;
    LONG		 dtd_Width;
    LONG		 dtd_Height;
    LONG		 dtd_TopHoriz;
    LONG		 dtd_TopVert;
    struct TagItem	*dtd_AttrList;		/* Additional attributes */
};

/* DTM_WRITE */
struct dtWrite
{
    ULONG		 MethodID;
    struct GadgetInfo	*dtw_GInfo;		/* Gadget information */
    BPTR		 dtw_FileHandle;	/* File handle to write to */
    ULONG		 dtw_Mode;
    struct TagItem	*dtw_AttrList;		/* Additional attributes */
};

/* Save data as IFF data */
/* Save data as local data format */
/*****************************************************************************/

/*****************************************************************************/

/* Picture attributes */

/* Mode ID of the picture (ULONG) */
/* Bitmap header information (struct BitMapHeader *) */
/* Pointer to a class-allocated bitmap, that will end
 * up being freed by picture.class when DisposeDTObject()
 * is called (struct BitMap *).
 */
/* Picture colour table (struct ColorRegister *) */
/* Color table to use with SetRGB32CM() (ULONG *) */
/* Color table; this table is initialized during the layout
 * process and will contain the colours the picture will use
 * after remapping. If no remapping takes place, these colours
 * will match those in the PDTA_CRegs table (ULONG *).
 */
/* Shared pen table; this table is initialized during the layout
 * process while the picture is being remapped (UBYTE *).
 */
/* Shared pen table; in most places this table will be identical to
 * the PDTA_ColorTable table. Some of the colours in this table might
 * match the original colour palette a little better than the colours
 * picked for the other table. The picture.datatype uses the two tables
 * during remapping, alternating for each pixel (UBYTE *).
 */
/* OBSOLETE; DO NOT USE */
/* Number of colors used by the picture. (UWORD) */
/* Number of colors allocated by the picture (UWORD) */
/* Remap the picture (BOOL); defaults to TRUE */
/* Screen to remap to (struct Screen *) */
/* Free the source bitmap after remapping (BOOL) */
/* Pointer to a Point structure */
/* Pointer to the destination (remapped) bitmap */
/* Pointer to class-allocated bitmap, that will end
 * up being freed by the class after DisposeDTObject()
 * is called (struct BitMap *)
 */
/* Number of colors used for sparse remapping (UWORD) */
/* Pointer to a table of pen numbers indicating
 * which colors should be used when remapping the image.
 * This array must contain as many entries as there
 * are colors specified with PDTA_NumSparse (UBYTE *).
 */
/* Index number of the picture to load (ULONG). (V44) */
/* Get the number of pictures stored in the file (ULONG *). (V44) */
/* Maximum number of colours to use for dithering (ULONG). (V44) */
/* Quality of the dithering algorithm to be used during colour
 * quantization (ULONG). (V44)
 */
/* Pointer to the allocated pen table (UBYTE *). (V44) */
/* Quality for scaling. (V45) */
/*****************************************************************************/

/* When querying the number of pictures stored in a file, the
 * following value denotes "the number of pictures is unknown".
 */
/*****************************************************************************/

/* V43 extensions (attributes) */

/* Set the sub datatype interface mode (LONG); see "Interface modes" below */
/* Set the app datatype interface mode (LONG); see "Interface modes" below */
/* Allocates the resulting bitmap as a friend bitmap (BOOL) */
/* NULL or mask plane for use with BltMaskBitMapRastPort() (PLANEPTR) */
/* not in AmigaOS for m68k */
/* V47 extensions (attributes) */

/* ask for permission to write directly into the backbuffer pixmap (APTR) */
/* pass this request alongside PDTA_SourceMode=PMODE_V43, DTA_NominalHoriz
   and DTA_NominalVert to SetDTAttrs(), the argument shall be a pointer to
   struct pdtBlitPixelArray*
   If successful, pbpa_PixelData will be nonzero and the other parameters
   of that struct are set up for direct writing into pbpa_PixelData.
   In case that pbpa_PixelFormat is unsupported by the sub-datatype, then
   please revert to PDTM_WRITEPIXELARRAY. Please don't mix direct writes
   and PDTM_WRITEPIXELARRAY method invocations.
*/
/* allow a dirty (non zeroed) frame buffer in V43 mode (LONG)
   1=yes, the subclass will write the whole frame buffer
   0=clear buffer by picture.datatype upon allocation (default) */
/* Returns TRUE if the bitmap has a valid alpha channel (BOOL), added in 47.9 */
/*****************************************************************************/

/* Interface modes */
/*****************************************************************************/

/* V43 extensions (methods) */

/* Transfer pixel data to the picture object in the specified format */
/* Transfer pixel data from the picture object in the specified format */
/* PDTM_WRITEPIXELARRAY, PDTM_READPIXELARRAY */
struct pdtBlitPixelArray
{
	ULONG	MethodID;
	APTR	pbpa_PixelData;		/* The pixel data to transfer to/from */
	ULONG	pbpa_PixelFormat;	/* Format of the pixel data (see "Pixel Formats" below) */
	ULONG	pbpa_PixelArrayMod;	/* Number of bytes per row */
	ULONG	pbpa_Left;		/* Left edge of the rectangle to transfer pixels to/from */
	ULONG	pbpa_Top;		/* Top edge of the rectangle to transfer pixels to/from */
	ULONG	pbpa_Width;		/* Width of the rectangle to transfer pixels to/from */
	ULONG	pbpa_Height;		/* Height of the rectangle to transfer pixels to/from */
};

/* Pixel formats */
/*****************************************************************************/

/* V45 extensions (methods) */

/* Scale pixel data to the specified size */
/* PDTM_SCALE */
struct pdtScale
{
	ULONG MethodID;
	ULONG ps_NewWidth;	/* The new width the pixel data should have */
	ULONG ps_NewHeight;	/* The new height the pixel data should have */
	ULONG ps_Flags;		/* should be 0 for now */
};

/*****************************************************************************/

/* V47 extensions (methods) */

/* Obtain direct access to the internal pixel data */
/* PDTM_OBTAINPIXELARRAY */
struct pdtObtainPixelArray
{
    ULONG MethodID;
    struct pdtBlitPixelArray *popa_PixelArray; /* Get pixel data buffer details */
    ULONG popa_Flags;
};

/* If array is 8-bit, request it be set for PBPAFMT_GREY8 writes instead of LUT8 */
/*****************************************************************************/

/* Masking techniques */
/* Compression techniques */
/* Bitmap header (BMHD) structure */
struct BitMapHeader
{
	UWORD	bmh_Width;		/* Width in pixels */
	UWORD	bmh_Height;		/* Height in pixels */
	WORD	bmh_Left;		/* Left position */
	WORD	bmh_Top;		/* Top position */
	UBYTE	bmh_Depth;		/* Number of planes */
	UBYTE	bmh_Masking;		/* Masking type */
	UBYTE	bmh_Compression;	/* Compression type */
	UBYTE	bmh_Pad;
	UWORD	bmh_Transparent;	/* Transparent color */
	UBYTE	bmh_XAspect;
	UBYTE	bmh_YAspect;
	WORD	bmh_PageWidth;
	WORD	bmh_PageHeight;
};

/*****************************************************************************/

/* Color register structure */
struct ColorRegister
{
	UBYTE red, green, blue;
};

/*****************************************************************************/

/* IFF types that may be in pictures */
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/* Error reporting (LONG *) */
/* Points to the tag item that caused the error (struct TagItem **). */
/****************************************************************************/

/* Global options for IconControlA() */

/* Screen to use for remapping Workbench icons to (struct Screen *) */
/* Icon color remapping precision; defaults to PRECISION_ICON (LONG) */
/* Icon frame size dimensions (struct Rectangle *) */
/* Render image without frame (BOOL) */
/* Enable NewIcons support (BOOL) */
/* Enable color icon support (BOOL) */
/* Set/Get the hook to be called when identifying a file (struct Hook *) */
/* Set/get the maximum length of a file/drawer name supported
 * by icon.library (LONG).
 */
/****************************************************************************/

/* Per icon local options for IconControlA() */

/* Get the icon rendering masks (PLANEPTR) */
/* Transparent image color; set to -1 if opaque */
/* Image color palette (struct ColorRegister *) */
/* Size of image color palette (LONG) */
/* Image data; one by per pixel (UBYTE *) */
/* Render image without frame (BOOL) */
/* Enable NewIcons support (BOOL) */
/* Icon aspect ratio (UBYTE *) */
/* Icon dimensions; valid only for palette mapped icon images (LONG) */
/* Check whether the icon is palette mapped (LONG *). */
/* Get the screen the icon is attached to (struct Screen **). */
/* Check whether the icon has a real select image (LONG *). */
/* Check whether the icon is of the NewIcon type (LONG *). */
/* Check whether this icon was allocated by icon.library
 * or if consists solely of a statically allocated
 * struct DiskObject. (LONG *).
 */
/****************************************************************************/

/* Icon aspect ratio is not known. */
/* Pack the aspect ratio into a single byte. */
/* Unpack the aspect ratio stored in a single byte. */
/****************************************************************************/

/* Tags for use with GetIconTagList() */

/* Default icon type to retrieve (LONG) */
/* Retrieve default icon for the given name (STRPTR) */
/* Return a default icon if the requested icon
 * file cannot be found (BOOL).
 */
/* If possible, retrieve a palette mapped icon (BOOL). */
/* Set if the icon returned is a default icon (BOOL *). */
/* Remap the icon to the default screen, if possible (BOOL). */
/* Generate icon image masks (BOOL). */
/* Label text to be assigned to the icon (STRPTR). */
/* Screen to remap the icon to (struct Screen *). */
/* Request the type of the actual file whose icon is being loaded
 * by GetIconTagList(), as determined by the global identification
 * hook. If this tag is passed, the file is identified even if it
 * has a real icon. The value of this tag must be a pointer to a
 * text buffer of at least 256 bytes, or NULL (which is ignored).
 * This tag only works with actual files (tools and projects), not
 * volumes, directories or trashcans.
 * If the file cannot get identified, the supplied buffer will
 * contain an empty string (a single NUL byte) on the function's
 * return. Defaults to NULL (STRPTR). (V47)
 */
/* When using ICONGETA_IdentifyBuffer, you can ask not to actually
 * load an icon, but just identify the file with the specified name
 * (which is quicker if all you need is the file type, and doesn't
 * require you to free an icon afterwards). If you pass this tag
 * with a TRUE value, GetIconTagList() will return NULL with a zero
 * error code in ICONA_ErrorCode. Note: this tag will be ignored if
 * a valid buffer is not also passed with ICONGETA_IdentifyBuffer.
 * Defaults to FALSE (BOOL). (V47)
 */
/****************************************************************************/

/* Tags for use with PutIconTagList() */

/* Notify Workbench of the icon being written (BOOL) */
/* Store icon as the default for this type (LONG) */
/* Store icon as a default for the given name (STRPTR) */
/* When storing a palette mapped icon, don't save the
 * the original planar icon image with the file. Replace
 * it with a tiny replacement image.
 */
/* Don't write the chunky icon image data to disk. */
/* Don't write the NewIcons tool types to disk. */
/* If this tag is enabled, the writer will examine the
 * icon image data to find out whether it can compress
 * it more efficiently. This may take extra time and
 * is not generally recommended. Not yet implemented.
 */
/* Don't write the entire icon file back to disk,
 * only change the do->do_CurrentX/do->do_CurrentY
 * members.
 */
/* Before writing a palette mapped icon back to disk,
 * icon.library will make sure that the original
 * planar image data is stored in the file. If you
 * don't want that to happen, set this option to
 * FALSE. This will allow you to change the planar icon
 * image data written back to disk.
 */
/****************************************************************************/

/* For use with the file identification hook. */

struct IconIdentifyMsg
{
	/* Libraries that are already opened for your use. */
	struct Library *	iim_SysBase;
	struct Library *	iim_DOSBase;
	struct Library *	iim_UtilityBase;
	struct Library *	iim_IconBase;

	/* File context information. */
	BPTR			iim_FileLock;	/* Lock on the object to return an icon for. */
	BPTR			iim_ParentLock;	/* Lock on the object's parent directory, if available. */
	struct FileInfoBlock *	iim_FIB;	/* Already initialized for you. */
	BPTR			iim_FileHandle;	/* If non-NULL, pointer to the file to examine,
						 * positioned right at the first byte, ready
						 * for you to use.
						 */
	struct TagItem *	iim_Tags;	/* Tags passed to GetIconTagList(). */
};

/****************************************************************************/

/* Tags for use with DupDiskObjectA() */

/* Duplicate do_DrawerData */
/* Duplicate the Image structures. */
/* Duplicate the image data (Image->ImageData) itself. */
/* Duplicate the default tool. */
/* Duplicate the tool types list. */
/* Duplicate the tool window. */
/* If the icon to be duplicated is in fact a palette mapped
 * icon which has never been set up to be displayed on the
 * screen, turn the duplicate into that palette mapped icon.
 */
/****************************************************************************/

/* Tags for use with DrawIconStateA() and GetIconRectangleA(). */

/* Drawing information to use (struct DrawInfo *). */
/* Draw the icon without the surrounding frame (BOOL). */
/* Erase the background before drawing a frameless icon (BOOL). */
/* Draw the icon without the surrounding border and frame (BOOL). */
/* The icon to be drawn refers to a linked object (BOOL). */
/* Draw the icon label with shadow (BOOL). V47. */
/* Draw the icon label with outline (BOOL). V47. */
/****************************************************************************/

/* Reserved tags; don't use! */
/****************************************************************************/

/****************************************************************************/

/*
**	$VER: exec.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: execbase.h 47.2 (24.8.2019)
**
**	Definition of the exec.library base structure.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: interrupts.h 47.1 (28.6.2019)
**
**	Callback structures used by hardware & software interrupts
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct Interrupt {
    struct  Node is_Node;
    APTR    is_Data;		    /* server data segment  */
    void    (*is_Code)();	    /* server code entry    */
};


struct IntVector {		/* For EXEC use ONLY! */
    APTR    iv_Data;
    void    (*iv_Code)();
    struct  Node *iv_Node;
};


struct SoftIntList {		/* For EXEC use ONLY! */
    struct List sh_List;
    UWORD  sh_Pad;
};

/* this is a fake INT definition, used only for AddIntServer and the like */
/* Definition of the Exec library base structure (pointed to by location 4).
** Most fields are not to be viewed or modified by user programs.  Use
** extreme caution.
*/
struct ExecBase {
	struct Library LibNode; /* Standard library node */

/******** Static System Variables ********/

	UWORD	SoftVer;	/* kickstart release number (obs.) */
	WORD	LowMemChkSum;	/* checksum of 68000 trap vectors */
	ULONG	ChkBase;	/* system base pointer complement */
	APTR	ColdCapture;	/* coldstart soft capture vector */
	APTR	CoolCapture;	/* coolstart soft capture vector */
	APTR	WarmCapture;	/* warmstart soft capture vector */
	APTR	SysStkUpper;	/* system stack base   (upper bound) */
	APTR	SysStkLower;	/* top of system stack (lower bound) */
	ULONG	MaxLocMem;	/* top of chip memory */
	APTR	DebugEntry;	/* global debugger entry point */
	APTR	DebugData;	/* global debugger data segment */
	APTR	AlertData;	/* alert data segment */
	APTR	MaxExtMem;	/* top of extended mem, or null if none */

	UWORD	ChkSum;	/* for all of the above (minus 2) */

/****** Interrupt Related ***************************************/

	struct	IntVector IntVects[16];

/****** Dynamic System Variables *************************************/

	struct	Task *ThisTask; /* pointer to current task (readable) */

	ULONG	IdleCount;	/* idle counter */
	ULONG	DispCount;	/* dispatch counter */
	UWORD	Quantum;	/* time slice quantum */
	UWORD	Elapsed;	/* current quantum ticks */
	UWORD	SysFlags;	/* misc internal system flags */
	BYTE	IDNestCnt;	/* interrupt disable nesting count */
	BYTE	TDNestCnt;	/* task disable nesting count */

	UWORD	AttnFlags;	/* special attention flags (readable) */

	UWORD	AttnResched;	/* rescheduling attention */
	APTR	ResModules;	/* resident module array pointer */
	APTR	TaskTrapCode;
	APTR	TaskExceptCode;
	APTR	TaskExitCode;
	ULONG	TaskSigAlloc;
	UWORD	TaskTrapAlloc;


/****** System Lists (private!) ********************************/

	struct	List MemList;
	struct	List ResourceList;
	struct	List DeviceList;
	struct	List IntrList;
	struct	List LibList;
	struct	List PortList;
	struct	List TaskReady;
	struct	List TaskWait;

	struct	SoftIntList SoftInts[5];

/****** Other Globals *******************************************/

	LONG	LastAlert[4];

	/* these next two variables are provided to allow
	** system developers to have a rough idea of the
	** period of two externally controlled signals --
	** the time between vertical blank interrupts and the
	** external line rate (which is counted by CIA A's
	** "time of day" clock).  In general these values
	** will be 50 or 60, and may or may not track each
	** other.  These values replace the obsolete AFB_PAL
	** and AFB_50HZ flags.
	*/
	UBYTE	VBlankFrequency;	/* (readable) */
	UBYTE	PowerSupplyFrequency;	/* (readable) */

	struct	List SemaphoreList;

	/* these next two are to be able to kickstart into user ram.
	** KickMemPtr holds a singly linked list of MemLists which
	** will be removed from the memory list via AllocAbs.  If
	** all the AllocAbs's succeeded, then the KickTagPtr will
	** be added to the rom tag list.
	*/
	APTR	KickMemPtr;	/* ptr to queue of mem lists */
	APTR	KickTagPtr;	/* ptr to rom tag queue */
	APTR	KickCheckSum;	/* checksum for mem and tags */

/****** V36 Exec additions start here **************************************/

	UWORD	ex_Pad0;		/* Private internal use */
	ULONG	ex_LaunchPoint;		/* Private to Launch/Switch */
	APTR	ex_RamLibPrivate;
	/* The next ULONG contains the system "E" clock frequency,
	** expressed in Hertz.	The E clock is used as a timebase for
	** the Amiga's 8520 I/O chips. (E is connected to "02").
	** Typical values are 715909 for NTSC, or 709379 for PAL.
	*/
	ULONG	ex_EClockFrequency;	/* (readable) */
	ULONG	ex_CacheControl;	/* Private to CacheControl calls */
	ULONG	ex_TaskID;		/* Next available task ID */

	ULONG	ex_Reserved1[5];

	APTR	ex_MMULock;		/* private */

	ULONG	ex_Reserved2[3];

/****** V39 Exec additions start here **************************************/

	/* The following list and data element are used
	 * for V39 exec's low memory handler...
	 */
	struct	MinList	ex_MemHandlers;	/* The handler list */
	APTR	ex_MemHandler;		/* Private! handler pointer */
};


/****** Bit defines for AttnFlags (see above) ******************************/

/*  Processors and Co-processors: */
/*
 * The AFB_FPU40 bit is set when a working 68040 FPU
 * is in the system.  If this bit is set and both the
 * AFB_68881 and AFB_68882 bits are not set, then the 68040
 * math emulation code has not been loaded and only 68040
 * FPU instructions are available.  This bit is valid *ONLY*
 * if the AFB_68040 bit is set.
 */

/* #define AFB_RESERVED8   8 */
/* #define AFB_RESERVED9   9 */


/****** Selected flag definitions for Cache manipulation calls **********/

					  /* External caches should track the
					   * state of the internal caches
					   * such that they do not cache anything
					   * that the internal cache turned off for. */
extern struct ExecBase * SysBase;

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: exec_pragmas.h 47.5 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: exec_protos.h 47.5 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: memory.h 47.1 (28.6.2019)
**
**	Definitions and structures used by the memory allocation system
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****** MemChunk ****************************************************/

struct	MemChunk {
    struct  MemChunk *mc_Next;	/* pointer to next chunk */
    ULONG   mc_Bytes;		/* chunk byte size	*/
};


/****** MemHeader ***************************************************/

struct	MemHeader {
    struct  Node mh_Node;
    UWORD   mh_Attributes;	/* characteristics of this region */
    struct  MemChunk *mh_First; /* first free region		*/
    APTR    mh_Lower;		/* lower memory bound		*/
    APTR    mh_Upper;		/* upper memory bound+1	*/
    ULONG   mh_Free;		/* total number of free bytes	*/
};


/****** MemEntry ****************************************************/

struct	MemEntry {
union {
    ULONG   meu_Reqs;		/* the AllocMem requirements */
    APTR    meu_Addr;		/* the address of this memory region */
    } me_Un;
    ULONG   me_Length;		/* the length of this memory region */
};

/****** MemList *****************************************************/

/* Note: sizeof(struct MemList) includes the size of the first MemEntry! */
struct	MemList {
    struct  Node ml_Node;
    UWORD   ml_NumEntries;	/* number of entries in this struct */
    struct  MemEntry ml_ME[1];	/* the first entry	*/
};

/*----- Memory Requirement Types ---------------------------*/
/*----- See the AllocMem() documentation for details--------*/

/*----- Current alignment rules for memory blocks (may increase) -----*/
/****** MemHandlerData **********************************************/
/* Note:  This structure is *READ ONLY* and only EXEC can create it!*/
struct MemHandlerData
{
	ULONG	memh_RequestSize;	/* Requested allocation size */
	ULONG	memh_RequestFlags;	/* Requested allocation flags */
	ULONG	memh_Flags;		/* Flags (see below) */
};

/****** Low Memory handler return values ***************************/
/*
**	$VER: devices.h 47.1 (28.6.2019)
**
**	Include file for use by Exec device drivers
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/****** Device ******************************************************/

struct Device { 
    struct  Library dd_Library;
};


/****** Unit ********************************************************/

struct Unit {
    struct  MsgPort unit_MsgPort;	/* queue for unprocessed messages */
					/* instance of msgport is recommended */
    UBYTE   unit_flags;
    UBYTE   unit_pad;
    UWORD   unit_OpenCnt;		/* number of active opens */
};


/*
**	$VER: resident.h 47.1 (28.6.2019)
**
**	Resident/ROMTag stuff.	Used to identify and initialize code modules.
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct Resident {
    UWORD rt_MatchWord;	/* word to match on (ILLEGAL)	*/
    struct Resident *rt_MatchTag; /* pointer to the above	*/
    APTR  rt_EndSkip;		/* address to continue scan	*/
    UBYTE rt_Flags;		/* various tag flags		*/
    UBYTE rt_Version;		/* release version number	*/
    UBYTE rt_Type;		/* type of module (NT_XXXXXX)	*/
    BYTE  rt_Pri;		/* initialization priority */
    char  *rt_Name;		/* pointer to node name	*/
    char  *rt_IdString;	/* pointer to identification string */
    APTR  rt_Init;		/* pointer to init code	*/
};

/* Compatibility: (obsolete) */
/* #define RTM_WHEN	   3 */
/*------ misc ---------------------------------------------------------*/
ULONG Supervisor( ULONG (*userFunction)() );
/*------ special patchable hooks to internal exec activity ------------*/
/*------ module creation ----------------------------------------------*/
void InitCode( ULONG startClass, ULONG version );
void InitStruct( CONST_APTR initTable, APTR memory, ULONG size );
struct Library *MakeLibrary( CONST_APTR funcInit, CONST_APTR structInit, ULONG (*libInit)(), ULONG dataSize, ULONG segList );
void MakeFunctions( APTR target, CONST_APTR functionArray, ULONG funcDispBase );
struct Resident *FindResident( CONST_STRPTR name );
APTR InitResident( const struct Resident *resident, ULONG segList );
/*------ diagnostics --------------------------------------------------*/
void Alert( ULONG alertNum );
void Debug( ULONG flags );
/*------ interrupts ---------------------------------------------------*/
void Disable( void );
void Enable( void );
void Forbid( void );
void Permit( void );
ULONG SetSR( ULONG newSR, ULONG mask );
APTR SuperState( void );
void UserState( APTR sysStack );
struct Interrupt *SetIntVector( LONG intNumber, struct Interrupt *interrupt );
void AddIntServer( LONG intNumber, struct Interrupt *interrupt );
void RemIntServer( LONG intNumber, struct Interrupt *interrupt );
void Cause( const struct Interrupt *interrupt );
/*------ memory allocation --------------------------------------------*/
APTR Allocate( struct MemHeader *freeList, ULONG byteSize );
void Deallocate( struct MemHeader *freeList, APTR memoryBlock, ULONG byteSize );
APTR AllocMem( ULONG byteSize, ULONG requirements );
APTR AllocAbs( ULONG byteSize, APTR location );
void FreeMem( APTR memoryBlock, ULONG byteSize );
ULONG AvailMem( ULONG requirements );
struct MemList *AllocEntry( const struct MemList *entry );
void FreeEntry( struct MemList *entry );
/*------ lists --------------------------------------------------------*/
void Insert( struct List *list, struct Node *node, struct Node *pred );
void InsertMinNode( struct MinList *minlist, struct MinNode *minnode, struct MinNode *minpred );
void AddHead( struct List *list, struct Node *node );
void AddHeadMinList( struct MinList *minlist, struct MinNode *minnode );
void AddTail( struct List *list, struct Node *node );
void AddTailMinList( struct MinList *minlist, struct MinNode *minnode );
void Remove( struct Node *node );
void RemoveMinNode( struct MinNode *minnode );
struct Node *RemHead( struct List *list );
struct MinNode *RemHeadMinList( struct MinList *minlist );
struct Node *RemTail( struct List *list );
struct MinNode *RemTailMinList( struct MinList *minlist );
void Enqueue( struct List *list, struct Node *node );
struct Node *FindName( struct List *list, CONST_STRPTR name );
/*------ tasks --------------------------------------------------------*/
APTR AddTask( struct Task *task, APTR initPC, APTR finalPC );
void RemTask( struct Task *task );
struct Task *FindTask( CONST_STRPTR name );
BYTE SetTaskPri( struct Task *task, LONG priority );
ULONG SetSignal( ULONG newSignals, ULONG signalSet );
ULONG SetExcept( ULONG newSignals, ULONG signalSet );
ULONG Wait( ULONG signalSet );
void Signal( struct Task *task, ULONG signalSet );
BYTE AllocSignal( LONG signalNum );
void FreeSignal( LONG signalNum );
LONG AllocTrap( LONG trapNum );
void FreeTrap( LONG trapNum );
/*------ messages -----------------------------------------------------*/
void AddPort( struct MsgPort *port );
void RemPort( struct MsgPort *port );
void PutMsg( struct MsgPort *port, struct Message *message );
struct Message *GetMsg( struct MsgPort *port );
void ReplyMsg( struct Message *message );
struct Message *WaitPort( struct MsgPort *port );
struct MsgPort *FindPort( CONST_STRPTR name );
/*------ libraries ----------------------------------------------------*/
void AddLibrary( struct Library *library );
void RemLibrary( struct Library *library );
struct Library *OldOpenLibrary( CONST_STRPTR libName );
void CloseLibrary( struct Library *library );
APTR SetFunction( struct Library *library, LONG funcOffset, ULONG (*newFunction)() );
void SumLibrary( struct Library *library );
/*------ devices ------------------------------------------------------*/
void AddDevice( struct Device *device );
void RemDevice( struct Device *device );
BYTE OpenDevice( CONST_STRPTR devName, ULONG unit, struct IORequest *ioRequest, ULONG flags );
void CloseDevice( struct IORequest *ioRequest );
BYTE DoIO( struct IORequest *ioRequest );
void SendIO( struct IORequest *ioRequest );
struct IORequest *CheckIO( const struct IORequest *ioRequest );
BYTE WaitIO( struct IORequest *ioRequest );
void AbortIO( struct IORequest *ioRequest );
/*------ resources ----------------------------------------------------*/
void AddResource( APTR resource );
void RemResource( APTR resource );
APTR OpenResource( CONST_STRPTR resName );
/*------ private diagnostic support -----------------------------------*/
/*------ misc ---------------------------------------------------------*/
APTR RawDoFmt( CONST_STRPTR formatString, APTR dataStream, void (*putChProc)(), APTR putChData );
ULONG GetCC( void );
ULONG TypeOfMem( CONST_APTR address );
ULONG Procure( struct SignalSemaphore *sigSem, struct SemaphoreMessage *bidMsg );
void Vacate( struct SignalSemaphore *sigSem, struct SemaphoreMessage *bidMsg );
struct Library *OpenLibrary( CONST_STRPTR libName, ULONG version );
/*--- functions in V33 or higher (Release 1.2) ---*/
/*------ signal semaphores (note funny registers)----------------------*/
void InitSemaphore( struct SignalSemaphore *sigSem );
void ObtainSemaphore( struct SignalSemaphore *sigSem );
void ReleaseSemaphore( struct SignalSemaphore *sigSem );
ULONG AttemptSemaphore( struct SignalSemaphore *sigSem );
void ObtainSemaphoreList( struct List *sigSem );
void ReleaseSemaphoreList( struct List *sigSem );
struct SignalSemaphore *FindSemaphore( CONST_STRPTR name );
void AddSemaphore( struct SignalSemaphore *sigSem );
void RemSemaphore( struct SignalSemaphore *sigSem );
/*------ kickmem support ----------------------------------------------*/
ULONG SumKickData( void );
/*------ more memory support ------------------------------------------*/
void AddMemList( ULONG size, ULONG attributes, LONG pri, APTR base, STRPTR name );
void CopyMem( CONST_APTR source, APTR dest, ULONG size );
void CopyMemQuick( CONST_APTR source, APTR dest, ULONG size );
/*------ cache --------------------------------------------------------*/
/*--- functions in V36 or higher (Release 2.0) ---*/
void CacheClearU( void );
void CacheClearE( APTR address, ULONG length, ULONG caches );
ULONG CacheControl( ULONG cacheBits, ULONG cacheMask );
/*------ misc ---------------------------------------------------------*/
APTR CreateIORequest( struct MsgPort *port, ULONG size );
void DeleteIORequest( APTR iorequest );
struct MsgPort *CreateMsgPort( void );
void DeleteMsgPort( struct MsgPort *port );
void ObtainSemaphoreShared( struct SignalSemaphore *sigSem );
/*------ even more memory support -------------------------------------*/
APTR AllocVec( ULONG byteSize, ULONG requirements );
void FreeVec( APTR memoryBlock );
/*------ V39 Pool LVOs...*/
APTR CreatePool( ULONG requirements, ULONG puddleSize, ULONG threshSize );
void DeletePool( APTR poolHeader );
APTR AllocPooled( APTR poolHeader, ULONG memSize );
void FreePooled( APTR poolHeader, APTR memory, ULONG memSize );
/*------ misc ---------------------------------------------------------*/
ULONG AttemptSemaphoreShared( struct SignalSemaphore *sigSem );
void ColdReboot( void );
void StackSwap( struct StackSwapStruct *newStack );
/*------ task trees ---------------------------------------------------*/
/*------ future expansion ---------------------------------------------*/
APTR CachePreDMA( CONST_APTR address, ULONG *length, ULONG flags );
void CachePostDMA( CONST_APTR address, ULONG *length, ULONG flags );
/*------ New, for V39*/
/*--- functions in V39 or higher (Release 3.0) ---*/
/*------ Low memory handler functions*/
void AddMemHandler( struct Interrupt *memhand );
void RemMemHandler( struct Interrupt *memhand );
/*------ Function to attempt to obtain a Quick Interrupt Vector...*/
ULONG ObtainQuickVector( APTR interruptCode );
/*--- functions in V45 or higher (Release 3.9) ---*/
/*------ Finally the list functions are complete*/
void NewMinList( struct MinList *minlist );

/* "exec.library" */
/*------ misc ---------------------------------------------------------*/
#pragma syscall Supervisor 1e D01
/*------ special patchable hooks to internal exec activity ------------*/
/*------ module creation ----------------------------------------------*/
#pragma syscall InitCode 48 1002
#pragma syscall InitStruct 4e 0A903
#pragma syscall MakeLibrary 54 10A9805
#pragma syscall MakeFunctions 5a A9803
#pragma syscall FindResident 60 901
#pragma syscall InitResident 66 1902
/*------ diagnostics --------------------------------------------------*/
#pragma syscall Alert 6c 701
#pragma syscall Debug 72 001
/*------ interrupts ---------------------------------------------------*/
#pragma syscall Disable 78 00
#pragma syscall Enable 7e 00
#pragma syscall Forbid 84 00
#pragma syscall Permit 8a 00
#pragma syscall SetSR 90 1002
#pragma syscall SuperState 96 00
#pragma syscall UserState 9c 001
#pragma syscall SetIntVector a2 9002
#pragma syscall AddIntServer a8 9002
#pragma syscall RemIntServer ae 9002
#pragma syscall Cause b4 901
/*------ memory allocation --------------------------------------------*/
#pragma syscall Allocate ba 0802
#pragma syscall Deallocate c0 09803
#pragma syscall AllocMem c6 1002
#pragma syscall AllocAbs cc 9002
#pragma syscall FreeMem d2 0902
#pragma syscall AvailMem d8 101
#pragma syscall AllocEntry de 801
#pragma syscall FreeEntry e4 801
/*------ lists --------------------------------------------------------*/
#pragma syscall Insert ea A9803
#pragma syscall InsertMinNode ea A9803
#pragma syscall AddHead f0 9802
#pragma syscall AddHeadMinList f0 9802
#pragma syscall AddTail f6 9802
#pragma syscall AddTailMinList f6 9802
#pragma syscall Remove fc 901
#pragma syscall RemoveMinNode fc 901
#pragma syscall RemHead 102 801
#pragma syscall RemHeadMinList 102 801
#pragma syscall RemTail 108 801
#pragma syscall RemTailMinList 108 801
#pragma syscall Enqueue 10e 9802
#pragma syscall FindName 114 9802
/*------ tasks --------------------------------------------------------*/
#pragma syscall AddTask 11a BA903
#pragma syscall RemTask 120 901
#pragma syscall FindTask 126 901
#pragma syscall SetTaskPri 12c 0902
#pragma syscall SetSignal 132 1002
#pragma syscall SetExcept 138 1002
#pragma syscall Wait 13e 001
#pragma syscall Signal 144 0902
#pragma syscall AllocSignal 14a 001
#pragma syscall FreeSignal 150 001
#pragma syscall AllocTrap 156 001
#pragma syscall FreeTrap 15c 001
/*------ messages -----------------------------------------------------*/
#pragma syscall AddPort 162 901
#pragma syscall RemPort 168 901
#pragma syscall PutMsg 16e 9802
#pragma syscall GetMsg 174 801
#pragma syscall ReplyMsg 17a 901
#pragma syscall WaitPort 180 801
#pragma syscall FindPort 186 901
/*------ libraries ----------------------------------------------------*/
#pragma syscall AddLibrary 18c 901
#pragma syscall RemLibrary 192 901
#pragma syscall OldOpenLibrary 198 901
#pragma syscall CloseLibrary 19e 901
#pragma syscall SetFunction 1a4 08903
#pragma syscall SumLibrary 1aa 901
/*------ devices ------------------------------------------------------*/
#pragma syscall AddDevice 1b0 901
#pragma syscall RemDevice 1b6 901
#pragma syscall OpenDevice 1bc 190804
#pragma syscall CloseDevice 1c2 901
#pragma syscall DoIO 1c8 901
#pragma syscall SendIO 1ce 901
#pragma syscall CheckIO 1d4 901
#pragma syscall WaitIO 1da 901
#pragma syscall AbortIO 1e0 901
/*------ resources ----------------------------------------------------*/
#pragma syscall AddResource 1e6 901
#pragma syscall RemResource 1ec 901
#pragma syscall OpenResource 1f2 901
/*------ private diagnostic support -----------------------------------*/
/*------ misc ---------------------------------------------------------*/
#pragma syscall RawDoFmt 20a BA9804
#pragma syscall GetCC 210 00
#pragma syscall TypeOfMem 216 901
#pragma syscall Procure 21c 9802
#pragma syscall Vacate 222 9802
#pragma syscall OpenLibrary 228 0902
/*--- functions in V33 or higher (Release 1.2) ---*/
/*------ signal semaphores (note funny registers)----------------------*/
#pragma syscall InitSemaphore 22e 801
#pragma syscall ObtainSemaphore 234 801
#pragma syscall ReleaseSemaphore 23a 801
#pragma syscall AttemptSemaphore 240 801
#pragma syscall ObtainSemaphoreList 246 801
#pragma syscall ReleaseSemaphoreList 24c 801
#pragma syscall FindSemaphore 252 901
#pragma syscall AddSemaphore 258 901
#pragma syscall RemSemaphore 25e 901
/*------ kickmem support ----------------------------------------------*/
#pragma syscall SumKickData 264 00
/*------ more memory support ------------------------------------------*/
#pragma syscall AddMemList 26a 9821005
#pragma syscall CopyMem 270 09803
#pragma syscall CopyMemQuick 276 09803
/*------ cache --------------------------------------------------------*/
/*--- functions in V36 or higher (Release 2.0) ---*/
#pragma syscall CacheClearU 27c 00
#pragma syscall CacheClearE 282 10803
#pragma syscall CacheControl 288 1002
/*------ misc ---------------------------------------------------------*/
#pragma syscall CreateIORequest 28e 0802
#pragma syscall DeleteIORequest 294 801
#pragma syscall CreateMsgPort 29a 00
#pragma syscall DeleteMsgPort 2a0 801
#pragma syscall ObtainSemaphoreShared 2a6 801
/*------ even more memory support -------------------------------------*/
#pragma syscall AllocVec 2ac 1002
#pragma syscall FreeVec 2b2 901
/*------ V39 Pool LVOs...*/
#pragma syscall CreatePool 2b8 21003
#pragma syscall DeletePool 2be 801
#pragma syscall AllocPooled 2c4 0802
#pragma syscall FreePooled 2ca 09803
/*------ misc ---------------------------------------------------------*/
#pragma syscall AttemptSemaphoreShared 2d0 801
#pragma syscall ColdReboot 2d6 00
#pragma syscall StackSwap 2dc 801
/*------ task trees ---------------------------------------------------*/
/*------ future expansion ---------------------------------------------*/
#pragma syscall CachePreDMA 2fa 09803
#pragma syscall CachePostDMA 300 09803
/*------ New, for V39*/
/*--- functions in V39 or higher (Release 3.0) ---*/
/*------ Low memory handler functions*/
#pragma syscall AddMemHandler 306 901
#pragma syscall RemMemHandler 30c 901
/*------ Function to attempt to obtain a Quick Interrupt Vector...*/
#pragma syscall ObtainQuickVector 312 801
/* For ROM Space, a tagged OpenLibrary */
/* More reserved LVOs */
/*--- functions in V45 or higher (Release 3.9) ---*/
/*------ Finally the list functions are complete*/
#pragma syscall NewMinList 33c 801
/*------ AVL tree support was introduced in V45 and was removed again in V46*/
/*--- (10 function slots reserved here) ---*/

/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: dos.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

extern struct DosLibrary * DOSBase;
/****************************************************************************/

/*
**	$VER: dos_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: dos_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**
**	$VER: record.h 47.1 (29.7.2019)
**
**	include file for record locking
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/* Modes for LockRecord/LockRecords() */
/* struct to be passed to LockRecords()/UnLockRecords() */

struct RecordLock {
	BPTR	rec_FH;		/* filehandle */
	ULONG	rec_Offset;	/* offset in file */
	ULONG	rec_Length;	/* length of file to be locked */
	ULONG	rec_Mode;	/* Type of lock */
};

/*
**	$VER: rdargs.h 47.2 (16.11.2021)
**
**	ReadArgs() structure definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

/**********************************************************************
 *
 * The CSource data structure defines the input source for "ReadItem()"
 * as well as the ReadArgs call.  It is a publicly defined structure
 * which may be used by applications which use code that follows the
 * conventions defined for access.
 *
 * When passed to the dos.library functions, the value passed as
 * struct *CSource is defined as follows:
 *	if ( CSource == 0)	Use buffered IO "ReadChar()" as data source
 *	else			Use CSource for input character stream
 *
 * The following two pseudo-code routines define how the CSource structure
 * is used:
 *
 * long CS_ReadChar( struct CSource *CSource )
 * {
 *	if ( CSource == 0 )	return ReadChar();
 *	if ( CSource->CurChr >= CSource->Length )	return ENDSTREAMCHAR;
 *	return CSource->Buffer[ CSource->CurChr++ ];
 * }
 *
 * BOOL CS_UnReadChar( struct CSource *CSource )
 * {
 *	if ( CSource == 0 )	return UnReadChar();
 *	if ( CSource->CurChr <= 0 )	return FALSE;
 *	CSource->CurChr--;
 *	return TRUE;
 * }
 *
 * To initialize a struct CSource, you set CSource->CS_Buffer to
 * a string which is used as the data source, and set CS_Length to
 * the number of characters in the string.  Normally CS_CurChr should
 * be initialized to ZERO, or left as it was from prior use as
 * a CSource.
 *
 **********************************************************************/

struct CSource {
	STRPTR	CS_Buffer;
	LONG	CS_Length;
	LONG	CS_CurChr;
};

/**********************************************************************
 *
 * The RDArgs data structure is the input parameter passed to the DOS
 * ReadArgs() function call.
 *
 * The RDA_Source structure is a CSource as defined above;
 * if RDA_Source.CS_Buffer is non-null, RDA_Source is used as the input
 * character stream to parse, else the input comes from the buffered STDIN
 * calls ReadChar/UnReadChar.
 *
 * RDA_DAList is a private address which is used internally to track
 * allocations which are freed by FreeArgs().  This MUST be initialized
 * to NULL prior to the first call to ReadArgs().
 *
 * The RDA_Buffer and RDA_BufSiz fields allow the application to supply
 * a fixed-size buffer in which to store the parsed data.  This allows
 * the application to pre-allocate a buffer rather than requiring buffer
 * space to be allocated.  If either RDA_Buffer or RDA_BufSiz is NULL,
 * the application has not supplied a buffer.
 *
 * RDA_ExtHelp is a text string which will be displayed instead of the
 * template string, if the user is prompted for input.
 *
 * RDA_Flags bits control how ReadArgs() works.  The flag bits are
 * defined below.  Defaults are initialized to ZERO.
 *
 * When parsing the input string needs to end with a newline
 *
 **********************************************************************/

struct RDArgs {
	struct	CSource RDA_Source;	/* Select input source */
	LONG	RDA_DAList;		/* PRIVATE. */
	STRPTR	RDA_Buffer;		/* Optional string parsing space. */
	LONG	RDA_BufSiz;		/* Size of RDA_Buffer (0..n) */
	STRPTR	RDA_ExtHelp;		/* Optional extended help */
	LONG	RDA_Flags;		/* Flags for any required control */
};

/**********************************************************************
 * Maximum number of template keywords which can be in a template passed
 * to ReadArgs(). IMPLEMENTOR NOTE - must be a multiple of 4.
 **********************************************************************/
/**********************************************************************
 * Maximum number of MULTIARG items returned by ReadArgs(), before
 * an ERROR_LINE_TOO_LONG.  These two limitations are due to stack
 * usage.  Applications should allow "a lot" of stack to use ReadArgs().
 **********************************************************************/
/*
**	$VER: dosasl.h 47.2 (16.11.2021)
**
**	Pattern-matching structure definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/***********************************************************************
************************ PATTERN MATCHING ******************************
************************************************************************

* structure expected by MatchFirst, MatchNext.
* Allocate this structure and initialize it as follows:
*
* Set ap_BreakBits to the signal bits (CDEF) that you want to take a
* break on, or NULL, if you don't want to convenience the user.
*
* If you want to have the FULL PATH NAME of the files you found,
* allocate a buffer at the END of this structure, and put the size of
* it into ap_Strlen.  If you don't want the full path name, make sure
* you set ap_Strlen to zero.  In this case, the name of the file, and stats
* are available in the ap_Info, as per usual.
*
* Then call MatchFirst() and then afterwards, MatchNext() with this structure.
* You should check the return value each time (see below) and take the
* appropriate action, ultimately calling MatchEnd() when there are
* no more files and you are done.  You can tell when you are done by
* checking for the normal AmigaDOS return code ERROR_NO_MORE_ENTRIES.
*
*/

struct AnchorPath {
	struct AChain	*ap_Base;	/* pointer to first anchor */
	struct AChain	*ap_Last;	/* pointer to last anchor */
	LONG	ap_BreakBits;	/* Bits we want to break on */
	LONG	ap_FoundBreak;	/* Bits we broke on. Also returns ERROR_BREAK */
	BYTE	ap_Flags;	/* New use for extra word. */
	BYTE	ap_Reserved;
	WORD	ap_Strlen;	/* This is what ap_Length used to be */
	struct	FileInfoBlock ap_Info;
	TEXT	ap_Buf[1];	/* Buffer for path name, allocated by user */
	/* FIX! */
};


				/* (means that there's a wildcard	 */
				/* in the pattern after calling		 */
				/* MatchFirst).				 */

				/* bit after MatchFirst/MatchNext to AVOID */
				/* entering a dir. */

struct AChain {
	struct AChain *an_Child;
	struct AChain *an_Parent;
	BPTR	an_Lock;
	struct FileInfoBlock an_Info;
	BYTE	an_Flags;
	TEXT	an_String[1];	/* FIX!! */
};

/*
 * Constants used by wildcard routines, these are the pre-parsed tokens
 * referred to by pattern match.  It is not necessary for you to do
 * anything about these, MatchFirst() MatchNext() handle all these for you.
 */

/* Values for an_Status, NOTE: These are the actual bit numbers */

/*
 * Returns from MatchFirst(), MatchNext()
 * You can also get dos error returns, such as ERROR_NO_MORE_ENTRIES,
 * these are in the dos.h file.
 */

/*
**	$VER: var.h 47.2 (16.11.2021)
**
**	include file for dos local and environment variables
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* the structure in the pr_LocalVars list */
/* Do NOT allocate yourself, use SetVar()!!! This structure may grow in */
/* future releases!  The list should be left in alphabetical order, and */
/* may have multiple entries with the same name but different types.	*/

struct LocalVar {
	struct Node lv_Node;
	UWORD	lv_Flags;
	STRPTR	lv_Value;
	ULONG	lv_Len;
};

/*
 * The lv_Flags bits are available to the application.	The unused
 * lv_Node.ln_Pri bits are reserved for system use.
 */

/* bit definitions for lv_Node.ln_Type: */
/* to be or'ed into type: */
/* definitions of flags passed to GetVar()/SetVar()/DeleteVar() */
/* bit defs to be OR'ed with the type: */
/* item will be treated as a single line of text unless BINARY_VAR is used */
/* this is only supported in >= V39 dos.  V37 dos ignores this. */
/* this causes SetVar to affect ENVARC: as well as ENV:.	*/
/*
**
**	$VER: notify.h 47.2 (16.11.2021)
**
**	dos file and directory change notification definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* use of Class and code is discouraged for the time being - we might want to
   change things */
/* --- NotifyMessage Class ------------------------------------------------ */
/* --- NotifyMessage Codes ------------------------------------------------ */
/* Sent to the application if SEND_MESSAGE is specified.		    */

struct NotifyMessage {
    struct Message nm_ExecMessage;
    ULONG  nm_Class;
    UWORD  nm_Code;
    struct NotifyRequest *nm_NReq;	/* don't modify the request! */
    ULONG  nm_DoNotTouch;		/* like it says!  For use by handlers */
    ULONG  nm_DoNotTouch2;		/* ditto */
};

/* Do not modify or reuse the notifyrequest while active.		    */
/* note: the first LONG of nr_Data has the length transfered		    */

struct NotifyRequest {
	STRPTR nr_Name;
	STRPTR nr_FullName;		/* set by dos - don't touch */
	ULONG nr_UserData;		/* for applications use */
	ULONG nr_Flags;

	union {

	    struct {
		struct MsgPort *nr_Port;	/* for SEND_MESSAGE */
	    } nr_Msg;

	    struct {
		struct Task *nr_Task;		/* for SEND_SIGNAL */
		UBYTE nr_SignalNum;		/* for SEND_SIGNAL */
		UBYTE nr_pad[3];
	    } nr_Signal;
	} nr_stuff;

	ULONG nr_Reserved[4];		/* leave 0 for now */

	/* internal use by handlers */
	ULONG nr_MsgCount;		/* # of outstanding msgs */
	struct MsgPort *nr_Handler;	/* handler sent to (for EndNotify) */
};

/* --- NotifyRequest Flags ------------------------------------------------ */
/* do NOT set or remove NRF_MAGIC!  Only for use by handlers! */
/* bit numbers */
/* Flags reserved for private use by the handler: */
/*
**	$VER: datetime.h 47.27 (2.12.2021)
**
**	Date and time conversion for AmigaDOS
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* Data structures and constants used by the AmigaDOS functions
 * StrtoDate() and DatetoStr(), first available in dos.library
 * V36 (V1.4)
 */

/* String/Date & Time conversion */
struct DateTime
{
	struct DateStamp	dat_Stamp;	/* DOS DateStamp */
	UBYTE			dat_Format;	/* controls appearance of dat_StrDate */
	UBYTE			dat_Flags;	/* see flags below */
	STRPTR			dat_StrDay;	/* day of the week string (see LEN_DATSTRING below) */
	STRPTR			dat_StrDate;	/* date string (see LEN_DATSTRING below) */
	STRPTR			dat_StrTime;	/* time string (see LEN_DATSTRING below) */
};

/* The DateTime.dat_StrDay/dat_StrDate/dat_StrTime members must
 * point to string buffers at least as large as this:
 */
/* Flags for dat_Flags */
/* Date format values */
/* NOTE: dos.library supports only four built-in formats for
 *       date and time conversion (FORMAT_DOS, FORMAT_INT, FORMAT_USA
 *       and FORMAT_CDN). This is why FORMAT_MAX is set to FORMAT_CDN.
 *       The use of FORMAT_DEF requires locale.library to be active,
 *       otherwise dos.library will fall back onto using FORMAT_DOS
 *       instead.
 */
/*
**	$VER: exall.h 47.2 (16.11.2019)
**
**	include file for ExAll() data structures
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* NOTE: V37 dos.library, when doing ExAll() emulation, and V37 filesystems  */
/* will return an error if passed ED_OWNER.  If you get ERROR_BAD_NUMBER,    */
/* retry with ED_COMMENT to get everything but owner info.  All filesystems  */
/* supporting ExAll() must support through ED_COMMENT, and must check Type   */
/* and return ERROR_BAD_NUMBER if they don't support the type.		     */

/* values that can be passed for what data you want from ExAll() */
/* each higher value includes those below it (numerically)	 */
/* you MUST chose one of these values */
/*
 *   Structure in which exall results are returned in.	Note that only the
 *   fields asked for will exist!
 */

struct ExAllData {
	struct ExAllData *ed_Next;
	STRPTR	ed_Name;
	LONG	ed_Type;
	ULONG	ed_Size;
	ULONG	ed_Prot;
	ULONG	ed_Days;
	ULONG	ed_Mins;
	ULONG	ed_Ticks;
	STRPTR	ed_Comment;	/* strings will be after last used field */
	UWORD	ed_OwnerUID;	/* new for V39 */
	UWORD	ed_OwnerGID;
};

/*
 *   Control structure passed to ExAll.  Unused fields MUST be initialized to
 *   0, expecially eac_LastKey.
 *
 *   eac_MatchFunc is a hook (see utility.library documentation for usage)
 *   It should return true if the entry is to returned, false if it is to be
 *   ignored.
 *
 *   This structure MUST be allocated by AllocDosObject()!
 */

struct ExAllControl {
	ULONG	eac_Entries;	 /* number of entries returned in buffer      */
	ULONG	eac_LastKey;	 /* Don't touch inbetween linked ExAll calls! */
	STRPTR	eac_MatchString; /* wildcard string for pattern match or NULL */
	struct Hook *eac_MatchFunc; /* optional private wildcard function     */
};

BPTR Open( CONST_STRPTR name, LONG accessMode );
LONG Close( BPTR file );
LONG Read( BPTR file, APTR buffer, LONG length );
LONG Write( BPTR file, CONST_APTR buffer, LONG length );
BPTR Input( void );
BPTR Output( void );
LONG Seek( BPTR file, LONG position, LONG offset );
LONG DeleteFile( CONST_STRPTR name );
LONG Rename( CONST_STRPTR oldName, CONST_STRPTR newName );
BPTR Lock( CONST_STRPTR name, LONG type );
void UnLock( BPTR lock );
BPTR DupLock( BPTR lock );
LONG Examine( BPTR lock, struct FileInfoBlock *fileInfoBlock );
LONG ExNext( BPTR lock, struct FileInfoBlock *fileInfoBlock );
LONG Info( BPTR lock, struct InfoData *parameterBlock );
BPTR CreateDir( CONST_STRPTR name );
BPTR CurrentDir( BPTR lock );
LONG IoErr( void );
struct MsgPort *CreateProc( CONST_STRPTR name, LONG pri, BPTR segList, LONG stackSize );
void Exit( LONG returnCode );
BPTR LoadSeg( CONST_STRPTR name );
void UnLoadSeg( BPTR seglist );
struct MsgPort *DeviceProc( CONST_STRPTR name );
LONG SetComment( CONST_STRPTR name, CONST_STRPTR comment );
LONG SetProtection( CONST_STRPTR name, LONG protect );
struct DateStamp *DateStamp( struct DateStamp *date );
void Delay( LONG timeout );
LONG WaitForChar( BPTR file, LONG timeout );
BPTR ParentDir( BPTR lock );
LONG IsInteractive( BPTR file );
LONG Execute( CONST_STRPTR string, BPTR file, BPTR file2 );
/*--- functions in V36 or higher (Release 2.0) ---*/
/*	DOS Object creation/deletion */
APTR AllocDosObject( ULONG type, const struct TagItem *tags );
APTR AllocDosObjectTagList( ULONG type, const struct TagItem *tags );
APTR AllocDosObjectTags( ULONG type, ULONG tag1type, ... );
void FreeDosObject( ULONG type, APTR ptr );
/*	Packet Level routines */
LONG DoPkt( struct MsgPort *port, LONG action, LONG arg1, LONG arg2, LONG arg3, LONG arg4, LONG arg5 );
LONG DoPkt0( struct MsgPort *port, LONG action );
LONG DoPkt1( struct MsgPort *port, LONG action, LONG arg1 );
LONG DoPkt2( struct MsgPort *port, LONG action, LONG arg1, LONG arg2 );
LONG DoPkt3( struct MsgPort *port, LONG action, LONG arg1, LONG arg2, LONG arg3 );
LONG DoPkt4( struct MsgPort *port, LONG action, LONG arg1, LONG arg2, LONG arg3, LONG arg4 );
void SendPkt( struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport );
struct DosPacket *WaitPkt( void );
void ReplyPkt( struct DosPacket *dp, LONG res1, LONG res2 );
void AbortPkt( struct MsgPort *port, struct DosPacket *pkt );
/*	Record Locking */
BOOL LockRecord( BPTR fh, ULONG offset, ULONG length, ULONG mode, ULONG timeout );
BOOL LockRecords( const struct RecordLock *recArray, ULONG timeout );
BOOL UnLockRecord( BPTR fh, ULONG offset, ULONG length );
BOOL UnLockRecords( const struct RecordLock *recArray );
/*	Buffered File I/O */
BPTR SelectInput( BPTR fh );
BPTR SelectOutput( BPTR fh );
LONG FGetC( BPTR fh );
LONG FPutC( BPTR fh, LONG ch );
LONG UnGetC( BPTR fh, LONG character );
LONG FRead( BPTR fh, APTR block, ULONG blocklen, ULONG number );
LONG FWrite( BPTR fh, CONST_APTR block, ULONG blocklen, ULONG number );
STRPTR FGets( BPTR fh, STRPTR buf, ULONG buflen );
LONG FPuts( BPTR fh, CONST_STRPTR str );
void VFWritef( BPTR fh, CONST_STRPTR format, const LONG *argarray );
void FWritef( BPTR fh, CONST_STRPTR format, ... );
LONG VFPrintf( BPTR fh, CONST_STRPTR format, CONST_APTR argarray );
LONG FPrintf( BPTR fh, CONST_STRPTR format, ... );
LONG Flush( BPTR fh );
LONG SetVBuf( BPTR fh, STRPTR buff, LONG type, LONG size );
/*	DOS Object Management */
BPTR DupLockFromFH( BPTR fh );
BPTR OpenFromLock( BPTR lock );
BPTR ParentOfFH( BPTR fh );
BOOL ExamineFH( BPTR fh, struct FileInfoBlock *fib );
LONG SetFileDate( CONST_STRPTR name, const struct DateStamp *date );
LONG NameFromLock( BPTR lock, STRPTR buffer, LONG len );
LONG NameFromFH( BPTR fh, STRPTR buffer, LONG len );
WORD SplitName( CONST_STRPTR name, ULONG separator, STRPTR buf, LONG oldpos, LONG size );
LONG SameLock( BPTR lock1, BPTR lock2 );
LONG SetMode( BPTR fh, LONG mode );
LONG ExAll( BPTR lock, struct ExAllData *buffer, LONG size, LONG data, struct ExAllControl *control );
LONG ReadLink( struct MsgPort *port, BPTR lock, CONST_STRPTR path, STRPTR buffer, ULONG size );
LONG MakeLink( CONST_STRPTR name, LONG dest, LONG soft );
LONG ChangeMode( LONG type, BPTR fh, LONG newmode );
LONG SetFileSize( BPTR fh, LONG pos, LONG mode );
/*	Error Handling */
LONG SetIoErr( LONG result );
BOOL Fault( LONG code, CONST_STRPTR header, STRPTR buffer, LONG len );
BOOL PrintFault( LONG code, CONST_STRPTR header );
LONG ErrorReport( LONG code, LONG type, ULONG arg1, struct MsgPort *device );
/*	Process Management */
struct CommandLineInterface *Cli( void );
struct Process *CreateNewProc( const struct TagItem *tags );
struct Process *CreateNewProcTagList( const struct TagItem *tags );
struct Process *CreateNewProcTags( ULONG tag1type, ... );
LONG RunCommand( BPTR seg, LONG stack, CONST_STRPTR paramptr, LONG paramlen );
struct MsgPort *GetConsoleTask( void );
struct MsgPort *SetConsoleTask( struct MsgPort *task );
struct MsgPort *GetFileSysTask( void );
struct MsgPort *SetFileSysTask( struct MsgPort *task );
STRPTR GetArgStr( void );
STRPTR SetArgStr( STRPTR string );
struct Process *FindCliProc( ULONG num );
ULONG MaxCli( void );
BOOL SetCurrentDirName( CONST_STRPTR name );
BOOL GetCurrentDirName( STRPTR buf, LONG len );
BOOL SetProgramName( CONST_STRPTR name );
BOOL GetProgramName( STRPTR buf, LONG len );
BOOL SetPrompt( CONST_STRPTR name );
BOOL GetPrompt( STRPTR buf, LONG len );
BPTR SetProgramDir( BPTR lock );
BPTR GetProgramDir( void );
/*	Device List Management */
LONG SystemTagList( CONST_STRPTR command, const struct TagItem *tags );
LONG System( CONST_STRPTR command, const struct TagItem *tags );
LONG SystemTags( CONST_STRPTR command, ULONG tag1type, ... );
LONG AssignLock( CONST_STRPTR name, BPTR lock );
BOOL AssignLate( CONST_STRPTR name, CONST_STRPTR path );
BOOL AssignPath( CONST_STRPTR name, CONST_STRPTR path );
BOOL AssignAdd( CONST_STRPTR name, BPTR lock );
LONG RemAssignList( CONST_STRPTR name, BPTR lock );
struct DevProc *GetDeviceProc( CONST_STRPTR name, struct DevProc *dp );
void FreeDeviceProc( struct DevProc *dp );
struct DosList *LockDosList( ULONG flags );
void UnLockDosList( ULONG flags );
struct DosList *AttemptLockDosList( ULONG flags );
BOOL RemDosEntry( struct DosList *dlist );
LONG AddDosEntry( struct DosList *dlist );
struct DosList *FindDosEntry( const struct DosList *dlist, CONST_STRPTR name, ULONG flags );
struct DosList *NextDosEntry( const struct DosList *dlist, ULONG flags );
struct DosList *MakeDosEntry( CONST_STRPTR name, LONG type );
void FreeDosEntry( struct DosList *dlist );
BOOL IsFileSystem( CONST_STRPTR name );
/*	Handler Interface */
BOOL Format( CONST_STRPTR filesystem, CONST_STRPTR volumename, ULONG dostype );
LONG Relabel( CONST_STRPTR drive, CONST_STRPTR newname );
LONG Inhibit( CONST_STRPTR name, LONG onoff );
LONG AddBuffers( CONST_STRPTR name, LONG number );
/*	Date, Time Routines */
LONG CompareDates( const struct DateStamp *date1, const struct DateStamp *date2 );
LONG DateToStr( struct DateTime *datetime );
LONG StrToDate( struct DateTime *datetime );
/*	Image Management */
BPTR InternalLoadSeg( BPTR fh, BPTR table, const LONG *funcarray, LONG *stack );
BOOL InternalUnLoadSeg( BPTR seglist, void (*freefunc)() );
BPTR NewLoadSeg( CONST_STRPTR file, const struct TagItem *tags );
BPTR NewLoadSegTagList( CONST_STRPTR file, const struct TagItem *tags );
BPTR NewLoadSegTags( CONST_STRPTR file, ULONG tag1type, ... );
LONG AddSegment( CONST_STRPTR name, BPTR seg, LONG system );
struct Segment *FindSegment( CONST_STRPTR name, const struct Segment *seg, LONG system );
LONG RemSegment( struct Segment *seg );
/*	Command Support */
LONG CheckSignal( LONG mask );
struct RDArgs *ReadArgs( CONST_STRPTR arg_template, LONG *array, struct RDArgs *args );
LONG FindArg( CONST_STRPTR keyword, CONST_STRPTR arg_template );
LONG ReadItem( CONST_STRPTR name, LONG maxchars, struct CSource *cSource );
LONG StrToLong( CONST_STRPTR string, LONG *value );
LONG MatchFirst( CONST_STRPTR pat, struct AnchorPath *anchor );
LONG MatchNext( struct AnchorPath *anchor );
void MatchEnd( struct AnchorPath *anchor );
LONG ParsePattern( CONST_STRPTR pat, UBYTE *patbuf, LONG patbuflen );
BOOL MatchPattern( const UBYTE *patbuf, CONST_STRPTR str );
void FreeArgs( struct RDArgs *args );
STRPTR FilePart( CONST_STRPTR path );
STRPTR PathPart( CONST_STRPTR path );
BOOL AddPart( STRPTR dirname, CONST_STRPTR filename, ULONG size );
/*	Notification */
BOOL StartNotify( struct NotifyRequest *notify );
void EndNotify( struct NotifyRequest *notify );
/*	Environment Variable functions */
BOOL SetVar( CONST_STRPTR name, CONST_STRPTR buffer, LONG size, LONG flags );
LONG GetVar( CONST_STRPTR name, STRPTR buffer, LONG size, LONG flags );
LONG DeleteVar( CONST_STRPTR name, ULONG flags );
struct LocalVar *FindVar( CONST_STRPTR name, ULONG type );
LONG CliInitNewcli( struct DosPacket *dp );
LONG CliInitRun( struct DosPacket *dp );
LONG WriteChars( CONST_STRPTR buf, ULONG buflen );
LONG PutStr( CONST_STRPTR str );
LONG VPrintf( CONST_STRPTR format, CONST_APTR argarray );
LONG Printf( CONST_STRPTR format, ... );
/* these were unimplemented until dos 36.147 */
LONG ParsePatternNoCase( CONST_STRPTR pat, UBYTE *patbuf, LONG patbuflen );
BOOL MatchPatternNoCase( const UBYTE *patbuf, CONST_STRPTR str );
/* this was added for V37 dos, returned 0 before then. */
BOOL SameDevice( BPTR lock1, BPTR lock2 );

/* NOTE: the following entries did NOT exist before ks 36.303 (2.02) */
/* If you are going to use them, open dos.library with version 37 */

/* These calls were added for V39 dos: */
void ExAllEnd( BPTR lock, struct ExAllData *buffer, LONG size, LONG data, struct ExAllControl *control );
BOOL SetOwner( CONST_STRPTR name, LONG owner_info );
/* Added with dos 47.4 */
LONG VolumeRequestHook( CONST_STRPTR vol );
BPTR GetCurrentDir( void );
LONG PutErrStr( CONST_STRPTR str );
LONG ErrorOutput( void );
LONG SelectError( BPTR fh );
APTR DoShellMethodTagList( ULONG method, const struct TagItem *tags );
APTR DoShellMethod( ULONG method, ULONG tag1type, ... );
LONG ScanStackToken( BPTR seg, LONG defaultstack );

/* "dos.library" */
#pragma libcall DOSBase Open 1e 2102
#pragma libcall DOSBase Close 24 101
#pragma libcall DOSBase Read 2a 32103
#pragma libcall DOSBase Write 30 32103
#pragma libcall DOSBase Input 36 00
#pragma libcall DOSBase Output 3c 00
#pragma libcall DOSBase Seek 42 32103
#pragma libcall DOSBase DeleteFile 48 101
#pragma libcall DOSBase Rename 4e 2102
#pragma libcall DOSBase Lock 54 2102
#pragma libcall DOSBase UnLock 5a 101
#pragma libcall DOSBase DupLock 60 101
#pragma libcall DOSBase Examine 66 2102
#pragma libcall DOSBase ExNext 6c 2102
#pragma libcall DOSBase Info 72 2102
#pragma libcall DOSBase CreateDir 78 101
#pragma libcall DOSBase CurrentDir 7e 101
#pragma libcall DOSBase IoErr 84 00
#pragma libcall DOSBase CreateProc 8a 432104
#pragma libcall DOSBase Exit 90 101
#pragma libcall DOSBase LoadSeg 96 101
#pragma libcall DOSBase UnLoadSeg 9c 101
#pragma libcall DOSBase DeviceProc ae 101
#pragma libcall DOSBase SetComment b4 2102
#pragma libcall DOSBase SetProtection ba 2102
#pragma libcall DOSBase DateStamp c0 101
#pragma libcall DOSBase Delay c6 101
#pragma libcall DOSBase WaitForChar cc 2102
#pragma libcall DOSBase ParentDir d2 101
#pragma libcall DOSBase IsInteractive d8 101
#pragma libcall DOSBase Execute de 32103
/*--- functions in V36 or higher (Release 2.0) ---*/
/*	DOS Object creation/deletion */
#pragma libcall DOSBase AllocDosObject e4 2102
#pragma libcall DOSBase AllocDosObjectTagList e4 2102
#pragma tagcall DOSBase AllocDosObjectTags e4 2102
#pragma libcall DOSBase FreeDosObject ea 2102
/*	Packet Level routines */
#pragma libcall DOSBase DoPkt f0 765432107
#pragma libcall DOSBase DoPkt0 f0 2102
#pragma libcall DOSBase DoPkt1 f0 32103
#pragma libcall DOSBase DoPkt2 f0 432104
#pragma libcall DOSBase DoPkt3 f0 5432105
#pragma libcall DOSBase DoPkt4 f0 65432106
#pragma libcall DOSBase SendPkt f6 32103
#pragma libcall DOSBase WaitPkt fc 00
#pragma libcall DOSBase ReplyPkt 102 32103
#pragma libcall DOSBase AbortPkt 108 2102
/*	Record Locking */
#pragma libcall DOSBase LockRecord 10e 5432105
#pragma libcall DOSBase LockRecords 114 2102
#pragma libcall DOSBase UnLockRecord 11a 32103
#pragma libcall DOSBase UnLockRecords 120 101
/*	Buffered File I/O */
#pragma libcall DOSBase SelectInput 126 101
#pragma libcall DOSBase SelectOutput 12c 101
#pragma libcall DOSBase FGetC 132 101
#pragma libcall DOSBase FPutC 138 2102
#pragma libcall DOSBase UnGetC 13e 2102
#pragma libcall DOSBase FRead 144 432104
#pragma libcall DOSBase FWrite 14a 432104
#pragma libcall DOSBase FGets 150 32103
#pragma libcall DOSBase FPuts 156 2102
#pragma libcall DOSBase VFWritef 15c 32103
#pragma tagcall DOSBase FWritef 15c 32103
#pragma libcall DOSBase VFPrintf 162 32103
#pragma tagcall DOSBase FPrintf 162 32103
#pragma libcall DOSBase Flush 168 101
#pragma libcall DOSBase SetVBuf 16e 432104
/*	DOS Object Management */
#pragma libcall DOSBase DupLockFromFH 174 101
#pragma libcall DOSBase OpenFromLock 17a 101
#pragma libcall DOSBase ParentOfFH 180 101
#pragma libcall DOSBase ExamineFH 186 2102
#pragma libcall DOSBase SetFileDate 18c 2102
#pragma libcall DOSBase NameFromLock 192 32103
#pragma libcall DOSBase NameFromFH 198 32103
#pragma libcall DOSBase SplitName 19e 5432105
#pragma libcall DOSBase SameLock 1a4 2102
#pragma libcall DOSBase SetMode 1aa 2102
#pragma libcall DOSBase ExAll 1b0 5432105
#pragma libcall DOSBase ReadLink 1b6 5432105
#pragma libcall DOSBase MakeLink 1bc 32103
#pragma libcall DOSBase ChangeMode 1c2 32103
#pragma libcall DOSBase SetFileSize 1c8 32103
/*	Error Handling */
#pragma libcall DOSBase SetIoErr 1ce 101
#pragma libcall DOSBase Fault 1d4 432104
#pragma libcall DOSBase PrintFault 1da 2102
#pragma libcall DOSBase ErrorReport 1e0 432104
/*--- (1 function slot reserved here) ---*/
/*	Process Management */
#pragma libcall DOSBase Cli 1ec 00
#pragma libcall DOSBase CreateNewProc 1f2 101
#pragma libcall DOSBase CreateNewProcTagList 1f2 101
#pragma tagcall DOSBase CreateNewProcTags 1f2 101
#pragma libcall DOSBase RunCommand 1f8 432104
#pragma libcall DOSBase GetConsoleTask 1fe 00
#pragma libcall DOSBase SetConsoleTask 204 101
#pragma libcall DOSBase GetFileSysTask 20a 00
#pragma libcall DOSBase SetFileSysTask 210 101
#pragma libcall DOSBase GetArgStr 216 00
#pragma libcall DOSBase SetArgStr 21c 101
#pragma libcall DOSBase FindCliProc 222 101
#pragma libcall DOSBase MaxCli 228 00
#pragma libcall DOSBase SetCurrentDirName 22e 101
#pragma libcall DOSBase GetCurrentDirName 234 2102
#pragma libcall DOSBase SetProgramName 23a 101
#pragma libcall DOSBase GetProgramName 240 2102
#pragma libcall DOSBase SetPrompt 246 101
#pragma libcall DOSBase GetPrompt 24c 2102
#pragma libcall DOSBase SetProgramDir 252 101
#pragma libcall DOSBase GetProgramDir 258 00
/*	Device List Management */
#pragma libcall DOSBase SystemTagList 25e 2102
#pragma libcall DOSBase System 25e 2102
#pragma tagcall DOSBase SystemTags 25e 2102
#pragma libcall DOSBase AssignLock 264 2102
#pragma libcall DOSBase AssignLate 26a 2102
#pragma libcall DOSBase AssignPath 270 2102
#pragma libcall DOSBase AssignAdd 276 2102
#pragma libcall DOSBase RemAssignList 27c 2102
#pragma libcall DOSBase GetDeviceProc 282 2102
#pragma libcall DOSBase FreeDeviceProc 288 101
#pragma libcall DOSBase LockDosList 28e 101
#pragma libcall DOSBase UnLockDosList 294 101
#pragma libcall DOSBase AttemptLockDosList 29a 101
#pragma libcall DOSBase RemDosEntry 2a0 101
#pragma libcall DOSBase AddDosEntry 2a6 101
#pragma libcall DOSBase FindDosEntry 2ac 32103
#pragma libcall DOSBase NextDosEntry 2b2 2102
#pragma libcall DOSBase MakeDosEntry 2b8 2102
#pragma libcall DOSBase FreeDosEntry 2be 101
#pragma libcall DOSBase IsFileSystem 2c4 101
/*	Handler Interface */
#pragma libcall DOSBase Format 2ca 32103
#pragma libcall DOSBase Relabel 2d0 2102
#pragma libcall DOSBase Inhibit 2d6 2102
#pragma libcall DOSBase AddBuffers 2dc 2102
/*	Date, Time Routines */
#pragma libcall DOSBase CompareDates 2e2 2102
#pragma libcall DOSBase DateToStr 2e8 101
#pragma libcall DOSBase StrToDate 2ee 101
/*	Image Management */
#pragma libcall DOSBase InternalLoadSeg 2f4 A98004
#pragma libcall DOSBase InternalUnLoadSeg 2fa 9102
#pragma libcall DOSBase NewLoadSeg 300 2102
#pragma libcall DOSBase NewLoadSegTagList 300 2102
#pragma tagcall DOSBase NewLoadSegTags 300 2102
#pragma libcall DOSBase AddSegment 306 32103
#pragma libcall DOSBase FindSegment 30c 32103
#pragma libcall DOSBase RemSegment 312 101
/*	Command Support */
#pragma libcall DOSBase CheckSignal 318 101
#pragma libcall DOSBase ReadArgs 31e 32103
#pragma libcall DOSBase FindArg 324 2102
#pragma libcall DOSBase ReadItem 32a 32103
#pragma libcall DOSBase StrToLong 330 2102
#pragma libcall DOSBase MatchFirst 336 2102
#pragma libcall DOSBase MatchNext 33c 101
#pragma libcall DOSBase MatchEnd 342 101
#pragma libcall DOSBase ParsePattern 348 32103
#pragma libcall DOSBase MatchPattern 34e 2102
/* Not currently implemented. */
#pragma libcall DOSBase FreeArgs 35a 101
/*--- (1 function slot reserved here) ---*/
#pragma libcall DOSBase FilePart 366 101
#pragma libcall DOSBase PathPart 36c 101
#pragma libcall DOSBase AddPart 372 32103
/*	Notification */
#pragma libcall DOSBase StartNotify 378 101
#pragma libcall DOSBase EndNotify 37e 101
/*	Environment Variable functions */
#pragma libcall DOSBase SetVar 384 432104
#pragma libcall DOSBase GetVar 38a 432104
#pragma libcall DOSBase DeleteVar 390 2102
#pragma libcall DOSBase FindVar 396 2102
#pragma libcall DOSBase CliInitNewcli 3a2 801
#pragma libcall DOSBase CliInitRun 3a8 801
#pragma libcall DOSBase WriteChars 3ae 2102
#pragma libcall DOSBase PutStr 3b4 101
#pragma libcall DOSBase VPrintf 3ba 2102
#pragma tagcall DOSBase Printf 3ba 2102
/* new in dos V47 */
/* these were unimplemented until dos 36.147 */
#pragma libcall DOSBase ParsePatternNoCase 3c6 32103
#pragma libcall DOSBase MatchPatternNoCase 3cc 2102
/* this was added for V37 dos, returned 0 before then. */
#pragma libcall DOSBase SameDevice 3d8 2102
/* NOTE: the following entries did NOT exist before ks 36.303 (2.02) */
/* If you are going to use them, open dos.library with version 37 */

/* These calls were added for V39 dos: */
#pragma libcall DOSBase ExAllEnd 3de 5432105
#pragma libcall DOSBase SetOwner 3e4 2102
/* These were added in dos 36.147 */
/*--- (2 function slots reserved here) ---*/
/* Added with dos 47.4 */
#pragma libcall DOSBase VolumeRequestHook 3f6 101
#pragma libcall DOSBase GetCurrentDir 402 00
/*--- (16 function slots reserved here) ---*/
#pragma libcall DOSBase PutErrStr 468 101
#pragma libcall DOSBase ErrorOutput 46e 00
#pragma libcall DOSBase SelectError 474 101
/*--- (1 function slot reserved here) ---*/
#pragma libcall DOSBase DoShellMethodTagList 480 8002
#pragma tagcall DOSBase DoShellMethod 480 8002
#pragma libcall DOSBase ScanStackToken 486 2102
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: icon.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

extern struct Library * IconBase;
/****************************************************************************/

/*
**	$VER: icon_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: icon_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

void FreeFreeList( struct FreeList *freelist );
BOOL AddFreeList( struct FreeList *freelist, CONST_APTR mem, ULONG size );
struct DiskObject *GetDiskObject( CONST_STRPTR name );
BOOL PutDiskObject( CONST_STRPTR name, const struct DiskObject *diskobj );
void FreeDiskObject( struct DiskObject *diskobj );
UBYTE *FindToolType( CONST_STRPTR *toolTypeArray, CONST_STRPTR typeName );
BOOL MatchToolValue( CONST_STRPTR typeString, CONST_STRPTR value );
STRPTR BumpRevision( STRPTR newname, CONST_STRPTR oldname );
APTR FreeAlloc( struct FreeList *free, ULONG len, ULONG type );
/*--- functions in V36 or higher (Release 2.0) ---*/
struct DiskObject *GetDefDiskObject( LONG type );
BOOL PutDefDiskObject( const struct DiskObject *diskObject );
struct DiskObject *GetDiskObjectNew( CONST_STRPTR name );
/*--- functions in V37 or higher (Release 2.04) ---*/
BOOL DeleteDiskObject( CONST_STRPTR name );
/*--- functions in V44 or higher (Release 3.5) ---*/
void FreeFree( struct FreeList *fl, APTR address );
struct DiskObject *DupDiskObjectA( const struct DiskObject *diskObject, const struct TagItem *tags );
struct DiskObject *DupDiskObject( const struct DiskObject *diskObject, ... );
ULONG IconControlA( struct DiskObject *icon, const struct TagItem *tags );
ULONG IconControl( struct DiskObject *icon, ... );
void DrawIconStateA( struct RastPort *rp, const struct DiskObject *icon, CONST_STRPTR label, LONG leftOffset, LONG topOffset, ULONG state, const struct TagItem *tags );
void DrawIconState( struct RastPort *rp, const struct DiskObject *icon, CONST_STRPTR label, LONG leftOffset, LONG topOffset, ULONG state, ... );
BOOL GetIconRectangleA( struct RastPort *rp, const struct DiskObject *icon, CONST_STRPTR label, struct Rectangle *rect, const struct TagItem *tags );
BOOL GetIconRectangle( struct RastPort *rp, const struct DiskObject *icon, CONST_STRPTR label, struct Rectangle *rect, ... );
struct DiskObject *NewDiskObject( LONG type );
struct DiskObject *GetIconTagList( CONST_STRPTR name, const struct TagItem *tags );
struct DiskObject *GetIconTags( CONST_STRPTR name, ... );
BOOL PutIconTagList( CONST_STRPTR name, const struct DiskObject *icon, const struct TagItem *tags );
BOOL PutIconTags( CONST_STRPTR name, const struct DiskObject *icon, ... );
BOOL LayoutIconA( struct DiskObject *icon, struct Screen *screen, struct TagItem *tags );
BOOL LayoutIcon( struct DiskObject *icon, struct Screen *screen, ... );
void ChangeToSelectedIconColor( struct ColorRegister *cr );
STRPTR BumpRevisionLength( STRPTR newname, CONST_STRPTR oldname, ULONG maxLength );

/* "icon.library" */
/*	Use DiskObjects instead of obsolete WBObjects */
#pragma libcall IconBase FreeFreeList 36 801
#pragma libcall IconBase AddFreeList 48 A9803
#pragma libcall IconBase GetDiskObject 4e 801
#pragma libcall IconBase PutDiskObject 54 9802
#pragma libcall IconBase FreeDiskObject 5a 801
#pragma libcall IconBase FindToolType 60 9802
#pragma libcall IconBase MatchToolValue 66 9802
#pragma libcall IconBase BumpRevision 6c 9802
#pragma libcall IconBase FreeAlloc 72 A9803
/*--- functions in V36 or higher (Release 2.0) ---*/
#pragma libcall IconBase GetDefDiskObject 78 001
#pragma libcall IconBase PutDefDiskObject 7e 801
#pragma libcall IconBase GetDiskObjectNew 84 801
/*--- functions in V37 or higher (Release 2.04) ---*/
#pragma libcall IconBase DeleteDiskObject 8a 801
/*--- functions in V44 or higher (Release 3.5) ---*/
#pragma libcall IconBase FreeFree 90 9802
#pragma libcall IconBase DupDiskObjectA 96 9802
#pragma tagcall IconBase DupDiskObject 96 9802
#pragma libcall IconBase IconControlA 9c 9802
#pragma tagcall IconBase IconControl 9c 9802
#pragma libcall IconBase DrawIconStateA a2 B210A9807
#pragma tagcall IconBase DrawIconState a2 B210A9807
#pragma libcall IconBase GetIconRectangleA a8 CBA9805
#pragma tagcall IconBase GetIconRectangle a8 CBA9805
#pragma libcall IconBase NewDiskObject ae 001
#pragma libcall IconBase GetIconTagList b4 9802
#pragma tagcall IconBase GetIconTags b4 9802
#pragma libcall IconBase PutIconTagList ba A9803
#pragma tagcall IconBase PutIconTags ba A9803
#pragma libcall IconBase LayoutIconA c0 A9803
#pragma tagcall IconBase LayoutIcon c0 A9803
#pragma libcall IconBase ChangeToSelectedIconColor c6 801
#pragma libcall IconBase BumpRevisionLength cc 09803
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: intuition.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: intuitionbase.h 47.1 (1.8.2019)
**
**	Public part of IntuitionBase structure and supporting structures
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* these are the display modes for which we have corresponding parameter
 *  settings in the config arrays
 */
/* these are the system Gadget defines */
/* ======================================================================== */
/* === IntuitionBase ====================================================== */
/* ======================================================================== */
/*
 * Be sure to protect yourself against someone modifying these data as
 * you look at them.  This is done by calling:
 *
 * lock = LockIBase(0), which returns a ULONG.	When done call
 * UnlockIBase(lock) where lock is what LockIBase() returned.
 */

/* This structure is strictly READ ONLY */
struct IntuitionBase
{
    struct Library LibNode;

    struct View ViewLord;

    struct Window *ActiveWindow;
    struct Screen *ActiveScreen;

    /* the FirstScreen variable points to the frontmost Screen.  Screens are
     * then maintained in a front to back order using Screen.NextScreen
     */
    struct Screen *FirstScreen; /* for linked list of all screens */

    ULONG Flags;	/* values are all system private */
    WORD	MouseY, MouseX;
			/* note "backwards" order of these		*/

    ULONG Seconds;	/* timestamp of most current input event */
    ULONG Micros;	/* timestamp of most current input event */

    /* I told you this was private.
     * The data beyond this point has changed, is changing, and
     * will continue to change.
     */
};

extern struct IntuitionBase * IntuitionBase;
/****************************************************************************/

/*
**	$VER: intuition_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: intuition_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: classes.h 47.2 (26.12.2021)
**
**	Used only by class implementors
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/

/*
**	$VER: classusr.h 47.1 (1.8.2019)
**
**	For application users of Intuition object classes
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/


/*** User visible handles on objects, classes, messages ***/
typedef ULONG	Object;		/* abstract handle */

typedef	STRPTR ClassID;

/* you can use this type to point to a "generic" message,
 * in the object-oriented programming parlance.  Based on
 * the value of 'MethodID', you dispatch to processing
 * for the various message types.  The meaningful parameter
 * packet structure definitions are defined below.
 */
typedef struct {
    ULONG MethodID;
    /* method-specific data follows, some examples below */
}		*Msg;

/*
 * Class id strings for Intuition classes.
 * There's no real reason to use the uppercase constants
 * over the lowercase strings, but this makes a good place
 * to list the names of the built-in classes.
 */
/* Dispatched method ID's
 * NOTE: Applications should use Intuition entry points, not direct
 * DoMethod() calls, for NewObject, DisposeObject, SetAttrs,
 * SetGadgetAttrs, and GetAttr.
 */

/* Parameter "Messages" passed to methods	*/

/* OM_NEW and OM_SET	*/
struct opSet {
    ULONG		MethodID;
    struct TagItem	*ops_AttrList;	/* new attributes	*/
    struct GadgetInfo	*ops_GInfo;	/* always there for gadgets,
					 * when SetGadgetAttrs() is used,
					 * but will be NULL for OM_NEW
					 */
};

/* OM_NOTIFY, and OM_UPDATE	*/
struct opUpdate {
    ULONG		MethodID;
    struct TagItem	*opu_AttrList;	/* new attributes	*/
    struct GadgetInfo	*opu_GInfo;	/* non-NULL when SetGadgetAttrs or
					 * notification resulting from gadget
					 * input occurs.
					 */
    ULONG		opu_Flags;	/* defined below	*/
};

/* this flag means that the update message is being issued from
 * something like an active gadget, a la GACT_FOLLOWMOUSE.  When
 * the gadget goes inactive, it will issue a final update
 * message with this bit cleared.  Examples of use are for
 * GACT_FOLLOWMOUSE equivalents for propgadclass, and repeat strobes
 * for buttons.
 */
/* OM_GET	*/
struct opGet {
    ULONG		MethodID;
    ULONG		opg_AttrID;
    ULONG		*opg_Storage;	/* may be other types, but "int"
					 * types are all ULONG
					 */
};

/* OM_ADDTAIL	*/
struct opAddTail {
    ULONG		MethodID;
    struct List		*opat_List;
};

/* OM_ADDMEMBER, OM_REMMEMBER	*/
struct opMember {
    ULONG		MethodID;
    Object		*opam_Object;
};


/*****************************************************************************/
/***************** "White Box" access to struct IClass ***********************/
/*****************************************************************************/

/* This structure is READ-ONLY, and allocated only by Intuition */
typedef struct IClass
{
    struct Hook		 cl_Dispatcher;		/* Class dispatcher */
    ULONG		 cl_Reserved;		/* Must be 0  */
    struct IClass	*cl_Super;		/* Pointer to superclass */
    ClassID		 cl_ID;			/* Class ID */

    UWORD		 cl_InstOffset;		/* Offset of instance data */
    UWORD		 cl_InstSize;		/* Size of instance data */

    ULONG		 cl_UserData;		/* Class global data */
    ULONG		 cl_SubclassCount;	/* Number of subclasses */
    ULONG		 cl_ObjectCount;	/* Number of objects */
    ULONG		 cl_Flags;

} Class;

    /* class is in public class list */

/*****************************************************************************/

/* add offset for instance data to an object handle */
/*****************************************************************************/

/* sizeof the instance data for a given class */
/*****************************************************************************/
/***************** "White box" access to struct _Object **********************/
/*****************************************************************************/

/* We have this, the instance data of the root class, PRECEDING the "object".
 * This is so that Gadget objects are Gadget pointers, and so on.  If this
 * structure grows, it will always have o_Class at the end, so the macro
 * OCLASS(o) will always have the same offset back from the pointer returned
 * from NewObject().
 *
 * This data structure is subject to change.  Do not use the o_Node embedded
 * structure. */
struct _Object
{
    struct MinNode	 o_Node;
    struct IClass	*o_Class;

};

/*****************************************************************************/

/* convenient typecast	*/
/* get "public" handle on baseclass instance from real beginning of obj data */
/* get back to object data struct from public handle */
/* get class pointer from an object handle	*/
/*****************************************************************************/

/* BOOPSI class libraries should use this structure as the base for their
 * library data.  This allows developers to obtain the class pointer for
 * performing object-less inquiries. */
struct ClassLibrary
{
    struct Library	 cl_Lib;	/* Embedded library */
    UWORD		 cl_Pad;	/* Align the structure */
    Class		*cl_Class;	/* Class pointer */

};

/*****************************************************************************/

/*
**	$VER: cghooks.h 47.1 (1.8.2019)
**
**	Custom Gadget processing
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
 * Package of information passed to custom and 'boopsi'
 * gadget "hook" functions.  This structure is READ ONLY.
 */
struct GadgetInfo {

    struct Screen		*gi_Screen;
    struct Window		*gi_Window;	/* null for screen gadgets */
    struct Requester		*gi_Requester;	/* null if not GTYP_REQGADGET */

    /* rendering information:
     * don't use these without cloning/locking.
     * Official way is to call ObtainRPort()
     */
    struct RastPort		*gi_RastPort;
    struct Layer		*gi_Layer;

    /* copy of dimensions of screen/window/g00/req(/group)
     * that gadget resides in.	Left/Top of this box is
     * offset from window mouse coordinates to gadget coordinates
     *		screen gadgets:			0,0 (from screen coords)
     *	window gadgets (no g00):	0,0
     *	GTYP_GZZGADGETs (borderlayer):		0,0
     *	GZZ innerlayer gadget:		borderleft, bordertop
     *	Requester gadgets:		reqleft, reqtop
     */
    struct IBox			gi_Domain;

    /* these are the pens for the window or screen	*/
    struct {
	UBYTE	DetailPen;
	UBYTE	BlockPen;
    }				gi_Pens;

    /* the Detail and Block pens in gi_DrInfo->dri_Pens[] are
     * for the screen.	Use the above for window-sensitive
     * colors.
     */
    struct DrawInfo		*gi_DrInfo;

    /* reserved space: this structure is extensible
     * anyway, but using these saves some recompilation
     */
    ULONG			gi_Reserved[6];
};

/*** system private data structure for now ***/
/* prop gadget extra info	*/
struct PGX	{
    struct IBox	pgx_Container;
    struct IBox	pgx_NewKnob;
};

/* this casts MutualExclude for easy assignment of a hook
 * pointer to the unused MutualExclude field of a custom gadget
 */
/* Public functions OpenIntuition() and Intuition() are intentionally */
/* not documented. */
void OpenIntuition( void );
void Intuition( struct InputEvent *iEvent );
UWORD AddGadget( struct Window *window, struct Gadget *gadget, ULONG position );
BOOL ClearDMRequest( struct Window *window );
void ClearMenuStrip( struct Window *window );
void ClearPointer( struct Window *window );
BOOL CloseScreen( struct Screen *screen );
void CloseWindow( struct Window *window );
LONG CloseWorkBench( void );
void CurrentTime( ULONG *seconds, ULONG *micros );
BOOL DisplayAlert( ULONG alertNumber, CONST_STRPTR string, ULONG height );
void DisplayBeep( struct Screen *screen );
BOOL DoubleClick( ULONG sSeconds, ULONG sMicros, ULONG cSeconds, ULONG cMicros );
void DrawBorder( struct RastPort *rp, const struct Border *border, LONG leftOffset, LONG topOffset );
void DrawImage( struct RastPort *rp, const struct Image *image, LONG leftOffset, LONG topOffset );
void EndRequest( struct Requester *requester, struct Window *window );
struct Preferences *GetDefPrefs( struct Preferences *preferences, LONG size );
struct Preferences *GetPrefs( struct Preferences *preferences, LONG size );
void InitRequester( struct Requester *requester );
struct MenuItem *ItemAddress( const struct Menu *menuStrip, ULONG menuNumber );
BOOL ModifyIDCMP( struct Window *window, ULONG flags );
void ModifyProp( struct Gadget *gadget, struct Window *window, struct Requester *requester, ULONG flags, ULONG horizPot, ULONG vertPot, ULONG horizBody, ULONG vertBody );
void MoveScreen( struct Screen *screen, LONG dx, LONG dy );
void MoveWindow( struct Window *window, LONG dx, LONG dy );
void OffGadget( struct Gadget *gadget, struct Window *window, struct Requester *requester );
void OffMenu( struct Window *window, ULONG menuNumber );
void OnGadget( struct Gadget *gadget, struct Window *window, struct Requester *requester );
void OnMenu( struct Window *window, ULONG menuNumber );
struct Screen *OpenScreen( const struct NewScreen *newScreen );
struct Window *OpenWindow( const struct NewWindow *newWindow );
ULONG OpenWorkBench( void );
void PrintIText( struct RastPort *rp, const struct IntuiText *iText, LONG left, LONG top );
void RefreshGadgets( struct Gadget *gadgets, struct Window *window, struct Requester *requester );
UWORD RemoveGadget( struct Window *window, struct Gadget *gadget );
/* The official calling sequence for ReportMouse is given below. */
/* Note the register order.  For the complete story, read the ReportMouse */
/* autodoc. */
void ReportMouse( LONG flag, struct Window *window );
void ReportMouse1( struct Window *window, LONG flag );
BOOL Request( struct Requester *requester, struct Window *window );
void ScreenToBack( struct Screen *screen );
void ScreenToFront( struct Screen *screen );
BOOL SetDMRequest( struct Window *window, struct Requester *requester );
BOOL SetMenuStrip( struct Window *window, struct Menu *menu );
void SetPointer( struct Window *window, UWORD *pointer, LONG height, LONG width, LONG xOffset, LONG yOffset );
void SetWindowTitles( struct Window *window, CONST_STRPTR windowTitle, CONST_STRPTR screenTitle );
void ShowTitle( struct Screen *screen, LONG showIt );
void SizeWindow( struct Window *window, LONG dx, LONG dy );
struct View *ViewAddress( void );
struct ViewPort *ViewPortAddress( const struct Window *window );
void WindowToBack( struct Window *window );
void WindowToFront( struct Window *window );
BOOL WindowLimits( struct Window *window, LONG widthMin, LONG heightMin, ULONG widthMax, ULONG heightMax );
/*--- start of next generation of names -------------------------------------*/
struct Preferences *SetPrefs( const struct Preferences *preferences, LONG size, LONG inform );
/*--- start of next next generation of names --------------------------------*/
LONG IntuiTextLength( const struct IntuiText *iText );
BOOL WBenchToBack( void );
BOOL WBenchToFront( void );
/*--- start of next next next generation of names ---------------------------*/
BOOL AutoRequest( struct Window *window, const struct IntuiText *body, const struct IntuiText *posText, const struct IntuiText *negText, ULONG pFlag, ULONG nFlag, ULONG width, ULONG height );
void BeginRefresh( struct Window *window );
struct Window *BuildSysRequest( struct Window *window, const struct IntuiText *body, const struct IntuiText *posText, const struct IntuiText *negText, ULONG flags, ULONG width, ULONG height );
void EndRefresh( struct Window *window, LONG complete );
void FreeSysRequest( struct Window *window );
/* The return codes for MakeScreen(), RemakeDisplay(), and RethinkDisplay() */
/* are only valid under V39 and greater.  Do not examine them when running */
/* on pre-V39 systems! */
LONG MakeScreen( struct Screen *screen );
LONG RemakeDisplay( void );
LONG RethinkDisplay( void );
/*--- start of next next next next generation of names ----------------------*/
APTR AllocRemember( struct Remember **rememberKey, ULONG size, ULONG flags );
/* Public function AlohaWorkbench() is intentionally not documented */
void AlohaWorkbench( LONG wbport );
void FreeRemember( struct Remember **rememberKey, LONG reallyForget );
/*--- start of 15 Nov 85 names ------------------------*/
ULONG LockIBase( ULONG dontknow );
void UnlockIBase( ULONG ibLock );
/*--- functions in V33 or higher (Release 1.2) ---*/
LONG GetScreenData( APTR buffer, ULONG size, ULONG type, const struct Screen *screen );
void RefreshGList( struct Gadget *gadgets, struct Window *window, struct Requester *requester, LONG numGad );
UWORD AddGList( struct Window *window, struct Gadget *gadget, ULONG position, LONG numGad, struct Requester *requester );
UWORD RemoveGList( struct Window *remPtr, struct Gadget *gadget, LONG numGad );
void ActivateWindow( struct Window *window );
void RefreshWindowFrame( struct Window *window );
BOOL ActivateGadget( struct Gadget *gadgets, struct Window *window, struct Requester *requester );
void NewModifyProp( struct Gadget *gadget, struct Window *window, struct Requester *requester, ULONG flags, ULONG horizPot, ULONG vertPot, ULONG horizBody, ULONG vertBody, LONG numGad );
/*--- functions in V36 or higher (Release 2.0) ---*/
LONG QueryOverscan( ULONG displayID, struct Rectangle *rect, LONG oScanType );
void MoveWindowInFrontOf( struct Window *window, struct Window *behindWindow );
void ChangeWindowBox( struct Window *window, LONG left, LONG top, LONG width, LONG height );
struct Hook *SetEditHook( struct Hook *hook );
LONG SetMouseQueue( struct Window *window, ULONG queueLength );
void ZipWindow( struct Window *window );
/*--- public screens ---*/
struct Screen *LockPubScreen( CONST_STRPTR name );
void UnlockPubScreen( CONST_STRPTR name, struct Screen *screen );
struct List *LockPubScreenList( void );
void UnlockPubScreenList( void );
STRPTR NextPubScreen( const struct Screen *screen, STRPTR namebuf );
void SetDefaultPubScreen( CONST_STRPTR name );
UWORD SetPubScreenModes( ULONG modes );
UWORD PubScreenStatus( struct Screen *screen, ULONG statusFlags );

struct RastPort *ObtainGIRPort( struct GadgetInfo *gInfo );
void ReleaseGIRPort( struct RastPort *rp );
void GadgetMouse( struct Gadget *gadget, struct GadgetInfo *gInfo, WORD *mousePoint );
void GetDefaultPubScreen( STRPTR nameBuffer );
LONG EasyRequestArgs( struct Window *window, const struct EasyStruct *easyStruct, ULONG *idcmpPtr, CONST_APTR args );
LONG EasyRequest( struct Window *window, const struct EasyStruct *easyStruct, ULONG *idcmpPtr, ... );
struct Window *BuildEasyRequestArgs( struct Window *window, const struct EasyStruct *easyStruct, ULONG idcmp, CONST_APTR args );
struct Window *BuildEasyRequest( struct Window *window, const struct EasyStruct *easyStruct, ULONG idcmp, ... );
LONG SysReqHandler( struct Window *window, ULONG *idcmpPtr, LONG waitInput );
struct Window *OpenWindowTagList( const struct NewWindow *newWindow, const struct TagItem *tagList );
struct Window *OpenWindowTags( const struct NewWindow *newWindow, ULONG tag1Type, ... );
struct Screen *OpenScreenTagList( const struct NewScreen *newScreen, const struct TagItem *tagList );
struct Screen *OpenScreenTags( const struct NewScreen *newScreen, ULONG tag1Type, ... );

/*	new Image functions */
void DrawImageState( struct RastPort *rp, const struct Image *image, LONG leftOffset, LONG topOffset, ULONG state, struct DrawInfo *drawInfo );
BOOL PointInImage( ULONG point, const struct Image *image );
void EraseImage( struct RastPort *rp, const struct Image *image, LONG leftOffset, LONG topOffset );

APTR NewObjectA( struct IClass *classPtr, CONST_STRPTR classID, const struct TagItem *tagList );
APTR NewObject( struct IClass *classPtr, CONST_STRPTR classID, ULONG tag1, ... );

void DisposeObject( APTR object );
ULONG SetAttrsA( APTR object, const struct TagItem *tagList );
ULONG SetAttrs( APTR object, ULONG tag1, ... );

ULONG GetAttr( ULONG attrID, APTR object, ULONG *storagePtr );

/* 	special set attribute call for gadgets */
ULONG SetGadgetAttrsA( struct Gadget *gadget, struct Window *window, struct Requester *requester, const struct TagItem *tagList );
ULONG SetGadgetAttrs( struct Gadget *gadget, struct Window *window, struct Requester *requester, ULONG tag1, ... );

/*	for class implementors only */
APTR NextObject( CONST_APTR objectPtrPtr );
struct IClass *MakeClass( CONST_STRPTR classID, CONST_STRPTR superClassID, const struct IClass *superClassPtr, ULONG instanceSize, ULONG flags );
void AddClass( struct IClass *classPtr );


struct DrawInfo *GetScreenDrawInfo( struct Screen *screen );
void FreeScreenDrawInfo( struct Screen *screen, struct DrawInfo *drawInfo );

BOOL ResetMenuStrip( struct Window *window, struct Menu *menu );
void RemoveClass( struct IClass *classPtr );
BOOL FreeClass( struct IClass *classPtr );
/*--- functions in V39 or higher (Release 3.0) ---*/
struct ScreenBuffer *AllocScreenBuffer( struct Screen *sc, struct BitMap *bm, ULONG flags );
void FreeScreenBuffer( struct Screen *sc, struct ScreenBuffer *sb );
ULONG ChangeScreenBuffer( struct Screen *sc, struct ScreenBuffer *sb );
void ScreenDepth( struct Screen *screen, ULONG flags, APTR reserved );
void ScreenPosition( struct Screen *screen, ULONG flags, LONG x1, LONG y1, LONG x2, LONG y2 );
void ScrollWindowRaster( struct Window *win, LONG dx, LONG dy, LONG xMin, LONG yMin, LONG xMax, LONG yMax );
void LendMenus( struct Window *fromwindow, struct Window *towindow );
ULONG DoGadgetMethodA( struct Gadget *gad, struct Window *win, struct Requester *req, Msg message );
ULONG DoGadgetMethod( struct Gadget *gad, struct Window *win, struct Requester *req, ULONG methodID, ... );
void SetWindowPointerA( struct Window *win, const struct TagItem *taglist );
void SetWindowPointer( struct Window *win, ULONG tag1, ... );
BOOL TimedDisplayAlert( ULONG alertNumber, CONST_STRPTR string, ULONG height, ULONG time );
void HelpControl( struct Window *win, ULONG flags );
/*--- functions in V46 or higher (Release 3.1.4) ---*/
BOOL ShowWindow( struct Window *window, struct Window *other );
BOOL HideWindow( struct Window *window );
/*--- functions in V47 or higher (Release 3.2) ---*/
ULONG IntuitionControlA( APTR object, const struct TagItem *taglist );
ULONG IntuitionControl( APTR object, ... );

/* "intuition.library" */
/* Public functions OpenIntuition() and Intuition() are intentionally */
/* not documented. */
#pragma libcall IntuitionBase OpenIntuition 1e 00
#pragma libcall IntuitionBase Intuition 24 801
#pragma libcall IntuitionBase AddGadget 2a 09803
#pragma libcall IntuitionBase ClearDMRequest 30 801
#pragma libcall IntuitionBase ClearMenuStrip 36 801
#pragma libcall IntuitionBase ClearPointer 3c 801
#pragma libcall IntuitionBase CloseScreen 42 801
#pragma libcall IntuitionBase CloseWindow 48 801
#pragma libcall IntuitionBase CloseWorkBench 4e 00
#pragma libcall IntuitionBase CurrentTime 54 9802
#pragma libcall IntuitionBase DisplayAlert 5a 18003
#pragma libcall IntuitionBase DisplayBeep 60 801
#pragma libcall IntuitionBase DoubleClick 66 321004
#pragma libcall IntuitionBase DrawBorder 6c 109804
#pragma libcall IntuitionBase DrawImage 72 109804
#pragma libcall IntuitionBase EndRequest 78 9802
#pragma libcall IntuitionBase GetDefPrefs 7e 0802
#pragma libcall IntuitionBase GetPrefs 84 0802
#pragma libcall IntuitionBase InitRequester 8a 801
#pragma libcall IntuitionBase ItemAddress 90 0802
#pragma libcall IntuitionBase ModifyIDCMP 96 0802
#pragma libcall IntuitionBase ModifyProp 9c 43210A9808
#pragma libcall IntuitionBase MoveScreen a2 10803
#pragma libcall IntuitionBase MoveWindow a8 10803
#pragma libcall IntuitionBase OffGadget ae A9803
#pragma libcall IntuitionBase OffMenu b4 0802
#pragma libcall IntuitionBase OnGadget ba A9803
#pragma libcall IntuitionBase OnMenu c0 0802
#pragma libcall IntuitionBase OpenScreen c6 801
#pragma libcall IntuitionBase OpenWindow cc 801
#pragma libcall IntuitionBase OpenWorkBench d2 00
#pragma libcall IntuitionBase PrintIText d8 109804
#pragma libcall IntuitionBase RefreshGadgets de A9803
#pragma libcall IntuitionBase RemoveGadget e4 9802
/* The official calling sequence for ReportMouse is given below. */
/* Note the register order.  For the complete story, read the ReportMouse */
/* autodoc. */
#pragma libcall IntuitionBase ReportMouse ea 8002
#pragma libcall IntuitionBase ReportMouse1 ea 0802
#pragma libcall IntuitionBase Request f0 9802
#pragma libcall IntuitionBase ScreenToBack f6 801
#pragma libcall IntuitionBase ScreenToFront fc 801
#pragma libcall IntuitionBase SetDMRequest 102 9802
#pragma libcall IntuitionBase SetMenuStrip 108 9802
#pragma libcall IntuitionBase SetPointer 10e 32109806
#pragma libcall IntuitionBase SetWindowTitles 114 A9803
#pragma libcall IntuitionBase ShowTitle 11a 0802
#pragma libcall IntuitionBase SizeWindow 120 10803
#pragma libcall IntuitionBase ViewAddress 126 00
#pragma libcall IntuitionBase ViewPortAddress 12c 801
#pragma libcall IntuitionBase WindowToBack 132 801
#pragma libcall IntuitionBase WindowToFront 138 801
#pragma libcall IntuitionBase WindowLimits 13e 3210805
/*--- start of next generation of names -------------------------------------*/
#pragma libcall IntuitionBase SetPrefs 144 10803
/*--- start of next next generation of names --------------------------------*/
#pragma libcall IntuitionBase IntuiTextLength 14a 801
#pragma libcall IntuitionBase WBenchToBack 150 00
#pragma libcall IntuitionBase WBenchToFront 156 00
/*--- start of next next next generation of names ---------------------------*/
#pragma libcall IntuitionBase AutoRequest 15c 3210BA9808
#pragma libcall IntuitionBase BeginRefresh 162 801
#pragma libcall IntuitionBase BuildSysRequest 168 210BA9807
#pragma libcall IntuitionBase EndRefresh 16e 0802
#pragma libcall IntuitionBase FreeSysRequest 174 801
/* The return codes for MakeScreen(), RemakeDisplay(), and RethinkDisplay() */
/* are only valid under V39 and greater.  Do not examine them when running */
/* on pre-V39 systems! */
#pragma libcall IntuitionBase MakeScreen 17a 801
#pragma libcall IntuitionBase RemakeDisplay 180 00
#pragma libcall IntuitionBase RethinkDisplay 186 00
/*--- start of next next next next generation of names ----------------------*/
#pragma libcall IntuitionBase AllocRemember 18c 10803
/* Public function AlohaWorkbench() is intentionally not documented */
#pragma libcall IntuitionBase AlohaWorkbench 192 801
#pragma libcall IntuitionBase FreeRemember 198 0802
/*--- start of 15 Nov 85 names ------------------------*/
#pragma libcall IntuitionBase LockIBase 19e 001
#pragma libcall IntuitionBase UnlockIBase 1a4 801
/*--- functions in V33 or higher (Release 1.2) ---*/
#pragma libcall IntuitionBase GetScreenData 1aa 910804
#pragma libcall IntuitionBase RefreshGList 1b0 0A9804
#pragma libcall IntuitionBase AddGList 1b6 A109805
#pragma libcall IntuitionBase RemoveGList 1bc 09803
#pragma libcall IntuitionBase ActivateWindow 1c2 801
#pragma libcall IntuitionBase RefreshWindowFrame 1c8 801
#pragma libcall IntuitionBase ActivateGadget 1ce A9803
#pragma libcall IntuitionBase NewModifyProp 1d4 543210A9809
/*--- functions in V36 or higher (Release 2.0) ---*/
#pragma libcall IntuitionBase QueryOverscan 1da 09803
#pragma libcall IntuitionBase MoveWindowInFrontOf 1e0 9802
#pragma libcall IntuitionBase ChangeWindowBox 1e6 3210805
#pragma libcall IntuitionBase SetEditHook 1ec 801
#pragma libcall IntuitionBase SetMouseQueue 1f2 0802
#pragma libcall IntuitionBase ZipWindow 1f8 801
/*--- public screens ---*/
#pragma libcall IntuitionBase LockPubScreen 1fe 801
#pragma libcall IntuitionBase UnlockPubScreen 204 9802
#pragma libcall IntuitionBase LockPubScreenList 20a 00
#pragma libcall IntuitionBase UnlockPubScreenList 210 00
#pragma libcall IntuitionBase NextPubScreen 216 9802
#pragma libcall IntuitionBase SetDefaultPubScreen 21c 801
#pragma libcall IntuitionBase SetPubScreenModes 222 001
#pragma libcall IntuitionBase PubScreenStatus 228 0802
#pragma libcall IntuitionBase ObtainGIRPort 22e 801
#pragma libcall IntuitionBase ReleaseGIRPort 234 801
#pragma libcall IntuitionBase GadgetMouse 23a A9803
/* system private and not to be used by applications: */
#pragma libcall IntuitionBase GetDefaultPubScreen 246 801
#pragma libcall IntuitionBase EasyRequestArgs 24c BA9804
#pragma tagcall IntuitionBase EasyRequest 24c BA9804
#pragma libcall IntuitionBase BuildEasyRequestArgs 252 B09804
#pragma tagcall IntuitionBase BuildEasyRequest 252 B09804
#pragma libcall IntuitionBase SysReqHandler 258 09803
#pragma libcall IntuitionBase OpenWindowTagList 25e 9802
#pragma tagcall IntuitionBase OpenWindowTags 25e 9802
#pragma libcall IntuitionBase OpenScreenTagList 264 9802
#pragma tagcall IntuitionBase OpenScreenTags 264 9802
/*	new Image functions */
#pragma libcall IntuitionBase DrawImageState 26a A2109806
#pragma libcall IntuitionBase PointInImage 270 8002
#pragma libcall IntuitionBase EraseImage 276 109804
#pragma libcall IntuitionBase NewObjectA 27c A9803
#pragma tagcall IntuitionBase NewObject 27c A9803
#pragma libcall IntuitionBase DisposeObject 282 801
#pragma libcall IntuitionBase SetAttrsA 288 9802
#pragma tagcall IntuitionBase SetAttrs 288 9802
#pragma libcall IntuitionBase GetAttr 28e 98003
/* 	special set attribute call for gadgets */
#pragma libcall IntuitionBase SetGadgetAttrsA 294 BA9804
#pragma tagcall IntuitionBase SetGadgetAttrs 294 BA9804
/*	for class implementors only */
#pragma libcall IntuitionBase NextObject 29a 801
#pragma libcall IntuitionBase MakeClass 2a6 10A9805
#pragma libcall IntuitionBase AddClass 2ac 801
#pragma libcall IntuitionBase GetScreenDrawInfo 2b2 801
#pragma libcall IntuitionBase FreeScreenDrawInfo 2b8 9802
#pragma libcall IntuitionBase ResetMenuStrip 2be 9802
#pragma libcall IntuitionBase RemoveClass 2c4 801
#pragma libcall IntuitionBase FreeClass 2ca 801
/* Six spare vectors */
/*--- (6 function slots reserved here) ---*/
/*--- functions in V39 or higher (Release 3.0) ---*/
#pragma libcall IntuitionBase AllocScreenBuffer 300 09803
#pragma libcall IntuitionBase FreeScreenBuffer 306 9802
#pragma libcall IntuitionBase ChangeScreenBuffer 30c 9802
#pragma libcall IntuitionBase ScreenDepth 312 90803
#pragma libcall IntuitionBase ScreenPosition 318 43210806
#pragma libcall IntuitionBase ScrollWindowRaster 31e 543210907
#pragma libcall IntuitionBase LendMenus 324 9802
#pragma libcall IntuitionBase DoGadgetMethodA 32a BA9804
#pragma tagcall IntuitionBase DoGadgetMethod 32a BA9804
#pragma libcall IntuitionBase SetWindowPointerA 330 9802
#pragma tagcall IntuitionBase SetWindowPointer 330 9802
#pragma libcall IntuitionBase TimedDisplayAlert 336 918004
#pragma libcall IntuitionBase HelpControl 33c 0802
/*--- functions in V46 or higher (Release 3.1.4) ---*/
#pragma libcall IntuitionBase ShowWindow 342 9802
#pragma libcall IntuitionBase HideWindow 348 801
/*--- functions in V47 or higher (Release 3.2) ---*/
/*--- (61 function slots reserved here) ---*/
#pragma libcall IntuitionBase IntuitionControlA 4bc 9802
#pragma tagcall IntuitionBase IntuitionControl 4bc 9802
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: graphics.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: gfxbase.h 47.1 (30.7.2019)
**
**	graphics base definitions
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
**
*/

struct GfxBase
{
	struct	Library  LibNode;
	struct	View *ActiView;
	struct	copinit *copinit;	/* ptr to copper start up list */
	LONG	*cia;			/* for 8520 resource use */
	LONG	*blitter;		/* for future blitter resource use */
	UWORD	*LOFlist;
	UWORD	*SHFlist;
	struct	bltnode *blthd,*blttl;
	struct	bltnode *bsblthd,*bsblttl;
	struct	Interrupt vbsrv,timsrv,bltsrv;
	struct	List     TextFonts;
	struct	TextFont *DefaultFont;
	UWORD	Modes;			/* copy of current first bplcon0 */
	BYTE	VBlank;
	BYTE	Debug;
	WORD	BeamSync;
	WORD	system_bplcon0;		/* it is ored into each bplcon0 for display */
	UBYTE	SpriteReserved;
	UBYTE	bytereserved;
	UWORD	Flags;
	WORD	BlitLock;
	WORD	BlitNest;

	struct	List BlitWaitQ;
	struct	Task *BlitOwner;
	struct	List TOF_WaitQ;
	UWORD	DisplayFlags;		/* NTSC PAL GENLOC etc*/
					/* flags initialized at power on */
	struct	SimpleSprite **SimpleSprites;
	UWORD	MaxDisplayRow;		/* hardware stuff, do not use */
	UWORD	MaxDisplayColumn;	/* hardware stuff, do not use */	
	UWORD	NormalDisplayRows;
	UWORD	NormalDisplayColumns;
	/* the following are for standard non interlace, 1/2 wb width */
	UWORD	NormalDPMX;		/* Dots per meter on display */
	UWORD	NormalDPMY;		/* Dots per meter on display */
	struct	SignalSemaphore *LastChanceMemory;
	UWORD	*LCMptr;
	UWORD	MicrosPerLine;		/* 256 time usec/line */
	UWORD	MinDisplayColumn;
	UBYTE	ChipRevBits0;
	UBYTE	MemType;
	UBYTE	crb_reserved[4];
	UWORD	monitor_id;
	ULONG	hedley[8];
	ULONG	hedley_sprites[8];	/* sprite ptrs for intuition mouse */
	ULONG	hedley_sprites1[8];	/* sprite ptrs for intuition mouse */
	WORD	hedley_count;
	UWORD	hedley_flags;
	WORD	hedley_tmp;
	LONG	*hash_table;
	UWORD	current_tot_rows;
	UWORD	current_tot_cclks;
	UBYTE	hedley_hint;
	UBYTE	hedley_hint2;
	ULONG	nreserved[4];
	LONG	*a2024_sync_raster;
	UWORD	control_delta_pal;
	UWORD	control_delta_ntsc;
	struct	MonitorSpec *current_monitor;
	struct	List MonitorList;
	struct	MonitorSpec *default_monitor;
	struct	SignalSemaphore *MonitorListSemaphore;
	void	*DisplayInfoDataBase;
	UWORD	TopLine;
	struct	SignalSemaphore *ActiViewCprSemaphore;
        struct  Library 	*UtilBase; /* for hook and tag utilities. had to change because of name clash 	*/
        struct  Library	        *ExecBase; /* to link with rom.lib 	*/
	UBYTE	*bwshifts;
	UWORD	*StrtFetchMasks;
	UWORD	*StopFetchMasks;
	UWORD	*Overrun;
	WORD	*RealStops;
	UWORD	SpriteWidth;	/* current width (in words) of sprites */
	UWORD	SpriteFMode;		/* current sprite fmode bits	*/
	BYTE	SoftSprites;	/* bit mask of size change knowledgeable sprites */
	BYTE	arraywidth;
	UWORD	DefaultSpriteWidth;	/* what width intuition wants */
	BYTE	SprMoveDisable;
	UBYTE   WantChips;
	UBYTE	BoardMemType;
	UBYTE	Bugs;
	ULONG	*gb_LayersBase;
	ULONG 	ColorMask;
	APTR	IVector;
	APTR	IData;
	ULONG	SpecialCounter;		/* special for double buffering */
	APTR	DBList;
	UWORD	MonitorFlags;
	UBYTE	ScanDoubledSprites;
	UBYTE	BP3Bits;
	struct	AnalogSignalInterval MonitorVBlank;
	struct	MonitorSpec *natural_monitor;
	APTR	ProgData;
	UBYTE	ExtSprites;
	UBYTE	pad3;
	UWORD	GfxFlags;
	ULONG	VBCounter;
	struct	SignalSemaphore *HashTableSemaphore;
	ULONG	*HWEmul[9];
        struct  RegionRectangle *Scratch;
        ULONG   ScratchSize;
};

/* Values for GfxBase->DisplayFlags */
				/* LightPen software could set this bit if the
				 * "lpen-with-interlace" fix put in for V39
				 * does not work. This is true of a number of
				 * Agnus chips.
				 * (V40).
				 */

/* bits defs for ChipRevBits */
/* Pass ONE of these to SetChipRev() */
/* memory type */
/* GfxFlags (private) */
extern struct GfxBase * GfxBase;
/****************************************************************************/

/*
**	$VER: graphics_pragmas.h 47.2 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: graphics_protos.h 47.2 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: gels.h 47.1 (28.7.2019)
**
**	include file for AMIGA GELS (Graphics Elements)
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* VSprite flags */
/* user-set VSprite flags: */
/* system-set VSprite flags: */
/* Bob flags */
/* these are the user flag bits */
/* these are the system flag bits */
/* defines for the animation procedures */
/* UserStuff definitions
 *  the user can define these to be a single variable or a sub-structure
 *  if undefined by the user, the system turns these into innocuous variables
 *  see the manual for a thorough definition of the UserStuff definitions
 *
 */
/*********************** GEL STRUCTURES ***********************************/

struct VSprite
{
/* --------------------- SYSTEM VARIABLES ------------------------------- */
/* GEL linked list forward/backward pointers sorted by y,x value */
    struct VSprite   *NextVSprite;
    struct VSprite   *PrevVSprite;

/* GEL draw list constructed in the order the Bobs are actually drawn, then
 *  list is copied to clear list
 *  must be here in VSprite for system boundary detection
 */
    struct VSprite   *DrawPath;     /* pointer of overlay drawing */
    struct VSprite   *ClearPath;    /* pointer for overlay clearing */

/* the VSprite positions are defined in (y,x) order to make sorting
 *  sorting easier, since (y,x) as a long integer
 */
    WORD OldY, OldX;	      /* previous position */

/* --------------------- COMMON VARIABLES --------------------------------- */
    WORD Flags;	      /* VSprite flags */


/* --------------------- USER VARIABLES ----------------------------------- */
/* the VSprite positions are defined in (y,x) order to make sorting
 *  sorting easier, since (y,x) as a long integer
 */
    WORD Y, X;		      /* screen position */

    WORD Height;
    WORD Width;	      /* number of words per row of image data */
    WORD Depth;	      /* number of planes of data */

    WORD MeMask;	      /* which types can collide with this VSprite*/
    WORD HitMask;	      /* which types this VSprite can collide with*/

    WORD *ImageData;	      /* pointer to VSprite image */

/* borderLine is the one-dimensional logical OR of all
 *  the VSprite bits, used for fast collision detection of edge
 */
    WORD *BorderLine;	      /* logical OR of all VSprite bits */
    WORD *CollMask;	      /* similar to above except this is a matrix */

/* pointer to this VSprite's color definitions (not used by Bobs) */
    WORD *SprColors;

    struct Bob *VSBob;	      /* points home if this VSprite is part of
				   a Bob */

/* planePick flag:  set bit selects a plane from image, clear bit selects
 *  use of shadow mask for that plane
 * OnOff flag: if using shadow mask to fill plane, this bit (corresponding
 *  to bit in planePick) describes whether to fill with 0's or 1's
 * There are two uses for these flags:
 *	- if this is the VSprite of a Bob, these flags describe how the Bob
 *	  is to be drawn into memory
 *	- if this is a simple VSprite and the user intends on setting the
 *	  MUSTDRAW flag of the VSprite, these flags must be set too to describe
 *	  which color registers the user wants for the image
 */
    BYTE PlanePick;
    BYTE PlaneOnOff;

    WORD VUserExt;      /* user definable:  see note above */
};

struct Bob
/* blitter-objects */
{
/* --------------------- SYSTEM VARIABLES --------------------------------- */

/* --------------------- COMMON VARIABLES --------------------------------- */
    WORD Flags;	/* general purpose flags (see definitions below) */

/* --------------------- USER VARIABLES ----------------------------------- */
    WORD *SaveBuffer;	/* pointer to the buffer for background save */

/* used by Bobs for "cookie-cutting" and multi-plane masking */
    WORD *ImageShadow;

/* pointer to BOBs for sequenced drawing of Bobs
 *  for correct overlaying of multiple component animations
 */
    struct Bob *Before; /* draw this Bob before Bob pointed to by before */
    struct Bob *After;	/* draw this Bob after Bob pointed to by after */

    struct VSprite   *BobVSprite;   /* this Bob's VSprite definition */

    struct AnimComp  *BobComp;	    /* pointer to this Bob's AnimComp def */

    struct DBufPacket *DBuffer;     /* pointer to this Bob's dBuf packet */

    WORD BUserExt;	    /* Bob user extension */
};

struct AnimComp
{
/* --------------------- SYSTEM VARIABLES --------------------------------- */

/* --------------------- COMMON VARIABLES --------------------------------- */
    WORD Flags;		    /* AnimComp flags for system & user */

/* timer defines how long to keep this component active:
 *  if set non-zero, timer decrements to zero then switches to nextSeq
 *  if set to zero, AnimComp never switches
 */
    WORD Timer;

/* --------------------- USER VARIABLES ----------------------------------- */
/* initial value for timer when the AnimComp is activated by the system */
    WORD TimeSet;

/* pointer to next and previous components of animation object */
    struct AnimComp  *NextComp;
    struct AnimComp  *PrevComp;

/* pointer to component component definition of next image in sequence */
    struct AnimComp  *NextSeq;
    struct AnimComp  *PrevSeq;

/* address of special animation procedure */
    WORD (* __stdargs AnimCRoutine) (struct AnimComp *);

    WORD YTrans;     /* initial y translation (if this is a component) */
    WORD XTrans;     /* initial x translation (if this is a component) */

    struct AnimOb    *HeadOb;

    struct Bob	     *AnimBob;
};

struct AnimOb
{
/* --------------------- SYSTEM VARIABLES --------------------------------- */
    struct AnimOb    *NextOb, *PrevOb;

/* number of calls to Animate this AnimOb has endured */
    LONG Clock;

    WORD AnOldY, AnOldX;	    /* old y,x coordinates */

/* --------------------- COMMON VARIABLES --------------------------------- */
    WORD AnY, AnX;		    /* y,x coordinates of the AnimOb */

/* --------------------- USER VARIABLES ----------------------------------- */
    WORD YVel, XVel;		    /* velocities of this object */
    WORD YAccel, XAccel;	    /* accelerations of this object */

    WORD RingYTrans, RingXTrans;    /* ring translation values */

    				    /* address of special animation
				       procedure */
    WORD (* __stdargs AnimORoutine) (struct AnimOb *);

    struct AnimComp  *HeadComp;     /* pointer to first component */

    WORD AUserExt;	    /* AnimOb user extension */
};

/* dBufPacket defines the values needed to be saved across buffer to buffer
 *  when in double-buffer mode
 */
struct DBufPacket
{
    WORD BufY, BufX;		    /* save the other buffers screen coordinates */
    struct VSprite   *BufPath;	    /* carry the draw path over the gap */

/* these pointers must be filled in by the user */
/* pointer to other buffer's background save buffer */
    WORD *BufBuffer;
};



/* ************************************************************************ */

/* these are GEL functions that are currently simple enough to exist as a
 *  definition.  It should not be assumed that this will always be the case
 */
/* ************************************************************************ */

/* ************************************************************************ */

/* a structure to contain the 16 collision procedure addresses */
struct collTable
{
    /* NOTE: This table actually consists of two different types of
     *       pointers. The first table entry is for collision testing,
     *       the other are for reporting collisions. The first function
     *       pointer looks like this:
     *
     *          LONG (*collPtrs[0])(struct VSprite *,WORD);
     *
     *       The remaining 15 function pointers look like this:
     *
     *          VOID (*collPtrs[1..15])(struct VSprite *,struct VSPrite *);
     */
    LONG (* __stdargs collPtrs[16]) (struct VSprite *,struct VSprite *);
};

/*
**	$VER: regions.h 47.1 (31.7.2019)
**
**	damage region management
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct RegionRectangle
{
    struct RegionRectangle *Next,*Prev;
    struct Rectangle bounds;
};

struct Region
{
    struct Rectangle bounds;
    struct RegionRectangle *RegionRectangle;
};

/*
**	$VER: sprite.h 47.1 (31.7.2019)
**
**	SimpleSprite management
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct SimpleSprite
{
    UWORD *posctldata;
    UWORD height;
    UWORD   x,y;    /* current position */
    UWORD   num;
};

struct ExtSprite
{
	struct SimpleSprite es_SimpleSprite;	/* conventional simple sprite structure */
	UWORD	es_wordwidth;			/* graphics use only, subject to change */
	UWORD	es_flags;			/* graphics use only, subject to change */
};



/* tags for AllocSpriteData() */
/* tags for GetExtSprite() */
/* tags valid for either GetExtSprite or ChangeExtSprite */
/*
**	$VER: blit.h 47.2 (26.12.2021)
**
**	Defines for direct hardware use of the blitter.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* all agnii support horizontal blit of at least 1024 bits (128 bytes) wide */
/* some agnii support horizontal blit of up to 32768 bits (4096 bytes) wide */

/* definitions for blitter control register 0 */

/* some commonly used operations */
/* definations for blitter control register 1 */
/* stuff for blit qeuer */
struct bltnode
{
    struct  bltnode *n;
    int     (*function)();
    char    stat;
    short   blitsize;
    short   beamsync;
    int     (*cleanup)();
};

/* defined bits for bltstat */
/*
**	$VER: scale.h 47.1 (31.7.2019)
**
**	structure argument to BitMapScale()
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct BitScaleArgs {
    UWORD   bsa_SrcX, bsa_SrcY;			/* source origin */
    UWORD   bsa_SrcWidth, bsa_SrcHeight;	/* source size */
    UWORD   bsa_XSrcFactor, bsa_YSrcFactor;	/* scale factor denominators */
    UWORD   bsa_DestX, bsa_DestY;		/* destination origin */
    UWORD   bsa_DestWidth, bsa_DestHeight;	/* destination size result */
    UWORD   bsa_XDestFactor, bsa_YDestFactor;	/* scale factor numerators */
    struct BitMap *bsa_SrcBitMap;		/* source BitMap */
    struct BitMap *bsa_DestBitMap;		/* destination BitMap */
    ULONG   bsa_Flags;				/* reserved.  Must be zero! */
    UWORD   bsa_XDDA, bsa_YDDA;			/* reserved */
    LONG    bsa_Reserved1;
    LONG    bsa_Reserved2;
};
/*------ BitMap primitives ------*/
LONG BltBitMap( const struct BitMap *srcBitMap, LONG xSrc, LONG ySrc, struct BitMap *destBitMap, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm, ULONG mask, PLANEPTR tempA );
void BltTemplate( const PLANEPTR source, LONG xSrc, LONG srcMod, struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize );
/*------ Text routines ------*/
void ClearEOL( struct RastPort *rp );
void ClearScreen( struct RastPort *rp );
WORD TextLength( struct RastPort *rp, CONST_STRPTR string, ULONG count );
LONG Text( struct RastPort *rp, CONST_STRPTR string, ULONG count );
LONG SetFont( struct RastPort *rp, struct TextFont *textFont );
struct TextFont *OpenFont( const struct TextAttr *textAttr );
void CloseFont( struct TextFont *textFont );
ULONG AskSoftStyle( struct RastPort *rp );
ULONG SetSoftStyle( struct RastPort *rp, ULONG style, ULONG enable );
/*------	Gels routines ------*/
void AddBob( struct Bob *bob, struct RastPort *rp );
void AddVSprite( struct VSprite *vSprite, struct RastPort *rp );
void DoCollision( struct RastPort *rp );
void DrawGList( struct RastPort *rp, struct ViewPort *vp );
void InitGels( struct VSprite *head, struct VSprite *tail, struct GelsInfo *gelsInfo );
void InitMasks( struct VSprite *vSprite );
void RemIBob( struct Bob *bob, struct RastPort *rp, struct ViewPort *vp );
void RemVSprite( struct VSprite *vSprite );
void SetCollision( ULONG num, void (*routine)(struct VSprite *gelA, struct VSprite *gelB), struct GelsInfo *gelsInfo );
void SortGList( struct RastPort *rp );
void AddAnimOb( struct AnimOb *anOb, struct AnimOb **anKey, struct RastPort *rp );
void Animate( struct AnimOb **anKey, struct RastPort *rp );
BOOL GetGBuffers( struct AnimOb *anOb, struct RastPort *rp, LONG flag );
void InitGMasks( struct AnimOb *anOb );
/*------	General graphics routines ------*/
void DrawEllipse( struct RastPort *rp, LONG xCenter, LONG yCenter, LONG a, LONG b );
LONG AreaEllipse( struct RastPort *rp, LONG xCenter, LONG yCenter, LONG a, LONG b );
void LoadRGB4( struct ViewPort *vp, const UWORD *colors, LONG count );
void InitRastPort( struct RastPort *rp );
void InitVPort( struct ViewPort *vp );
ULONG MrgCop( struct View *view );
ULONG MakeVPort( struct View *view, struct ViewPort *vp );
void LoadView( struct View *view );
void WaitBlit( void );
void SetRast( struct RastPort *rp, ULONG pen );
void Move( struct RastPort *rp, LONG x, LONG y );
void Draw( struct RastPort *rp, LONG x, LONG y );
LONG AreaMove( struct RastPort *rp, LONG x, LONG y );
LONG AreaDraw( struct RastPort *rp, LONG x, LONG y );
LONG AreaEnd( struct RastPort *rp );
void WaitTOF( void );
void QBlit( struct bltnode *blit );
void InitArea( struct AreaInfo *areaInfo, APTR vectorBuffer, LONG maxVectors );
void SetRGB4( struct ViewPort *vp, LONG index, ULONG red, ULONG green, ULONG blue );
void QBSBlit( struct bltnode *blit );
void BltClear( PLANEPTR memBlock, ULONG byteCount, ULONG flags );
void RectFill( struct RastPort *rp, LONG xMin, LONG yMin, LONG xMax, LONG yMax );
void BltPattern( struct RastPort *rp, const PLANEPTR mask, LONG xMin, LONG yMin, LONG xMax, LONG yMax, ULONG maskBPR );
ULONG ReadPixel( struct RastPort *rp, LONG x, LONG y );
LONG WritePixel( struct RastPort *rp, LONG x, LONG y );
BOOL Flood( struct RastPort *rp, ULONG mode, LONG x, LONG y );
void PolyDraw( struct RastPort *rp, LONG count, const WORD *polyTable );
void SetAPen( struct RastPort *rp, ULONG pen );
void SetBPen( struct RastPort *rp, ULONG pen );
void SetDrMd( struct RastPort *rp, ULONG drawMode );
void InitView( struct View *view );
void CBump( struct UCopList *copList );
LONG CMove( struct UCopList *copList, APTR destination, LONG data );
LONG CWait( struct UCopList *copList, LONG v, LONG h );
LONG VBeamPos( void );
void InitBitMap( struct BitMap *bitMap, LONG depth, LONG width, LONG height );
void ScrollRaster( struct RastPort *rp, LONG dx, LONG dy, LONG xMin, LONG yMin, LONG xMax, LONG yMax );
void WaitBOVP( struct ViewPort *vp );
WORD GetSprite( struct SimpleSprite *sprite, LONG num );
void FreeSprite( LONG num );
void ChangeSprite( struct ViewPort *vp, struct SimpleSprite *sprite, UWORD *newData );
void MoveSprite( struct ViewPort *vp, struct SimpleSprite *sprite, LONG x, LONG y );
void LockLayerRom( struct Layer *layer );
void UnlockLayerRom( struct Layer *layer );
void SyncSBitMap( struct Layer *layer );
void CopySBitMap( struct Layer *layer );
void OwnBlitter( void );
void DisownBlitter( void );
struct TmpRas *InitTmpRas( struct TmpRas *tmpRas, PLANEPTR buffer, LONG size );
void AskFont( struct RastPort *rp, struct TextAttr *textAttr );
void AddFont( struct TextFont *textFont );
void RemFont( struct TextFont *textFont );
PLANEPTR AllocRaster( ULONG width, ULONG height );
void FreeRaster( PLANEPTR p, ULONG width, ULONG height );
void AndRectRegion( struct Region *region, const struct Rectangle *rectangle );
BOOL OrRectRegion( struct Region *region, const struct Rectangle *rectangle );
struct Region *NewRegion( void );
BOOL ClearRectRegion( struct Region *region, const struct Rectangle *rectangle );
void ClearRegion( struct Region *region );
void DisposeRegion( struct Region *region );
void FreeVPortCopLists( struct ViewPort *vp );
void FreeCopList( struct CopList *copList );
void ClipBlit( struct RastPort *srcRP, LONG xSrc, LONG ySrc, struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm );
BOOL XorRectRegion( struct Region *region, const struct Rectangle *rectangle );
void FreeCprList( struct cprlist *cprList );
struct ColorMap *GetColorMap( LONG entries );
void FreeColorMap( struct ColorMap *colorMap );
ULONG GetRGB4( struct ColorMap *colorMap, LONG entry );
void ScrollVPort( struct ViewPort *vp );
struct CopList *UCopperListInit( struct UCopList *uCopList, LONG n );
void FreeGBuffers( struct AnimOb *anOb, struct RastPort *rp, LONG flag );
void BltBitMapRastPort( const struct BitMap *srcBitMap, LONG xSrc, LONG ySrc, struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm );
BOOL OrRegionRegion( const struct Region *srcRegion, struct Region *destRegion );
BOOL XorRegionRegion( const struct Region *srcRegion, struct Region *destRegion );
BOOL AndRegionRegion( const struct Region *srcRegion, struct Region *destRegion );
void SetRGB4CM( struct ColorMap *colorMap, LONG index, ULONG red, ULONG green, ULONG blue );
void BltMaskBitMapRastPort( const struct BitMap *srcBitMap, LONG xSrc, LONG ySrc, struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm, const PLANEPTR bltMask );
BOOL AttemptLockLayerRom( struct Layer *layer );
/*--- functions in V36 or higher (Release 2.0) ---*/
APTR GfxNew( ULONG gfxNodeType );
void GfxFree( APTR gfxNodePtr );
void GfxAssociate( APTR associateNode, APTR gfxNodePtr );
void BitMapScale( struct BitScaleArgs *bitScaleArgs );
UWORD ScalerDiv( ULONG factor, ULONG numerator, ULONG denominator );
WORD TextExtent( struct RastPort *rp, CONST_STRPTR string, LONG count, struct TextExtent *textExtent );
ULONG TextFit( struct RastPort *rp, CONST_STRPTR string, ULONG strLen, const struct TextExtent *textExtent, const struct TextExtent *constrainingExtent, LONG strDirection, ULONG constrainingBitWidth, ULONG constrainingBitHeight );
APTR GfxLookUp( CONST_APTR associateNode );
BOOL VideoControl( struct ColorMap *colorMap, struct TagItem *tagarray );
BOOL VideoControlTags( struct ColorMap *colorMap, ULONG tag1Type, ... );
struct MonitorSpec *OpenMonitor( CONST_STRPTR monitorName, ULONG displayID );
BOOL CloseMonitor( struct MonitorSpec *monitorSpec );
DisplayInfoHandle FindDisplayInfo( ULONG displayID );
ULONG NextDisplayInfo( ULONG displayID );
ULONG GetDisplayInfoData( DisplayInfoHandle handle, APTR buf, ULONG size, ULONG tagID, ULONG displayID );
void FontExtent( const struct TextFont *font, struct TextExtent *fontExtent );
LONG ReadPixelLine8( struct RastPort *rp, ULONG xstart, ULONG ystart, ULONG width, UBYTE *array, struct RastPort *tempRP );
LONG WritePixelLine8( struct RastPort *rp, ULONG xstart, ULONG ystart, ULONG width, UBYTE *array, struct RastPort *tempRP );
LONG ReadPixelArray8( struct RastPort *rp, ULONG xstart, ULONG ystart, ULONG xstop, ULONG ystop, UBYTE *array, struct RastPort *temprp );
LONG WritePixelArray8( struct RastPort *rp, ULONG xstart, ULONG ystart, ULONG xstop, ULONG ystop, UBYTE *array, struct RastPort *temprp );
LONG GetVPModeID( const struct ViewPort *vp );
LONG ModeNotAvailable( ULONG modeID );
WORD WeighTAMatch( const struct TextAttr *reqTextAttr, const struct TextAttr *targetTextAttr, const struct TagItem *targetTags );
WORD WeighTAMatchTags( const struct TextAttr *reqTextAttr, const struct TextAttr *targetTextAttr, ULONG tag1Type, ... );
void EraseRect( struct RastPort *rp, LONG xMin, LONG yMin, LONG xMax, LONG yMax );
ULONG ExtendFont( struct TextFont *font, const struct TagItem *fontTags );
ULONG ExtendFontTags( struct TextFont *font, ULONG tag1Type, ... );
void StripFont( struct TextFont *font );
/*--- functions in V39 or higher (Release 3.0) ---*/
UWORD CalcIVG( struct View *v, struct ViewPort *vp );
LONG AttachPalExtra( struct ColorMap *cm, struct ViewPort *vp );
LONG ObtainBestPenA( struct ColorMap *cm, ULONG r, ULONG g, ULONG b, const struct TagItem *tags );
LONG ObtainBestPen( struct ColorMap *cm, ULONG r, ULONG g, ULONG b, ULONG tag1Type, ... );
void SetRGB32( struct ViewPort *vp, ULONG n, ULONG r, ULONG g, ULONG b );
ULONG GetAPen( struct RastPort *rp );
ULONG GetBPen( struct RastPort *rp );
ULONG GetDrMd( struct RastPort *rp );
ULONG GetOutlinePen( struct RastPort *rp );
void LoadRGB32( struct ViewPort *vp, const ULONG *table );
ULONG SetChipRev( ULONG want );
void SetABPenDrMd( struct RastPort *rp, ULONG apen, ULONG bpen, ULONG drawmode );
void GetRGB32( const struct ColorMap *cm, ULONG firstcolor, ULONG ncolors, ULONG *table );
struct BitMap *AllocBitMap( ULONG sizex, ULONG sizey, ULONG depth, ULONG flags, const struct BitMap *friend_bitmap );
void FreeBitMap( struct BitMap *bm );
LONG GetExtSpriteA( struct ExtSprite *ss, const struct TagItem *tags );
LONG GetExtSprite( struct ExtSprite *ss, ULONG tag1Type, ... );
ULONG CoerceMode( struct ViewPort *vp, ULONG monitorid, ULONG flags );
void ChangeVPBitMap( struct ViewPort *vp, struct BitMap *bm, struct DBufInfo *db );
void ReleasePen( struct ColorMap *cm, ULONG n );
ULONG ObtainPen( struct ColorMap *cm, ULONG n, ULONG r, ULONG g, ULONG b, LONG f );
ULONG GetBitMapAttr( const struct BitMap *bm, ULONG attrnum );
struct DBufInfo *AllocDBufInfo( struct ViewPort *vp );
void FreeDBufInfo( struct DBufInfo *dbi );
ULONG SetOutlinePen( struct RastPort *rp, ULONG pen );
ULONG SetWriteMask( struct RastPort *rp, ULONG msk );
void SetMaxPen( struct RastPort *rp, ULONG maxpen );
void SetRGB32CM( struct ColorMap *cm, ULONG n, ULONG r, ULONG g, ULONG b );
void ScrollRasterBF( struct RastPort *rp, LONG dx, LONG dy, LONG xMin, LONG yMin, LONG xMax, LONG yMax );
LONG FindColor( struct ColorMap *cm, ULONG r, ULONG g, ULONG b, LONG maxcolor );
struct ExtSprite *AllocSpriteDataA( const struct BitMap *bm, const struct TagItem *tags );
struct ExtSprite *AllocSpriteData( const struct BitMap *bm, ULONG tag1Type, ... );
LONG ChangeExtSpriteA( struct ViewPort *vp, struct ExtSprite *oldsprite, struct ExtSprite *newsprite, const struct TagItem *tags );
LONG ChangeExtSprite( struct ViewPort *vp, struct ExtSprite *oldsprite, struct ExtSprite *newsprite, ULONG tag1Type, ... );
void FreeSpriteData( struct ExtSprite *sp );
void SetRPAttrsA( struct RastPort *rp, const struct TagItem *tags );
void SetRPAttrs( struct RastPort *rp, ULONG tag1Type, ... );
void GetRPAttrsA( struct RastPort *rp, const struct TagItem *tags );
void GetRPAttrs( struct RastPort *rp, ULONG tag1Type, ... );
ULONG BestModeIDA( const struct TagItem *tags );
ULONG BestModeID( ULONG tag1Type, ... );
/*--- functions in V40 or higher (Release 3.1) ---*/
void WriteChunkyPixels( struct RastPort *rp, ULONG xstart, ULONG ystart, ULONG xstop, ULONG ystop, UBYTE *array, LONG bytesperrow );
/*--- functions in V47 or higher (Release 3.2) ---*/

/* "graphics.library" */
/*------ BitMap primitives ------*/
#pragma libcall GfxBase BltBitMap 1e A76543291080B
#pragma libcall GfxBase BltTemplate 24 5432910808
/*------ Text routines ------*/
#pragma libcall GfxBase ClearEOL 2a 901
#pragma libcall GfxBase ClearScreen 30 901
#pragma libcall GfxBase TextLength 36 08903
#pragma libcall GfxBase Text 3c 08903
#pragma libcall GfxBase SetFont 42 8902
#pragma libcall GfxBase OpenFont 48 801
#pragma libcall GfxBase CloseFont 4e 901
#pragma libcall GfxBase AskSoftStyle 54 901
#pragma libcall GfxBase SetSoftStyle 5a 10903
/*------	Gels routines ------*/
#pragma libcall GfxBase AddBob 60 9802
#pragma libcall GfxBase AddVSprite 66 9802
#pragma libcall GfxBase DoCollision 6c 901
#pragma libcall GfxBase DrawGList 72 8902
#pragma libcall GfxBase InitGels 78 A9803
#pragma libcall GfxBase InitMasks 7e 801
#pragma libcall GfxBase RemIBob 84 A9803
#pragma libcall GfxBase RemVSprite 8a 801
#pragma libcall GfxBase SetCollision 90 98003
#pragma libcall GfxBase SortGList 96 901
#pragma libcall GfxBase AddAnimOb 9c A9803
#pragma libcall GfxBase Animate a2 9802
#pragma libcall GfxBase GetGBuffers a8 09803
#pragma libcall GfxBase InitGMasks ae 801
/*------	General graphics routines ------*/
#pragma libcall GfxBase DrawEllipse b4 3210905
#pragma libcall GfxBase AreaEllipse ba 3210905
#pragma libcall GfxBase LoadRGB4 c0 09803
#pragma libcall GfxBase InitRastPort c6 901
#pragma libcall GfxBase InitVPort cc 801
#pragma libcall GfxBase MrgCop d2 901
#pragma libcall GfxBase MakeVPort d8 9802
#pragma libcall GfxBase LoadView de 901
#pragma libcall GfxBase WaitBlit e4 00
#pragma libcall GfxBase SetRast ea 0902
#pragma libcall GfxBase Move f0 10903
#pragma libcall GfxBase Draw f6 10903
#pragma libcall GfxBase AreaMove fc 10903
#pragma libcall GfxBase AreaDraw 102 10903
#pragma libcall GfxBase AreaEnd 108 901
#pragma libcall GfxBase WaitTOF 10e 00
#pragma libcall GfxBase QBlit 114 901
#pragma libcall GfxBase InitArea 11a 09803
#pragma libcall GfxBase SetRGB4 120 3210805
#pragma libcall GfxBase QBSBlit 126 901
#pragma libcall GfxBase BltClear 12c 10903
#pragma libcall GfxBase RectFill 132 3210905
#pragma libcall GfxBase BltPattern 138 432108907
#pragma libcall GfxBase ReadPixel 13e 10903
#pragma libcall GfxBase WritePixel 144 10903
#pragma libcall GfxBase Flood 14a 102904
#pragma libcall GfxBase PolyDraw 150 80903
#pragma libcall GfxBase SetAPen 156 0902
#pragma libcall GfxBase SetBPen 15c 0902
#pragma libcall GfxBase SetDrMd 162 0902
#pragma libcall GfxBase InitView 168 901
#pragma libcall GfxBase CBump 16e 901
#pragma libcall GfxBase CMove 174 10903
#pragma libcall GfxBase CWait 17a 10903
#pragma libcall GfxBase VBeamPos 180 00
#pragma libcall GfxBase InitBitMap 186 210804
#pragma libcall GfxBase ScrollRaster 18c 543210907
#pragma libcall GfxBase WaitBOVP 192 801
#pragma libcall GfxBase GetSprite 198 0802
#pragma libcall GfxBase FreeSprite 19e 001
#pragma libcall GfxBase ChangeSprite 1a4 A9803
#pragma libcall GfxBase MoveSprite 1aa 109804
#pragma libcall GfxBase LockLayerRom 1b0 D01
#pragma libcall GfxBase UnlockLayerRom 1b6 D01
#pragma libcall GfxBase SyncSBitMap 1bc 801
#pragma libcall GfxBase CopySBitMap 1c2 801
#pragma libcall GfxBase OwnBlitter 1c8 00
#pragma libcall GfxBase DisownBlitter 1ce 00
#pragma libcall GfxBase InitTmpRas 1d4 09803
#pragma libcall GfxBase AskFont 1da 8902
#pragma libcall GfxBase AddFont 1e0 901
#pragma libcall GfxBase RemFont 1e6 901
#pragma libcall GfxBase AllocRaster 1ec 1002
#pragma libcall GfxBase FreeRaster 1f2 10803
#pragma libcall GfxBase AndRectRegion 1f8 9802
#pragma libcall GfxBase OrRectRegion 1fe 9802
#pragma libcall GfxBase NewRegion 204 00
#pragma libcall GfxBase ClearRectRegion 20a 9802
#pragma libcall GfxBase ClearRegion 210 801
#pragma libcall GfxBase DisposeRegion 216 801
#pragma libcall GfxBase FreeVPortCopLists 21c 801
#pragma libcall GfxBase FreeCopList 222 801
#pragma libcall GfxBase ClipBlit 228 65432910809
#pragma libcall GfxBase XorRectRegion 22e 9802
#pragma libcall GfxBase FreeCprList 234 801
#pragma libcall GfxBase GetColorMap 23a 001
#pragma libcall GfxBase FreeColorMap 240 801
#pragma libcall GfxBase GetRGB4 246 0802
#pragma libcall GfxBase ScrollVPort 24c 801
#pragma libcall GfxBase UCopperListInit 252 0802
#pragma libcall GfxBase FreeGBuffers 258 09803
#pragma libcall GfxBase BltBitMapRastPort 25e 65432910809
#pragma libcall GfxBase OrRegionRegion 264 9802
#pragma libcall GfxBase XorRegionRegion 26a 9802
#pragma libcall GfxBase AndRegionRegion 270 9802
#pragma libcall GfxBase SetRGB4CM 276 3210805
#pragma libcall GfxBase BltMaskBitMapRastPort 27c A6543291080A
#pragma libcall GfxBase AttemptLockLayerRom 28e D01
/*--- functions in V36 or higher (Release 2.0) ---*/
#pragma libcall GfxBase GfxNew 294 001
#pragma libcall GfxBase GfxFree 29a 801
#pragma libcall GfxBase GfxAssociate 2a0 9802
#pragma libcall GfxBase BitMapScale 2a6 801
#pragma libcall GfxBase ScalerDiv 2ac 21003
#pragma libcall GfxBase TextExtent 2b2 A08904
#pragma libcall GfxBase TextFit 2b8 321BA08908
#pragma libcall GfxBase GfxLookUp 2be 801
#pragma libcall GfxBase VideoControl 2c4 9802
#pragma tagcall GfxBase VideoControlTags 2c4 9802
#pragma libcall GfxBase OpenMonitor 2ca 0902
#pragma libcall GfxBase CloseMonitor 2d0 801
#pragma libcall GfxBase FindDisplayInfo 2d6 001
#pragma libcall GfxBase NextDisplayInfo 2dc 001
#pragma libcall GfxBase GetDisplayInfoData 2f4 2109805
#pragma libcall GfxBase FontExtent 2fa 9802
#pragma libcall GfxBase ReadPixelLine8 300 9A210806
#pragma libcall GfxBase WritePixelLine8 306 9A210806
#pragma libcall GfxBase ReadPixelArray8 30c 9A3210807
#pragma libcall GfxBase WritePixelArray8 312 9A3210807
#pragma libcall GfxBase GetVPModeID 318 801
#pragma libcall GfxBase ModeNotAvailable 31e 001
#pragma libcall GfxBase WeighTAMatch 324 A9803
#pragma tagcall GfxBase WeighTAMatchTags 324 A9803
#pragma libcall GfxBase EraseRect 32a 3210905
#pragma libcall GfxBase ExtendFont 330 9802
#pragma tagcall GfxBase ExtendFontTags 330 9802
#pragma libcall GfxBase StripFont 336 801
/*--- functions in V39 or higher (Release 3.0) ---*/
#pragma libcall GfxBase CalcIVG 33c 9802
#pragma libcall GfxBase AttachPalExtra 342 9802
#pragma libcall GfxBase ObtainBestPenA 348 9321805
#pragma tagcall GfxBase ObtainBestPen 348 9321805
#pragma libcall GfxBase SetRGB32 354 3210805
#pragma libcall GfxBase GetAPen 35a 801
#pragma libcall GfxBase GetBPen 360 801
#pragma libcall GfxBase GetDrMd 366 801
#pragma libcall GfxBase GetOutlinePen 36c 801
#pragma libcall GfxBase LoadRGB32 372 9802
#pragma libcall GfxBase SetChipRev 378 001
#pragma libcall GfxBase SetABPenDrMd 37e 210904
#pragma libcall GfxBase GetRGB32 384 910804
#pragma libcall GfxBase AllocBitMap 396 8321005
#pragma libcall GfxBase FreeBitMap 39c 801
#pragma libcall GfxBase GetExtSpriteA 3a2 9A02
#pragma tagcall GfxBase GetExtSprite 3a2 9A02
#pragma libcall GfxBase CoerceMode 3a8 10803
#pragma libcall GfxBase ChangeVPBitMap 3ae A9803
#pragma libcall GfxBase ReleasePen 3b4 0802
#pragma libcall GfxBase ObtainPen 3ba 43210806
#pragma libcall GfxBase GetBitMapAttr 3c0 1802
#pragma libcall GfxBase AllocDBufInfo 3c6 801
#pragma libcall GfxBase FreeDBufInfo 3cc 901
#pragma libcall GfxBase SetOutlinePen 3d2 0802
#pragma libcall GfxBase SetWriteMask 3d8 0802
#pragma libcall GfxBase SetMaxPen 3de 0802
#pragma libcall GfxBase SetRGB32CM 3e4 3210805
#pragma libcall GfxBase ScrollRasterBF 3ea 543210907
#pragma libcall GfxBase FindColor 3f0 4321B05
#pragma libcall GfxBase AllocSpriteDataA 3fc 9A02
#pragma tagcall GfxBase AllocSpriteData 3fc 9A02
#pragma libcall GfxBase ChangeExtSpriteA 402 BA9804
#pragma tagcall GfxBase ChangeExtSprite 402 BA9804
#pragma libcall GfxBase FreeSpriteData 408 A01
#pragma libcall GfxBase SetRPAttrsA 40e 9802
#pragma tagcall GfxBase SetRPAttrs 40e 9802
#pragma libcall GfxBase GetRPAttrsA 414 9802
#pragma tagcall GfxBase GetRPAttrs 414 9802
#pragma libcall GfxBase BestModeIDA 41a 801
#pragma tagcall GfxBase BestModeID 41a 801
/*--- functions in V40 or higher (Release 3.1) ---*/
#pragma libcall GfxBase WriteChunkyPixels 420 4A3210807
/* Three reserved functions */
/*--- (3 function slots reserved here) ---*/
/*--- functions in V47 or higher (Release 3.2) ---*/

/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

typedef unsigned int size_t;
/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

typedef char wchar_t;
/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

typedef struct {
            int quot;
            int rem;
        } div_t;

typedef struct {
            long int quot;
            long int rem;
        } ldiv_t;

extern char __mb_cur_max;

extern double atof(const char *);
extern int atoi(const char *);
extern long int atol(const char *);

extern double strtod(const char *, char **);
extern long int strtol(const char *, char **, int);
extern unsigned long int strtoul(const char *, char **, int);


extern int rand(void);
extern void srand(unsigned int);


/***
*
*     ANSI memory management functions
*
***/

extern void *calloc(size_t, size_t);
extern void free(void *);
extern void *malloc(size_t);
extern void *realloc(void *, size_t);

extern void *halloc(unsigned long);              /*  Extension  */
extern void *__halloc(unsigned long);            /*  Extension  */
/***
*
*     ANSI environment functions
*
***/

extern void abort(void);
extern int atexit(void (*)(void));
extern void exit(int);
extern char *__getenv(const char *);
extern char *getenv(const char *);
extern int system(const char *);


/***
*
*     ANSI searching and sorting functions
*
***/

extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*)(const void *, const void *));
extern void qsort(void *, size_t, size_t, int (*)(const void *, const void *));


/***
*
*     ANSI integer arithmetic functions
*
***/
extern int abs(int);
extern div_t div(int, int);
extern long int labs(long int);
extern ldiv_t ldiv(long int, long int);


/***
*
*     ANSI multibyte character functions
*
***/

extern int mblen(const char *, size_t);
extern int mbtowc(wchar_t *, const char *, size_t);
extern int wctomb(char *, wchar_t);
extern size_t mbstowcs(wchar_t *, const char *, size_t);
extern size_t wcstombs(char *, const wchar_t *, size_t);


/***
*
*     SAS Level 2 memory allocation functions
*
***/

extern void *getmem(unsigned int);
extern void *getml(long);
extern void rlsmem(void *, unsigned int);
extern void rlsml(void *, long);
extern int bldmem(int);
extern long sizmem(void);
extern long chkml(void);
extern void rstmem(void);


/***
*
*     SAS Level 1 memory allocation functions
*
***/

extern void *sbrk(unsigned int);
extern void *lsbrk(long);
extern int rbrk(void);
extern void __stdargs _MemCleanup(void);

extern unsigned long _MemType;
extern void *_MemHeap;
extern unsigned long _MSTEP;


/**
*
* SAS Sort functions
*
*/

extern void dqsort(double *, size_t);
extern void fqsort(float *, size_t);
extern void lqsort(long *, size_t);
extern void sqsort(short *, size_t);
extern void tqsort(char **, size_t);


/***
*
*     SAS startup, exit and environment functions.
*
***/

extern void __exit(int);
extern void __stdargs __main(char *);
extern void __stdargs __tinymain(char *);
extern void __stdargs _XCEXIT(long);
extern char *argopt(int, char**, char *, int *, char *);
extern int iabs(int);
extern int onexit(void (*)(void));
extern int putenv(const char *);

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

#pragma msg 148 ignore push   /* Ignore message if tag is undefined*/
extern struct WBStartup *_WBenchMsg;  /* WorkBench startup, if the */
#pragma msg 148 pop                   /* WorkBench.   Same as argv.*/
/* The following two externs give you the information in the   */
/* WBStartup structure parsed out to look like an (argc, argv) */
/* pair.  Don't define them in your code;  just include this   */
/* file and use them.  If the program was not run from         */
/* WorkBench, _WBArgc will be zero.                            */

extern int _WBArgc;    /* Count of the number of WorkBench arguments */
extern char **_WBArgv; /* The actual arguments                       */

/*
**	$VER: startup.h 47.2 (16.11.2021)
**
**	workbench startup definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

struct WBStartup {
    struct Message	sm_Message;	/* a standard message structure */
    struct MsgPort *	sm_Process;	/* the process descriptor for you */
    BPTR		sm_Segment;	/* a descriptor for your code */
    LONG		sm_NumArgs;	/* the number of elements in ArgList */
    STRPTR		sm_ToolWindow;	/* description of window */
    struct WBArg *	sm_ArgList;	/* the arguments themselves */
};

struct WBArg {
    BPTR		wa_Lock;	/* a lock descriptor */
    STRPTR		wa_Name;	/* a string relative to that lock */
};

/*
**	$VER: wb.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

extern struct Library * WorkbenchBase;
/****************************************************************************/

/*
**	$VER: wb_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: wb_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*--- functions in V36 or higher (Release 2.0) ---*/
void UpdateWorkbench( CONST_STRPTR name, BPTR lock, LONG action );

struct AppWindow *AddAppWindowA( ULONG id, ULONG userdata, struct Window *window, struct MsgPort *msgport, const struct TagItem *taglist );
struct AppWindow *AddAppWindow( ULONG id, ULONG userdata, struct Window *window, struct MsgPort *msgport, ... );

BOOL RemoveAppWindow( struct AppWindow *appWindow );

struct AppIcon *AddAppIconA( ULONG id, ULONG userdata, CONST_STRPTR text, struct MsgPort *msgport, BPTR lock, struct DiskObject *diskobj, const struct TagItem *taglist );
struct AppIcon *AddAppIcon( ULONG id, ULONG userdata, CONST_STRPTR text, struct MsgPort *msgport, BPTR lock, struct DiskObject *diskobj, ... );

BOOL RemoveAppIcon( struct AppIcon *appIcon );

struct AppMenuItem *AddAppMenuItemA( ULONG id, ULONG userdata, CONST_STRPTR text, struct MsgPort *msgport, const struct TagItem *taglist );
struct AppMenuItem *AddAppMenuItem( ULONG id, ULONG userdata, CONST_STRPTR text, struct MsgPort *msgport, ... );

BOOL RemoveAppMenuItem( struct AppMenuItem *appMenuItem );

/*--- functions in V39 or higher (Release 3.0) ---*/


ULONG WBInfo( BPTR lock, CONST_STRPTR name, struct Screen *screen );

/*--- functions in V44 or higher (Release 3.5) ---*/
BOOL OpenWorkbenchObjectA( CONST_STRPTR name, const struct TagItem *tags );
BOOL OpenWorkbenchObject( CONST_STRPTR name, ... );
BOOL CloseWorkbenchObjectA( CONST_STRPTR name, const struct TagItem *tags );
BOOL CloseWorkbenchObject( CONST_STRPTR name, ... );
BOOL WorkbenchControlA( CONST_STRPTR name, const struct TagItem *tags );
BOOL WorkbenchControl( CONST_STRPTR name, ... );
struct AppWindowDropZone *AddAppWindowDropZoneA( struct AppWindow *aw, ULONG id, ULONG userdata, const struct TagItem *tags );
struct AppWindowDropZone *AddAppWindowDropZone( struct AppWindow *aw, ULONG id, ULONG userdata, ... );
BOOL RemoveAppWindowDropZone( struct AppWindow *aw, struct AppWindowDropZone *dropZone );
BOOL ChangeWorkbenchSelectionA( CONST_STRPTR name, struct Hook *hook, const struct TagItem *tags );
BOOL ChangeWorkbenchSelection( CONST_STRPTR name, struct Hook *hook, ... );
BOOL MakeWorkbenchObjectVisibleA( CONST_STRPTR name, const struct TagItem *tags );
BOOL MakeWorkbenchObjectVisible( CONST_STRPTR name, ... );

/*--- functions in V47 or higher (Release 3.2) ---*/
ULONG WhichWorkbenchObjectA( struct Window *window, LONG x, LONG y, const struct TagItem *tags );
ULONG WhichWorkbenchObject( struct Window *window, LONG x, LONG y, ... );


/* "workbench.library" */
/*--- functions in V36 or higher (Release 2.0) ---*/


#pragma libcall WorkbenchBase UpdateWorkbench 1e 09803
#pragma libcall WorkbenchBase AddAppWindowA 30 A981005
#pragma tagcall WorkbenchBase AddAppWindow 30 A981005
#pragma libcall WorkbenchBase RemoveAppWindow 36 801
#pragma libcall WorkbenchBase AddAppIconA 3c CBA981007
#pragma tagcall WorkbenchBase AddAppIcon 3c CBA981007
#pragma libcall WorkbenchBase RemoveAppIcon 42 801
#pragma libcall WorkbenchBase AddAppMenuItemA 48 A981005
#pragma tagcall WorkbenchBase AddAppMenuItem 48 A981005
#pragma libcall WorkbenchBase RemoveAppMenuItem 4e 801
/*--- functions in V39 or higher (Release 3.0) ---*/


#pragma libcall WorkbenchBase WBInfo 5a A9803
/*--- functions in V44 or higher (Release 3.5) ---*/
#pragma libcall WorkbenchBase OpenWorkbenchObjectA 60 9802
#pragma tagcall WorkbenchBase OpenWorkbenchObject 60 9802
#pragma libcall WorkbenchBase CloseWorkbenchObjectA 66 9802
#pragma tagcall WorkbenchBase CloseWorkbenchObject 66 9802
#pragma libcall WorkbenchBase WorkbenchControlA 6c 9802
#pragma tagcall WorkbenchBase WorkbenchControl 6c 9802
#pragma libcall WorkbenchBase AddAppWindowDropZoneA 72 910804
#pragma tagcall WorkbenchBase AddAppWindowDropZone 72 910804
#pragma libcall WorkbenchBase RemoveAppWindowDropZone 78 9802
#pragma libcall WorkbenchBase ChangeWorkbenchSelectionA 7e A9803
#pragma tagcall WorkbenchBase ChangeWorkbenchSelection 7e A9803
#pragma libcall WorkbenchBase MakeWorkbenchObjectVisibleA 84 9802
#pragma tagcall WorkbenchBase MakeWorkbenchObjectVisible 84 9802
/*--- functions in V47 or higher (Release 3.2) ---*/
#pragma libcall WorkbenchBase WhichWorkbenchObjectA 8a 910804
#pragma tagcall WorkbenchBase WhichWorkbenchObject 8a 910804
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/*
**	$VER: diskfont.h 47.3 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

extern struct Library * DiskfontBase;
/****************************************************************************/

/*
**	$VER: diskfont_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: diskfont_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*
**	$VER: diskfont.h 47.2 (16.11.2021)
**
**	diskfont library definitions
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/* this really belongs to <graphics/text.h> */
struct FontContents {
    TEXT    fc_FileName[256];
    UWORD   fc_YSize;
    UBYTE   fc_Style;
    UBYTE   fc_Flags;
};

struct TFontContents {
    TEXT    tfc_FileName[256 -2];
    UWORD   tfc_TagCount;	/* including the TAG_DONE tag */
    /*
     *	if tfc_TagCount is non-zero, tfc_FileName is overlayed with
     *	Text Tags starting at:	(struct TagItem *)
     *	    &tfc_FileName[MAXFONTPATH-(tfc_TagCount*sizeof(struct TagItem))]
     */
    UWORD   tfc_YSize;
    UBYTE   tfc_Style;
    UBYTE   tfc_Flags;
};


struct FontContentsHeader {
    UWORD   fch_FileID;		/* FCH_ID */
    UWORD   fch_NumEntries;	/* the number of FontContents elements */
    /* struct FontContents fch_FC[], or struct TFontContents fch_TFC[]; */
};


struct DiskFontHeader {
    /* the following 8 bytes are not actually considered a part of the	*/
    /* DiskFontHeader, but immediately preceed it. The NextSegment is	*/
    /* supplied by the linker/loader, and the ReturnCode is the code	*/
    /* at the beginning of the font in case someone runs it...		*/
    /*	 ULONG dfh_NextSegment;			\* actually a BPTR	*/
    /*	 ULONG dfh_ReturnCode;			\* MOVEQ #0,D0 : RTS	*/
    /* here then is the official start of the DiskFontHeader...		*/
    struct Node dfh_DF;		/* node to link disk fonts */
    UWORD   dfh_FileID;		/* DFH_ID */
    UWORD   dfh_Revision;	/* the font revision */
    LONG    dfh_Segment;	/* the segment address when loaded */
    TEXT    dfh_Name[32]; /* stripped font name (null terminated) */
    struct TextFont dfh_TF;	/* loaded TextFont structure */
				/* dfh_TF.tf_Message.mn_Node.ln_Name */
				/* points to the full font name */
};

/* unfortunately, this needs to be explicitly typed */
/* used only if dfh_TF.tf_Style FSB_TAGGED bit is set */
/* moved to dfh_TF.tf_Extension->tfe_Tags during loading */
				/* AFF_DISK|AFF_BITMAP for bitmap fonts, */
				/* AFF_DISK|AFF_OTAG for .otag fonts, */
				/* AFF_DISK|AFF_OTAG|AFF_SCALED for */
				/* scalable .otag fonts. */
struct AvailFonts {
    UWORD   af_Type;		/* MEMORY, DISK, or SCALED */
    struct TextAttr af_Attr;	/* text attributes for font */
};

struct TAvailFonts {
    UWORD   taf_Type;		/* MEMORY, DISK, or SCALED */
    struct TTextAttr taf_Attr;	/* text attributes for font */
};

struct AvailFontsHeader {
    UWORD   afh_NumEntries;	 /* number of AvailFonts elements */
    /* struct AvailFonts afh_AF[], or struct TAvailFonts afh_TAF[]; */
};

/* structure used by EOpenEngine() ESetInfo() etc (V47) */
struct EGlyphEngine {
    APTR                ege_Reserved;     /* System reserved, don't touch */
    struct Library     *ege_BulletBase;
    struct GlyphEngine *ege_GlyphEngine;
};

/* flags for OpenOutlineFont() (V47) */
/* structure returned by OpenOutlineFont() (V47) */
struct OutlineFont {
    STRPTR               olf_OTagPath;    /* full path & name of the .otag file */
    struct TagItem      *olf_OTagList;    /* relocated .otag file in memory     */
    STRPTR               olf_EngineName;  /* OT_Engine name                     */
    STRPTR               olf_LibraryName; /* OT_Engine name + ".library"        */
    struct EGlyphEngine  olf_EEngine;     /* All NULL if OFF_OPEN not specified */
    APTR                 olf_Reserved;    /* for future expansion               */
    APTR                 olf_UserData;    /* for private use                    */
};

struct TextFont *OpenDiskFont( struct TextAttr *textAttr );
LONG AvailFonts( struct AvailFontsHeader *buffer, LONG bufBytes, ULONG flags );
/*--- functions in V34 or higher (Release 1.3) ---*/
struct FontContentsHeader *NewFontContents( BPTR fontsLock, CONST_STRPTR fontName );
void DisposeFontContents( struct FontContentsHeader *fontContentsHeader );
/*--- functions in V36 or higher (Release 2.0) ---*/
struct DiskFont *NewScaledDiskFont( struct TextFont *sourceFont, struct TextAttr *destTextAttr );
/*--- functions in V45 or higher (Release 3.9) ---*/
LONG GetDiskFontCtrl( LONG tagid );
void SetDiskFontCtrlA( const struct TagItem *taglist );
void SetDiskFontCtrl( Tag tag1, ... );
/*--- functions in V47 or higher (Release 3.2) ---*/
LONG EOpenEngine( struct EGlyphEngine *eEngine );
void ECloseEngine( struct EGlyphEngine *eEngine );
ULONG ESetInfoA( struct EGlyphEngine *eEngine, const struct TagItem *taglist );
ULONG ESetInfo( struct EGlyphEngine *eEngine, ... );
ULONG EObtainInfoA( struct EGlyphEngine *eEngine, const struct TagItem *taglist );
ULONG EObtainInfo( struct EGlyphEngine *eEngine, ... );
ULONG EReleaseInfoA( struct EGlyphEngine *eEngine, const struct TagItem *taglist );
ULONG EReleaseInfo( struct EGlyphEngine *eEngine, ... );
struct OutlineFont *OpenOutlineFont( CONST_STRPTR name, struct List *list, ULONG flags );
void CloseOutlineFont( struct OutlineFont *olf, struct List *list );
LONG WriteFontContents( BPTR fontsLock, CONST_STRPTR fontName, const struct FontContentsHeader *fontContentsHeader );
LONG WriteDiskFontHeaderA( const struct TextFont *font, CONST_STRPTR fileName, const struct TagItem *tagList );
LONG WriteDiskFontHeader( const struct TextFont *font, CONST_STRPTR fileName, ... );
ULONG ObtainCharsetInfo( ULONG knownTag, ULONG knownValue, ULONG wantedTag );

/* "diskfont.library" */
#pragma libcall DiskfontBase OpenDiskFont 1e 801
#pragma libcall DiskfontBase AvailFonts 24 10803
/*--- functions in V34 or higher (Release 1.3) ---*/
#pragma libcall DiskfontBase NewFontContents 2a 9802
#pragma libcall DiskfontBase DisposeFontContents 30 901
/*--- functions in V36 or higher (Release 2.0) ---*/
#pragma libcall DiskfontBase NewScaledDiskFont 36 9802
/*--- functions in V45 or higher (Release 3.9) ---*/
#pragma libcall DiskfontBase GetDiskFontCtrl 3c 001
#pragma libcall DiskfontBase SetDiskFontCtrlA 42 801
#pragma tagcall DiskfontBase SetDiskFontCtrl 42 801
/*--- functions in V47 or higher (Release 3.2) ---*/
#pragma libcall DiskfontBase EOpenEngine 48 801
#pragma libcall DiskfontBase ECloseEngine 4e 801
#pragma libcall DiskfontBase ESetInfoA 54 9802
#pragma tagcall DiskfontBase ESetInfo 54 9802
#pragma libcall DiskfontBase EObtainInfoA 5a 9802
#pragma tagcall DiskfontBase EObtainInfo 5a 9802
#pragma libcall DiskfontBase EReleaseInfoA 60 9802
#pragma tagcall DiskfontBase EReleaseInfo 60 9802
#pragma libcall DiskfontBase OpenOutlineFont 66 09803
#pragma libcall DiskfontBase CloseOutlineFont 6c 9802
#pragma libcall DiskfontBase WriteFontContents 72 A9803
#pragma libcall DiskfontBase WriteDiskFontHeaderA 78 A9803
#pragma tagcall DiskfontBase WriteDiskFontHeader 78 A9803
#pragma libcall DiskfontBase ObtainCharsetInfo 7e 21003
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/***
*
*     ANSI copying functions
*
***/

extern void *memcpy(void *, const void *, size_t);
extern void *memmove(void *, const void *, size_t);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);


/***
*
*     ANSI concatenation functions
*
***/

extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);


/***
*
*     ANSI comparison functions
*
***/

extern int memcmp(const void *, const void *, size_t);
extern int strcmp(const char *, const char *);
extern int strcoll(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern size_t strxfrm(char *, const char *, size_t);


/***
*
*     ANSI search functions
*
***/

extern void *memchr(const void *, int, size_t);
extern char *strchr(const char *, int);
extern size_t strcspn(const char *, const char *);
extern char *strpbrk(const char *, const char *);
extern char *strrchr(const char *, int);
extern size_t strspn(const char *, const char *);
extern char *strstr(const char *, const char *);
extern char *strtok(char *, const char *);


/***
*
*     ANSI miscellaneous functions
*
***/

extern void *memset(void *, int, size_t);
extern char *strerror(int);
extern size_t strlen(const char *);

/***
*
*     SAS string and memory functions.
*
***/

extern int stcarg(const char *, const char *);
extern int stccpy(char *, const char *, int);
extern int stcgfe(char *, const char *);
extern int stcgfn(char *, const char *);
extern int stcis(const char *, const char *);
extern int stcisn(const char *, const char *);
extern int __stcd_i(const char *, int *);
extern int __stcd_l(const char *, long *);
extern int stch_i(const char *, int *);
extern int stch_l(const char *, long *);
extern int stci_d(char *, int);
extern int stci_h(char *, int);
extern int stci_o(char *, int);
extern int stcl_d(char *, long);
extern int __stcl_h(char *, long);
extern int __stcl_o(char *, long);
extern int stco_i(const char *, int *);
extern int stco_l(const char *, long *);
extern int stcpm(const char *, const char *, char **);
extern int stcpma(const char *, const char *);
extern int stcsma(const char *, const char *);
extern int astcsma(const char *, const char *);
extern int stcu_d(char *, unsigned);
extern int __stcul_d(char *, unsigned long);

extern char *stpblk(const char *);
extern char *stpbrk(const char *, const char *);
extern char *stpchr(const char *, int);
extern char *stpcpy(char *, const char *);
extern char *__stpcpy(char *, const char *);
extern char *stpdate(char *, int, const char *);
extern char *stpsym(const char *, char *, int);
extern char *stptime(char *, int, const char *);
extern char *stptok(const char *, char *, int, const char *);

extern int strbpl(char **, int, const char *);
extern int stricmp(const char *, const char *);
extern char *strdup(const char *);
extern void strins(char *, const char *);
extern int strmid(const char *, char *, int, int);
extern char *__strlwr(char *);
extern void strmfe(char *, const char *, const char *);
extern void strmfn(char *, const char *, const char *, const char *, 
                   const char *);
extern void strmfp(char *, const char *, const char *);
extern int strnicmp(const char *, const char *, size_t);
extern char *strnset(char *, int, int);

extern char *stpchrn(const char *, int);
extern char *strrev(char *);
extern char *strset(char *, int);
extern void strsfn(const char *, char *, char *, char *, char *);
extern char *__strupr(char *);
extern int stspfp(char *, int *);
extern void strsrt(char *[], int);

extern int stcgfp(char *, const char *);

extern void *memccpy(void *, const void *, int, unsigned);
extern void movmem(const void *, void *, unsigned);
extern void repmem(void *, const void *, size_t, size_t);
extern void setmem(void *, unsigned, int);
extern void __swmem(void *, void *, unsigned);
/* for BSD compatibility */
/**
*
* Builtin function definitions
*
**/

extern size_t  __builtin_strlen(const char *);
extern int     __builtin_strcmp(const char *, const char *);
extern char   *__builtin_strcpy(char *, const char *);

extern void   *__builtin_memset(void *, int, size_t);
extern int     __builtin_memcmp(const void *, const void *, size_t);
extern void   *__builtin_memcpy(void *, const void *, size_t);

extern int __builtin_max(int, int);
extern int __builtin_min(int, int);
extern int __builtin_abs(int);

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

typedef char *va_list;
typedef unsigned long fpos_t;

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/**
*
* Definitions associated with __iobuf._flag
*
**/

struct __iobuf {
    struct __iobuf *_next;
    unsigned char *_ptr;	/* current buffer pointer */
    int _rcnt;		        /* current byte count for reading */
    int _wcnt;		        /* current byte count for writing */
    unsigned char *_base;	/* base address of I/O buffer */
    int _size;			/* size of buffer */
    int _flag;	        	/* control flags */
    int _file;		        /* file descriptor */
    unsigned char _cbuff;	/* single char buffer */
};

typedef struct __iobuf FILE;

extern struct __iobuf __iob[];

/***
*
*     Prototypes for ANSI standard functions.
*
***/


extern int remove(const char *);
extern int rename(const char *, const char *);
extern FILE *tmpfile(void);
extern char *tmpnam(char *s);

extern int fclose(FILE *);
extern int fflush(FILE *);
extern FILE *fopen(const char *, const char *);
extern FILE *freopen(const char *, const char *, FILE *);
extern void setbuf(FILE *, char *);
extern int setvbuf(FILE *, char *, int, size_t);

extern int fprintf(FILE *, const char *, ...);
extern int fscanf(FILE *, const char *, ...);
extern int printf(const char *, ...);
extern int __builtin_printf(const char *, ...);
extern int scanf(const char *, ...);
extern int sprintf(char *, const char *, ...);
extern int sscanf(const char *, const char *, ...);
extern int vfprintf(FILE *, const char *, va_list);
extern int vprintf(const char *, va_list);
extern int vsprintf(char *, const char *, va_list);

extern int fgetc(FILE *);
extern char *fgets(char *, int, FILE *);
extern int fputc(int, FILE *);
extern int fputs(const char *, FILE *);
extern int getc(FILE *);
extern int getchar(void);
extern char *gets(char *);
extern int putc(int, FILE *);

extern int putchar(int);
extern int puts(const char *);
extern int ungetc(int, FILE *);

extern size_t fread(void *, size_t, size_t, FILE *);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
extern int fgetpos(FILE *, fpos_t *);
extern int fseek(FILE *, long int, int);
extern int fsetpos(FILE *, const fpos_t *);
extern long int ftell(FILE *);
extern void rewind(FILE *);
extern void clearerr(FILE *);
extern int feof(FILE *);
extern int ferror(FILE *);
extern void perror(const char *);

/* defines for mode of access() */
/***
*
*     Prototypes for Non-ANSI functions.
*
***/

extern int __io2errno(int);
extern int fcloseall(void);
extern FILE *fdopen(int, const char *);
extern int fhopen(long, int);
extern int fgetchar(void);
extern int fileno(FILE *);
extern int flushall(void);
extern void fmode(FILE *, int);
extern int _writes(const char *, ...);
extern int _tinyprintf(char *, ...);
extern int fputchar(int);
extern void setnbf(FILE *);
extern int __fillbuff(FILE *);
extern int __flushbuff(int, FILE *);
extern int __access(const char *, int);
extern int access(const char *, int);
extern int chdir(const char *);
extern int chmod(const char *, int);
extern char *getcwd(char *, int);
extern int unlink(const char *);
extern int poserr(const char *);

/***
*
*     The following routines allow for raw console I/O.
*
***/

int rawcon(int);
int getch(void);

extern unsigned long __fmask;
extern int __fmode;

/*
**	$VER: prefhdr.h 47.1 (2.8.2019)
**
**	File format for preferences header
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*****************************************************************************/


struct PrefHeader
{
    UBYTE ph_Version;	/* version of following data */
    UBYTE ph_Type;	/* type of following data    */
    ULONG ph_Flags;	/* always set to 0 for now   */
};


/*****************************************************************************/


/*
**	$VER: font.h 47.2 (17.02.2020)
**
**	File format for font preferences
**
**	Copyright (C) 2019 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*****************************************************************************/


struct FontPrefs
{
    LONG	    fp_Reserved[3];
    UWORD	    fp_Reserved2;
    UWORD	    fp_Type;
    UBYTE	    fp_FrontPen;
    UBYTE	    fp_BackPen;
    UBYTE	    fp_DrawMode;
    UBYTE	    fp_SpecialDrawMode;
    struct TextAttr fp_TextAttr;
    BYTE	    fp_Name[(128)];
};


/* constants for FontPrefs.fp_Type */
/*****************************************************************************/


/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved. */


/**
*
* This header file defines various ASCII character manipulation macros,
* as follows:
*
*       isalpha(c)    non-zero if c is alpha
*       isupper(c)    non-zero if c is upper case
*       islower(c)    non-zero if c is lower case
*       isdigit(c)    non-zero if c is a digit (0 to 9)
*       isxdigit(c)   non-zero if c is a hexadecimal digit (0 to 9, A to F,
*                   a to f)
*       isspace(c)    non-zero if c is white space
*       ispunct(c)    non-zero if c is punctuation
*       isalnum(c)    non-zero if c is alpha or digit
*       isprint(c)    non-zero if c is printable (including blank)
*       isgraph(c)    non-zero if c is graphic (excluding blank)
*       iscntrl(c)    non-zero if c is control character
*       isascii(c)    non-zero if c is ASCII
*       iscsym(c)     non-zero if valid character for C symbols
*       iscsymf(c)    non-zero if valid first character for C symbols
*
**/

extern int isalnum(int);
extern int isalpha(int);
extern int iscntrl(int);
extern int isdigit(int);
extern int isgraph(int);
extern int islower(int);
extern int isprint(int);
extern int ispunct(int);
extern int isspace(int);
extern int isupper(int);
extern int isxdigit(int);

extern int tolower(int);
extern int toupper(int);

extern char __ctype[];   /* character type table */

/****
*
*   Extensions to the ANSI standard.
*
*
****/

extern int isascii(int);
extern int iscsym(int);
extern int iscsymf(int);

extern int toascii(int);


// IControlPrefs.h

/*
**	$VER: icontrol.h 47.3 (30.8.2021)
**
**	File format for intuition control preferences
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*****************************************************************************/


/*****************************************************************************/


struct IControlPrefs
{
    LONG  ic_Reserved[4];	/* System reserved		*/
    UWORD ic_TimeOut;		/* Verify timeout		*/
    WORD  ic_MetaDrag;		/* Meta drag mouse event	*/
    ULONG ic_Flags;		/* IControl flags (see below)	*/
    UBYTE ic_WBtoFront;		/* CKey: WB to front		*/
    UBYTE ic_FrontToBack;	/* CKey: front screen to back	*/
    UBYTE ic_ReqTrue;		/* CKey: Requester TRUE		*/
    UBYTE ic_ReqFalse;		/* CKey: Requester FALSE	*/

    /* Below is valid if Flags ICB_VERSIONED is set and onward if right version */
    UWORD ic_Version;		/* Version of this struct	*/
    UWORD ic_VersionMagic;	/* must be NULL			*/
    UBYTE ic_HoverSlugishness : 3; /* version 2 onward, number of intuiticks to wait when moving fast */
    UBYTE ic_HoverFlags : 5 ;	/* version 2 onward, see below	*/
    UBYTE ic_Pad;
    UBYTE ic_GUIGeometry[4];	/* Titlebar and border geometry	*/
};

/* flags for IControlPrefs.ic_Flags */
/* bits 6..14 are used by OS4 */
/* bits 6..14 are used by OS4 */
struct IExceptionPrefs
{
    struct TagItem ie_Tags[0];	/* Relocatable tag list including TAG_END, 0 */
};

/*****************************************************************************/


/*
**	$VER: iffparse.h 47.4 (6.12.2021)
**
**	Lattice 'C' style prototype/pragma header file combo
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/****************************************************************************/

/****************************************************************************/

extern struct Library * IFFParseBase;
/****************************************************************************/

/*
**	$VER: iffparse_pragmas.h 47.1 (30.11.2021)
**
**	Lattice 'C', Aztec 'C', SAS/C and DICE format pragma files.
*/

/*
**	$VER: iffparse_protos.h 47.1 (30.11.2021)
**
**	'C' prototypes. For use with 32 bit integers only.
**
**	Copyright (C) 2019-2022 Hyperion Entertainment CVBA.
**	    Developed under license.
*/

/*--- functions in V36 or higher (Release 2.0) ---*/

/* Basic functions */

struct IFFHandle *AllocIFF( void );
LONG OpenIFF( struct IFFHandle *iff, LONG rwMode );
LONG ParseIFF( struct IFFHandle *iff, LONG control );
void CloseIFF( struct IFFHandle *iff );
void FreeIFF( struct IFFHandle *iff );

/* Read/Write functions */

LONG ReadChunkBytes( struct IFFHandle *iff, APTR buf, LONG numBytes );
LONG WriteChunkBytes( struct IFFHandle *iff, CONST_APTR buf, LONG numBytes );
LONG ReadChunkRecords( struct IFFHandle *iff, APTR buf, LONG bytesPerRecord, LONG numRecords );
LONG WriteChunkRecords( struct IFFHandle *iff, CONST_APTR buf, LONG bytesPerRecord, LONG numRecords );

/* Context entry/exit */

LONG PushChunk( struct IFFHandle *iff, LONG type, LONG id, LONG size );
LONG PopChunk( struct IFFHandle *iff );

/* Low-level handler installation */

LONG EntryHandler( struct IFFHandle *iff, LONG type, LONG id, LONG position, struct Hook *handler, APTR object );
LONG ExitHandler( struct IFFHandle *iff, LONG type, LONG id, LONG position, struct Hook *handler, APTR object );

/* Built-in chunk/property handlers */

LONG PropChunk( struct IFFHandle *iff, LONG type, LONG id );
LONG PropChunks( struct IFFHandle *iff, const LONG *propArray, LONG numPairs );
LONG StopChunk( struct IFFHandle *iff, LONG type, LONG id );
LONG StopChunks( struct IFFHandle *iff, const LONG *propArray, LONG numPairs );
LONG CollectionChunk( struct IFFHandle *iff, LONG type, LONG id );
LONG CollectionChunks( struct IFFHandle *iff, const LONG *propArray, LONG numPairs );
LONG StopOnExit( struct IFFHandle *iff, LONG type, LONG id );

/* Context utilities */

struct StoredProperty *FindProp( struct IFFHandle *iff, LONG type, LONG id );
struct CollectionItem *FindCollection( struct IFFHandle *iff, LONG type, LONG id );
struct ContextNode *FindPropContext( struct IFFHandle *iff );
struct ContextNode *CurrentChunk( struct IFFHandle *iff );
struct ContextNode *ParentChunk( struct ContextNode *contextNode );

/* LocalContextItem support functions */

struct LocalContextItem *AllocLocalItem( LONG type, LONG id, LONG ident, LONG dataSize );
APTR LocalItemData( struct LocalContextItem *localItem );
void SetLocalItemPurge( struct LocalContextItem *localItem, struct Hook *purgeHook );
void FreeLocalItem( struct LocalContextItem *localItem );
struct LocalContextItem *FindLocalItem( struct IFFHandle *iff, LONG type, LONG id, LONG ident );
LONG StoreLocalItem( struct IFFHandle *iff, struct LocalContextItem *localItem, LONG position );
void StoreItemInContext( struct IFFHandle *iff, struct LocalContextItem *localItem, struct ContextNode *contextNode );

/* IFFHandle initialization */

void InitIFF( struct IFFHandle *iff, LONG flags, struct Hook *streamHook );
void InitIFFasDOS( struct IFFHandle *iff );
void InitIFFasClip( struct IFFHandle *iff );

/* Internal clipboard support */

struct ClipboardHandle *OpenClipboard( LONG unitNumber );
void CloseClipboard( struct ClipboardHandle *clipHandle );

/* Miscellaneous */

LONG GoodID( LONG id );
LONG GoodType( LONG type );
STRPTR IDtoStr( LONG id, STRPTR buf );

/* "iffparse.library" */
/*--- functions in V36 or higher (Release 2.0) ---*/

/* Basic functions */

#pragma libcall IFFParseBase AllocIFF 1e 00
#pragma libcall IFFParseBase OpenIFF 24 0802
#pragma libcall IFFParseBase ParseIFF 2a 0802
#pragma libcall IFFParseBase CloseIFF 30 801
#pragma libcall IFFParseBase FreeIFF 36 801
/* Read/Write functions */

#pragma libcall IFFParseBase ReadChunkBytes 3c 09803
#pragma libcall IFFParseBase WriteChunkBytes 42 09803
#pragma libcall IFFParseBase ReadChunkRecords 48 109804
#pragma libcall IFFParseBase WriteChunkRecords 4e 109804
/* Context entry/exit */

#pragma libcall IFFParseBase PushChunk 54 210804
#pragma libcall IFFParseBase PopChunk 5a 801
/*--- (1 function slot reserved here) ---*/

/* Low-level handler installation */

#pragma libcall IFFParseBase EntryHandler 66 A9210806
#pragma libcall IFFParseBase ExitHandler 6c A9210806
/* Built-in chunk/property handlers */

#pragma libcall IFFParseBase PropChunk 72 10803
#pragma libcall IFFParseBase PropChunks 78 09803
#pragma libcall IFFParseBase StopChunk 7e 10803
#pragma libcall IFFParseBase StopChunks 84 09803
#pragma libcall IFFParseBase CollectionChunk 8a 10803
#pragma libcall IFFParseBase CollectionChunks 90 09803
#pragma libcall IFFParseBase StopOnExit 96 10803
/* Context utilities */

#pragma libcall IFFParseBase FindProp 9c 10803
#pragma libcall IFFParseBase FindCollection a2 10803
#pragma libcall IFFParseBase FindPropContext a8 801
#pragma libcall IFFParseBase CurrentChunk ae 801
#pragma libcall IFFParseBase ParentChunk b4 801
/* LocalContextItem support functions */

#pragma libcall IFFParseBase AllocLocalItem ba 321004
#pragma libcall IFFParseBase LocalItemData c0 801
#pragma libcall IFFParseBase SetLocalItemPurge c6 9802
#pragma libcall IFFParseBase FreeLocalItem cc 801
#pragma libcall IFFParseBase FindLocalItem d2 210804
#pragma libcall IFFParseBase StoreLocalItem d8 09803
#pragma libcall IFFParseBase StoreItemInContext de A9803
/* IFFHandle initialization */

#pragma libcall IFFParseBase InitIFF e4 90803
#pragma libcall IFFParseBase InitIFFasDOS ea 801
#pragma libcall IFFParseBase InitIFFasClip f0 801
/* Internal clipboard support */

#pragma libcall IFFParseBase OpenClipboard f6 001
#pragma libcall IFFParseBase CloseClipboard fc 801
/* Miscellaneous */

#pragma libcall IFFParseBase GoodID 102 001
#pragma libcall IFFParseBase GoodType 108 001
#pragma libcall IFFParseBase IDtoStr 10e 8002
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/***
*     ANSI trigonometric functions
***/

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/* Use the __ARGS macro below to suppress errors  */
/* if these functions have been #defined by other */
/* header files.                                  */

extern double acos  (double);
extern double asin  (double);
extern double atan  (double);
extern double atan2 (double, double);
extern double cos   (double);
extern double sin   (double);
extern double tan   (double);


/***
*     ANSI hyperbolic functions
***/

extern double cosh (double);
extern double sinh (double);
extern double tanh (double);


/***
*     ANSI exponential and logarithmic functions
***/

extern double exp (double);
extern double frexp (double, int *);
extern double ldexp (double, int);
extern double log (double);
extern double log10 (double);
extern double modf (double, double *);


/***
*     ANSI power functions
***/

extern double pow (double, double);
extern double sqrt (double);


/***
*     ANSI nearest integer, absolute value, and remainder functions
***/

extern double ceil (double);
extern double fabs (double);
extern double floor (double);
extern double fmod (double, double);


/***
*     Structure to hold information about math exceptions
***/

struct __exception {
    int     type;          /* error type */
    char    *name;         /* math function name */
    double  arg1, arg2;    /* function arguments */
    double  retval;        /* proposed return value */
};

/***
*     Exception type codes, found exception.type
***/

/***
*     Error codes generated by basic arithmetic operations (+ - * /)
***/

extern int _FPERR;

/***
*     Floating point constants
***/

/***
*     SAS functions (Non-ANSI)
***/

void __stdargs _CXFERR(int);
extern double cot(double);
extern double drand48(void);
extern double erand48(unsigned short *);
extern double except(int, const char *, double, double, double);
extern char   *ecvt(double, int, int *, int *);
extern char   *fcvt(double, int, int *, int *);
extern char   *gcvt(double, int, char *);
extern long   jrand48(unsigned short *);
extern void   lcong48(unsigned short *);
extern long   lrand48(void);
extern double __except(int, const char *, double, double, double);
extern int    __matherr(struct __exception *);
extern long   mrand48(void);
extern long   nrand48(unsigned short *);
extern double pow2(double);
extern unsigned short *seed48(unsigned short *);
extern void   srand48(long);

// Define the IControlPrefsDetails structure
struct IControlPrefsDetails {
    ULONG flags;
    BOOL coerceColors;
    BOOL coerceLace;
    BOOL strGadFilter;
    BOOL menuSnap;
    BOOL modePromote;
    BOOL correctRatio;
    BOOL offScrnWin;
    BOOL moreSizeGadgets;
    BOOL versioned;
    BOOL legacyLook; 
    BOOL ratio_9_7;
    BOOL ratio_9_8;
    BOOL ratio_1_1;
    BOOL ratio_8_9;
    UWORD screenTitleBarExtraHeight;
    UWORD windowTitleBarExtraHeight;
    BOOL titleBar_50;
    BOOL titleBar_67;
    BOOL titleBar_75;
    BOOL titleBar_100;
    BOOL squareProportionalLook; 
    UWORD currentLeftBarWidth;
    UWORD currentBarWidth;
    UWORD currentBarHeight;
    UWORD currentCGaugeWidth;
    UWORD currentTitleBarHeight;
    UWORD currentWindowBarHeight;
};

// Declare the fetchIControlSettings function
int fetchIControlSettings(struct IControlPrefsDetails *details);

/* Define the WorkbenchSettings structure */
struct WorkbenchSettings {
    BOOL borderless;
    LONG embossRectangleSize;
    LONG maxNameLength;
    BOOL newIconsSupport;
    BOOL colorIconSupport;
    BOOL disableTitleBar;
    BOOL disableVolumeGauge;
};

/* Function to fetch Workbench settings */
void fetchWorkbenchSettings(struct WorkbenchSettings *settings);

typedef struct {
    int left;
    int top;
    int width;
    int height;
} folderWindowSize;

typedef struct {
    int x;
    int y;
} IconPosition;

typedef struct {
    int icon_x;
    int icon_y;
    int icon_width;
    int icon_height;
    int text_width;
    int text_height;
    int icon_max_width;
    int icon_max_height;
    BOOL has_border;
    char *icon_text;
    char *icon_full_path;
    BOOL is_folder;
} FullIconDetails;

typedef struct {
    FullIconDetails *array;
    size_t size;
    size_t capacity;
    size_t BiggestWidthPX;
    BOOL hasOnlyBorderlessIcons;
} IconArray;

typedef struct {
    int width;
    int height;
} IconSize;

extern struct Screen *screen;
extern struct Window *window;
extern struct RastPort *rastPort;
extern struct TextFont *font;
extern struct WorkbenchSettings prefsWorkbench;
extern struct IControlPrefsDetails prefsIControl;
extern int screenHight;
extern int screenWidth;

BOOL checkIconFrame(const char *iconName);
int Compare(const void *a, const void *b);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
void CalculateTextExtent(const char *text, struct TextExtent *textExtent);
void dumpIconArrayToScreen(IconArray *iconArray);
void pause_program(void);
void removeInfoExtension(const char *input, char *output);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);
int saveIconsPositionsToDisk(IconArray *iconArray);

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
BOOL IsNewIcon(struct DiskObject *diskObject);
BOOL IsNewIconPath(const STRPTR filePath);
IconArray *CreateIconArray(void);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
IconPosition GetIconPositionFromPath(const char *iconPath);
int getOS35IconSize(const char *filename, IconSize *size);
int isOS35IconFormat(const char *filename);
void FreeIconArray(IconArray *iconArray);
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
int ArrangeIcons(BPTR lock, char *dirPath, int newWidth);
int CompareByFolderAndName(const void *a, const void *b);

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

/* Copyright (c) 1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* This header file contains common preprocessor symbol   */
/* definitions that were previously duplicated throughout */
/* the header files. Those definitions were moved here    */
/* and replaced with a #include of this header file.      */
/* This was done to purify the header files for GST       */
/* processing.                                            */

typedef long int ptrdiff_t;

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */

/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


void CleanupWindow(void);
int FormatIconsAndWindow(char *folder);
void resizeFolderToContents(char *dirPath, IconArray *iconArray);
int InitializeWindow(void);
void repoistionWindow(char *dirPath, int winWidth, int winHeight);

int HasSlaveFile(char *path);
void ProcessDirectory(char *path, BOOL processSubDirs);
void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize);
int IsRootDirectorySimple(char *path);

int HasSlaveFile(char *path) {
    BPTR lock;
    struct FileInfoBlock *fib;
    int hasSlave = 0;

    lock = Lock((STRPTR)path, -2);
    if (lock == 0) {
        Printf("Failed to lock directory: %s\n", path);
        return 0;
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), (1UL<<0) | (1UL<<16));
    if (fib == 0L) {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return 0;
    }

    if (Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
            if (fib->fib_DirEntryType < 0) {
                const char *ext = strrchr(fib->fib_FileName, '.');
                if (ext && strncasecmp_custom(ext, ".slave", 6) == 0) {
                    hasSlave = 1;
                    break;
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
    return hasSlave;
}

void ProcessDirectory(char *path, BOOL processSubDirs) {
    BPTR lock;
    struct FileInfoBlock *fib;
    char subdir[4096];

    lock = Lock((STRPTR)path, -2);
    if (lock == 0) {
        Printf("Failed to lock directory: %s\n", path);
        return;
    }

    if (HasSlaveFile(path)) {
        resizeFolderToContents(path, CreateIconArrayFromPath(lock, path));
        UnLock(lock);
        return;
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), (1UL<<0) | (1UL<<16));
    if (fib == 0L) {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return;
    }

    if (Examine(lock, fib)) {
        FormatIconsAndWindow(path);
        if (processSubDirs == 1) {
            while (ExNext(lock, fib)) {
                if (fib->fib_DirEntryType > 0) {
                    sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                    ProcessDirectory(subdir, 1);
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
}

void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize) {
    int dirLen;

    if (directory == 0L || fib == 0L || fullPath == 0L || fullPathSize <= 0) {
        return;
    }

    strncpy(fullPath, directory, fullPathSize - 1);
    fullPath[fullPathSize - 1] = '\0';

    dirLen = __builtin_strlen(fullPath);
    if (dirLen > 0 && fullPath[dirLen - 1] != '/' && fullPath[dirLen - 1] != ':') {
        if (dirLen + 1 < fullPathSize) {
            strncat(fullPath, "/", fullPathSize - dirLen - 1);
            dirLen++;
        }
    }

    strncat(fullPath, fib->fib_FileName, fullPathSize - dirLen - 1);
    fullPath[fullPathSize - 1] = '\0';
}

int IsRootDirectorySimple(char *path) {
    size_t length = __builtin_strlen(path);
    if (length > 0 && path[length - 1] == ':') {
        return 1;
    }
    return 0;
}
