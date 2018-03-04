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
//  PreferencesWindowController.h
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 02.11.11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

// abgeleitet von NSWindowController, implementiert die beiden Protokolle ...Delegate und ...DataSource
@interface PreferencesWindowController : NSWindowController <NSTableViewDelegate, NSTableViewDataSource>
{
	IBOutlet NSTextField *outletAtariMemory;
	IBOutlet NSButton *outletAtariAutostart;
	IBOutlet NSButton *outletHideHostMousepointer;
	IBOutlet NSTextField *outletAtariScreenWidth;
	IBOutlet NSTextField *outletAtariScreenHeight;
	IBOutlet NSPopUpButton *outletAtariScreenColourMode;
	IBOutlet NSPopUpButton *outletAtariLanguage;
	IBOutlet NSScrollView *outletAtariDrivesScrollView;
	IBOutlet NSTableView *outletAtariDrivesTableView;
	IBOutlet NSTextField *outletAtariPrintCommand;
	IBOutlet NSTextField *outletAtariSerialDevice;
	IBOutlet NSButton *outletAtariDrivePathSelect;
	IBOutlet NSButton *outletAtariDriveRemove;
	IBOutlet NSButton *outletAtariDrive8p3;
	IBOutlet NSButton *outletStretchX;
	IBOutlet NSButton *outletStretchY;

#if 0
	NSMutableArray *_tableContents;
#else
	NSMutableDictionary *m_AtariDrivesUrlDict;		// key: integer, value: NSString of Drive-URL
#endif
}

- (void)windowDidLoad;

- (IBAction)actionCancel:(id)sender;
- (IBAction)actionOk:(id)sender;
- (IBAction)actionRegister:(id)sender;
- (IBAction)actionAtariMemoryStepper:(id)sender;
- (IBAction)actionAtariScreenWidthStepper:(id)sender;
- (IBAction)actionAtariScreenHeightStepper:(id)sender;
- (IBAction)actionAtariDrivePathSelect:(id)sender;
- (IBAction)actionAtariDrive8p3:(id)sender;
- (IBAction)actionAtariDriveRemove:(id)sender;
- (IBAction)actionAtariDrivesTable:(id)sender;

@end
