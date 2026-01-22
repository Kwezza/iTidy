/*
*	Function:	STRPTR getProgDir(void)
*	Purpose:	Gets directory where program is located,
*						returns (STRPTR) PROGDIR
*						(solved Amiga-stylish)
*	Version:	1.0
*	Author:		Michael Bergmann
*/
STRPTR getProgDir(void)
{
	#define MAXPATHLEN 512
	
	static UBYTE progdir[MAXPATHLEN];
	BPTR lock = Lock("PROGDIR:", ACCESS_READ);
	
	NameFromLock(lock, progdir, MAXPATHLEN);
	UnLock(lock);
	
	return(STRPTR)&progdir;
}
