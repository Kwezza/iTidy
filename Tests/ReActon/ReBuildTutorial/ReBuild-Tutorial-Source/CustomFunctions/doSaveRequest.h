/*
*	Function:	STRPTR doSaveRequest( ... )
*	Purpose:	Opens an ASL Save-type Requester, returns filename as STRPTR
*	Version:	1.0
*	Author:		Michael Bergmann
*/
STRPTR doSaveRequest(char *path, char *pattern, char *file, 
char *old_file, char *title, struct Window *window, 
long width, long height, BOOL reject)
{
	/* ASL-Requester: */
	struct FileRequester *request;	// Instanciate as  ASL-Save-Requester
	static UBYTE fname[255], errorcheck[80];
	
	strcpy(fname, ""); // start with empty filename
	
	/* FileRequester Structure ... */
	request = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
	ASL_Hail, title, (struct TagItem *)TAG_DONE);
	
	/* ...show Save-Requester */
	if (AslRequestTags(request,
						ASL_FuncFlags, FILF_DOWILDFUNC | FILF_DOMSGFUNC | FILF_SAVE,
						ASLFR_Window, window,
						ASLFR_SleepWindow,TRUE,
						ASLFR_InitialLeftEdge, 20,
						ASLFR_InitialTopEdge, 20,
						ASLFR_InitialWidth, width,
						ASLFR_InitialHeight, height,
						ASLFR_RejectIcons, reject,							// NO Icons:: TRUE, FALSE
						ASLFR_InitialDrawer, path,
						ASLFR_InitialFile, file,
						ASLFR_InitialPattern, (ULONG) pattern,  // Example: "~rexx#?|math#?",
						ASLFR_DoPatterns,TRUE,				   				// show file-filtering Stringgadget
						ASLFR_RejectPattern, (ULONG) pattern,   // Filter  example: "~rexx#?|math#?",                  
						ASLFR_PositiveText, "Save",							// user-defined button text
						ASLFR_NegativeText, "No, thanx!",				// user-defined button text		
						(struct TagItem *)TAG_DONE))
	{
		/* save rf_Dir for testing empty String */
		strcpy(errorcheck, request->rf_Dir);
		
		/* build path name */
		strcat( fname, request->rf_Dir);
		
		if (fname[strlen(fname)-1] != (UBYTE)58) /* Check for : */
		strcat(fname, "/");
		
		strcat(fname, request->rf_File);
	}
	
	/* Free Requester-Structure! */
	if (request) 
		FreeAslRequest(request);
	
	/* check if return value is empty */
	if ( (strcmp(errorcheck, " ") != 0))
		return(fname); 
	
	else
		return("");		
}
