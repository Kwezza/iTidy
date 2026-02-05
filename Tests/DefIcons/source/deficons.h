#define ACT_MATCH		1	/* arg1/2 = offset arg3 = length str = string */
#define ACT_SEARCH		2	/* arg2 = length str = string */
#define ACT_SEARCHSKIPSPACES	3	/* arg2 = length str = string */
/* if length < 0, do a case unsensitive match */

#define ACT_FILESIZE	4	/* arg2 = file size */
#define ACT_NAMEPATTERN	5	/* str = filename pattern */
#define ACT_PROTECTION	6	/* arg1 = mask arg2 = protbits&mask */
#define ACT_OR			7	/* an alternative description follows */
#define ACT_ISASCII		8	/* this is used only by AsciiType */
#define	ACT_MACROCLASS	20	/* this must be the ONLY ACT_xxx of the node. */
							/* (apart from ACT_END) */
							/* following descriptions are son of this one, but */
							/* this one will never be considered valid, i.e. */
							/* if none of the sons matches the file, we will */
							/* proceed with the following description. The parent */
							/* will be used only for icon picking purposes (e.g. */
							/* a class "picture" may contain "gif", "jpeg" and so */
							/* on, and if def_gif is missing def_picture will be used */
#define ACT_END			0



#define TYPE_DOWN_LEVEL	1	/* following description is son of previous one */
#define TYPE_UP_LEVEL	2	/* following description is brother of parent of previous one */
#define TYPE_END		0	/* end of list */
