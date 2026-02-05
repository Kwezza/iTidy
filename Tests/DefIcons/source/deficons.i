ACT_MATCH	EQU	1	* arg1/2 = offset arg3 = length str = string
ACT_SEARCH	EQU	2	* arg2 = length str = string
ACT_SEARCHSKIPSPACES	EQU	3	* arg2 = length str = string
					* if length < 0, do a case unsensitive match */

ACT_FILESIZE	EQU	4	* arg2 = file size
ACT_NAMEPATTERN	EQU	5	* str = filename pattern
ACT_PROTECTION	EQU	6	* arg1 = mask arg2 = protbits&mask
ACT_OR	EQU	7	* an alternative description follows
ACT_ISASCII	EQU	8	* this is used only by AsciiType
ACT_MACROCLASS	EQU	20	* this must be the ONLY ACT_xxx of the node.
						* (apart from ACT_END)
						* following descriptions are son of this one, but
						* this one will never be considered valid, i.e.
						* if none of the sons matches the file, we will
						* proceed with the following description. The parent
						* will be used only for icon picking purposes (e.g.
						* a class "picture" may contain "gif", "jpeg" and so
						* on, and if def_gif is missing def_picture will be used.
ACT_END	EQU	0

TYPE_DOWN_LEVEL	EQU	1	* following description is son of previous one
TYPE_UP_LEVEL	EQU	2	* following description is brother of parent of previous one
TYPE_END	EQU	0	* end of list
