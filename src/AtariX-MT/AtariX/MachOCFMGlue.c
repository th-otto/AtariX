/* =============================================================================
	PROJECT:	Cubix (PB1)
	FILE:		MachOCFMGlue.c

    PURPOSE:	Code that does the magic needed to call MachO functions
				from CFM and CFM functions from Carbon.

    COPYRIGHT:	Copyright (c) 2002 by M. Uli Kusterer, all rights reserved.
				Thanks to George Warner, Chris Silverberg and Ricky Sharp for
				clues on how to do this and implementation snippets.

    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   ========================================================================== */

/* -----------------------------------------------------------------------------
    Headers:
   -------------------------------------------------------------------------- */

#include <Carbon/Carbon.h>
#include "MachOCFMGlue.h"


/* -----------------------------------------------------------------------------
    Globals:
   -------------------------------------------------------------------------- */

UInt32 gGlueTemplate[6] =
{
	0x3D800000,		// lis r12,0xYYYY
	0x618C0000,		// ori r12,r12,0xZZZZ
	0x800C0000,		// lwz r0,0(r12)
	0x804C0004,		// lwz RTOC,4(r12)
	0x7C0903A6,		// mtcr r0
	0x4E800420			// bctr
};


/* -----------------------------------------------------------------------------
    CFMFunctionPointerForMachOFunctionPointer:
        Creates a fake Transition Vector that lets a CFM code fragment call a
		MachO function.

	TAKES:
		machOFP	-	ProcPtr to the MachO function you want to make CFM-callable.

	GIVeS:
		void*	-	Pointer to the transition vector to pass to the CFM fragment
					as the ProcPtr. When you're done with this, dispose of it
					using DisposeCFMFunctionPointer().

    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   -------------------------------------------------------------------------- */

void*	CFMFunctionPointerForMachOFunctionPointer( void* inMachProcPtr )
{
    TVector_rec		*vTVector;

    vTVector = (TVector_rec*) malloc( sizeof(TVector_rec) );

    if( MemError() == noErr && vTVector != NULL )
	{
        vTVector->fProcPtr = (ProcPtr) inMachProcPtr;
        vTVector->fTOC = 0;  // ignored
    }

	return( (void *) vTVector );
}


/* -----------------------------------------------------------------------------
    DisposeCFMFunctionPointer:
        Disposes of the fake TVector created by
		CFMFunctionPointerForMachOFunctionPointer().

    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   -------------------------------------------------------------------------- */

void	DisposeCFMFunctionPointer( void* inCfmProcPtr )
{
    if( inCfmProcPtr )
		free( inCfmProcPtr );
}


/* -----------------------------------------------------------------------------
    MachOFunctionPointerForCFMFunctionPointer:
       This function allocates a block of CFM glue code which contains the
	   instructions to call a CFM routine from a MachO application.

	   Syntax analogous to CFMFunctionPointerForMachOFunctionPointer().

    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   -------------------------------------------------------------------------- */

void*	MachOFunctionPointerForCFMFunctionPointer( void* inCfmProcPtr )
{
    UInt32	*vMachProcPtr = (UInt32*) NewPtr( sizeof(gGlueTemplate) );	// sizeof() really returns the data size here, not the size of a pointer. Trust me.

    vMachProcPtr[0] = gGlueTemplate[0] | ((UInt32)inCfmProcPtr >> 16);
    vMachProcPtr[1] = gGlueTemplate[1] | ((UInt32)inCfmProcPtr & 0xFFFF);
    vMachProcPtr[2] = gGlueTemplate[2];
    vMachProcPtr[3] = gGlueTemplate[3];
    vMachProcPtr[4] = gGlueTemplate[4];
    vMachProcPtr[5] = gGlueTemplate[5];
    MakeDataExecutable( vMachProcPtr, sizeof(gGlueTemplate) );

    return( vMachProcPtr );
}


/* -----------------------------------------------------------------------------
    DisposeMachOFunctionPointer:
        Disposes of the fake TVector created by
		CFMFunctionPointerForMachOFunctionPointer().

    REVISIONS:
        Fri Jul 26 2002	witness	Created.
   -------------------------------------------------------------------------- */

void DisposeMachOFunctionPointer( void *inMachProcPtr )
{
    if( inMachProcPtr )
		DisposePtr( inMachProcPtr );
}