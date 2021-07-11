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
* Enthält alle globalen Variablen
*
*/

#include "config.h"
// System-Header
#include <CoreFoundation/CoreFoundation.h>
// Programm-Header
#include "Globals.h"
#include "Debug.h"

const char scrapFileName[] = "/GEMSYS/GEMSCRAP/SCRAP.TXT";


bool CGlobals::s_bRunning;
char CGlobals::s_atariKernelPathUrl[MAXPATHNAMELEN];
char CGlobals::s_atariRootfsPathUrl[MAXPATHNAMELEN];
char CGlobals::s_atariScrapFileUnixPath[MAXPATHNAMELEN];

/*
 * hier liegt die ausführbare Datei
 * FIXME: only used by printing, for the temp print files
 * FIXME2: there should be a better place for those files
 */
char CGlobals::s_ThisPathNameUnix[MAXPATHNAMELEN];

NumVersion CGlobals::s_ProgramVersion;
CFURLRef CGlobals::s_MagiCKernelUrl;
CFURLRef CGlobals::s_rootfsUrl;

CMyPreferences CGlobals::s_Preferences;
bool CGlobals::s_XFSDrvWasChanged[NDRIVES];
bool CGlobals::s_bShowMacMenu;
bool CGlobals::s_bAtariScreenManualSize;
unsigned short CGlobals::s_AtariScreenX;
unsigned short CGlobals::s_AtariScreenY;
unsigned short CGlobals::s_AtariScreenWidth;
unsigned short CGlobals::s_AtariScreenHeight;

CGlobals Globals;


/*****************************************************************
*
* Konstruktor. Wird VOR (!) main() aufgerufen.
* Achtung: Wird dieser ganze Sermon NICHT gemacht, knallt es!
*
******************************************************************/

CGlobals::CGlobals()
{
//	InitCursor ();
//	RegisterAppearanceClient();
}



/*****************************************************************
*
*  explizite Initialisierung
*
******************************************************************/

int CGlobals::Init(void)
{
	CFStringRef theString;
	CFStringRef executableUrl;

	if (s_atariKernelPathUrl[0])
	{
		// convert UTF8 encoded byte array containing URL to CFString
		theString = CFStringCreateWithCString(
								NULL,							// CFAllocatorRef alloc,
								s_atariKernelPathUrl,			// const UInt8 *bytes,
								kCFStringEncodingUTF8			// CFStringEncoding encoding,
								);


		// convert CFString to CFURL
		s_MagiCKernelUrl = CFURLCreateWithString(
								NULL,		// CFAllocatorRef allocator,
								theString,	// CFStringRef    filePath,
								NULL
								);
		CFRelease(theString);
		DebugInfo("CGlobals::Init() -- URLRef for kernel = %p", (void *)s_MagiCKernelUrl);
	}

	// convert UTF8 encoded byte array containing URL to CFString
	theString = CFStringCreateWithCString(
										NULL,							// CFAllocatorRef alloc,
										s_atariRootfsPathUrl,			// const UInt8 *bytes,
										kCFStringEncodingUTF8			// CFStringEncoding encoding,
										);
	
	
	// convert CFString to CFURL
	s_rootfsUrl = CFURLCreateWithString(
							 NULL,		// CFAllocatorRef allocator,
							 theString,	// CFStringRef    filePath,
							 NULL
							 );
	CFRelease(theString);
	DebugInfo("CGlobals::Init() -- URLRef for rootfs = %p", (void *)s_rootfsUrl);

	/*
	 * Get Unix path for Atari Clipboard file
	 */

	// Get Atari scrap file URL
	if (CFURLGetFileSystemRepresentation(CGlobals::s_rootfsUrl,
										 true,
										 (uint8_t *)s_atariScrapFileUnixPath,
										 sizeof(s_atariScrapFileUnixPath) - strlen(scrapFileName)))
	{
		strcat(s_atariScrapFileUnixPath, scrapFileName);
		DebugInfo("CClipboard::Mac2Atari() --- scrap file is \"%s\".\n", s_atariScrapFileUnixPath);
	}

	// determine path to executable
	executableUrl = CFURLCopyFileSystemPath(CFBundleCopyExecutableURL(CFBundleGetMainBundle()), kCFURLPOSIXPathStyle);
	CFStringGetCString(executableUrl, s_ThisPathNameUnix, sizeof(s_ThisPathNameUnix), kCFStringEncodingUTF8);
	CFRelease(executableUrl);
	{
		char *s = strrchr(s_ThisPathNameUnix, '/');
		if (s)
			s[0] = '\0';
		else
			strcpy(s_ThisPathNameUnix, "./");
	}
	DebugInfo("CGlobals::Init() -- Fullpath (Unix) = %s", s_ThisPathNameUnix);

	// Ermittle den "CFBundleVersion"-Eintrag in der "Info.plist" (new style)

	UInt32 uVersion = CFBundleGetVersionNumber(CFBundleGetMainBundle());
	*((UInt32 *) &s_ProgramVersion) = uVersion; 
	// or use CFBundleGetValueForInfoDictionaryKey with the key kCFBundleVersionKey

	return(0);
}
