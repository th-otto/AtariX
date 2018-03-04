/* =============================================================================
	PROJECT:	Cubix (PB1)
	FILE:		MachOCFMGlue.h
	
	PURPOSE:	Code that does the magic needed to call MachO functions
				from CFM and CFM functions from Carbon.
				    
	COPYRIGHT:	Copyright (c) 2002 M. Uli Kusterer, all rights reserved.
				Thanks to George Warner, Chris Silverberg and Ricky Sharp for
				clues on how to do this and implementation snippets.
	
    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   ========================================================================== */

#ifndef MACH_O_CFM_GLUE_H
#define MACH_O_CFM_GLUE_H	1

/* -----------------------------------------------------------------------------
    Headers:
   -------------------------------------------------------------------------- */

#include <Carbon/Carbon.h>


#ifdef __cplusplus
extern "C" {
#endif


/* -----------------------------------------------------------------------------
    Data Structures:
		This is what CFM actually uses instead of function pointers.
   -------------------------------------------------------------------------- */

typedef struct TVector_struct
{
    ProcPtr fProcPtr;
    UInt32 fTOC;
} TVector_rec, *TVector_ptr;


/* -----------------------------------------------------------------------------
    Prototypes:
   -------------------------------------------------------------------------- */

void*	CFMFunctionPointerForMachOFunctionPointer( void* inMachProcPtr );
void	DisposeCFMFunctionPointer( void* inCfmProcPtr );

void*	MachOFunctionPointerForCFMFunctionPointer( void* inCfmProcPtr );
void	DisposeMachOFunctionPointer( void* inMachProcPtr );


#ifdef __cplusplus
}
#endif

#endif /*MACH_O_CFM_GLUE_H*/
