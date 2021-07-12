#import "AppDelegate.h"
#include "EmulationMain.h"
#include "Debug.h"

/* these were renamed in SDK 10.12 and above */
#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#define NSAlertStyleWarning NSWarningAlertStyle
#define NSAlertStyleInformational NSInformationalAlertStyle
#define NSAlertStyleCritical NSCriticalAlertStyle
#endif


static NSString *DMKRootfsPathUrlKey = @"rootfsPathUrl";
static NSString *DMKAtariMemorySizeKey = @"atariMemorySize";
static NSString *DMKAtariScreenWidthKey = @"atariScreenWidth";
static NSString *DMKAtariScreenHeightKey = @"atariScreenHeight";
static NSString *DMKAtariPrintCommandKey = @"atariPrintCommand";
static NSString *DMKAtariSerialDeviceKey = @"atariSerialDevice";
static NSString *DMKAtariAutostartKey = @"atariAutostart";
static NSString *DMKAtariHideHostMouseKey = @"atariHideHostMouse";
static NSString *DMKAtariScreenColourModeKey = @"atariScreenColourMode";
static NSString *DMKAtariLanguageKey = @"atariLanguage";
static NSString *DMKAtariDrivesTableKey = @"atariDrivesTable";
static NSString *DMKAtariScreenStretchXKey = @"atariScreenStretchX";
static NSString *DMKAtariScreenStretchYKey = @"atariScreenStretchY";

@implementation AppDelegate

@synthesize preferencesController;

#pragma mark -
#pragma mark Services


/*****************************************************************************************************
 *
 * This is the second function to be called
 *
 ****************************************************************************************************/

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	DebugTrace("%s()", __func__);

	BOOL bRootFsValid = [self checkRootfs];
	if (!bRootFsValid)
	{
		// MAGX.INF does not exist

		NSAlert *alert;
		alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"OK"];
		[alert setMessageText:@"The Atari root file system is not initialised or is invalid.\nPlease first choose an existing one, or create one, then restart the application!"];
		[alert setInformativeText:@" The necessary file MAGX.INF could not be found in the current Atari root file system path.\n If you want to create a root file system using the currently defined path, you might select \"Revert Root FS\" from the menu.\n Alternatively you can choose a different path to either an already initialised root file system or to your desired root FS directory.\n In the latter case you must afterwards create the root file sytem as described."];
		[alert setAlertStyle:NSAlertStyleWarning];
		[alert runModal];
		[alert release];
	}

	m_configEmulationDone = false;

	// Test: Was passiert, wenn wir hier hängenbleiben
	/*
	for (;;)
		;
	*/
	// Ergebnis: Das Programm hängt ad infinitum

    // Direct service requests to self.
    [NSApp setServicesProvider:self];

/*
    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0 context:NULL subtype:0 data1:0 data2:0] atStart:NO];
 http://cocoadev.com/wiki/NSEvent
 http://stackoverflow.com/questions/10734349/simulate-keypress-for-system-wide-hotkeys
Of course, this assumes your delegate responds to shouldHandleEvents and handleEvent:. Going back to the fullscreen game example, this allows for one to have a Cocoa main
*/
	// Initialisierung
	EmulationInit();

	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	NSUserDefaults *myDefaults = [sharedController defaults /* values ?*/];
	BOOL atariAutostart = [myDefaults boolForKey:DMKAtariAutostartKey];
	if (bRootFsValid && atariAutostart)
	{
		DebugInfo("%s() -- Autostart", __func__);
		[self performSelector:@selector(actionRun:)];
	}

	// Wir starten hier die "event loop" für SDL. Wir könnten das auch später machen.
	// Es wird noch kein Fenster geöffnet.
	EmulationRunSdl();

	// hier kommen wir nie hin, bzw. nicht vor Programm-Ende
}


/*****************************************************************************************************
 *
 * This is the first function to be called
 *
 ****************************************************************************************************/

