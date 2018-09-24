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
* Enth�lt Definitionen f�r MagicMacX-XCMD-Module
*
*/

#ifndef _XCMDDEFS_H_INCLUDED
#define _XCMDDEFS_H_INCLUDED

// So sieht eine XCMD-Funktion aus

typedef UInt32 (XCMDFunction)(UInt32 parm, unsigned char *AdrOffset68k);


// Mit dieser Struktur werden Informationen von MagicMacX an
// ein XCMD-Modul �bermittelt:

typedef int (XCmdCallbackFunctionProcType) (UInt32 cmd, void *pParm);
enum XCmdCallbackFunction
{
	eXCmdCF_BeginDialog = 1,
	eXCmdCF_EndDialog = 2
};

struct strXCmdInfo
{
	UInt32	xcmd_Length;		// Strukturl�nge
	UInt32	xcmd_Version;		// Version
	XCmdCallbackFunctionProcType	*xcmd_Callback;		// Callback-Funktion
};

// Diese Funktion wird von MagicMacX in jedem XCMD aufgerufen
#define XCMD_INIT_FN_NAME "\pXCmdInit"
#define XCMD_INIT_FN XCmdInit

typedef int (XCmdInitFunctionProcType) (struct strXCmdInfo *pInfo);

#endif
