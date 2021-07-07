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

static NSString *DMKAtariDrivesTableKey = @"atariDrivesTable";


// 1st column: identifier is "DriveCell"
// 2nd column: identifier is "PathCell"

// Sample data we will display
#define ATARI_NUM_DRIVES ('Z' - 'A' + 1)
static const NSString *ATTableData[ATARI_NUM_DRIVES + 1] =
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
#define ATARI_DRIVE_A ('A' - 'A')
#define ATARI_DRIVE_C ('C' - 'A')
#define ATARI_DRIVE_D ('D' - 'A')
#define ATARI_DRIVE_M ('M' - 'A')
#define ATARI_DRIVE_U ('U' - 'A')
#define ATARI_DRIVE_Z ('Z' - 'A')

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
#if 1
	[m_AtariDrivesUrlDict release];
#else
    [_tableContents release];
#endif
	[super dealloc];
}


/*****************************************************************************************************
 *
 * Fenster: Wird vor dem Öffnen aufgerufen
 *
 ****************************************************************************************************/

- (void)windowDidLoad
{
	printf("%s()\n", __FUNCTION__);
    [super windowDidLoad];
	
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.

	//	[self showWindow:nil];

	//[outletAtariMemory setStringValue:@"Bliblablub"];

#if 1
	// create dictionary of key/value pairs
	m_AtariDrivesUrlDict = [[NSMutableDictionary alloc] initWithCapacity:ATARI_NUM_DRIVES];
#if 1
	// note that sharedUserDefaultsController returns (id), so compiler does not know which type, therefore we have "shared".
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	// make sure changes to the defaults are not applied immediately so that we can revert them on "cancel"
	[sharedController setAppliesImmediately:FALSE];
	// read Atari drive dictionary from preferences
	NSUserDefaults *myDefaults = [sharedController defaults /* values ?*/];
	[m_AtariDrivesUrlDict setDictionary:[myDefaults dictionaryForKey:DMKAtariDrivesTableKey]];
#else
	// Add some demo stuff
	[m_AtariDrivesUrlDict setObject:@"URL://Floppy-A-Demo-Url" forKey:ATARI_DRIVE_STR(ATARI_DRIVE_A)];
	[m_AtariDrivesUrlDict setObject:@"URL://Drive-D-Demo-Url" forKey:ATARI_DRIVE_STR(ATARI_DRIVE_D)];
//	[m_AtariDrivesUrlDict setValue:@"URL://Drive-D-Demo-Url" forKey:ATARI_DRIVE_STR(ATARI_DRIVE_D)];	// does not accept const (!) NSstring
#endif
#else
    // Load up our sample data
    _tableContents = [NSMutableArray new];
    // Walk each string in the array until we hit the end (nil)
    NSString **data = &ATTableData[0];
    while (*data != nil)
	{
        NSString *name = *data;
        NSImage *image = [NSImage imageNamed:name];
        // our model will consist of a dictionary with Name/Image key pairs
        NSDictionary *dictionary = [[[NSDictionary alloc] initWithObjectsAndKeys:name, @"Name", image, @"Image", nil] autorelease];
        [_tableContents addObject:dictionary];
        data++;
    }
    [outletAtariDrivesTableView reloadData];
#endif

	// disable buttons
	[outletAtariDrivePathSelect setEnabled:NO];
	[outletAtariDriveRemove setEnabled:NO];
	[outletAtariDrive8p3 setEnabled:NO];

/*
	extern int myKeyboardHack;
	extern int myKeyBoardHackFunction(void);

	myKeyBoardHackFunction();
	myKeyboardHack = 1;
*/
/*
	extern int myKeyboardHack2;
	extern int myKeyBoardHackFunction2(void);
	
	myKeyBoardHackFunction2();
	myKeyboardHack2 = 1;
*/
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

		NSLog(@"get object value for column %s and row %d", columnId.UTF8String, (int)rowIndex);

#if 1
		NSParameterAssert(rowIndex >= 0 && rowIndex < ATARI_NUM_DRIVES);
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
				theValue = @"---------- root drive (reserved) ----------";
			}
			else
			if (rowIndex == ATARI_DRIVE_M)
			{
				theValue = @"---------- host file sytem (reserved) ----------";
			}
			else
			if (rowIndex == ATARI_DRIVE_U)
			{
				theValue = @"---------- Atari file sytem (reserved) ----------";
			}
			else
			{
#if 1
				theValue = [m_AtariDrivesUrlDict objectForKey:ATARI_DRIVE_STR(rowIndex)];
				if (theValue == nil)
				{
					theValue = @"(unused)";
				}
#else
				id theRecord;
				theRecord = [_tableContents objectAtIndex:rowIndex];
				theValue = [theRecord objectForKey:[aTableColumn identifier]];
				theValue = @"bla";
#endif
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
	return ATARI_NUM_DRIVES;
#else
	NSInteger retVal = [_tableContents count];
	NSLog(@"%s(): number = %d", __FUNCTION__, retVal);
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
	NSLog(@"close button pushed");
	return YES;
}


/*****************************************************************************************************
 *
 * Fenster: Cancel-Button
 *
 ****************************************************************************************************/

