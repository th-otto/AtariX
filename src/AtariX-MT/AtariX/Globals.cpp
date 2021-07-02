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
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Globals.h"
#include "PascalStrings.h"
extern "C" {
#include "MyMoreFiles.h"
}
#include "FullPath.h"
#include "Debug.h"

const char scrapFileName[] = "/GEMSYS/GEMSCRAP/SCRAP.TXT";


bool CGlobals::s_bRunning;
ProcessInfoRec CGlobals::s_ProcessInfo;
uint8_t CGlobals::s_atariKernelPathUrl[1024];
uint8_t CGlobals::s_atariRootfsPathUrl[1024];
uint8_t CGlobals::s_atariScrapFileUnixPath[1024];
FSSpec CGlobals::s_ProcDir;
long CGlobals::s_ProcDirID;					// hier liegt das Bundle
long CGlobals::s_ExecutableDirID;				// hier liegt die ausführbare Datei
char CGlobals::s_ThisPathNameCarbon[256];
char CGlobals::s_ThisPathNameUnix[256];

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

	if (s_atariKernelPathUrl[0])
	{
		// convert UTF8 encoded byte array containing URL to CFString
		theString = CFStringCreateWithBytes(
								NULL,							// CFAllocatorRef alloc,
								s_atariKernelPathUrl,			// const UInt8 *bytes,
								strlen((const char *) s_atariKernelPathUrl),	// CFIndex numBytes,
								kCFStringEncodingUTF8,			// CFStringEncoding encoding,
								false							// Boolean isExternalRepresentation
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
//	const char *s = (const char *) s_atariRootfsPathUrl;

	// convert UTF8 encoded byte array containing URL to CFString
	theString = CFStringCreateWithBytes(
										NULL,							// CFAllocatorRef alloc,
										s_atariRootfsPathUrl,			// const UInt8 *bytes,
										strlen((const char *) s_atariRootfsPathUrl),	// CFIndex numBytes,
										kCFStringEncodingUTF8,			// CFStringEncoding encoding,
										false							// Boolean isExternalRepresentation
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
										 s_atariScrapFileUnixPath,
										 sizeof(s_atariScrapFileUnixPath) - strlen(scrapFileName)))
	{
		strcat((char *) s_atariScrapFileUnixPath, scrapFileName);
		DebugInfo("CClipboard::Mac2Atari() --- scrap file is \"%s\".\n", s_atariScrapFileUnixPath);
	}

	// Carbon-Pfad ermitteln

	{
		Handle hFullPath;
		short FullPathLen;
		OSErr err2;

		err2 = FSpGetFullPath(&s_ProcDir, &FullPathLen, &hFullPath);
		if	((!err2) && (FullPathLen))
		{
			HLock(hFullPath);
			if	(FullPathLen < 256)
			{
				memcpy(s_ThisPathNameCarbon, (const unsigned char *) (*hFullPath), FullPathLen);
				s_ThisPathNameCarbon[FullPathLen] = '\0';
				DebugInfo("CGlobals::Init() -- Fullpath (Carbon) = %s", s_ThisPathNameCarbon);
			}
			DisposeHandle(hFullPath);
		}
	}

	// Berechne Unix-Pfad aus Carbon-Pfad (Versuch)

	{
		char *s;

		s = strchr(s_ThisPathNameCarbon, ':');
		if	(s)
		{
			if	(s_ProcDir.vRefNum == -100)
			{
				DebugInfo("CGlobals::Init() -- Boot-Volume");
				s_ThisPathNameUnix[0] = '/';
				strcpy(s_ThisPathNameUnix+1, s+1);
			}
			else
			{
				DebugInfo("CGlobals::Init() -- Nicht Boot-Volume");
				strcpy(s_ThisPathNameUnix, "/Volumes/");
				strcat(s_ThisPathNameUnix, s_ThisPathNameCarbon);
			}

			while((s = strchr(s_ThisPathNameUnix, ':')) != NULL)
			{
				*s = '/';
			}

			DebugInfo("CGlobals::Init() -- Fullpath (Unix) = %s", s_ThisPathNameUnix);
		}
		else
			DebugError("CGlobals::Init() -- falscher Pfad?");
	}

	// Ermittle den "CFBundleVersion"-Eintrag in der "Info.plist" (new style)

	UInt32 uVersion = CFBundleGetVersionNumber(CFBundleGetMainBundle());
	*((UInt32 *) &s_ProgramVersion) = uVersion; 
	// or use CFBundleGetValueForInfoDictionaryKey with the key kCFBundleVersionKey

	return(0);
}


/*****************************************************************
*
*  Berechne DOS-Pfad (M:\xxxx) aus FSSpec
*
******************************************************************/

OSErr CGlobals::GetDosPath
(
	const FSSpec *pSpec,
	char *pBuf,
	unsigned uBufLen
)
{
	OSErr err;
	Handle hFullPath;
	short FullPathLen;
	char *s;


	err = FSpGetFullPath(pSpec, &FullPathLen, &hFullPath);
	if	((!err) && (FullPathLen))
	{
		HLock(hFullPath);
		if	((unsigned) FullPathLen < uBufLen-4)
		{
			memcpy(pBuf, "M:\\", 3);
			memcpy(pBuf + 3, (const unsigned char *) (*hFullPath), FullPathLen);
			pBuf[FullPathLen + 3] = '\0';
			DebugInfo("CGlobals::GetDosPath() -- Fullpath (Carbon) = %s", pBuf + 3);
			while((s = strchr(pBuf + 3, ':')) != NULL)
			{
				*s = '\\';
			}
			DebugInfo("CGlobals::GetDosPath() -- Fullpath (DOS) = %s", pBuf);
		}
		DisposeHandle(hFullPath);
	}

	return(err);
}
