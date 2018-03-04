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
#include <Carbon/Carbon.h>
#include <machine/endian.h>
#include <StdLib.h>
#include <String.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "PascalStrings.h"
#include "XCmd.h"
#include "Atari.h"

extern "C" {
#include "MyMoreFiles.h"
#if TARGET_RT_MAC_MACHO
#include "MachOCFMGlue.h"
#endif
}
// Schalter

#if TARGET_RT_MAC_MACHO
// Tabelle der geladenen Plugins
CXCmd::tsLoadedPlugin CXCmd::s_Plugins[MAX_PLUGINS];
#endif


/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CXCmd::CXCmd()
{
	m_XCMDFolderSpec.vRefNum = 0;
	m_XCMDFolderSpec.parID = 0;

	// Diese Struktur geht an CFM/PEF PlugIns

	m_XCmdInfo.xcmd_Length = sizeof(m_XCmdInfo);
	m_XCmdInfo.xcmd_Version = 0;
#if TARGET_RT_MAC_MACHO
	m_XCmdInfo.xcmd_Callback = (XCmdCallbackFunctionProcType *) CFMFunctionPointerForMachOFunctionPointer((void *) Callback);
#else
	m_XCmdInfo.xcmd_Callback = Callback;
#endif

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
	OSErr err;
	Boolean isDir;

	err = FSMakeFSSpec(
			CGlobals::s_ProcDir.vRefNum,
			CGlobals::s_ProcDirID,
			"\pPreload-XCMDs",
			&m_XCMDFolderSpec);
	if	(err)
	{
		DebugError("CXCmd::Init() -- Fehler %d bei FSMakeFSSpec", err);
		m_XCMDFolderSpec.vRefNum = 0;
		m_XCMDFolderSpec.parID = 0;
		return(err);
	}

	FSpResolveAlias(&m_XCMDFolderSpec);
	err = FSpGetDirectoryID (&m_XCMDFolderSpec, &m_XCMDFolderSpec.parID, &isDir);
	m_XCMDFolderSpec.name[0] = 0;
	if (err || !isDir)
	{
		DebugError("CXCmd::Init() -- Verzeichnis \"pPreload-XCMDs\" nicht gefunden", err);
		m_XCMDFolderSpec.vRefNum = 0;
		m_XCMDFolderSpec.parID = 0;
		if	(!err)
			err = dirNFErr;
		return(err);
	}

	if (m_XCMDFolderSpec.parID)
		err = Preload();

	return(err);
}


/**********************************************************************
*
* (INTERN) Alle XCmds im Verzeichnis laden
*
**********************************************************************/

OSErr CXCmd::Preload(void)
{
	CFragConnectionID ConnectionId;
	short	index;
	OSErr	err, err2;
	CInfoPBRec myCPB;
	Str63	fName/*, dirname*/;
	FSSpec spec;


//	GetDirName (m_XCMDFolderSpec.vRefNum, m_XCMDFolderSpec.parID, dirname);

	myCPB.hFileInfo.ioNamePtr = fName;
	myCPB.hFileInfo.ioVRefNum = m_XCMDFolderSpec.vRefNum;

	// durchsuche das Verzeichnis, bis ein Fehler auftritt
	for	(index = 1, err = 0; err == 0; index++)
	{
		myCPB.hFileInfo.ioFDirIndex = index;	// set up index
		myCPB.hFileInfo.ioDirID = m_XCMDFolderSpec.parID;

		// hole den (index)-ten Verzeichniseintrag
		err = PBGetCatInfoSync (&myCPB);
		if	(err)	// Ende
			break;

		if	((myCPB.hFileInfo.ioFlAttrib & ioDirMask) || (myCPB.hFileInfo.ioFlFndrInfo.fdFlags & fInvisible))
			continue;	// Verzeichnisse und unsichtbare Dateien überspringen

		// Datei gefunden. FSSpec erstellen
		err2 = FSMakeFSSpec (myCPB.hFileInfo.ioVRefNum, myCPB.hFileInfo.ioFlParID, fName, &spec);
		if	(!err2)
			err2 = FSpResolveAlias (&spec);
		if	(err2)
		{
			P2C(fName);
			DebugWarning("CXCmd::Preload() -- Fehler beim Öffnen von %s", fName+1);
			continue;	// überspringen
		}

		(void) Load(&spec, &ConnectionId);
	};

	return(0);
}


