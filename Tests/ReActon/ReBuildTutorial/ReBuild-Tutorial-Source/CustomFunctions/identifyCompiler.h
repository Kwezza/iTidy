/*
*	Function:	STRPTR identifyCompiler()
*	Purpose:	returns the C-compiler used for compilation of a program
*	Version:	1.0
*	Author:		Michael Bergmann
*/	
STRPTR identifyCompiler(void)
{
	char myCompiler[512];
	
	#ifdef __SASC
	const int sasversion = __VERSION__;
	const int sasrev = __REVISION__;
	#endif
	
	#if defined(__STORM__)
	/* Compiler is StormC3 */
	sprintf(myCompiler, "StormC3");
	#elif defined(__STORMGCC__)
	/* Compiler is StormGCC4 */
	sprintf(myCompiler, "GNU gcc, StormC4 flavour");
	#elif defined(__MAXON__)
	/* Compiler is Maxon/HiSoft C++ */
	sprintf(myCompiler, "Maxon/HiSoft C++");
	#elif defined(__GNUC__)
	/* Compiler is gcc */
	sprintf(myCompiler, "GNU gcc v%ld.%ld", __GNUC__, __GNUC_MINOR__);
	#ifdef __GNUC_PATCHLEVEL__
	strcat(myCompiler, ".");
	strcat(myCompiler, __GNUC_PATCHLEVEL__);
	#endif
	#elif defined(__VBCC__)
	/* Compiler is vbcc */
	sprintf(myCompiler, "vbcc - Volker Barthelmann's retargetable C compiler");
	#elif defined(__SASC)
	/* Compiler is SAS/C */
	sprintf(myCompiler, "SAS/C, Version %ld.%ld", sasversion, sasrev);
	#elif defined(LATTICE)
	/* Compiler is Lattice C */
	sprintf(myCompiler, "Lattice C");
	#elif defined(AZTEC_C)
	/* Compiler is Aztec C */
	sprintf(myCompiler, "Manx Aztec C v%ld", __VERSION);
	#elif defined(_DCC)
	/* Compiler is dice */
	sprintf(myCompiler, "Matt Dillon's DICE, propably v3.15");
	#else
	/* Compiler not identified */
	sprintf(myCompiler, "UNKNOWN");
	#endif
	
	
	return(myCompiler);
}