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

// System-Header
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "XCmd.h"
#include "Atari.h"
#include <dirent.h>
#include <sys/stat.h>

// Schalter

// Tabelle der geladenen Plugins
CXCmd::tsLoadedPlugin CXCmd::s_Plugins[MAX_PLUGINS];

typedef UInt8                           CFragSymbolClass;
enum {
                                        /* Values for type CFragSymbolClass.*/
  kCodeCFragSymbol              = 0,
  kDataCFragSymbol              = 1,
  kTVectorCFragSymbol           = 2,
  kTOCCFragSymbol               = 3,
  kGlueCFragSymbol              = 4
};

/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CXCmd::CXCmd()
{
	m_XCMDFolderSpec = 0;

	// Diese Struktur geht an CFM/PEF PlugIns

	m_XCmdInfo.xcmd_Length = sizeof(m_XCmdInfo);
	m_XCmdInfo.xcmd_Version = 0;
	m_XCmdInfo.xcmd_Callback = Callback;

	// Diese Struktur geht an MachO PlugIns

	m_XCmdPlugInInfo.xcmd_Length = sizeof(m_XCmdPlugInInfo);
	m_XCmdPlugInInfo.xcmd_Version = 0;
	m_XCmdPlugInInfo.xcmd_Callback = Callback;
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CXCmd::~CXCmd()
{
}



/**********************************************************************
*
* Initialisieren
*
**********************************************************************/

int CXCmd::Init(void)
{
	char dirname[MAXPATHNAMELEN];
	struct stat st;
	int err = 0;

	strcpy(dirname, CGlobals::s_ThisPathNameUnix);
	strcat(dirname, "Preload-XCMDs");
	
	if (stat(dirname, &st) != 0 || !S_ISDIR(st.st_mode))
	{
		err = errno;
		DebugError("CXCmd::Init() -- Verzeichnis \"Preload-XCMDs\" nicht gefunden");
		m_XCMDFolderSpec = NULL;
		return err;
	}

	m_XCMDFolderSpec = strdup(dirname);

	if (m_XCMDFolderSpec)
		err = Preload();

	return err;
}


/**********************************************************************
*
* (INTERN) Alle XCmds im Verzeichnis laden
*
**********************************************************************/

int CXCmd::Preload(void)
{
	DIR *dir;
	struct dirent *ent;
	char filename[MAXPATHNAMELEN];
	struct stat st;
	int i;

	dir = opendir(m_XCMDFolderSpec);
	if (dir == NULL)
		return errno;

	// durchsuche das Verzeichnis, bis ein Fehler auftritt
	while (i < MAX_PLUGINS && (ent = readdir(dir)) != NULL)
	{
		if (strcmp(ent->d_name, ".") == 0 ||
			strcmp(ent->d_name, "..") == 0 ||
			ent->d_name[0] == '.')
			continue;	// unsichtbare Dateien überspringen
		strcpy(filename, m_XCMDFolderSpec);
		strcat(filename, "/");
		strcat(filename, ent->d_name);
		// Verzeichnisse überspringen
		if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode))
			continue;

		// Datei gefunden. Laden
		if (Load(filename, &s_Plugins[i]) == 0)
			i++;
	}
	closedir(dir);

	return 0;
}


/**********************************************************************
*
* (STATISCH) Callback für XCmd
*
* Hiermit kann ein XCMD Funktionen von MagicMacX aufrufen.
*
**********************************************************************/

int CXCmd::Callback(uint32_t cmd, void *pParm)
{
	#pragma unused(pParm)

	DebugInfo("CXCmd::Callback(cmd = %d)", cmd);
//	ExitToShell();
	switch(cmd)
	{
		case eXCmdCF_BeginDialog:
			MMX_BeginDialog();
			return(0);

		case eXCmdCF_EndDialog:
			MMX_EndDialog();
			return(0);

		default:
			return(1);
	}
}


/**********************************************************************
*
* XCmd initialisieren
*
**********************************************************************/