+ (void)initialize
{
	DebugTrace("%s()", __func__);
    NSMutableDictionary *initialValues = [NSMutableDictionary dictionary];

	// default Atari rootfs ("C:" Drive)
	//NSString *home = NSHomeDirectory(); und dann verketten geht auch, aber das folgende ist eleganter:
	NSString *defaultRootfs = [[NSString stringWithUTF8String:"~/MAGIC_C/"] stringByExpandingTildeInPath];
	DebugInfo("default rootfs path = %s", [defaultRootfs UTF8String]);
	// wandle NSString in NSURL um, damit wir das "file://localhost/" vorne kriegen
#ifdef _DEBUG
	NSURL *pathUrl = [[NSURL alloc] initFileURLWithPath:defaultRootfs isDirectory:YES];
	// und jetzt wandeln wir die Url wieder in eine Zeichenkette um, damit wir sie anschauen können
	DebugInfo("current rootfs URL = %s", [[pathUrl absoluteString] UTF8String]);
#endif
	[initialValues setObject:defaultRootfs forKey:DMKRootfsPathUrlKey];

	// default Atari memory size: 16 MB
    [initialValues setObject:[NSNumber numberWithInteger:4] forKey:DMKAtariMemorySizeKey];
	// default Atari screen size: 1024 x 768
	[initialValues setObject:[NSNumber numberWithInteger:1024] forKey:DMKAtariScreenWidthKey];
	[initialValues setObject:[NSNumber numberWithInteger:768] forKey:DMKAtariScreenHeightKey];

	// send raw data to printer command
	[initialValues setObject:[NSString stringWithUTF8String:"lp -o raw %s"] forKey:DMKAtariPrintCommandKey];
	// modem device
	[initialValues setObject:[NSString stringWithUTF8String:"/dev/cu.Bluetooth-Modem"] forKey:DMKAtariSerialDeviceKey];

	[initialValues setObject:[NSNumber numberWithBool:NO] forKey:DMKAtariAutostartKey];
	[initialValues setObject:[NSNumber numberWithBool:NO] forKey:DMKAtariHideHostMouseKey];

	[initialValues setObject:[NSNumber numberWithInteger:3] forKey:DMKAtariScreenColourModeKey];
	[initialValues setObject:[NSNumber numberWithInteger:1] forKey:DMKAtariLanguageKey];

	[initialValues setObject:[NSNumber numberWithBool:NO] forKey:DMKAtariScreenStretchXKey];
	[initialValues setObject:[NSNumber numberWithBool:NO] forKey:DMKAtariScreenStretchYKey];

	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
    [sharedController setInitialValues:initialValues];
}


/*****************************************************************************************************
 *
 * Pass all configuration to the emulation runner. Can be done only once during process lifetime.
 *
 ****************************************************************************************************/
