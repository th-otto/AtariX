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
* "Preferences" für MagicMacX
*
*/

// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "MyPreferences.h"
#include "PascalStrings.h"
#include "globals.h"

// Schalter

#define MONITOR				"AtariMonitor"
#define ATARIMEM			"AtariMemSizeInKB"
#define SHOWMACMOUSE		"ShowMacMouse"
#define AUTOSTARTMAGIC		"AutoStartMagiC"
#define KEYCODEFORRIGHTMOUSEBUTTON	"KeyCodeForRightMouseButton"
#define PRINTINGCOMMAND		"PrintingCommand"
#define AUXPATH				"AuxPath"
#define SHOWMACMENU			"ShowMacMenu"
#define SETSCREENSIZEMANUALLY	"SetAtariScreenSizeManually"
#define SETSCREENSIZE_X		"SetAtariScreenSizeX"
#define SETSCREENSIZE_Y		"SetAtariScreenSizeY"
#define SETSCREENSIZE_WIDTH		"SetAtariScreenSizeWidth"
#define SETSCREENSIZE_HEIGHT	"SetAtariScreenSizeHeight"
#define SCREEN_FREQ			"ScreenRefreshFreq"
#define PVDI				"PVDI"

#if defined(__GNUC__)
#define DEF_DOCTYPES			"\0\1" "\x19" "APP,PRG,TTP,TOS=MgMx/Gem1"
#else
#define DEF_DOCTYPES			"\0\1" "\pAPP,PRG,TTP,TOS=MgMx/Gem1"
#endif

#ifdef DEMO
#define PREFS_FILE		"MagiCMacX2 Demo Prefs"
#else
#define PREFS_FILE		"MagiCMacX2 Prefs"
#endif

// statische Attribute:

/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMyPreferences::CMyPreferences()
{
	int i;

	m_AtariMemSize = ATARIMEMSIZEDEFAULT;
	for	(i = 0; i < NDRIVES; i++)
	{
		m_drvPath[i] = NULL;
		m_drvFlags[i] = 0;		// Lange Namen, vorwärts sortiert
	}
	m_drvFlags['C'-'A'] |= 2;		// C: hat 8+3

	/* no longer used
	m_DocTypes = NULL;
	m_DocTypesNum = 0;
	*/
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMyPreferences::~CMyPreferences()
{
}


/**********************************************************************
*
* Initialisierung: Bestimmt den "Preferences"-Ordner und legt die Datei
* fest.
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

int CMyPreferences::Init()
{
	return(CPreferences::Init(PREFS_FILE, MyCreator, MyPrefsType));
}


/**********************************************************************
*
* Alle Einstellungen holen
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

int CMyPreferences::GetPreferences()
{
	int ret;


	ret = Open();
	if	(ret)
		return(ret);

	m_AtariMemSize = (unsigned long) GetRsrcNum(CFSTR(ATARIMEM),
									ATARIMEMSIZEDEFAULT >> 10) << 10;
	if	(m_AtariMemSize > MAX_ATARIMEMSIZE)
		m_AtariMemSize = MAX_ATARIMEMSIZE;

	m_Monitor = (short) GetRsrcNum(CFSTR(MONITOR), 0);

	m_bShowMacMenu = (bool) GetRsrcNum(CFSTR(SHOWMACMENU), 1);

	m_bShowMacMouse = (bool) GetRsrcNum(CFSTR(SHOWMACMOUSE), 1);

	m_bAutoStartMagiC = (bool) GetRsrcNum(CFSTR(AUTOSTARTMAGIC), 0);

	// default: F15 (Pause)
	m_KeyCodeForRightMouseButton = (unsigned short) GetRsrcNum(CFSTR(KEYCODEFORRIGHTMOUSEBUTTON), 113);

	// Laufwerke (außer C: und M: und U:)

#if 0
	register int i;
	char szData[256];
	CFStringRef cfKey;

	for	(i = 0; i < NDRIVES; i++)
	{
		// Pfad

		if	(ATARIDRIVEISMAPPABLE(i))
		{
			sprintf(szData, "Drive_%c", i + 'A');
			cfKey = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, szData, kCFStringEncodingISOLatin1, kCFAllocatorNull);
			m_drvAlias[i] = GetRsrcAlias(cfKey);
			CFRelease(cfKey);
		}
		else
			m_drvAlias[i] = nil;

		// Flags

		sprintf(szData, "Drive_%c_Flags", i + 'A');
		cfKey = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, szData, kCFStringEncodingISOLatin1, kCFAllocatorNull);
		m_drvFlags[i] = GetRsrcNum(cfKey, m_drvFlags[i], false);
		CFRelease(cfKey);
		if	(i == 'M' - 'A')
			m_drvFlags[i] &= ~2;	// immer lange Namen, nicht 8+3
	}
#endif

	// Druck-Kommando

	GetRsrcStr(CFSTR(PRINTINGCOMMAND), m_szPrintingCommand, "lpr -P HP870cxi -o raw %s");

	// BSD-Pfad für Modem

	GetRsrcStr(CFSTR(AUXPATH), m_szAuxPath, "/dev/cu.modem");

	// Atari-Bildschirm

	m_bAtariScreenManualSize = (bool) GetRsrcNum(CFSTR(SETSCREENSIZEMANUALLY), 0);
	// wichtig: Wenn im Fenster, dann Mac-Menüleiste immer anzeigen
	if	(m_bAtariScreenManualSize)
		m_bShowMacMenu = true;
	m_AtariScreenX = (unsigned short) GetRsrcNum(CFSTR(SETSCREENSIZE_X), 5);
	m_AtariScreenY= (unsigned short) GetRsrcNum(CFSTR(SETSCREENSIZE_Y), 25);
	m_AtariScreenWidth = (unsigned short) GetRsrcNum(CFSTR(SETSCREENSIZE_WIDTH), 640);
	m_AtariScreenHeight= (unsigned short) GetRsrcNum(CFSTR(SETSCREENSIZE_HEIGHT), 480);

	m_ScreenRefreshFrequency = (unsigned short) GetRsrcNum(CFSTR(SCREEN_FREQ), 1);

	m_bPPC_VDI_Patch = (bool) GetRsrcNum(CFSTR(PVDI), 1);

	/* no longer used
	// Dateitypen

	GetRsrcStringArray("\pDOCTYPES", &m_DocTypes, &m_DocTypesNum, (void *) DEF_DOCTYPES, (short) sizeof(DEF_DOCTYPES));
	*/

	return(0);
}