void CXCmd::InitXCMD(struct tsLoadedPlugin *plugin)
{
#ifdef NOTYET
	XCmdInitFunctionProcType *MyFunctionProcPtr;


	DebugTrace("CXCmd::InitXCMD()");

	MyFunctionProcPtr = (XCmdInitFunctionProcType *)dlsym(plugin->handle, XCMD_INIT_FN_NAME);

	if (MyFunctionProcPtr != 0)
	{
		(void) (*MyFunctionProcPtr)(&m_XCmdInfo);
		DebugInfo("CXCmd::InitXCMD() -- Initialisierungsfunktion aufgerufen");
	}
#endif
}


/**********************************************************************
*
* XCmd über GetSharedLibrary laden
*
**********************************************************************/

int CXCmd::Load(const char *libName, struct tsLoadedPlugin *plugin)
{
	void *handle;

	DebugInfo("CXCmd::Load() -- Lade XCMD mit dem Bibliotheknamen \"%s\"", libName);

	handle = dlopen(libName, RTLD_LAZY);

	if (handle == 0)
	{
		DebugInfo(" CXCmd::Load() -- GetSharedLibrary => %s", dlerror());
		return ENOENT;
	}

	return 0;
}


/**********************************************************************
*
* XCmd als CFPlugin (MachO) laden
*
**********************************************************************/

int CXCmd::LoadPlugin
(
	const char *pUnixPath,			// Plugin name or complete path
	const char *SearchPath,			// path (e.g. "~/Library/MagicMacX/PlugIns")
	struct tsLoadedPlugin *plugin
)
{
	char szPath[MAXPATHNAMELEN];
	int err;
	CFURLRef plugInURL;
	CFStringRef cpUnixPath;


	// Pfad zusammenbauen. Baue "~/Library/MagicMacX/PlugIns" davor.
	// Es sei denn, es ist schon ein Pfad drin.

	szPath[255] = '\0';
	if	(strchr(pUnixPath, '/'))
	{
		// full path given
		strncpy(szPath, pUnixPath, 255);
	}
	else
	if (!SearchPath)
	{
		// name and no search path: look in application bundle contents/PlugIns

		// TODO: remove hack
		if (!strcmp(pUnixPath, "CSLSeparation"))
			pUnixPath = "MmxPluginSeparation.bundle";
		else
		if (!strcmp(pUnixPath, "CSLMacFlate"))
			pUnixPath = "MmxPluginDeflate.bundle";
		else
		if (!strcmp(pUnixPath, "CSLMacPreview"))
			pUnixPath = "MmxPluginPreview.bundle";
		else
		if (!strcmp(pUnixPath, "CSLMacPrintX"))
			pUnixPath = "MmxPluginPrintx.bundle";
		else
		if (!strcmp(pUnixPath, "CSLType1Import"))
			pUnixPath = "MmxPluginType1Importer.bundle";
		strncpy(szPath, pUnixPath, 255);
	}
	else
	{
		// name and search path
		strcpy(szPath, SearchPath);
		strcat(szPath, pUnixPath);
	}

	DebugInfo("CXCmd::LoadPlugin() -- szPath = \"%s\"", szPath);
	cpUnixPath = CFStringCreateWithCString(NULL, szPath, kCFStringEncodingMacRoman);
	if (SearchPath)
	{
		plugInURL = CFURLCreateWithFileSystemPath(NULL, cpUnixPath, kCFURLPOSIXPathStyle, TRUE);
	}
	else
	{
		CFURLRef bundleURL;
		// Obtain a URL to the PlugIns directory inside our application.
		bundleURL = CFBundleCopyBuiltInPlugInsURL(CFBundleGetMainBundle());
		CFShow(bundleURL);
		//  We just want to load our test plug-in, so append its name to the URL.
		plugInURL = CFURLCreateCopyAppendingPathComponent(NULL, bundleURL, cpUnixPath, FALSE /*no directory*/);
		CFRelease(bundleURL);
	}

	CFShow(plugInURL);
	plugin->PluginRef = CFPlugInCreate(NULL, plugInURL);
	CFRelease(plugInURL);
	CFRelease(cpUnixPath);

	if	(plugin->PluginRef == 0)
	{
		DebugError("CXCmd::LoadPlugin() -- CFPlugInCreate() failed with path \"%s\"", szPath);
		return ENOENT;
	}


	// See if this plug-in implements our type.
	// CFPlugInFindFactoriesForPlugInType
	// Searches all registered plug-ins for factory functions capable of creating an instance of the given type.
//	CFArrayRef factories = CFPlugInFindFactoriesForPlugInType(kMagicMacXPluginTypeID);
	// CFPlugInFindFactoriesForPlugInTypeInPlugIn
	// Searches the given plug-in for factory functions capable of creating an instance of the given type.

	CFArrayRef factories = CFPlugInFindFactoriesForPlugInTypeInPlugIn(kMagicMacXPluginTypeID, plugin->PluginRef);
	if	((factories != NULL) && (CFArrayGetCount(factories) > 0))
	{

		// Get the factory ID for the first location in the array of IDs.

		CFUUIDRef factoryID = (CFUUIDRef) CFArrayGetValueAtIndex(factories, 0);

		// Use the factory ID to get an IUnknown interface.
		// Here the code for the PlugIn is loaded.

		IUnknownVTbl **iunknown = (IUnknownVTbl **) CFPlugInInstanceCreate(NULL, factoryID, kMagicMacXPluginTypeID);

		// If this is an IUnknown interface, query for our interface.

		if	(iunknown)
		{
			DebugInfo("CXCmd::LoadPlugin() -- PlugIn loaded.\n");

			MagicMacXPluginInterfaceStruct **interfaceTable = NULL;
			(*iunknown)->QueryInterface(iunknown, CFUUIDGetUUIDBytes(kMagicMacXPluginInterfaceID), (LPVOID *)(&interfaceTable));
			// Now we are done with IUnknown
			(*iunknown)->Release(iunknown);
			if	(interfaceTable)
			{
				DebugInfo("CXCmd::LoadPlugin() -- MagicMacX interface found.\n");

				plugin->pInterface = interfaceTable[0];		// wir nehmen das erste Interface, das paßt
				err = plugin->pInterface->PluginInit(plugin->pInterface, &m_XCmdPlugInInfo);
				if (err)
				{
					DebugError("CXCmd::LoadPlugin() -- Error on PlugInInit()\n");
				}
				else
				{
					DebugInfo("CXCmd::LoadPlugin() -- PlugIn successfully opened\n");
				}
				// Now we are done with our interface

				// Done with our interface.
				// This causes the plug-in’s code to be unloaded.

				//plugin->pInterface->Release(plugin->pInterface);
			}
			else
			{
				DebugInfo("CXCmd::LoadPlugin() -- Failed to get interface.\n");
				err = ENOENT;
			}

			// MF:!! release the two interface refs...
		}
		else
		{
			DebugInfo("CXCmd::LoadPlugin() -- Failed to create instance.\n");
			err = ENOENT;
		}
	}
	else
	{
		DebugInfo("CXCmd::LoadPlugin() -- Could not find any factories.\n");
		err = ENOENT;
	}

	if	(factories)
		CFRelease(factories);

	// Release the CFPlugin.
	// Memory for the plug-in is deallocated here.


	return err;
}


