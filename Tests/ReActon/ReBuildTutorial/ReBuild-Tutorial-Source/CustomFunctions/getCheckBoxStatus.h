/*
*	Function:	void getCheckBoxStatus(struct Window *win, 
*									   struct Gadget *chkgad, struct Gadget *txtgad)
*	Purpose:	Get status of a CheckBox gadget, set text in a corresponding String gadget
*	Version:	1.0
*	Author:		Darren Coles
*/
void getCheckBoxStatus(struct Window *win, struct Gadget *chkgad, struct Gadget *txtgad)
{
	ULONG status = 0;
	ULONG success = 0;
	STRPTR isChecked;
	
	GetAttr(CHECKBOX_Checked, (APTR)chkgad, &status);    
	switch (status)
	{
		case 0:
		isChecked = "unchecked";
		break;
		case 1:
		isChecked = "checked";
		break;        
	}
	success = SetGadgetAttrs((APTR)txtgad, win, 0, STRINGA_TextVal, isChecked, TAG_END );
	
	
	printf("Status: %ld\n\n", status);
	printf("success: %ld\n\n", success);
	
}		