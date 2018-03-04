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

#ifndef _INCLUDED_GLOBALS_H
#define _INCLUDED_GLOBALS_H

// System-Header
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
// Programm-Header
#include "MyPreferences.h"

// Schalter

#define min(a,b) ((a<b) ? (a) : (b))
#define max(a,b) ((a>b) ? (a) : (b))
#define MAX_ATARIMEMSIZE	(2U*1024U*1024U*1024U)		// 2 Gigabytes


// global functions used by XCMD
extern void MMX_BeginDialog(void);
extern void MMX_EndDialog(void);
// global function used by CMagiCWindow to report closing
extern void SendWindowClose( void );
// global function used by CMagiCWindow to report collapsing
extern void SendWindowCollapsing( void );
// global function used by CMagiCWindow to report collapsed window has re-expanded
extern void SendWindowExpanded( void );
// global function used by CMagiCWindow to report keyboard focus acquired
extern void SendWindowFocusAcquired( void );
// global function used by CMagiCWindow to report keyboard focus relinguish
extern void SendWindowFocusRelinguish( void );
// global function used by CMagiCWindow to report mouse clicks
extern void SendWindowMoveHandler( UInt32 evkind, Rect *pNewRect );
// global function used by CMagiCWindow to report mouse clicks
extern int SendMouseButtonHandler( unsigned int NumOfButton, bool bIsDown );
// global function used by CMagiC to report 68k exceptions
extern void Send68kExceptionData(	
			UInt16 exc,
			UInt32 ErrAddr,
			char *AccessMode,
			UInt32 pc,
			UInt16 sr,
			UInt32 usp,
			UInt32 *pDx,
			UInt32 *pAx,
			const char *ProcPath,
			UInt32 pd);
// global function used by AtariSysHalt
extern void SendSysHaltReason(const char *Reason);
extern void UpdateAtariDoubleBuffer(void);
extern DialogItemIndex MyAlert(SInt16 alertID, AlertType alertType);

class CGlobals
{
     public:
	CGlobals();
	static uint8_t s_atariKernelPathUrl[1024];		// UTF8 encoded
	static uint8_t s_atariRootfsPathUrl[1024];		// UTF8 encoded
	static uint8_t s_atariScrapFileUnixPath[1024];
//	static void InitDirectories(void);
	static int Init(void);
/* ersetzt durch "localizable.strings"
	static void GetRsrcStr(const unsigned char * name, char *s);
*/
	static OSErr GetDosPath(
				const FSSpec *pSpec,
				char *pBuf,
				unsigned uBufLen);
	static bool s_bRunning;
//	static short s_ThisResFile;
//	static ProcessSerialNumber s_ownPSN;
	static ProcessInfoRec s_ProcessInfo;
	// die ausführbare Datei
/*
	static FSSpec s_ProcessPath;
*/
	// der Ordner, in dem die ausführbare Datei bzw. das Bundle liegt
	static FSSpec s_ProcDir;
	static long s_ProcDirID;			// hier liegt das Bundle bzw. die PEF-Datei
	static long s_ExecutableDirID;		// hier liegt die ausführbare Datei
	// dasselbe, aber als Carbon-Pfad
	static char s_ThisPathNameCarbon[256];
	// nochmal dasselbe, aber als UNIX-Pfad (Versuch!)
	static char s_ThisPathNameUnix[256];
	// Programmversion
	static NumVersion s_ProgramVersion;
#if 0
	// HFS-Pfad des MagiC-Kernels (lokalisiert!)
	static Str255 s_MagiCKernelFilename;
#endif
	static CFURLRef s_MagiCKernelUrl;
	static CFURLRef s_rootfsUrl;
	// Mac-Menü eingeschaltet lassen, kein Vollbild
	static bool s_bShowMacMenu;
	// aktuelle Atari-Bildschirmgröße
	static bool s_bAtariScreenManualSize;
	static unsigned short s_AtariScreenX;
	static unsigned short s_AtariScreenY;
	static unsigned short s_AtariScreenWidth;
	static unsigned short s_AtariScreenHeight;

	static CMyPreferences s_Preferences;
	static bool s_XFSDrvWasChanged[NDRIVES];

   private:
};

extern CGlobals Globals;
#endif