/**********************************************************************
*
* (STATISCH) Callback für XCmd
*
* Hiermit kann ein XCMD Funktionen von MagicMacX aufrufen.
*
**********************************************************************/

int CXCmd::Callback(UInt32 cmd, void *pParm)
{
	#pragma unused(pParm)

#ifdef _DEBUG
	CDebug::GeneralPurposeVariable++;
#endif
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

void CXCmd::InitXCMD(CFragConnectionID ConnectionId)
{
	OSErr err;
	XCmdInitFunctionProcType *MyFunctionProcPtr;


	DebugInfo("CXCmd::InitXCMD()");

	err = FindSymbol
			(
			ConnectionId,					// CFragConnectionID connID
			XCMD_INIT_FN_NAME,				// ConstStr255Param symName
			(Ptr *) &MyFunctionProcPtr,			// Ptr *symAddr
			NULL							// CFragSymbolClass *symClass
			);

	if	(!err)
	{
#if TARGET_RT_MAC_MACHO
		XCmdInitFunctionProcType *p = (XCmdInitFunctionProcType *) MachOFunctionPointerForCFMFunctionPointer((void *) MyFunctionProcPtr);
		(void) (*p)(&m_XCmdInfo);
		DisposeMachOFunctionPointer((void *) p);
#else
		(void) (*MyFunctionProcPtr)(&m_XCmdInfo);
#endif
		DebugInfo("CXCmd::InitXCMD() -- Initialisierungsfunktion aufgerufen");
	}
}


/**********************************************************************
*
* XCmd über FSSpec laden
*
**********************************************************************/

OSErr CXCmd::Load(FSSpec *pSpec, CFragConnectionID* pConnectionId)
{
	CInfoPBRec pb;
	Ptr mainAddr;
	Str255 errMessage;
	OSErr err;
	UInt32 LenDF;


	DebugInfo(" CXCmd::Load(FSSpec *) -- Lade XCMD mit dem Dateinamen \"%#s\"", pSpec->name);

	// Dateilänge ermitteln

	pb.hFileInfo.ioVRefNum = pSpec -> vRefNum;
	pb.hFileInfo.ioNamePtr = pSpec -> name;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = pSpec -> parID;
	err = PBGetCatInfoSync (&pb);
	if	(err)
		return(err);

	// Bibliothek laden

	LenDF = (UInt32) pb.hFileInfo.ioFlLgLen;
	err = GetDiskFragment(
			pSpec,				// const FSSpec *       fileSpec,
			0,					//  UInt32               offset,
			LenDF,				// UInt32               length,
			NULL,					// ConstStr63Param      fragName,         /* can be NULL */
			kPrivateCFragCopy,		// CFragLoadOptions     options,
	  		pConnectionId,			// CFragConnectionID *  connID,
			&mainAddr,				//  Ptr *                mainAddr,
	 		errMessage				// Str255               errMessage)
			);

 	P2C(errMessage);
	DebugInfo("CXCmd::Load() -- GetDiskFragment => %d,%s", err, errMessage+1);

	if	(!err)
		InitXCMD(*pConnectionId);

	return(err);
}


/**********************************************************************
*
* XCmd über GetSharedLibrary laden
*
**********************************************************************/

OSErr CXCmd::Load(ConstStr63Param libName, CFragConnectionID* pConnectionId)
{
#if !TARGET_RT_MAC_MACHO
	Ptr mainAddr;
	Str255 errMessage;
#endif
	OSErr err;


	DebugInfo("CXCmd::Load(ConstStr63Param *) -- Lade XCMD mit dem Bibliotheknamen \"%#s\"", libName);

#if TARGET_RT_MAC_MACHO
	FSSpec Spec;
	unsigned char buf[256];
	pstrcpy(buf, libName);			// kopiere Pascal-String
	P2C(buf);					// mit '\0' abschließen
	char *s = (char *) (buf+1);
	bool bExt;
	char *t;
	t = strrchr(s, '.');
	bExt = (t != NULL);
	if	(!t)
	{
		t = s + strlen(s);
		strcpy(t, ".lib");
	}

	// 1. Versuch: Dateiname = Bibliotheksname mit Endung ".lib" bzw. gegebener Endung
	C2P(buf);
	DebugInfo(" CXCmd::Load(ConstStr63Param *) -- Mache FSSpec mit dem Dateinamen \"%#s\"", buf);
	err = FSMakeFSSpec(
			CGlobals::s_ProcDir.vRefNum,
			CGlobals::s_ExecutableDirID,		// suche mitten im Bundle, da, wo die exe liegt
			buf,
			&Spec);
	// 1. Versuch: Dateiname = Bibliotheksname mit Endung ".shlb", wenn keine Endung vorgegeben
	if	((err) && (!bExt))
	{
		strcpy(t, ".Shlb");
		C2P(buf);
		DebugInfo(" CXCmd::Load(ConstStr63Param *) -- Mache FSSpec mit dem Dateinamen \"%#s\"", buf);
		err = FSMakeFSSpec(
				CGlobals::s_ProcDir.vRefNum,
				CGlobals::s_ExecutableDirID,		// suche mitten im Bundle, da, wo die exe liegt
				buf,
				&Spec);
	}
	if	(!err)
		err = Load(&Spec, pConnectionId);
#else
	err = GetSharedLibrary(
		libName,						//  ConstStr63Param      libName,
  		kPowerPCCFragArch,				// CFragArchitecture    archType,
  		kPrivateCFragCopy,				// CFragLoadOptions     options,
  		pConnectionId,					// CFragConnectionID *  connID,
		&mainAddr,						//  Ptr *                mainAddr,
 		errMessage						// Str255               errMessage)
 		);

 	P2C(errMessage);
	DebugInfo(" CXCmd::Load() -- GetSharedLibrary => %d,%s", err, errMessage+1);
	if	(!err)
		InitXCMD(*pConnectionId);
#endif

	return(err);
}


/**********************************************************************
*
* XCmd als CFPlugin (MachO) laden
*
**********************************************************************/

OSErr CXCmd::LoadPlugin
(
	const char *pUnixPath,			// Plugin name or complete path
	const char *SearchPath,			// path (e.g. "~/Library/MagicMacX/PlugIns")
	CFPlugInRef *pPluginRef,
	MagicMacXPluginInterfaceStruct **ppInterface
)
{
	char szPath[256];
	OSErr err;
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
	*pPluginRef = CFPlugInCreate(NULL, plugInURL);
	CFRelease(plugInURL);
	CFRelease(cpUnixPath);

	if	(!*pPluginRef)
	{
		DebugError("CXCmd::LoadPlugin() -- CFPlugInCreate() failed with path \"%s\"", szPath);
		return(fnfErr);
	}


	// See if this plug-in implements our type.
	// CFPlugInFindFactoriesForPlugInType
	// Searches all registered plug-ins for factory functions capable of creating an instance of the given type.
//	CFArrayRef factories = CFPlugInFindFactoriesForPlugInType(kMagicMacXPluginTypeID);
	// CFPlugInFindFactoriesForPlugInTypeInPlugIn
	// Searches the given plug-in for factory functions capable of creating an instance of the given type.

	CFArrayRef factories = CFPlugInFindFactoriesForPlugInTypeInPlugIn(kMagicMacXPluginTypeID, *pPluginRef);
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

				*ppInterface = interfaceTable[0];		// wir nehmen das erste Interface, das paßt
				err = (*ppInterface)->PluginInit(*ppInterface, &m_XCmdPlugInInfo);
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

				//(*ppInterface)->Release(*ppInterface);
			}
			else
			{
				DebugInfo("CXCmd::LoadPlugin() -- Failed to get interface.\n");
				err = fnfErr;
			}

			// MF:!! release the two interface refs...
		}
		else
		{
			DebugInfo("CXCmd::LoadPlugin() -- Failed to create instance.\n");
			err = fnfErr;
		}
	}
	else
	{
		DebugInfo("CXCmd::LoadPlugin() -- Could not find any factories.\n");
		err = fnfErr;
	}

	if	(factories)
		CFRelease(factories);

	// Release the CFPlugin.
	// Memory for the plug-in is deallocated here.


	return(err);
}