/**********************************************************************
*
* Aktualisieren
*
**********************************************************************/

void CMyPreferences::Update_Monitor(void)
{
	SetRsrcNum(CFSTR(MONITOR), (long) (m_Monitor));

	Update();	// write to disk
}

void CMyPreferences::Update_AtariMem(void)
{
	SetRsrcNum(CFSTR(ATARIMEM), (long) (m_AtariMemSize >> 10));

	Update();	// write to disk
}

void CMyPreferences::Update_GeneralSettings(void)
{
	SetRsrcNum(CFSTR(SHOWMACMOUSE), (long) m_bShowMacMouse);
	SetRsrcNum(CFSTR(AUTOSTARTMAGIC), (long) m_bAutoStartMagiC);
	SetRsrcNum(CFSTR(KEYCODEFORRIGHTMOUSEBUTTON), (long) m_KeyCodeForRightMouseButton);

	SetRsrcNum(CFSTR(PVDI), (long) m_bPPC_VDI_Patch);

	Update();	// write to disk
}

void CMyPreferences::Update_AtariScreen(void)
{
	SetRsrcNum(CFSTR(SHOWMACMENU), (long) m_bShowMacMenu);
	SetRsrcNum(CFSTR(SETSCREENSIZEMANUALLY), (long) m_bAtariScreenManualSize);
	SetRsrcNum(CFSTR(SETSCREENSIZE_X), (long) m_AtariScreenX);
	SetRsrcNum(CFSTR(SETSCREENSIZE_Y), (long) m_AtariScreenY);
	SetRsrcNum(CFSTR(SETSCREENSIZE_WIDTH), (long) m_AtariScreenWidth);
	SetRsrcNum(CFSTR(SETSCREENSIZE_HEIGHT), (long) m_AtariScreenHeight);
	SetRsrcNum(CFSTR(SCREEN_FREQ), (long) m_ScreenRefreshFrequency);

	Update();	// write to disk
}

void CMyPreferences::Update_Drives(void)
{
#if 0
	register int i;
	char szData[256];
	CFStringRef cfKey;

	for	(i = 0; i < NDRIVES; i++)
	{
		// Pfad

		if	(ATARIDRIVEISMAPPABLE(i))
		{
			// Alle Laufwerke außer C:, M: und U: können einen änderbaren Mac-Pfad haben.

			sprintf(szData, "Drive_%c", i + 'A');
			cfKey = CFStringCreateWithCString/*NoCopy*/(kCFAllocatorDefault, szData, kCFStringEncodingISOLatin1/*, kCFAllocatorNull*/);
			SetRsrcAlias(cfKey, m_drvAlias[i]);
			Update();	// write to disk
			CFRelease(cfKey);
		}

		// Flags

		if	(ATARIDRIVEISMAPPABLE(i) || (i == 'C' - 'A'))
		{
			// zusätzlich kann C: geänderte Flags haben, M: und U: jedoch nicht.

			sprintf(szData, "Drive_%c_Flags", i + 'A');
			cfKey = CFStringCreateWithCString/*NoCopy*/(kCFAllocatorDefault, szData, kCFStringEncodingISOLatin1/*, kCFAllocatorNull*/);
			if	((m_drvAlias[i]) || (i == 'M'-'A'))
			{
				//(void) GetRsrcNum(cfKey, m_drvFlags[i], true);	// ggf. hinzufügen
				SetRsrcNum(cfKey, m_drvFlags[i]);
				Update();	// save to disk (necessary!!!)
			}
			else
			{
				// Flags löschen, wenn existieren
				SetRsrcStr(cfKey, NULL);
				Update();
			}
			CFRelease(cfKey);
		}
	}
#endif

//	Update();	// too late? Flags will be ignored?!?
}

void CMyPreferences::Update_PrintingCommand(void)
{
	SetRsrcStr(CFSTR(PRINTINGCOMMAND), m_szPrintingCommand);

	Update();	// write to disk
}


void CMyPreferences::Update_AuxPath(void)
{
	SetRsrcStr(CFSTR(AUXPATH), m_szAuxPath);

	Update();	// write to disk
}
