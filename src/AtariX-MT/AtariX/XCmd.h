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

#define MAX_PLUGINS	128		// maximum of plugins

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
	// die zentrale Kommandofunktion
	int32_t Command(uint32_t params, unsigned char *AdrOffset68k);

   private:
	struct tsLoadedPlugin
	{
		CFPlugInRef PluginRef;
		void *handle;
		MagicMacXPluginInterfaceStruct *pInterface;
	};
	static tsLoadedPlugin s_Plugins[MAX_PLUGINS];

	OSErr Load(const char *libName, struct tsLoadedPlugin *plugin);
	OSErr LoadPlugin(
			const char *pPath,
			const char *SearchPath,
			struct tsLoadedPlugin *plugin);

	OSErr OnCommandLoadLibrary(
				const char *szLibName,
				bool bIsPath,
				uint32_t *pDescriptor,
				int32_t *pNumOfSymbols);
	OSErr OnCommandUnloadLibrary(uint32_t XCmdDescriptor);
	OSErr OnCommandFindSymbol(
				uint32_t XCmdDescriptor,
				char *pSymName,
				uint32_t SymNumber,
				unsigned char *pSymClass,
				void **pSymbolAddress
				);
   	int Preload(void);
   	void InitXCMD(struct tsLoadedPlugin *plugin);
   	static XCmdCallbackFunctionProcType Callback;

   	strXCmdInfo m_XCmdInfo;		// Info-Struktur geht an PEF/CFM PlugIns
   	struct XCmdInfo m_XCmdPlugInInfo;	// Info-Struktur geht an MachO PlugIns
	char *m_XCMDFolderSpec;
};
#endif
