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
//  PreferencesWindowController.m
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 02.11.11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PreferencesWindowController.h"
#include "Debug.h"
#include "EmulationMain.h"
#include "Globals.h"

static NSString *DMKAtariDrivesTableKey = @"atariDrivesTable";
static NSString *DMKAtariDrivesFlagsKey = @"atariDrivesFlags";


// 1st column: identifier is "DriveCell"
// 2nd column: identifier is "PathCell"

// Sample data we will display
static const NSString *ATTableData[NDRIVES + 1] =
{
    @"A:",
    @"B:",
    @"C:",
    @"D:",
    @"E:", 
    @"F:",
    @"G:",
    @"H:",
    @"I:",
    @"J:",
    @"K:",
    @"L:",
    @"M:",
    @"N:",
    @"O:",
    @"P:",
    @"Q:",
    @"R:",
    @"S:",
    @"T:",
    @"U:",
    @"V:",
    @"W:",
    @"X:",
    @"Y:",
    @"Z:",
    nil
};

// some demo drive numbers
#define ATARI_DRIVE_A DriveFromLetter('A')
#define ATARI_DRIVE_C DriveFromLetter('C')
#define ATARI_DRIVE_M DriveFromLetter('M')
#define ATARI_DRIVE_U DriveFromLetter('U')

// convert drive number (0..25) to NSString
#define ATARI_DRIVE_STR(n) (ATTableData[n])

@implementation PreferencesWindowController


/*****************************************************************************************************
 *
 * Free dialogue object
 *
 ****************************************************************************************************/

- (void)dealloc
{
	[m_AtariDrivesUrlDict release];
	[m_AtariDrivesFlagsDict release];
	[super dealloc];
}


/*****************************************************************************************************
 *
 * Fenster: Wird vor dem Öffnen aufgerufen
 *
 ****************************************************************************************************/

- (void)windowDidLoad
{
	DebugTrace("%s()", __FUNCTION__);
    [super windowDidLoad];
	
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.

	//	[self showWindow:nil];

	// create dictionary of key/value pairs
	m_AtariDrivesUrlDict = [[NSMutableDictionary alloc] initWithCapacity:NDRIVES];
	m_AtariDrivesFlagsDict = [[NSMutableDictionary alloc] initWithCapacity:NDRIVES];
	// note that sharedUserDefaultsController returns (id), so compiler does not know which type, therefore we have "shared".
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	// make sure changes to the defaults are not applied immediately so that we can revert them on "cancel"
	[sharedController setAppliesImmediately:FALSE];
	// read Atari drive dictionary from preferences
	NSUserDefaults *myDefaults = [sharedController defaults];
	[m_AtariDrivesUrlDict setDictionary:[myDefaults dictionaryForKey:DMKAtariDrivesTableKey]];
	[m_AtariDrivesFlagsDict setDictionary:[myDefaults dictionaryForKey:DMKAtariDrivesFlagsKey]];

	// disable buttons
	[outletAtariDrivePathSelect setEnabled:NO];
	[outletAtariDriveRemove setEnabled:NO];
	[outletAtariDrive8p3 setEnabled:NO];
	[outletAtariDrive8p3 setState:NO];
}


/*****************************************************************************************************
 *
 * Laufwerk-Tabelle: Gibt Objekt an Zeile/Spalte zurück
 *
 * von http://objc.toodarkpark.net/AppKit/Protocols/NSTableDataSource.html
 *
 ****************************************************************************************************/

- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
			row:(NSInteger)rowIndex
{
	if (aTableView == outletAtariDrivesTableView)
	{
		NSString *columnId = aTableColumn.identifier;
		id theValue;

#if 1
		NSParameterAssert(rowIndex >= 0 && rowIndex < NDRIVES);
#else
		NSParameterAssert(rowIndex >= 0 && rowIndex < [_tableContents count]);
#endif

		if ([columnId caseInsensitiveCompare:@"DriveCell"] == NSOrderedSame)
		{
			theValue = ATTableData[rowIndex];
		}
		else
		{
			if (rowIndex == ATARI_DRIVE_C)
			{
				theValue = (NSURL *)EmulationGetRootfsUrl();
				if (theValue == nil)
				{
					NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
					NSUserDefaults *myDefaults = [sharedController defaults];
					theValue = [myDefaults stringForKey:@"rootfsPathUrl"];
				}
				if (theValue == nil)
				{
					theValue = @"";
				}
				theValue = [theValue stringByAppendingString:@" (root drive)"];
			}
			else
			if (rowIndex == ATARI_DRIVE_U)
			{
				theValue = @"---------- Atari file system (reserved) ----------";
			}
			else
			{
				theValue = [m_AtariDrivesUrlDict objectForKey:ATARI_DRIVE_STR(rowIndex)];
				if (theValue == nil)
				{
					theValue = @"(unused)";
				}
				if (rowIndex == ATARI_DRIVE_M)
					theValue = [theValue stringByAppendingString:@" (host file system)"];
			}
		}

		return theValue;
	}
	else
	{
		// unknown table view
		return nil;
	}
}