/**********************************************************************
*
* Debug-Funktion: Gib alle Symbole einer PEF/CFM-Bibliothek aus
*
**********************************************************************/

#ifdef _DEBUG
static void DebugPrintSymbolsInPlugIn(MagicMacXPluginInterfaceStruct *pInterface, uint32_t nSymbols)
{
	tfXCMDFunction *pFn;
	uint32_t i;
	const char *s;


	DebugInfo("CXCmd::Command() -- Symbole:");
	for	(i = 0; i < nSymbols; i++)
	{
		pFn = pInterface->GetFunctionByIndex(pInterface, i, &s);
		if	(pFn)
			DebugInfo("CXCmd::Command() --    Symbol \"%s\"", s);
	}
}
#else
#define DebugPrintSymbolsInPlugIn(p, n)
#endif


/**********************************************************************
*
* Lade eine Bibliothek mit einem gegebenen Pfad oder Namen
*
**********************************************************************/

int CXCmd::OnCommandLoadLibrary
(
	const char *szLibName,		// name oder Pfad
	bool bIsPath,			// true: Pfad / false: Name
	uint32_t *pXCmdDescriptor,
	int32_t *pNumOfSymbols
)
{
	int err;
	uint32_t i;
	const char *pPluginName;
	const char *pPluginCreator;
	uint32_t ulPluginVersionMajor;
	uint32_t ulPluginVersionMinor;
	char szSearchPath[MAXPATHNAMELEN];
	uint32_t ulNumOfSym;


	DebugInfo("CXCmd::OnCommandLoadLibrary(\"%s\") by %s", szLibName, (bIsPath) ? "path" : "name");

	// suche freien Platz in Tabelle
	for	(i = 0; i < MAX_PLUGINS; i++)
	{
		if	(s_Plugins[i].handle == 0)
			goto found;
	}
	return EMFILE;

	found:

	if	(bIsPath)
	{
		//
		// Wird der Pfad angegeben, unterstützen wir nur CFM/PEF
		//

		err = Load(szLibName, &s_Plugins[i]);
	}
	else
	{
		//
		// Wir probieren erst ein CFPlugIn (MachO-Bundle) aus dem Application Bundle "Contents/PlugIns"
		// zu laden
		//

		err = LoadPlugin(
				szLibName,
				NULL,
				&s_Plugins[i]);

		//
		// Wir probieren ein CFPlugIn (MachO-Bundle) von "~/Library/MagicMacX/PlugIns"
		// zu laden
		//

		if (err)
		{
			strcpy(szSearchPath, getenv("HOME"));
			strcat(szSearchPath, "/Library/MagicMacX/PlugIns/");

			err = LoadPlugin(
					szLibName,
					szSearchPath,
					&s_Plugins[i]);
		}

		//
		// Jetzt suchen wir ein CFPlugIn (MachO-Bundle) in "/SystemLibrary/MagicMacX/PlugIns"
		//

		if	(err)
			err = LoadPlugin(
					szLibName,
					"/System/Library/MagicMacX/PlugIns/",
					&s_Plugins[i]);

		if	(err)
		{
			err = Load(szLibName, &s_Plugins[i]);
			// typical error: cfragNoLibraryErr (-2804)
		}
	}

	if	(err)
	{
		*pXCmdDescriptor = 0xffffffff;
		*pNumOfSymbols = 0;
	}
	else
	{
		MagicMacXPluginInterfaceStruct *pInterface;
		
		pInterface = s_Plugins[i].pInterface;

		*pXCmdDescriptor = i;

		pInterface->GetPluginInfo(
					pInterface,
					&pPluginName,
					&pPluginCreator,
					&ulPluginVersionMajor,
					&ulPluginVersionMinor,
					&ulNumOfSym);

		DebugInfo("CXCmd::OnCommandLoadLibrary() -- CFPlugIn geladen");
		DebugInfo("CXCmd::OnCommandLoadLibrary() --  Name = %s", pPluginName);
		DebugInfo("CXCmd::OnCommandLoadLibrary() --  Autor = %s", pPluginCreator);
		DebugInfo("CXCmd::OnCommandLoadLibrary() --  Version %u.%u", ulPluginVersionMajor, ulPluginVersionMinor);
		DebugPrintSymbolsInPlugIn(pInterface, ulNumOfSym);

		*pNumOfSymbols = ulNumOfSym;
	}

	return err;
}


