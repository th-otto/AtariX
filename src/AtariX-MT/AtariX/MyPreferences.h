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

#define	MyCreator 'MgMc'
#define	MyPrefsType 'Pref'

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

#define	NDRIVES ('Z' - 'A' + 1)

#define M_DRV_REVERSE_DIR_ORDER  0x01
#define M_DRV_DOSNAMES           0x02
#define M_DRV_READONLY           0x04

#ifdef __cplusplus
class CMyPreferences
{
public:
	// Variablen
	unsigned long m_AtariMemSize;
	enAtariScreenColourMode m_atariScreenColourMode;
	CFURLRef m_drvPath[NDRIVES];
	unsigned long m_drvFlags[NDRIVES];	// 1 == RevDir / 2 == 8+3
	char m_szPrintingCommand[256];
	char m_szAuxPath[256];
	bool m_bPPC_VDI_Patch;
};
#endif

#endif /* _INCLUDED_MYPREFERENCES_H */
