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
 *  MagicMacXPluginTypes.h
 *  CFPlugInDemo
 *
 *  Created by Andreas Kromke on Tue Jan 27 2004.
 *  Copyright (c) 2004 Andreas Kromke. All rights reserved.
 *
 */

//#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>

// Das  ist eine XCMD-Funktion
typedef UInt32 (tfXCMDFunction)(UInt32 parm, unsigned char *AdrOffset68k);

// Mit dieser Struktur werden Informationen von MagicMacX an
// ein XCMD-Modul übermittelt:

typedef int (tfXCmdCallback) (UInt32 cmd, void *pParm);
enum enXCmdCallbackCmd
{
	eXCmdCallbackCmd_BeginDialog = 1,
	eXCmdCallbackCmd_EndDialog = 2
};

struct XCmdInfo
{
	UInt32	xcmd_Length;		// Strukturlänge
	UInt32	xcmd_Version;		// Version
	tfXCmdCallback *xcmd_Callback;	// Callback-Funktion
};


// UUID wie bei Microsoft COM
#define kMagicMacXPluginTypeID (CFUUIDGetConstantUUIDWithBytes(NULL, 0x10, 0xBB,0x9D,0xB8,0x51,0x13,0x11,0xD8,0x87,0xD5,0x00,0x05,0x02,0x91,0xDE,0x40))

/* MagicMacXPluginType objects must implement MagicMacXPluginInterface */

#define kMagicMacXPluginInterfaceID (CFUUIDGetConstantUUIDWithBytes(NULL, 0xCB,0x60,0xAD,0xA6,0x51,0xDA,0x11,0xD8,0x90,0x76,0x00,0x05,0x02,0x91,0xDE,0x40))


typedef struct _MagicMacXPluginInterfaceStruct
{
	IUNKNOWN_C_GUTS;
	OSErr (*PluginInit)(void *pThis, const struct XCmdInfo *pInfo);
	OSErr (*PluginExit)(void *pThis);
	OSErr (*GetPluginInfo)(
			void *pThis,
			const char **pName,
			const char **pCreator,
			UInt32 *pVersionMajor,
			UInt32 *pVersionMinor,
			UInt32 *pNumOfSymbols);
	tfXCMDFunction *(*GetFunctionByName)(void *pThis, const char *pFnNam);
	tfXCMDFunction *(*GetFunctionByIndex)(void *pThis, UInt32 Fn, const char **pFnName);
} MagicMacXPluginInterfaceStruct;
