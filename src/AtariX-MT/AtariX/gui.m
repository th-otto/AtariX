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

/*
 *
 * Bedienelemente (GUI), als Cocoa
 *
 */

#include <Cocoa/Cocoa.h>
#include "AtariCrashWindowController.h"

/* these were renamed in SDK 10.12 and above */
#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#define NSAlertStyleWarning NSWarningAlertStyle
#define NSAlertStyleInformational NSInformationalAlertStyle
#define NSAlertStyleCritical NSCriticalAlertStyle
#endif



static int GuiMyMainThreadAlert(const char *msg_text, const char *info_text, int nButtons)
{
	// have to create pool to avoid debug error messages like:
	// "... Object 0x7a6429e0 of class ... autoreleased with no pool in place - just leaking - break on objc_autoreleaseNoPool() to debug"
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *nmsg  = [[NSString stringWithCString: msg_text encoding:NSUTF8StringEncoding] retain];
	NSString *ninfo = [[NSString stringWithCString: info_text encoding:NSUTF8StringEncoding] retain];

	NSAlert *alert = [[NSAlert alloc] init];
//	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert addButtonWithTitle:NSLocalizedString(@"OK", nil)];
	if (nButtons > 1)
	{
		[alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];
	}
	[alert setMessageText:nmsg];
	[alert setInformativeText:ninfo];
	[alert setAlertStyle:NSAlertStyleWarning];

	/*
	[alert beginSheetModalForWindow:[searchField window] modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:nil];
	 */
	[alert retain];		// avoid SEGV
#if 0
	// make sure Dialogue is run in main thread (necessary in Cocoa)
	NSInteger __block retcode;
	dispatch_sync(dispatch_get_main_queue(), ^
	{
		retcode = [alert runModal];
	});
#else
	NSInteger retcode = [alert runModal];
#endif
	[nmsg release];
	[ninfo release];
	[alert release];

	[pool release];
	return (int) retcode;
}

int GuiMyAlert(const char *msg_text, const char *info_text, int nButtons)
{
	// make sure Dialogue is run in main thread (necessary in Cocoa)
	NSInteger __block ret;
	dispatch_async(dispatch_get_main_queue(), ^
				  {
					  ret = GuiMyMainThreadAlert(msg_text, info_text, nButtons);
				  });
	return ret;
}

void GuiAtariCrash
(
	uint16_t exc,
	uint32_t ErrAddr,
	const char *AccessMode,
	uint32_t pc,
	uint16_t sr,
	uint32_t usp,
	const uint32_t *pDx,
	const uint32_t *pAx,
	const char *ProcPath,
	uint32_t pd
)
{
	// We create a lock and pass it to the dialog, so that we can wait for the dialog to finish,
	// TODO: Create a modal dialog instead
	id mylock = [[NSCondition alloc] init];
	int __block bDialogRunning = 1;
	[mylock lock];

	fprintf(stderr, "--- 0\n");
	dispatch_sync(dispatch_get_main_queue(), ^
	{
		fprintf(stderr, "--- A\n");
		// erzeuge das "AtariCrashController"-Objekt
		AtariCrashWindowController *winController = [AtariCrashWindowController alloc];

		// Sende Atari-Register an den Dialog
		winController->EmuReg_exc = exc;
		winController->EmuReg_ErrAddr = ErrAddr;
		winController->AccessMode = AccessMode;
		winController->EmuReg_pc = pc;
		winController->EmuReg_sr = sr;
		winController->EmuReg_usp = usp;
		memcpy(winController->EmuReg_Dx, pDx, sizeof(winController->EmuReg_Dx));
		memcpy(winController->EmuReg_Ax, pAx, sizeof(winController->EmuReg_Ax));
		winController->ProcPath = ProcPath;

		winController->finishedLock = mylock;
		winController->pFinishedCond = &bDialogRunning;
		// und setze es auch als "File's Owner", sonst wird AppDelegate der "File's Owner". KLAPPT NICHT.
		[winController initWithWindowNibName:@"AtariCrash" owner:winController];
		// das geht nur, wenn "window" ein "referencing outlet" vom Dialogfenster zum "File's Owner" ist, ansonsten ist
		// w immer NULL
		NSWindow *w = [winController window];
		[w makeKeyAndOrderFront:nil];
		[winController showWindow:nil];
		fprintf(stderr, "--- B\n");
	});

	fprintf(stderr, "--- 1\n");
	while(bDialogRunning)
		[mylock wait];
	fprintf(stderr, "--- 2\n");
	[mylock unlock];

	[mylock release];
}

void GuiShowMouse(void)
{
	[NSCursor unhide];
}