/**********************************************************************
*
* Entlade eine geladene Bibliothek
*
**********************************************************************/

int CXCmd::OnCommandUnloadLibrary
(
	uint32_t XCmdDescriptor
)
{
	MagicMacXPluginInterfaceStruct *pInterface;

	DebugInfo("CXCmd::OnCommandUnloadLibrary(%u)", XCmdDescriptor);

	if (XCmdDescriptor >= MAX_PLUGINS)
		return EBADF;
	pInterface = s_Plugins[XCmdDescriptor].pInterface;
	if (pInterface == 0)
		return ENOENT;
	// Plugin freigeben
	pInterface->Release(pInterface);
	// nochmal (?) freigeben
	CFRelease(s_Plugins[XCmdDescriptor].PluginRef);
	// auch hier abmelden
	s_Plugins[XCmdDescriptor].PluginRef = 0;
	s_Plugins[XCmdDescriptor].pInterface = 0;
	s_Plugins[XCmdDescriptor].handle = 0;
	return 0;
}


/**********************************************************************
*
* Finde ein Symbol in einer geladenen Bibliothek
*
**********************************************************************/

int CXCmd::OnCommandFindSymbol
(
	uint32_t XCmdDescriptor,
	char *pSymName,
	uint32_t SymIndex,
	unsigned char *pSymClass,
	void **pSymbolAddress
)
{
	MagicMacXPluginInterfaceStruct *pInterface;
	int err;
	const char *s;
	tfXCMDFunction *pFn;


#ifdef _DEBUG
	if	(SymIndex == 0xffffffff)
		DebugInfo("CXCmd::OnCommandFindSymbol(\"%s\")", pSymName);
	else
		DebugInfo("CXCmd::OnCommandFindSymbol(%u)", (unsigned int)SymIndex);
#endif
	if	(XCmdDescriptor >= MAX_PLUGINS)
	{
		DebugError("CXCmd::OnCommandFindSymbol()-- illegal Descriptor %u", (unsigned int)SymIndex);
		return ENOENT;
	}
	pInterface = s_Plugins[XCmdDescriptor].pInterface;

	if	(SymIndex == 0xffffffff)
	{
		pFn = pInterface->GetFunctionByName(pInterface, pSymName);
	}
	else
	{
		pFn = pInterface->GetFunctionByIndex(pInterface, SymIndex, &s);
		if	(pFn)
			strcpy(pSymName, s);
	}

	if	(pFn)
	{
		*pSymbolAddress = (void *) pFn;
		*pSymClass =  kCodeCFragSymbol;
		err = 0;
	}
	else
	{
		*pSymbolAddress = 0;
		err = ENOENT;
	}

#ifdef _DEBUG
	if	(err)
		DebugError("CXCmd::OnCommandFindSymbol() -- failure");
	else
		DebugInfo("CXCmd::OnCommandFindSymbol() -- success");
#endif

	return err;
}