/**********************************************************************
*
* Erzeuge neuen Glue-Code für ein Symbol
*
**********************************************************************/

#if TARGET_RT_MAC_MACHO
void *CXCmd::NewGlue
(
	void *pCFragPtr,
	UInt32 XCmdDescriptor,
	CFragSymbolClass symclass
)
{
	GlueCode *pGlueCode;

	if	(symclass ==  kDataCFragSymbol)
		return(pCFragPtr);
	else
	if	(symclass ==  kTVectorCFragSymbol /*kCodeCFragSymbol ???*/)
	{
		// neuen Eintrag für verkettete Liste erzeugen
		pGlueCode = (GlueCode *) NewPtr(sizeof(GlueCode));
		if	(!pGlueCode)
		{
			DebugError("CXCmd::NewGlue() -- zuwenig Speicher.");
			return(0);
		}
//		pGlueCode->id = id;
		pGlueCode->p = MachOFunctionPointerForCFMFunctionPointer(pCFragPtr);
		// einhängen
		pGlueCode->pNext = NULL;
		s_Plugins[XCmdDescriptor].pGlueList = pGlueCode;
		return(pGlueCode->p);
	}
	else
	{
		DebugWarning("CXCmd::NewGlue() -- unbekannte Symbolklasse %u.", symclass);
		return(pCFragPtr);
	}
}
#endif