/*****************************************************************************************************
 *
 * Laufwerk-Tabelle: Gibt Anzahl Zeilen zurück
 *
 ****************************************************************************************************/

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
#if 1
	return NDRIVES;
#else
	NSInteger retVal = [_tableContents count];
    return retVal;
#endif
}

/*
// This method is optional if you use bindings to provide the data
- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
	// Group our "model" object, which is a dictionary
	NSDictionary *dictionary = [_tableContents objectAtIndex:row];
    
	// In IB the tableColumn has the identifier set to the same string as the keys in our dictionary
	NSString *identifier = [tableColumn identifier];
    
	if ([identifier isEqualToString:@"MainCell"])
	{
        // We pass us as the owner so we can setup target/actions into this main controller object
        NSTableCellView *cellView = [tableView makeViewWithIdentifier:identifier owner:self];
        // Then setup properties on the cellView based on the column
        cellView.textField.stringValue = [dictionary objectForKey:@"Name"];
        cellView.imageView.objectValue = [dictionary objectForKey:@"Image"];
        return cellView;
    }
	else
	if ([identifier isEqualToString:@"SizeCell"])
	{
        NSTextField *textField = [tableView makeViewWithIdentifier:identifier owner:self];
        NSImage *image = [dictionary objectForKey:@"Image"];
        NSSize size = image ? [image size] : NSZeroSize;
        NSString *sizeString = [NSString stringWithFormat:@"%.0fx%.0f", size.width, size.height];
        textField.objectValue = sizeString;
        return textField;
    }
	else
	{
		NSAssert1(NO, @"Unhandled table column identifier %@", identifier);
	}
	return nil;
}
*/


/*****************************************************************************************************
 *
 * Fenster: Wird beim Schließen aufgerufen
 *
 ****************************************************************************************************/

- (BOOL)windowShouldClose:(id)sender
{
	DebugTrace("close button pushed");
	return YES;
}


/*****************************************************************************************************
 *
 * Fenster: Cancel-Button
 *
 ****************************************************************************************************/

- (IBAction)actionCancel:(id)sender {
	DebugTrace("%s()", __func__);
	// revert all changed preferences
	[[NSUserDefaultsController sharedUserDefaultsController] revert:self];
	// close window
	[self close];
}


/*****************************************************************************************************
 *
 * Fenster: OK-Button
 *
 ****************************************************************************************************/

- (IBAction)actionOk:(id)sender
{
	DebugTrace("%s()", __FUNCTION__);

	// note that sharedUserDefaultsController returns (id), so compiler does not know which type, therefore we have "shared".
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	// write Atari drive dictionary to preferences
	NSUserDefaults *myDefaults = [sharedController defaults];
	[myDefaults setObject:m_AtariDrivesUrlDict forKey:DMKAtariDrivesTableKey];
	[myDefaults setObject:m_AtariDrivesFlagsDict forKey:DMKAtariDrivesFlagsKey];

	// save all changed preferences
	[sharedController save:self];
	// close window
	[self close];
}

- (IBAction)actionRegister:(id)sender {
	DebugTrace("%s()", __FUNCTION__);
}

- (IBAction)actionAtariMemoryStepper:(NSStepper *)sender {
	DebugTrace("%s()", __FUNCTION__);
}

- (IBAction)actionAtariScreenWidthStepper:(id)sender {
	DebugTrace("%s()", __FUNCTION__);
}

- (IBAction)actionAtariScreenHeightStepper:(id)sender {
	DebugTrace("%s()", __FUNCTION__);
}