/**********************************************************************
*
* Callback des Emulators: XCmd-Kommando
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L866 */
int32_t CXCmd::Command(uint32_t params, unsigned char *AdrOffset68k)
{
	strXCMD *pCmd = (strXCMD *) (params + AdrOffset68k);
	int32_t ret;
	int err;


	DebugInfo("CXCmd::Command(%d)", pCmd->m_cmd);

	switch(pCmd->m_cmd)
	{
		case eXCMDVersion:
			err = 0;
			ret = 0;
			break;

		case eXCMDMaxCmd:
			err = 0;
			ret = 14;
			break;

		case eXCMDLoadByPath:
			err = OnCommandLoadLibrary(
						pCmd->m_10_11.m_PathOrName,
						true,
						&pCmd->m_LibHandle,
						&pCmd->m_10_11.m_nSymbols);
			ret = 0;
			break;

		case eXCMDLoadByLibName:
			err = OnCommandLoadLibrary(
						pCmd->m_10_11.m_PathOrName,
						false,
						&pCmd->m_LibHandle,
						&pCmd->m_10_11.m_nSymbols);
			ret = 0;
			break;

		case eXCMDGetSymbolByName:
			err = OnCommandFindSymbol(
					pCmd->m_LibHandle,
					pCmd->m_12_13.m_Name,
					0xffffffff,
					&pCmd->m_12_13.m_SymClass,
					&pCmd->m_12_13.m_SymPtr);
			ret = 0;
			break;

		case eXCMDGetSymbolByIndex:
			err = OnCommandFindSymbol(
					pCmd->m_LibHandle,
					pCmd->m_12_13.m_Name,
					(UInt32) pCmd->m_12_13.m_Index,
					&pCmd->m_12_13.m_SymClass,
					&pCmd->m_12_13.m_SymPtr);
			ret = 0;
			break;

		case eUnLoad:
			err = OnCommandUnloadLibrary((UInt32) pCmd->m_LibHandle);
			ret = 0;
			break;

		default:
			err = 0;
			ret = TOS_EUNCMD;
			break;
	}; 

	pCmd->m_MacError = err;
	if	(err)
		ret = TOS_ERROR;
	return ret;
}