/**********************************************************************
*
* Debug-Funktion: Gib alle Symbole einer PEF/CFM-Bibliothek aus
*
**********************************************************************/

#ifdef _DEBUG
static void DebugPrintSymbolsInPefLib(CFragConnectionID id, long nSymbols)
{
	CFragSymbolClass symClass;
	OSErr err;
	Str255 str;
	Ptr ptr;
	register long i;


	DebugInfo("CXCmd::Command() -- Symbole:");
	for	(i = 0; i < nSymbols; i++)
	{
		err = GetIndSymbol
				(
				id,					// CFragConnectionID connID
				i,					// long symIndex,
				str,					// Str255 symName
				&ptr,					// Ptr *symAddr
				&symClass				// CFragSymbolClass *symClass
				);
		if	(!err)
			DebugInfo("CXCmd::Command() --    Symbol \"%#s\", Klasse %d",
							str, (int) symClass);
	}
}
static void DebugPrintSymbolsInCFPlugIn(MagicMacXPluginInterfaceStruct *pInterface, UInt32 nSymbols)
{
	tfXCMDFunction *pFn;
	register UInt32 i;
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
#define DebugPrintSymbols(id, n)
#define DebugPrintSymbolsInCFPlugIn(p, n)
#endif


/**********************************************************************
*
* Lade eine Bibliothek mit einem gegebenen Pfad oder Namen
*
**********************************************************************/

OSErr CXCmd::OnCommandLoadLibrary
(
	const char *szLibName,		// name oder Pfad
	bool bIsPath,			// true: Pfad / false: Name
	UINT32 *pXCmdDescriptor,
	INT32 *pNumOfSymbols
)
{
	Str255 str;
	OSErr err;
	FSSpec Spec;
	register UINT32 i;
	enRunTimeFormat type;
	long NumOfSymbols;
	MagicMacXPluginInterfaceStruct *pInterface;
	const char *pPluginName;
	const char *pPluginCreator;
	UInt32 ulPluginVersionMajor;
	UInt32 ulPluginVersionMinor;
	char szSearchPath[256];
	UInt32 ulNumOfSym;


	DebugInfo("CXCmd::OnCommandLoadLibrary(\"%s\") by %s", szLibName, (bIsPath) ? "path" : "name");

	// suche freien Platz in Tabelle
	for	(i = 0; i < MAX_PLUGINS; i++)
	{
		if	(s_Plugins[i].RunTimeFormat == eUnused)
			goto found;
	}
	return(mFulErr);

	found:

	if	(bIsPath)
	{
		//
		// Wird der Pfad angegeben, unterstützen wir nur CFM/PEF
		//

		c2pstrcpy(str, szLibName);
		err = FSMakeFSSpec(
				CGlobals::s_ProcDir.vRefNum,	// wird ignoriert, wenn Pfad vollständig
				CGlobals::s_ProcDirID,		// wird ignoriert, wenn Pfad vollständig
				str,
				&Spec);
		if	(!err)
			err = Load(&Spec, &s_Plugins[i].ConnID);
		type = ePEF;
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
				&s_Plugins[i].PluginRef,
				&pInterface);

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
					&s_Plugins[i].PluginRef,
					&pInterface);
		}

		//
		// Jetzt suchen wir ein CFPlugIn (MachO-Bundle) in "/SystemLibrary/MagicMacX/PlugIns"
		//

		if	(err)
			err = LoadPlugin(
					szLibName,
					"/System/Library/MagicMacX/PlugIns/",
					&s_Plugins[i].PluginRef,
					&pInterface);

		if	(!err)
		{
			type = eMachO;
		}
		else
		{
			c2pstrcpy(str, szLibName);
			err = Load(str, &s_Plugins[i].ConnID);
			// typical error: cfragNoLibraryErr (-2804)
			type = ePEF;
		}
	}

	if	(err)
	{
		*pXCmdDescriptor = 0xffffffff;
		*pNumOfSymbols = 0;
	}
	else
	{
		s_Plugins[i].RunTimeFormat = type;
		s_Plugins[i].pInterface = pInterface,
		s_Plugins[i].pGlueList = NULL;

		*pXCmdDescriptor = i;

		if	(type == ePEF)
		{
			CountSymbols(
				s_Plugins[i].ConnID,
				&NumOfSymbols);

			// gib alle Symbole aus
			#ifdef _DEBUG
			DebugPrintSymbolsInPefLib(s_Plugins[i].ConnID, NumOfSymbols);
			#endif
		}
		else
		{
			pInterface->GetPluginInfo(
						pInterface,
						&pPluginName,
						&pPluginCreator,
						&ulPluginVersionMajor,
						&ulPluginVersionMinor,
						&ulNumOfSym);

			NumOfSymbols = (long) ulNumOfSym;
			DebugInfo("CXCmd::OnCommandLoadLibrary() -- CFPlugIn geladen");
			DebugInfo("CXCmd::OnCommandLoadLibrary() --  Name = %s", pPluginName);
			DebugInfo("CXCmd::OnCommandLoadLibrary() --  Autor = %s", pPluginCreator);
			DebugInfo("CXCmd::OnCommandLoadLibrary() --  Version %u.%u", ulPluginVersionMajor, ulPluginVersionMinor);
			DebugPrintSymbolsInCFPlugIn(pInterface, ulNumOfSym);
		}

		*pNumOfSymbols = (INT32) NumOfSymbols;
	}

	return(err);
}


