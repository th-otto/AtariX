//
//  AtariCrashWindowController.h
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 24.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AtariCrashWindowController : NSWindowController {
	IBOutlet NSTextField *outletPgmName;
	IBOutlet NSTextField *outletPgmAddr;
	IBOutlet NSTextField *outletPgmErr;
	IBOutlet NSTextField *outletRegPC;
	IBOutlet NSTextField *outletRegRelPC;
	IBOutlet NSTextField *outletRegUSP;
	IBOutlet NSTextField *outletRegSR;
	IBOutlet NSTextField *outletRegD0;
	IBOutlet NSTextField *outletRegD1;
	IBOutlet NSTextField *outletRegD2;
	IBOutlet NSTextField *outletRegD3;
	IBOutlet NSTextField *outletRegD4;
	IBOutlet NSTextField *outletRegD5;
	IBOutlet NSTextField *outletRegD6;
	IBOutlet NSTextField *outletRegD7;
	IBOutlet NSTextField *outletRegA0;
	IBOutlet NSTextField *outletRegA1;
	IBOutlet NSTextField *outletRegA2;
	IBOutlet NSTextField *outletRegA3;
	IBOutlet NSTextField *outletRegA4;
	IBOutlet NSTextField *outletRegA5;
	IBOutlet NSTextField *outletRegA6;
	IBOutlet NSTextField *outletRegA7;

	@public
	UInt16 EmuReg_exc;
	UInt32 EmuReg_ErrAddr;
	const char *AccessMode;
	UInt32 EmuReg_pc;
	UInt16 EmuReg_sr;
	UInt32 EmuReg_usp;
	UInt32 EmuReg_Dx[8];
	UInt32 EmuReg_Ax[8];
	const char *ProcPath;
	UInt32 EmuReg_pd;
	NSCondition *finishedLock;
	int *pFinishedCond;
}
- (IBAction)actionOk:(id)sender;

@end