- (void) configEmulation
{
	if (m_configEmulationDone)
	{
		DebugWarning("%s() -- Guest already configured, reconfiguration not supported.", __func__);
	}

	/*
	 * Pass various settings from the default Controller to the Emulator
	 */

	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];

	// get Atari drive map and tell emulator
	NSUserDefaults *myDefaults = [sharedController defaults /* values ?*/];
	NSDictionary *atariDrivesUrlDict = [NSDictionary dictionaryWithDictionary:[myDefaults dictionaryForKey:DMKAtariDrivesTableKey]];
	// go through dictionary
	[atariDrivesUrlDict enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop)
	 {
		 NSString *drvname = key;
		 unichar c = [drvname characterAtIndex:0];
		 unsigned drvnr = c - 'A';
		 NSString *path = obj;
		 DebugInfo("Enumerating Key %s and Value %s", [key UTF8String], [obj UTF8String]);
		 NSURL *drvUrl = [NSURL URLWithString:path];
		 // __bridge wird benötigt, wenn Automatic Reference Counting eingeschaltet ist
		 CFURLRef myCFURL = (__bridge CFURLRef) drvUrl;
		 EmulationChangeAtariDrive(drvnr, myCFURL);
		 // wir müssen hier den Referenzzähler erhöhen, weil wir die NSURL als CFURL weitergegeben haben und sie dort gespeichert wird.
		 [drvUrl retain];
	 }];
	
	// get Atari memory size and tell emulator
	NSInteger atariMemorySize       = [myDefaults integerForKey:DMKAtariMemorySizeKey];
	NSInteger atariScreenWidth      = [myDefaults integerForKey:DMKAtariScreenWidthKey];
	NSInteger atariScreenHeight     = [myDefaults integerForKey:DMKAtariScreenHeightKey];
	NSInteger atariScreenColourMode = [myDefaults integerForKey:DMKAtariScreenColourModeKey];
	BOOL      atariScreenStretchX   = [myDefaults boolForKey:DMKAtariScreenStretchXKey];
	BOOL      atariScreenStretchY   = [myDefaults boolForKey:DMKAtariScreenStretchYKey];
	NSInteger atariLanguage         = [myDefaults integerForKey:DMKAtariLanguageKey];
	BOOL      atariHideHostMouse    = [myDefaults boolForKey:DMKAtariHideHostMouseKey];
	//BOOL      atariAutostart     = [myDefaults boolForKey:DMKAtariAutostartKey];
	NSString *atariPrintCommand     = [myDefaults stringForKey:DMKAtariPrintCommandKey];
	NSString *atariSerialDevice     = [myDefaults stringForKey:DMKAtariSerialDeviceKey];
	NSString *atariRootfsUrlString  = [myDefaults stringForKey:DMKRootfsPathUrlKey];
	
	NSString *atariKernelUrlString = [self getAtariKernelUrl:atariLanguage];
	EmulationConfig(
					atariKernelUrlString ? [atariKernelUrlString cStringUsingEncoding:NSUTF8StringEncoding] : NULL,
					[atariRootfsUrlString cStringUsingEncoding:NSUTF8StringEncoding],
					atariMemorySize, atariScreenWidth, atariScreenHeight,
					atariScreenColourMode,
					atariScreenStretchX, atariScreenStretchY,
					atariLanguage,
					atariHideHostMouse,
					[atariPrintCommand cStringUsingEncoding:NSUTF8StringEncoding],
					[atariSerialDevice cStringUsingEncoding:NSUTF8StringEncoding]);
	//[atariKernelUrlString release];	CRASH?

	m_configEmulationDone = true;
}

- (void)showPreferences:(id)sender
{
	DebugTrace("%s()", __func__);
    if (preferencesController == nil)
	{
#if 1
		// erzeuge das "PreferencesController"-Objekt
		preferencesController = [PreferencesWindowController alloc];
		// und setze es auch als "File's Owner", sonst wird AppDelegate der "File's Owner". KLAPPT NICHT.
		[preferencesController initWithWindowNibName:@"Preferences" owner:preferencesController];
#else
        preferencesController = [[PreferencesWindowController alloc] initWithWindowNibName:@"Preferences"];
#endif
    }
    [preferencesController showWindow:self];
}


#pragma mark -
#pragma mark Memory management

- (void)dealloc
{
	DebugTrace("%s()", __func__);
    [preferencesController release];
    [super dealloc];
}


/*****************************************************************************************************
 *
 * Menüeintrag "About"
 *
 ****************************************************************************************************/

- (IBAction)actionAbout:(id)sender
{
	DebugTrace("%s()", __func__);
	About(sender);
}


/*****************************************************************************************************
 *
 * Menüeintrag "Preferences"
 *
 ****************************************************************************************************/

- (IBAction)actionPreferences:(id)sender
{
	DebugTrace("%s()", __func__);
	[self showPreferences:sender];
}


/*****************************************************************************************************
 *
 * Menüeintrag "Quit"
 *
 ****************************************************************************************************/

