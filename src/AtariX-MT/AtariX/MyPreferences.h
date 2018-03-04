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

#ifndef _INCLUDED_MYPREFERENCES_H
#define _INCLUDED_MYPREFERENCES_H

#include "Preferences.h"

#define	MyCreator 'MgMc'
#define	MyPrefsType 'Pref'
#define	ATARIMEMSIZEDEFAULT	(8*1024*1024)
#define	NDRIVES ('Z'-'A' + 1)
#define	ATARIDRIVEISMAPPABLE(d)	((d != 'C'-'A') && (d != 'M'-'A') && (d != 'U'-'A'))

// Atari screen colour mode
typedef enum
{
	atariScreenMode16M = 0,		// true colour, more than 16 millions of colours
	atariScreenModeHC = 1,		// high colour, 16 bits per pixel
	atariScreenMode256 = 2,		// 256 colours with palette
	atariScreenMode16 = 3,		// 16 colours with palette, packed pixels
	atariScreenMode16ip = 4,	// 16 colours with palette, interleaved plane
	atariScreenMode4ip = 5,		// 4 colours with palette, interleaved plane
	atariScreenMode2 = 6		// monochrome
} enAtariScreenColourMode;


class CMyPreferences : public CPreferences
{
   public:
	// Konstruktor
	CMyPreferences();
	// Destruktor
	~CMyPreferences();
	// Initialisierung
	int Init( void );
	// Alle Einstellungen holen
	int GetPreferences( void );
	void Update_Monitor( void );
	void Update_AtariMem( void );
	void Update_GeneralSettings( void );
	void Update_Drives( void );
	void Update_PrintingCommand( void );
	void Update_AuxPath( void );
	void Update_AtariScreen( void );

	// Variablen
	unsigned long m_AtariMemSize;
	bool m_bShowMacMenu;
	enAtariScreenColourMode m_atariScreenColourMode;
	bool m_bShowMacMouse;
	bool m_bAutoStartMagiC;
	CFURLRef m_drvPath[NDRIVES];
	long m_drvFlags[NDRIVES];	// 1 == RevDir / 2 == 8+3
	unsigned short m_KeyCodeForRightMouseButton;
	char m_szPrintingCommand[256];
	char m_szAuxPath[256];
	short m_Monitor;		// 0 == Hauptbildschirm
	bool m_bAtariScreenManualSize;
	unsigned short m_AtariScreenX;
	unsigned short m_AtariScreenY;
	unsigned short m_AtariScreenWidth;
	unsigned short m_AtariScreenHeight;
	unsigned short m_ScreenRefreshFrequency;
	bool m_bPPC_VDI_Patch;

	/*
	char **m_DocTypes;			// no longer used
	UInt16 m_DocTypesNum;
	*/
   private:
	// Funktionen
	// Attribute
};
#endif