/*
 *	Function:	int DoInfoRequest( ... )
 *	Purpose:	Opens a requester.class informal requester, returns selected button
 *	Version:	1.0
 *	Author:		Michael Bergmann
 */	
int doInfoRequest(struct Screen *screen, 
					const char *title, const char *body, 
					const char *buttons, ULONG image )
{  
	Object *req = 0;		// the requester itself
	int button;			// the button that was clicked by the user
	
	// fill in the requester structure
	req = NewObject(REQUESTER_GetClass(), NULL, 
	REQ_Type,       REQTYPE_INFO,
	REQ_TitleText,  (ULONG)title,
	REQ_BodyText,   (ULONG)body,
	REQ_GadgetText, (ULONG) buttons ,
	REQ_Image,      image,
	TAG_DONE);
	
	if (req) 
	{ 
		struct orRequest reqmsg;
		
		reqmsg.MethodID  = RM_OPENREQ;
		reqmsg.or_Attrs  = NULL;
		reqmsg.or_Window = NULL;
		reqmsg.or_Screen = screen;
		
		button = DoMethodA(req, (Msg) &reqmsg);
		DisposeObject(req);
		
		return button;
		
	}  else 
			printf("[doInfoRequest] Could not allocate requester\n");
	
	return 0;
}