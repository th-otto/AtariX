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
#include <sys/param.h>

// Schalter

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
			uint16_t exc,
			uint32_t ErrAddr,
			const char *AccessMode,
			uint32_t pc,
			uint16_t sr,
			uint32_t usp,
			uint32_t *pDx,
			uint32_t *pAx,
			const char *ProcPath,
			uint32_t pd);
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
	static int Init(void);
	static OSErr GetDosPath(
				const FSSpec *pSpec,
				char *pBuf,
				unsigned uBufLen);
	static bool s_bRunning;
	// die ausführbare Datei
	// der Ordner, in dem die ausführbare Datei bzw. das Bundle liegt
	static FSSpec s_ProcDir;
	static long s_ProcDirID;			// hier liegt das Bundle bzw. die PEF-Datei
	static long s_ExecutableDirID;		// hier liegt die ausführbare Datei
	// nochmal dasselbe, aber als UNIX-Pfad (Versuch!)
	static char s_ThisPathNameUnix[1024];
	// Programmversion
	static NumVersion s_ProgramVersion;
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