- (IBAction)actionQuit:(id)sender
{
	DebugTrace("%s()", __func__);
//	[NSApp terminate: nil];
	[NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
}

#if 0
- (void)longCode
{
	DebugTrace("%s()", __func__);
	EmulationRunSdl();
/*
    NSAutoreleasePool *pool;
    pool = [[NSAutoreleasePool alloc] init];
    BOOL keepGoing = YES;
	
    while ( keepGoing) {
        // Do something here that will eventually stop by
        // setting keepGoing to NO
    }
	
    [pool release];
*/
	DebugTrace("%s() =>", __func__);
}
#endif


/*****************************************************************************************************
 *
 * Menüeintrag "Run"
 *
 ****************************************************************************************************/

- (IBAction)actionRun:(id)sender
{
	DebugTrace("%s()", __func__);

#if 0
	// Initialisierung
	EmulationInit();
	// Wenn wir hier die "event loop" für SDL starten, kommen wir aus dieser Funktion
	// vor Prorammende nicht mehr raus, und der Menütitel bleibt "selektiert".
	EmulationRunSdl();
#endif

	if (!EmulationIsRunning())
	{
		[self configEmulation];		// is ignored, if already configured
		EmulationOpenWindow();
		EmulationRun();
#if 0
		// geht nicht, da SDL im "main thread" laufen muß
		[NSThread detachNewThreadSelector:@selector(longCode)
								 toTarget:self withObject:nil];
#endif
	}
	DebugTrace("%s() =>", __func__);
}


/*****************************************************************************************************
 *
 * Save new rootfs URL string to preferences
 *
 ****************************************************************************************************/

- (void)changeRootfsUrl:(NSString *)newUrl;
{
	// kopiert von den "Preferences":

	// note that sharedUserDefaultsController returns (id), so compiler does not know which type, therefore we have "shared".
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	// write Atari rootfs drive path to preferences
	NSUserDefaults *myDefaults = [sharedController defaults];
	[myDefaults setObject:newUrl forKey:DMKRootfsPathUrlKey];
	
	// save all changed preferences
	[sharedController save:self];
}


/*****************************************************************************************************
 *
 * Check if rootfs is valid, in fact just check existance of MAGX.INF
 *
 ****************************************************************************************************/

- (BOOL) checkRootfs
{
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	NSUserDefaults *myDefaults = [sharedController defaults /* values ?*/];
	NSString *pathUrlString = [myDefaults stringForKey:DMKRootfsPathUrlKey];
	[myDefaults release];

	NSURL *pathUrl = [NSURL URLWithString:pathUrlString];
	NSString *rootFsDir = [pathUrl path];
	
	DebugInfo("init rootfs pathURL = %s", [pathUrlString UTF8String]);
	DebugInfo("init rootfs path = %s", [rootFsDir UTF8String]);

	NSString *magxInf = [rootFsDir stringByAppendingPathComponent:@"MAGX.INF"];
	DebugInfo("changed path = %s", [magxInf UTF8String]);
	NSFileManager *fileManager = [NSFileManager defaultManager];

	return [fileManager isReadableFileAtPath:magxInf];
}


/*****************************************************************************************************
 *
 * Get kernel URL string
 *
 ****************************************************************************************************/

- (NSString *) getAtariKernelUrl:(NSInteger)atariLanguage;
{
	NSBundle *myBundle = [NSBundle mainBundle];
//	NSURL *resourceUrl = [myBundle resourceURL];
	DebugInfo("resource path = %s", [[myBundle resourcePath] UTF8String]);

	NSURL *pathUrl;
	if (atariLanguage == 0)
		pathUrl = [myBundle URLForResource:@"MagicMacX" withExtension:@"OS"];
	else
	if (atariLanguage == 1)
		pathUrl = [myBundle URLForResource:@"MagicMacX" withExtension:@"OS" subdirectory:nil localization:@"de"];
	else
	if (atariLanguage == 2)
		pathUrl = [myBundle URLForResource:@"MagicMacX" withExtension:@"OS" subdirectory:nil localization:@"English"];
	else
	if (atariLanguage == 3)
		pathUrl = [myBundle URLForResource:@"MagicMacX" withExtension:@"OS" subdirectory:nil localization:@"fr"];
	else
	{
		DebugWarning("invalid localisation code %d", (int) atariLanguage);
		pathUrl = NULL;
	}
	NSString *pathUrlString;
	if (pathUrl)
		pathUrlString = [pathUrl absoluteString];
	else
		pathUrlString = NULL;
	DebugInfo("kernel localized path = %s", pathUrlString ? [pathUrlString UTF8String] : "(nil)");
	return pathUrlString;
}


/*****************************************************************************************************
 *
 * Menüeintrag "Choose Rootfs"
 *
 ****************************************************************************************************/

- (IBAction)actionChooseRootfs:(id)sender
{
	DebugTrace("%s() =>", __func__);
	NSOpenPanel *chooser = [NSOpenPanel openPanel];
	chooser.title = NSLocalizedString(@"Choose Atari Root-FS Directory (Boot Drive)", nil);
	chooser.message = NSLocalizedString(@"The directory must be named MAGIC_C. It will appear as virtual drive C: inside the virtual machine.", nil);
	chooser.canChooseFiles = NO;
	//	[DirectoryChooser setCanChooseFiles:NO];	// equivalent
	chooser.canChooseDirectories = YES;
	chooser.canCreateDirectories = YES;
	chooser.allowsMultipleSelection = NO;
	NSInteger ret = [chooser runModal];

	DebugInfo("%s() : runModal() -> %d", __FUNCTION__, (int)ret);

	if (ret == NSFileHandlingPanelOKButton)
	{
		DebugInfo("%s() File Chooser exited with OK, change rootfs path", __FUNCTION__);
		NSArray *urls = chooser.URLs;
		// das geht nicht, und NSString bleibt eine URL?!?
		NSURL *pathUrl = urls.lastObject;
		NSString *pathUrlString = [pathUrl absoluteString];
		DebugInfo("path = %s", [pathUrlString UTF8String]);

		// if path does not end with MAGIC_C, ask

		NSString *dirName = [pathUrlString lastPathComponent];
		if (NSOrderedSame != [dirName caseInsensitiveCompare:@"MAGIC_C"])
		{
#if 0
//			extern int GuiMyAlert(const char *msg_text, const char *info_text, int nButtons);
//			NSInteger retcode = GuiMyAlert("dd", "bb", 2);
			NSInteger retcode = 3;
#else
			// CRASHES !!!

			NSAlert *alert = [[NSAlert alloc] init];
			[alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];
			[alert addButtonWithTitle:NSLocalizedString(@"Do not add", nil)];
			[alert addButtonWithTitle:NSLocalizedString(@"Add", nil)];
			[alert setMessageText:NSLocalizedString(@"Create directory MAGIC_C inside the provided path?", nil)];
			[alert setInformativeText:NSLocalizedString(@"The path provided does not end with directory MAGIC_C. It should be added automatically.", nil)];
			[alert setAlertStyle:NSAlertStyleWarning];
			ret = [alert runModal];
			[alert release];
#endif
			if ((ret == 1) || (ret == NSAlertFirstButtonReturn))
			{
				// cancel
				DebugInfo("cancelled");
				return;
			}
			else
			if ((ret == 2) || (ret == NSAlertSecondButtonReturn))
			{
				DebugInfo("leave path unchanged (not recommended)");
			}
			else
			if ((ret == 3) || (ret == NSAlertThirdButtonReturn))
			{
				pathUrlString = [pathUrlString stringByAppendingString:@"MAGIC_C/"];
				DebugInfo("changed path = %s", [pathUrlString UTF8String]);
			}
		}
	//	[dirName release];	// crashes. why?

		// save to prefrences
		[self changeRootfsUrl:pathUrlString];
	}
}


/*****************************************************************************************************
 *
 * Recursive copy
 *
 ****************************************************************************************************/

static BOOL PathCopy(NSString *destPath, NSString *srcPath)
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	BOOL result;
	NSError *error;
	BOOL finalResult = YES;


	NSDirectoryEnumerator *myEnumerator = [fileManager enumeratorAtPath:srcPath];
	NSString *sourceItem;
	while (sourceItem = [myEnumerator nextObject])
	{
		BOOL isDir;
		NSString *sourceItemPath      = [srcPath  stringByAppendingPathComponent:sourceItem];
		NSString *destinationItemPath = [destPath stringByAppendingPathComponent:sourceItem];
		[fileManager fileExistsAtPath:sourceItemPath isDirectory:&isDir];
		//		DebugInfo("source item path = %s", [sourceItemPath UTF8String]);
		//		DebugInfo("dest   item path = %s", [destinationItemPath UTF8String]);
		
		if (isDir)
		{
			// source item is a directory. Create destination directory.
			// Note that with "withIntermediateDirectories:NO" there would be errors if the directory exists
			DebugInfo("create dir \"%s\"", [destinationItemPath UTF8String]);
			result = [fileManager createDirectoryAtPath:destinationItemPath withIntermediateDirectories:YES attributes:nil error:&error];
			if (result == NO)
			{
				finalResult = NO;
				DebugError("%s", [[error localizedDescription] UTF8String]);
			}
		}
		else
		{
			// source item is a file. copy it.
			if ([fileManager fileExistsAtPath:destinationItemPath])
			{
				// file exists. delete, then overwrite.
				DebugInfo("remove file \"%s\"", [destinationItemPath UTF8String]);
				result = ([fileManager removeItemAtPath:destinationItemPath error:&error]);
				if (result == NO)
				{
					finalResult = NO;
					DebugWarning("%s", [[error localizedDescription] UTF8String]);
				}
			}

			DebugInfo("copy file \"%s\" to \"%s\"", [sourceItemPath UTF8String], [destinationItemPath UTF8String]);
			result = [fileManager copyItemAtPath:sourceItemPath toPath:destinationItemPath error:&error];
			if (result == NO)
			{
				finalResult = NO;
				DebugError("%s", [[error localizedDescription] UTF8String]);
			}
		}
	}

	return finalResult;
}