- (IBAction)actionCancel:(id)sender {
	printf("%s()\n", __func__);
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
	printf("%s()\n", __FUNCTION__);

	// note that sharedUserDefaultsController returns (id), so compiler does not know which type, therefore we have "shared".
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	// write Atari drive dictionary to preferences
	NSUserDefaults *myDefaults = [sharedController defaults];
	[myDefaults setObject:m_AtariDrivesUrlDict forKey:DMKAtariDrivesTableKey];

//	[myDefaults setValuesForKeysWithDictionary:m_AtariDrivesUrlDict];
	// save all changed preferences
	[sharedController save:self];
	// close window
	[self close];
}

- (IBAction)actionRegister:(id)sender {
	printf("%s()\n", __FUNCTION__);
}

- (IBAction)actionAtariMemoryStepper:(NSStepper *)sender {
	printf("%s()\n", __FUNCTION__);
}

- (IBAction)actionAtariScreenWidthStepper:(id)sender {
	printf("%s()\n", __FUNCTION__);
}

- (IBAction)actionAtariScreenHeightStepper:(id)sender {
	printf("%s()\n", __FUNCTION__);
}


/*****************************************************************************************************
 *
 * Fenster: Pfadauswahl für gewähltes Laufwerk
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrivePathSelect:(id)sender
{
	printf("%s()\n", __FUNCTION__);
	NSInteger driveNo = outletAtariDrivesTableView.selectedRow;
	//	[outletAtariDrivesTableView selectedColumn];	// equivalent
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_M && driveNo != ATARI_DRIVE_U && driveNo <= ATARI_DRIVE_Z)
	{
		NSOpenPanel *chooser = [NSOpenPanel openPanel];
		NSString *myTitle = [NSString stringWithFormat:@"Choose Path for Virtual Atari Drive %c:", (int)driveNo + 'A'];
		chooser.title = myTitle;
		chooser.canChooseFiles = NO;
		//	[DirectoryChooser setCanChooseFiles:NO];	// equivalent
		chooser.canChooseDirectories = YES;
		chooser.canCreateDirectories = YES;
		chooser.allowsMultipleSelection = NO;
		NSInteger ret = [chooser runModal];
		printf("%s() : runModal() -> %d\n", __FUNCTION__, (int)ret);
		if (ret == NSFileHandlingPanelOKButton)
		{
			printf("%s() File Chooser exited with OK, change drive %c:\n", __FUNCTION__, (int)driveNo + 'A');
			NSArray *urls = chooser.URLs;
			// das geht nicht, und NSString bleibt eine URL?!?
			NSURL *pathUrl = urls.lastObject;
			NSString *pathUrlString = [pathUrl absoluteString];
			[m_AtariDrivesUrlDict setObject:pathUrlString forKey:ATARI_DRIVE_STR(driveNo)];
			NSIndexSet *rowIndexSet = [NSIndexSet indexSetWithIndex:driveNo];
			NSIndexSet *colIndexSet = [NSIndexSet indexSetWithIndex:1];
			[outletAtariDrivesTableView reloadDataForRowIndexes:rowIndexSet columnIndexes:colIndexSet];
		}
	}
	else
	{
		printf("%s() : invalid selected drive %d\n", __FUNCTION__, (int)driveNo);
	}
}


/*****************************************************************************************************
 *
 * Fenster: Pfad entfernen
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDriveRemove:(id)sender
{
	printf("%s()\n", __FUNCTION__);
	NSInteger driveNo = outletAtariDrivesTableView.selectedRow;
	//	[outletAtariDrivesTableView selectedColumn];	// equivalent
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_M && driveNo != ATARI_DRIVE_U && driveNo <= ATARI_DRIVE_Z)
	{
		[m_AtariDrivesUrlDict removeObjectForKey:ATARI_DRIVE_STR(driveNo)];
		NSIndexSet *rowIndexSet = [NSIndexSet indexSetWithIndex:driveNo];
		NSIndexSet *colIndexSet = [NSIndexSet indexSetWithIndex:1];
		[outletAtariDrivesTableView reloadDataForRowIndexes:rowIndexSet columnIndexes:colIndexSet];
	}
	else
	{
		printf("%s() : invalid selected drive %d\n", __FUNCTION__, (int)driveNo);
	}
}


/*****************************************************************************************************
 *
 * Fenster: Laufwerk anklicken
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrivesTable:(id)sender
{
	printf("%s()\n", __FUNCTION__);
	NSInteger driveNo = outletAtariDrivesTableView.selectedRow;
	if (driveNo >= ATARI_DRIVE_A && driveNo != ATARI_DRIVE_C && driveNo != ATARI_DRIVE_M && driveNo != ATARI_DRIVE_U && driveNo <= ATARI_DRIVE_Z)
	{
		printf("%s() : selected drive %d\n", __FUNCTION__, (int)driveNo);
		id theValue;
		theValue = [m_AtariDrivesUrlDict objectForKey:ATARI_DRIVE_STR(driveNo)];
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
		}

		[outletAtariDrivePathSelect setEnabled:YES];	// select or change path
	}
	else
	{
		printf("%s() : invalid selected drive %d\n", __FUNCTION__, (int)driveNo);
		// disable buttons
		[outletAtariDrivePathSelect setEnabled:NO];
		[outletAtariDriveRemove setEnabled:NO];
		[outletAtariDrive8p3 setEnabled:NO];
	}
}


/*****************************************************************************************************
 *
 * Fenster: ungenutzte Aktionen
 *
 ****************************************************************************************************/

- (IBAction)actionAtariDrive8p3:(id)sender {
	printf("%s()\n", __FUNCTION__);
}


@end
