/*
*	Function:	STRPTR doFileRequest( ... )
*	Purpose:	Opens an ASL File-type Requester, returns filename as STRPTR
*	Version:	1.0
*	Author:		Michael Bergmann
*/
	
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/asl.h>

#include <stdio.h>
#include <string.h> 

/* Prototype */
STRPTR doFileRequest(char *path, char *pattern, char *file, 
char *old_file, char *title, struct Window *window, 
long width, long height, BOOL reject);

/*
		Amiga ASL File-Request Function
*/
STRPTR doFileRequest(char *path, char *pattern, char *file, 
char *old_file, char *title, struct Window *window, 
long width, long height, BOOL reject)
{
	/* ASL-Requester: */
	struct FileRequester *request;	// Instanciate ASL-File-Requester
	static UBYTE fname[255], errorcheck[80];
	
	strcpy(fname, "");
	
	printf("old_file = %s\n", old_file);
	
	/* FileRequester Structure ... */
	request = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
	ASL_Hail, title, (struct TagItem *)TAG_DONE);
	
	/* ...show File-Requester ... */
	if (AslRequestTags(request,
						ASLFR_Window, window,
						ASLFR_SleepWindow,TRUE,
						ASLFR_InitialLeftEdge, 20,
						ASLFR_InitialTopEdge, 20,
						ASLFR_InitialWidth, width,
						ASLFR_InitialHeight, height,
						ASLFR_RejectIcons, reject,				// NO Icons: TRUE, show: FALSE
						ASLFR_InitialDrawer, path,
						ASLFR_InitialFile, file,
						ASLFR_InitialPattern, (ULONG) pattern,  // e.g. "~rexx#?|math#?",
						ASLFR_DoPatterns,TRUE,				    // show Filter-Stringgadget
						ASLFR_RejectPattern, (ULONG) pattern,   // e.g. "~rexx#?|math#?",
						
						/* uncomment these in order to use own button texts... */
						//ASLFR_PositiveText, "Open",
						//ASLFR_NegativeText, "Cancel",				
						(struct TagItem *)TAG_DONE))
	{
		/* check rf_Dir for empty String */
		strcpy(errorcheck, request->rf_Dir);
		
		/* build path name */
		strcat( fname, request->rf_Dir);
		if (fname[strlen(fname)-1] != (UBYTE)58) /* Check for : */
			strcat(fname, "/");
		
		strcat(fname, request->rf_File);
	}
	
	/* free Requester-Structure */
	if (request) 
		FreeAslRequest(request);
	
	/* check if returned String is empty */
	if ( (strcmp(errorcheck, "") != 0))
		return(fname); 
	else
		return("");                 
}