/*****************************************************************************************************
 *
 * Menüeintrag "Revert Rootfs"
 *
 ****************************************************************************************************/

- (IBAction)actionRevertRootfs:(id)sender
{
	DebugTrace("%s() =>", __func__);
	NSInteger ret;
	NSAlert *alert;

	//
	// warning alert
	//

	alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];
	[alert addButtonWithTitle:NSLocalizedString(@"Create or Revert", nil)];
	[alert setMessageText:NSLocalizedString(@"Overwrite files inside the provided directory?", nil)];
	[alert setInformativeText:NSLocalizedString(@"You are going to create or revert the Atari boot drive. Existing files or directories inside the same directory will be overwritten.", nil)];
	[alert setAlertStyle:NSAlertStyleWarning];
	ret = [alert runModal];
	[alert release];
	if ((ret == 1) || (ret == NSAlertFirstButtonReturn))
	{
		// cancel
		DebugInfo("cancelled");
		return;
	}

	// get path URL to write rootfs
	NSUserDefaultsController *sharedController = [NSUserDefaultsController sharedUserDefaultsController];
	NSUserDefaults *myDefaults = [sharedController defaults /* values ?*/];
	NSInteger atariLanguage = [myDefaults integerForKey:DMKAtariLanguageKey];
	NSString *pathUrlString = [myDefaults stringForKey:DMKRootfsPathUrlKey];
	[myDefaults release];

	NSURL *pathUrl = [NSURL URLWithString:pathUrlString];
	NSString *rootFsDir = [pathUrl path];

	DebugInfo("init rootfs pathURL = %s", [pathUrlString UTF8String]);
	DebugInfo("init rootfs path = %s", [rootFsDir UTF8String]);
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSBundle *myBundle = [NSBundle mainBundle];

	DebugInfo("resource path = %s", [[myBundle resourcePath] UTF8String]);
	NSString *rootfsCommonPath = [myBundle pathForResource:@"rootfs-common" ofType:nil];
	NSString *rootfsLocalizedPath;
	if (atariLanguage == 0)
		rootfsLocalizedPath = [myBundle pathForResource:@"rootfs" ofType:nil];
	else
	if (atariLanguage == 1)
		rootfsLocalizedPath = [myBundle pathForResource:@"rootfs" ofType:nil inDirectory:nil forLocalization:@"de"];
	else
	if (atariLanguage == 2)
		rootfsLocalizedPath = [myBundle pathForResource:@"rootfs" ofType:nil inDirectory:nil forLocalization:@"English"];
	else
	if (atariLanguage == 3)
		rootfsLocalizedPath = [myBundle pathForResource:@"rootfs" ofType:nil inDirectory:nil forLocalization:@"fr"];
	else
	{
		DebugWarning("invalid localisation code %d", (int) atariLanguage);
	}

	DebugInfo("resource rootfs-common path = %s", [rootfsCommonPath UTF8String]);
	DebugInfo("resource rootfs localized path = %s", [rootfsLocalizedPath UTF8String]);

	BOOL finalResult;
	finalResult = [fileManager createDirectoryAtPath:rootFsDir withIntermediateDirectories:YES attributes:nil error:nil];

	//
	// copy common, i.e. unlocalised files and folders
	//