/**********************************************************************
*
* Entlade eine geladene Bibliothek
*
**********************************************************************/

OSErr CXCmd::OnCommandUnloadLibrary
(
	UInt32 XCmdDescriptor
)
{
	enRunTimeFormat type;
	MagicMacXPluginInterfaceStruct *pInterface;


	DebugInfo("CXCmd::OnCommandUnloadLibrary(%u)", XCmdDescriptor);

	if	(XCmdDescriptor > MAX_PLUGINS)
		return(fnfErr);
	type = s_Plugins[XCmdDescriptor].RunTimeFormat;
	if	(type == ePEF)
	{
#if TARGET_RT_MAC_MACHO
		// gib allen Glue-Code frei
		GlueCode *pG,**pGprev;
		pG = s_Plugins[XCmdDescriptor].pGlueList;
		pGprev = &s_Plugins[XCmdDescriptor].pGlueList;
		while(pG)
		{
			DisposeMachOFunctionPointer(pG->p);
			*pGprev = pG->pNext;
			::DisposePtr((char *) pG);
			pG = *pGprev;
		}
#endif
		s_Plugins[XCmdDescriptor].RunTimeFormat = eUnused;
		return(CloseConnection(&s_Plugins[XCmdDescriptor].ConnID));
	}
	else
	if	(type == eMachO)
	{
		pInterface = s_Plugins[XCmdDescriptor].pInterface;
		// Plugin freigeben
		pInterface->Release(pInterface);
		// nochmal (?) freigeben
		CFRelease(s_Plugins[XCmdDescriptor].PluginRef);
		// auch hier abmelden
		s_Plugins[XCmdDescriptor].RunTimeFormat = eUnused;
		return(noErr);
	}
	else
		return(fnfErr);
}


