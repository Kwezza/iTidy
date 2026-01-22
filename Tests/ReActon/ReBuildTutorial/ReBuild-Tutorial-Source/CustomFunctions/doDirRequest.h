/*
*	Function:	STRPTR doDirRequest( ... )
*	Purpose:	Opens an ASL directory-type Requester, returns folder as STRPTR
*	Version:	1.0
*	Author:		Michael Bergmann
*/	
STRPTR dirRequest(char *path, char *olddir, char *title, 
struct Window *window, long width, long height)
{
	struct FileRequester *request;	// Instanciate...
	static UBYTE fname[255], errorcheck[80];
	strcpy(fname, "");
	
	/*build  FileRequester Structure  */
	request = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
	ASL_Hail, title, (struct TagItem *)TAG_DONE);
	
	/* ...show Directory-Requester */
	if (AslRequestTags(request,
						ASLFR_Window, window, 		     // Pointer to calling window
						ASLFR_SleepWindow,TRUE,		     // modale Mode, Mousepointer wird "busy"...
						ASLFR_InitialLeftEdge, 20,	   // Coordinates
						ASLFR_InitialTopEdge, 20,
						ASLFR_InitialWidth, width,	   // initial sizing
						ASLFR_InitialHeight, height,
						ASLFR_DrawersOnly, TRUE,		   // show folders only
						ASLFR_InitialDrawer, path,     // initial folder
						ASLFR_Activate, TRUE,	         // from WB 3.9 and up only!!		
						(struct TagItem *)TAG_DONE))
	{
		/* rf_Dir empty String ? */
		strcpy(errorcheck, request->rf_Dir);
		
		/* build pathname for return */
		strcat( fname, request->rf_Dir);
	}
	
	/* ...free Requester-Structure! */
	if (request) 
		FreeAslRequest(request);
	
	/* Test returned String */
	if ( (strcmp(errorcheck, "") != 0))
		return(fname); 
	else
		return ("");  
} 
