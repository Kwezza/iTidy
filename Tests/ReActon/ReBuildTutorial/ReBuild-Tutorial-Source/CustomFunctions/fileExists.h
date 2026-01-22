/*
*	Function:	BOOL fileExists(STRPTR fname)
*	Purpose:	Check if a certain file exists, returns (BOOL)
*						(solved UNIX-stylish)
*	Version:	1.0
*	Author:		Michael Bergmann
*/
BOOL fileExists(STRPTR fname)
{    
	FILE *file;
	
	file = fopen(fname, "r");
	if (file )
	{
		fclose(file);
		return TRUE;
	}
	return FALSE;
}	
	