/**********************************************************************
*
* Finde ein Symbol in einer geladenen Bibliothek
*
**********************************************************************/

OSErr CXCmd::OnCommandFindSymbol
(
	UInt32 XCmdDescriptor,
	char *pSymName,
	UInt32 SymIndex,
	unsigned char *pSymClass,
	UInt32 *pSymbolAddress
)
{
	enRunTimeFormat type;
	MagicMacXPluginInterfaceStruct *pInterface;
	Str255 str;
	Ptr ptr;
	OSErr err;
	CFragSymbolClass symClass;
	const char *s;
	tfXCMDFunction *pFn;


#ifdef _DEBUG
	if	(SymIndex == 0xffffffff)
		DebugInfo("CXCmd::OnCommandFindSymbol(\"%s\")", pSymName);
	else
		DebugInfo("CXCmd::OnCommandFindSymbol(%u)", SymIndex);
#endif
	if	(XCmdDescriptor > MAX_PLUGINS)
	{
		DebugError("CXCmd::OnCommandFindSymbol()-- ungültiger Deskriptor", SymIndex);
		return(fnfErr);
	}
	type = s_Plugins[XCmdDescriptor].RunTimeFormat;
	if	(type == ePEF)
	{
		if	(SymIndex == 0xffffffff)
		{
			c2pstrcpy(str, pSymName);
			err = FindSymbol
					(
					s_Plugins[XCmdDescriptor].ConnID,		// CFragConnectionID connID
					str,							// ConstStr255Param symName
					&ptr,							// Ptr *symAddr
					&symClass						// CFragSymbolClass *symClass
					);
		}
		else
		{
			err = GetIndSymbol
					(
					s_Plugins[XCmdDescriptor].ConnID,		// CFragConnectionID connID
					(long) SymIndex,					// long symIndex,
					str,							// Str255 symName
					&ptr,							// Ptr *symAddr
					&symClass						// CFragSymbolClass *symClass
					);
			p2cstrcpy(pSymName, str);
		}

		if	(!err)
		{
			*pSymClass = (unsigned char) symClass;
#if TARGET_RT_MAC_MACHO
			*pSymbolAddress = (UINT32) NewGlue(ptr, XCmdDescriptor, symClass);
#else
			*pSymbolAddress = (UInt32) ptr;
#endif
		}
		else
		{
			*pSymbolAddress = (UInt32) 0;
		}
	}
	else
	if	(type == eMachO)
	{
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
			*pSymbolAddress = (UInt32) pFn;
			*pSymClass =  kCodeCFragSymbol;
			err = noErr;
		}
		else
		{
			*pSymbolAddress = (UInt32) 0;
			err = fnfErr;
		}
	}
	else
		err = fnfErr;

#ifdef _DEBUG
	if	(err)
		DebugError("CXCmd::OnCommandFindSymbol() -- failure");
	else
		DebugInfo("CXCmd::OnCommandFindSymbol() -- success");
#endif

	return(err);
}


/**********************************************************************
*
* Callback des Emulators: XCmd-Kommando
*
**********************************************************************/

INT32 CXCmd::Command(UINT32 params, unsigned char *AdrOffset68k)
{
	strXCMD *pCmd = (strXCMD *) (params + AdrOffset68k);
	INT32 ret;
	OSErr err;


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
					(UInt32 *) &pCmd->m_12_13.m_SymPtr);
			ret = 0;
			break;

		case eXCMDGetSymbolByIndex:
			err = OnCommandFindSymbol(
					pCmd->m_LibHandle,
					pCmd->m_12_13.m_Name,
					(UInt32) pCmd->m_12_13.m_Index,
					&pCmd->m_12_13.m_SymClass,
					(UInt32 *) &pCmd->m_12_13.m_SymPtr);
			ret = 0;
			break;

		case eUnLoad:
			err = OnCommandUnloadLibrary((UInt32) pCmd->m_LibHandle);
			ret = 0;
			break;

		default:
			err = 0;
			ret = EUNCMD;
			break;
	}; 

	pCmd->m_MacError = err;
	if	(err)
		ret = ERROR;
	return(ret);
}