/*****************************************************************************************************
 *
 * Fenster: Pfadauswahl für gewähltes Laufwerk
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrivePathSelect:(id)sender
{
	DebugTrace("%s()", __FUNCTION__);
	int driveNo = (int)outletAtariDrivesTableView.selectedRow;
	//	[outletAtariDrivesTableView selectedColumn];	// equivalent
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_U && driveNo < NDRIVES)
	{
		NSOpenPanel *chooser = [NSOpenPanel openPanel];
		NSString *myTitle = [NSString stringWithFormat:@"Choose Path for Virtual Atari Drive %c:", DriveToLetter(driveNo)];
		chooser.title = myTitle;
		chooser.canChooseFiles = NO;
		//	[DirectoryChooser setCanChooseFiles:NO];	// equivalent
		chooser.canChooseDirectories = YES;
		chooser.canCreateDirectories = YES;
		chooser.allowsMultipleSelection = NO;
		NSInteger ret = [chooser runModal];
		DebugInfo("%s() : runModal() -> %d", __FUNCTION__, (int)ret);
		if (ret == NSFileHandlingPanelOKButton)
		{
			DebugInfo("%s() File Chooser exited with OK, change drive %c:", __FUNCTION__, DriveToLetter(driveNo));
			NSArray *urls = chooser.URLs;
			// das geht nicht, und NSString bleibt eine URL?!?
			NSURL *pathUrl = urls.lastObject;
			NSString *pathUrlString = [pathUrl absoluteString];
			[m_AtariDrivesUrlDict setObject:pathUrlString forKey:ATARI_DRIVE_STR(driveNo)];
			NSIndexSet *rowIndexSet = [NSIndexSet indexSetWithIndex:driveNo];
			NSIndexSet *colIndexSet = [NSIndexSet indexSetWithIndex:1];
			[outletAtariDrivesTableView reloadDataForRowIndexes:rowIndexSet columnIndexes:colIndexSet];
			[self actionAtariDrivesTable:sender];
		}
	}
	else
	{
		DebugInfo("%s() : invalid selected drive %d", __FUNCTION__, driveNo);
	}
}


/*****************************************************************************************************
 *
 * Fenster: Pfad entfernen
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDriveRemove:(id)sender
{
	DebugTrace("%s()", __FUNCTION__);
	NSInteger driveNo = outletAtariDrivesTableView.selectedRow;
	//	[outletAtariDrivesTableView selectedColumn];	// equivalent
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_U && driveNo < NDRIVES)
	{
		[m_AtariDrivesUrlDict removeObjectForKey:ATARI_DRIVE_STR(driveNo)];
		[m_AtariDrivesFlagsDict removeObjectForKey:ATARI_DRIVE_STR(driveNo)];
		NSIndexSet *rowIndexSet = [NSIndexSet indexSetWithIndex:driveNo];
		NSIndexSet *colIndexSet = [NSIndexSet indexSetWithIndex:1];
		[outletAtariDrivesTableView reloadDataForRowIndexes:rowIndexSet columnIndexes:colIndexSet];
		[self actionAtariDrivesTable:sender];
	}
	else
	{
		DebugInfo("%s() : invalid selected drive %d", __FUNCTION__, (int)driveNo);
	}
}


/*****************************************************************************************************
 *
 * Fenster: Laufwerk anklicken
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrivesTable:(id)sender
{
	DebugTrace("%s()", __FUNCTION__);
	int driveNo = (int)outletAtariDrivesTableView.selectedRow;
	unsigned long flagsVal = 0;
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_U && driveNo < NDRIVES)
	{
		DebugInfo("%s() : selected drive %d", __FUNCTION__, driveNo);
		id theValue;
		id theFlags;
		theValue = [m_AtariDrivesUrlDict objectForKey:ATARI_DRIVE_STR(driveNo)];
		theFlags = [m_AtariDrivesFlagsDict objectForKey:ATARI_DRIVE_STR(driveNo)];
		if (theValue == nil)
		{
			// disable buttons
			[outletAtariDriveRemove setEnabled:NO];
			[outletAtariDrive8p3 setEnabled:NO];
		}
		else
		{
			// enable buttons
			[outletAtariDriveRemove setEnabled:YES];
			[outletAtariDrive8p3 setEnabled:YES];
			if (theFlags)
				flagsVal = [theFlags unsignedLongValue];
		}

		[outletAtariDrivePathSelect setEnabled:YES];	// select or change path
	}
	else
	{
		DebugInfo("%s() : invalid selected drive %d", __FUNCTION__, driveNo);
		// disable buttons
		[outletAtariDrivePathSelect setEnabled:NO];
		[outletAtariDriveRemove setEnabled:NO];
		[outletAtariDrive8p3 setEnabled:NO];
	}
	[outletAtariDrive8p3 setState:(flagsVal & M_DRV_DOSNAMES) != 0];
}


/*****************************************************************************************************
 *
 * Fenster: ungenutzte Aktionen
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrive8p3:(id)sender {
	DebugTrace("%s()", __FUNCTION__);
	int driveNo = (int)outletAtariDrivesTableView.selectedRow;
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_U && driveNo < NDRIVES)
	{
		NSNumber *theFlags;
		theFlags = [m_AtariDrivesFlagsDict objectForKey:ATARI_DRIVE_STR(driveNo)];
		if (theFlags == nil)
			theFlags = [NSNumber numberWithUnsignedLong:0];
		if ([outletAtariDrive8p3 state] == NO)
			theFlags = [NSNumber numberWithUnsignedLong:theFlags.unsignedLongValue & ~M_DRV_DOSNAMES];
		else
			theFlags = [NSNumber numberWithUnsignedLong:theFlags.unsignedLongValue | M_DRV_DOSNAMES];
		[m_AtariDrivesFlagsDict setObject:theFlags forKey:ATARI_DRIVE_STR(driveNo)];
	}
}


@end