#if 0
	NSArray *files = [fileManager contentsOfDirectoryAtPath:rootfsCommonPath error:nil];
	for (NSString *file in files)
	{
		// TODO: skip hiden files (starting with '.')
		NSString *sourceItem      = [rootfsCommonPath stringByAppendingPathComponent:file];
		NSString *destinationItem = [rootFsDir        stringByAppendingPathComponent:file];
		DebugInfo("copy source item %s to destination item %s", [sourceItem UTF8String], [destinationItem UTF8String]);
        result = [fileManager copyItemAtPath:sourceItem toPath:destinationItem error:&error];
        if (result == NO)
        {
			finalResult = NO;
            DebugError("%s", [[error localizedDescription] UTF8String]);
		}
	}
#endif

	if (finalResult == YES)
	{
		BOOL finalResult1 = PathCopy(rootFsDir, rootfsCommonPath);
		BOOL finalResult2 = PathCopy(rootFsDir, rootfsLocalizedPath);
		finalResult = finalResult1 || finalResult2;
	}


#if 0
	//
	// copy localised files. unfortunately directories cannot be merged in a simple way.
	//

	// TODO: use it the correct way, i.e. merge recursively

	rootfsLocalizedPath = [rootfsLocalizedPath stringByAppendingString:@"/GEMSYS/GEMDESK"];
	rootFsDir           = [rootFsDir           stringByAppendingString:@"/GEMSYS/GEMDESK"];

	NSArray *localFiles = [fileManager contentsOfDirectoryAtPath:rootfsLocalizedPath error:nil];
	for (NSString *file in localFiles)
	{
		NSString *sourceItem      = [rootfsLocalizedPath stringByAppendingPathComponent:file];
		NSString *destinationItem = [rootFsDir        stringByAppendingPathComponent:file];
		DebugInfo("copy source item %s to destination item %s", [sourceItem UTF8String], [destinationItem UTF8String]);
        result = [fileManager copyItemAtPath:sourceItem toPath:destinationItem error:&error];
        if (result == NO)
        {
			finalResult = NO;
            DebugError("%s", [[error localizedDescription] UTF8String]);
		}
	}
#endif

	//
	// User feedback with result
	//

	alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	if (finalResult == YES)
	{
		[alert setMessageText:NSLocalizedString(@"Atari boot drive successfully created.", nil)];
		[alert setInformativeText:NSLocalizedString(@"All files and folders were successfully copied to the Atari boot drive directory.", nil)];
		[alert setAlertStyle:NSAlertStyleInformational];
	}
	else
	{
		[alert setMessageText:NSLocalizedString(@"There were errors during creation.", nil)];
		[alert setInformativeText:NSLocalizedString(@"Not all files and folders could be copied successfully. Your Atari boot drive may be incomplete.", nil)];
		[alert setAlertStyle:NSAlertStyleWarning];
	}
	[alert runModal];
	[alert release];
}

- (IBAction)actionCut:(id)sender {
}

- (IBAction)actionCopy:(id)sender {
}

- (IBAction)actionPaste:(id)sender {
}

@end
