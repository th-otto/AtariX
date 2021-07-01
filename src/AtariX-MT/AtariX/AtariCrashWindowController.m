/*
 * Copyright (C) 1990-2018 Andreas Kromke, andreas.kromke@gmail.com
 *
 * This program is free software; you can redistribute it or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//
//  AtariCrashWindowController.m
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 24.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "AtariCrashWindowController.h"

@implementation AtariCrashWindowController

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}


/**********************************************************************
 *
 * Gibt die Art des "instruction stack frame" als Zeichenkette
 *
 **********************************************************************/

static void Exc2Str(UInt16 exc, char *s)
{
	static const char *ExcStr[] =
	{
		"reset sp",
		"reset pc",
		"Bus Error",
		"Address Error",
		"Illegal Instruction",
		"Zero Divide",
		"CHK(2)",
		"(cp)TRAP(cc/v)",
		
		"Privilege Violation",
		"Trace",
		"LineA",
		"LineF",
		"Vector 12",
		"Coprocessor Protocol Violation",
		"Format Error",
		"Uninitialized Interrupt",
		
		"Vector 16",
		"Vector 17",
		"Vector 18",
		"Vector 19",
		"Vector 20",
		"Vector 21",
		"Vector 22",
		"Vector 23",
		
		"Spurious Interrupt",
		"Level 1 Interrupt Autovector",
		"Level 2 Interrupt Autovector",
		"Level 3 Interrupt Autovector",
		"Level 4 Interrupt Autovector",
		"Level 5 Interrupt Autovector",
		"Level 6 Interrupt Autovector",
		"Level 7 Interrupt Autovector",
		
		"TRAP #0",
		"TRAP #1",
		"TRAP #2",
		"TRAP #3",
		"TRAP #4",
		"TRAP #5",
		"TRAP #6",
		"TRAP #7",
		
		"TRAP #8",
		"TRAP #9",
		"TRAP #10",
		"TRAP #11",
		"TRAP #12",
		"TRAP #13",
		"TRAP #14",
		"TRAP #15",
		
		"FPCP Branch or Set on Unordered Condition",
		"FPCP Inexact Result",
		"FPCP Divide by Zero",
		"FPCP Underflow",
		"FPCP Operand Error",
		"FPCP Overflow",
		"FPCP Signaling NAN",
		"Vector 55",
		"MMU Configuration Error",
		"Vector 57 (MC68851)",
		"Vector 58 (MC68851)",
		
		"Vector 59",
		"Vector 60",
		"Vector 61",
		"Vector 62",
		"Vector 63"
	};
	
	if	(exc < 64)
		strcpy(s, ExcStr[exc]);
	else
		if	(exc < 255)
			sprintf(s, "User Interrupt %d", (int) exc);
		else
			strcpy(s, "<malformed frame>");
}


#define SET_REGISTER_TO_GUI(theValue, theNs, theOutlet) \
sprintf(buf, "%08lx", (unsigned long)(theValue)); \
NSString *theNs = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding]; \
[theOutlet setStringValue:theNs];


- (void)windowDidLoad
{
    [super windowDidLoad];

    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.

	char buf[256];

	// Atari process name

	const char *ProcName;
	if	(ProcPath)
		ProcName = strrchr(ProcPath, '\\');
	else
		ProcName = NULL;
	if	(ProcName)
		ProcName++;
	else
		ProcName = "<unknown>";
	NSString *theProcName = [NSString stringWithCString:ProcName encoding:NSISOLatin1StringEncoding];
	[outletPgmName setStringValue:theProcName];

	// Atari exception text and code

	Exc2Str(EmuReg_exc, buf);
	sprintf(buf + strlen(buf), " (%u)", EmuReg_exc);
	NSString *thePgmErr = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding];
	[outletPgmErr setStringValue:thePgmErr];

	// Fehleradresse und Zugriffsmodus bei Adreß- oder Busfehler
	
	if	((EmuReg_exc == 2) || (EmuReg_exc == 3))
	{
		sprintf(buf, "0x%08lx (%s)", (unsigned long)EmuReg_ErrAddr, AccessMode);
	}
	else
	{
		sprintf(buf, "--------");
	}
	NSString *nerraddr = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding];
	[outletPgmAddr setStringValue:nerraddr];

	// Atari registers

	SET_REGISTER_TO_GUI(EmuReg_pc,  npc,  outletRegPC)
	SET_REGISTER_TO_GUI(EmuReg_pc - (EmuReg_pd + 256),  nrpc, outletRegRelPC)
	SET_REGISTER_TO_GUI(EmuReg_sr,  nsr,  outletRegSR)
	SET_REGISTER_TO_GUI(EmuReg_usp, nusp, outletRegUSP)

	SET_REGISTER_TO_GUI(EmuReg_Dx[0], nD0, outletRegD0)
	SET_REGISTER_TO_GUI(EmuReg_Dx[1], nD1, outletRegD1)
	SET_REGISTER_TO_GUI(EmuReg_Dx[2], nD2, outletRegD2)
	SET_REGISTER_TO_GUI(EmuReg_Dx[3], nD3, outletRegD3)
	SET_REGISTER_TO_GUI(EmuReg_Dx[4], nD4, outletRegD4)
	SET_REGISTER_TO_GUI(EmuReg_Dx[5], nD5, outletRegD5)
	SET_REGISTER_TO_GUI(EmuReg_Dx[6], nD6, outletRegD6)
	SET_REGISTER_TO_GUI(EmuReg_Dx[7], nD7, outletRegD7)

	SET_REGISTER_TO_GUI(EmuReg_Ax[0], nA0, outletRegA0)
	SET_REGISTER_TO_GUI(EmuReg_Ax[1], nA1, outletRegA1)
	SET_REGISTER_TO_GUI(EmuReg_Ax[2], nA2, outletRegA2)
	SET_REGISTER_TO_GUI(EmuReg_Ax[3], nA3, outletRegA3)
	SET_REGISTER_TO_GUI(EmuReg_Ax[4], nA4, outletRegA4)
	SET_REGISTER_TO_GUI(EmuReg_Ax[5], nA5, outletRegA5)
	SET_REGISTER_TO_GUI(EmuReg_Ax[6], nA6, outletRegA6)
	SET_REGISTER_TO_GUI(EmuReg_Ax[7], nA7, outletRegA7)

/*
	sprintf(buf, "0x%08lx", EmuReg_pc);
	NSString *npc  = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding];
	[outletRegPC setStringValue:npc];

	sprintf(buf, "0x%04x", EmuReg_sr);
	NSString *nsr  = [NSString stringWithCString:buf encoding:NSISOLatin1StringEncoding];
	[outletRegSR setStringValue:nsr];
*/
}

/*****************************************************************************************************
 *
 * Fenster: Wird beim Schließen aufgerufen
 *
 * "outlet" von delegate nach "File's Owner" erforderlich
 *
 ****************************************************************************************************/

- (BOOL)windowShouldClose:(id)sender
{
	NSLog(@"close button pushed");
	
	*pFinishedCond = 0;
	[finishedLock signal];	// tell worker thread that window is closed
	return YES;
}


/*****************************************************************************************************
 *
 * Fenster: OK-Button
 *
 ****************************************************************************************************/

- (IBAction)actionOk:(id)sender
{
	printf("%s()\n", __FUNCTION__);
	// tell that we are closing
	BOOL ret = [self windowShouldClose:nil];
	if (ret == YES)
	{
		// close window
		[self close];
	}
}

@end
