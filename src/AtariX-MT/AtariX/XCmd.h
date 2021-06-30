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
* Enthält die Verwaltung der nachladbaren Module
*
*/

#ifndef _XCMD_H_INCLUDED
#define _XCMD_H_INCLUDED

// System-Header
// Programm-Header
#include "XCmdDefs.h"
#include "MagicMacXPluginTypes.h"

// Schalter

#if TARGET_RT_MAC_MACHO
struct GlueCode
{
	GlueCode *pNext;
//	CFragConnectionID id;		// zugehörige Bibliothek
	void *p;				// Zeiger auf MachO-Glue-Code
};

enum enRunTimeFormat
{
	eUnused = 0,
	eMachO = 1,
	ePEF = 2
};

#define MAX_PLUGINS	128		// maximum of plugins
#endif

class CXCmd
{
   public:
	// Konstruktor
	CXCmd();
	// Destruktor
	~CXCmd();
	// initialisieren
	int Init(void);
	// XCmd laden
	OSErr Load(FSSpec *pSpec, CFragConnectionID* pConnectionId);
	OSErr Load(ConstStr63Param libName, CFragConnectionID* pConnectionId);
	OSErr LoadPlugin(
			const char *pPath,
			const char *SearchPath,
			CFPlugInRef *pRef,
			MagicMacXPluginInterfaceStruct **ppInterface);
	// die zentrale Kommandofunktion
	int32_t Command(uint32_t params, unsigned char *AdrOffset68k);

   private:
#if TARGET_RT_MAC_MACHO
	struct tsLoadedPlugin
	{
		enRunTimeFormat RunTimeFormat;
		// used for CFM/PEF
		CFragConnectionID ConnID;
		GlueCode *pGlueList;
		// used for MachO
		CFPlugInRef PluginRef;
		MagicMacXPluginInterfaceStruct *pInterface;
	};
	static tsLoadedPlugin s_Plugins[MAX_PLUGINS];
#endif
	OSErr OnCommandLoadLibrary(
				const char *szLibName,
				bool bIsPath,
				uint32_t *pDescriptor,
				int32_t *pNumOfSymbols);
	OSErr OnCommandUnloadLibrary(UInt32 XCmdDescriptor);
	OSErr OnCommandFindSymbol(
				UInt32 XCmdDescriptor,
				char *pSymName,
				UInt32 SymNumber,
				unsigned char *pSymClass,
				UInt32 *pSymbolAddress
				);
   	OSErr Preload(void);
   	void InitXCMD(CFragConnectionID ConnectionId);
   	static XCmdCallbackFunctionProcType Callback;

   	strXCmdInfo m_XCmdInfo;		// Info-Struktur geht an PEF/CFM PlugIns
   	struct XCmdInfo m_XCmdPlugInInfo;	// Info-Struktur geht an MachO PlugIns
	FSSpec m_XCMDFolderSpec;
#if TARGET_RT_MAC_MACHO
	void *NewGlue(void *pCFragPtr, UInt32 XCmdDescriptor, CFragSymbolClass symclass);
#endif
};
#endif
