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
* EnthŠlt alles, was mit der Atari-Tastatur zu tun hat
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "Resource.h"
#include "MagiCKeyboard.h"
#include "Atari.h"
#include <SDL2/SDL_scancode.h>

// Schalter

// statische Attribute:

const unsigned char *CMagiCKeyboard::s_tabScancodeMac2Atari;

const unsigned char CMagiCKeyboard::s_convtab[128] =
{
	30,		// 0: a
	31,		// 1: s
	32,		// 2: d
	33,		// 3: f
	35,		// 4: h
	34,		// 5: g
	44,		// 6: y
	45,		// 7: x
	46,		// 8: c
	47,		// 9: v
	43,		// 10: ^(deadkey!)
	48,		// 11: b
	16,		// 12: q
	17,		// 13: w
	18,		// 14: e
	19,		// 15: r
	21,		// 16: z
	20,		// 17: t
	2,		// 18: 1
	3,		// 19: 2
	4,		// 20: 3
	5,		// 21: 4
	7,		// 22: 6
	6,		// 23: 5
	13,		// 24: « (deadkey!)
	10,		// 25: 9
	8,		// 26: 7
	12,		// 27: §
	9,		// 28: 8
	11,		// 29: 0
	27,		// 30: +
	24,		// 31: o
	22,		// 32: u
	26,		// 33: Ÿ
	23,		// 34: i
	25,		// 35: p
	28,		// 36: Return
	38,		// 37: l
	36,		// 38: j
	40,		// 39: Š
	37,		// 40: k
	39,		// 41: š
	41,		// 42: #
	51,		// 43: ,
	53,		// 44: -
	49,		// 45: n
	50,		// 46: m
	52,		// 47: .
	15,		// 48: Tab
	57,		// 49: Leertaste
	96,		// 50: <
	14,		// 51: Backspace
	0,
	1,		// 53: Esc
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	113,		// 65: Num-,
	0,
	102,		// 67: Num-*
	0,
	78,		// 69: Num-+
	0,
	99,		// 71: Num-Lock => Num-(
	0,
	0,
	0,
	101,		// 75: Num-/
	114,		// 76: Num-Enter
	0,
	74,		// 78: Num-- ???
	0,
	0,
	100,		// 81: Num-= => Num-)
	112,		// 82: Num-0
	109,		// 83: Num-1
	110,		// 84: Num-2
	111,		// 85: Num-3
	106,		// 86: Num-4
	107,		// 87: Num-5
	108,		// 88: Num-6
	103,		// 89: Num-7
	0,
	104,		// 91: Num-8
	105,		// 92: Num-9
	0,
	0,
	0,
	63,		// 96: F5
	64,		// 97: F6
	65,		// 98: F7
	61,		// 99: F3
	66,		// 100: F8
	67,		// 101: F9
	0,
	69,		// 103: F11
	0,
	98,		// 105: Druck (F13) => Help
	0,
	97,		// 107: SystAnfr (F14) => Undo
	0,
	68,		// 109: F10
	0,
	70,		// 111: F12
	0,		//
	0,		// 113: Pause (F15)
	82,		// 114: Einfg.
	71,		// 115: Home
	73,		// 116: Bild hoch
	83,		// 117: Entf.
	62,		// 118: F4
	0x4f,		// 119: Ende
	60,		// 120: F2
	81,		// 121: Bild runter
	59,		// 122: F1
	75,		// 123: Cursor links
	77,		// 124: Cursor rechts
	80,		// 125: Cursor runter
	72,		// 126: Cursor hoch
	0
	};


const UInt32 CMagiCKeyboard::s_modconvtab[16] =
{
	cmdKey,0x37,
	shiftKey,42,
	rightShiftKey,54,
	alphaLock,58,
	optionKey,56,
	rightOptionKey,0x4c,		// AltGr
	controlKey,29,
	rightControlKey,29
};


/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMagiCKeyboard::CMagiCKeyboard()
{
	m_modifiers = 0;

#ifdef PATCH_DEAD_KEYS

	// wir patchen hier brutal die Zuordungstabelle 'KCHR' von
	// virtual key code zu ASCII-Code. Die Taste ' ist dann kein
	// dead key mehr

	char *pKCHR = (char *) GetScriptManagerVariable (smKCHRCache);
	pKCHR += 2+256;
	UInt16 nKCHRtables = *((UInt16 *) pKCHR);
	pKCHR += 2;

	// alte Tastaturtabellen retten
	m_OldKbTableLen = nKCHRtables * (unsigned long) 128;
	m_OldKbTable = new char [m_OldKbTableLen];
	if	(!m_OldKbTable)
	{
		DebugInfo("Zuwenig Speicher!");
		ExitToShell();
	}
	memcpy(m_OldKbTable, pKCHR, m_OldKbTableLen);

	// und jetzt "dead keys" entfernen

	while(nKCHRtables)
	{
		if	(pKCHR[24] == '\0')	// wenn "dead key"
		{
			pKCHR[24] = '«';
			DebugInfo("Tastaturtabelle gepatcht");
		}
		if	(pKCHR[10] == '\0')	// wenn "dead key"
		{
			pKCHR[10] = '^';
			DebugInfo("Tastaturtabelle gepatcht");
		}
		nKCHRtables--;
		pKCHR += 128;		// nŠchste Tabelle
	}
#endif
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMagiCKeyboard::~CMagiCKeyboard()
{
#ifdef PATCH_DEAD_KEYS
	// Tastaturtabelle rekonstruieren
	char *pKCHR = (char *) GetScriptManagerVariable (smKCHRCache);
	memcpy(pKCHR+2+256+2, m_OldKbTable, m_OldKbTableLen);
	delete[] m_OldKbTable;
#endif
}


/**********************************************************************
*
* statisch: Initialisieren
*
**********************************************************************/

int CMagiCKeyboard::Init(void)
{
//	Handle h;

//	UseResFile(CGlobals::s_ThisResFile);
//	h = Get1Resource('HEXA', HEXA_SCANCODE_MAC_ATARI);
//	if	(!h)
	{
		s_tabScancodeMac2Atari = s_convtab;
		return(1);
	}
//	else
//	{
//		HLockHi (h);
//		s_tabScancodeMac2Atari = (unsigned char *) (*h);
//		return(0);
//	}
}


/**********************************************************************
 *
 * Scancode umrechnen (modern)
 *
 **********************************************************************/

unsigned char CMagiCKeyboard::SdlScanCode2AtariScanCode(int s)
{
	switch(s)
	{
		case SDL_SCANCODE_A:		return ATARI_KBD_SCANCODE_A;
		case SDL_SCANCODE_B:		return ATARI_KBD_SCANCODE_B;
		case SDL_SCANCODE_C:		return ATARI_KBD_SCANCODE_C;
		case SDL_SCANCODE_D:		return ATARI_KBD_SCANCODE_D;
		case SDL_SCANCODE_E:		return ATARI_KBD_SCANCODE_E;
		case SDL_SCANCODE_F:		return ATARI_KBD_SCANCODE_F;
		case SDL_SCANCODE_G:		return ATARI_KBD_SCANCODE_G;
		case SDL_SCANCODE_H:		return ATARI_KBD_SCANCODE_H;
		case SDL_SCANCODE_I:		return ATARI_KBD_SCANCODE_I;
		case SDL_SCANCODE_J:		return ATARI_KBD_SCANCODE_J;
		case SDL_SCANCODE_K:		return ATARI_KBD_SCANCODE_K;
		case SDL_SCANCODE_L:		return ATARI_KBD_SCANCODE_L;
		case SDL_SCANCODE_M:		return ATARI_KBD_SCANCODE_M;
		case SDL_SCANCODE_N:		return ATARI_KBD_SCANCODE_N;
		case SDL_SCANCODE_O:		return ATARI_KBD_SCANCODE_O;
		case SDL_SCANCODE_P:		return ATARI_KBD_SCANCODE_P;
		case SDL_SCANCODE_Q:		return ATARI_KBD_SCANCODE_Q;
		case SDL_SCANCODE_R:		return ATARI_KBD_SCANCODE_R;
		case SDL_SCANCODE_S:		return ATARI_KBD_SCANCODE_S;
		case SDL_SCANCODE_T:		return ATARI_KBD_SCANCODE_T;
		case SDL_SCANCODE_U:		return ATARI_KBD_SCANCODE_U;
		case SDL_SCANCODE_V:		return ATARI_KBD_SCANCODE_V;
		case SDL_SCANCODE_W:		return ATARI_KBD_SCANCODE_W;
		case SDL_SCANCODE_X:		return ATARI_KBD_SCANCODE_X;
		case SDL_SCANCODE_Y:		return ATARI_KBD_SCANCODE_Y;
		case SDL_SCANCODE_Z:		return ATARI_KBD_SCANCODE_Z;

		case SDL_SCANCODE_1:		return ATARI_KBD_SCANCODE_1;
		case SDL_SCANCODE_2:		return ATARI_KBD_SCANCODE_2;
		case SDL_SCANCODE_3:		return ATARI_KBD_SCANCODE_3;
		case SDL_SCANCODE_4:		return ATARI_KBD_SCANCODE_4;
		case SDL_SCANCODE_5:		return ATARI_KBD_SCANCODE_5;
		case SDL_SCANCODE_6:		return ATARI_KBD_SCANCODE_6;
		case SDL_SCANCODE_7:		return ATARI_KBD_SCANCODE_7;
		case SDL_SCANCODE_8:		return ATARI_KBD_SCANCODE_8;
		case SDL_SCANCODE_9:		return ATARI_KBD_SCANCODE_9;
		case SDL_SCANCODE_0:		return ATARI_KBD_SCANCODE_0;

		case SDL_SCANCODE_RETURN:		return ATARI_KBD_SCANCODE_RETURN;
		case SDL_SCANCODE_ESCAPE:		return ATARI_KBD_SCANCODE_ESCAPE;
		case SDL_SCANCODE_BACKSPACE:	return ATARI_KBD_SCANCODE_BACKSPACE;
		case SDL_SCANCODE_TAB:			return ATARI_KBD_SCANCODE_TAB;
		case SDL_SCANCODE_SPACE:		return ATARI_KBD_SCANCODE_SPACE;

		case SDL_SCANCODE_MINUS:		return ATARI_KBD_SCANCODE_MINUS;
		case SDL_SCANCODE_EQUALS:		return ATARI_KBD_SCANCODE_EQUALS;
		case SDL_SCANCODE_LEFTBRACKET:	return ATARI_KBD_SCANCODE_LEFTBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET:	return ATARI_KBD_SCANCODE_RIGHTBRACKET;
		case SDL_SCANCODE_NONUSHASH:
		case SDL_SCANCODE_BACKSLASH:	return ATARI_KBD_SCANCODE_BACKSLASH;

		case SDL_SCANCODE_SEMICOLON:	return ATARI_KBD_SCANCODE_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE:	return ATARI_KBD_SCANCODE_APOSTROPHE;
		case SDL_SCANCODE_GRAVE:		return ATARI_KBD_SCANCODE_NUMBER;	// holds '^' and '¡' on a German keyboard

		case SDL_SCANCODE_COMMA:		return ATARI_KBD_SCANCODE_COMMA;
		case SDL_SCANCODE_PERIOD:		return ATARI_KBD_SCANCODE_PERIOD;
		case SDL_SCANCODE_SLASH:		return ATARI_KBD_SCANCODE_SLASH;

		case SDL_SCANCODE_CAPSLOCK:		return ATARI_KBD_SCANCODE_CAPSLOCK;

		case SDL_SCANCODE_F1:			return ATARI_KBD_SCANCODE_F1;
		case SDL_SCANCODE_F2:			return ATARI_KBD_SCANCODE_F2;
		case SDL_SCANCODE_F3:			return ATARI_KBD_SCANCODE_F3;
		case SDL_SCANCODE_F4:			return ATARI_KBD_SCANCODE_F4;
		case SDL_SCANCODE_F5:			return ATARI_KBD_SCANCODE_F5;
		case SDL_SCANCODE_F6:			return ATARI_KBD_SCANCODE_F6;
		case SDL_SCANCODE_F7:			return ATARI_KBD_SCANCODE_F7;
		case SDL_SCANCODE_F8:			return ATARI_KBD_SCANCODE_F8;
		case SDL_SCANCODE_F9:			return ATARI_KBD_SCANCODE_F9;
		case SDL_SCANCODE_F10:			return ATARI_KBD_SCANCODE_F10;
		case SDL_SCANCODE_F11:			return 0;
		case SDL_SCANCODE_F12:			return 0;

		case SDL_SCANCODE_PRINTSCREEN:		return 0;
		case SDL_SCANCODE_SCROLLLOCK:		return 0;
		case SDL_SCANCODE_PAUSE:			return 0;
		case SDL_SCANCODE_INSERT:			return ATARI_KBD_SCANCODE_INSERT;

		case SDL_SCANCODE_HOME:				return ATARI_KBD_SCANCODE_CLRHOME;
		case SDL_SCANCODE_PAGEUP:			return ATARI_KBD_SCANCODE_PAGEUP;
		case SDL_SCANCODE_DELETE:			return ATARI_KBD_SCANCODE_DELETE;
		case SDL_SCANCODE_END:				return ATARI_KBD_SCANCODE_END;
		case SDL_SCANCODE_PAGEDOWN:			return ATARI_KBD_SCANCODE_PAGEDOWN;
		case SDL_SCANCODE_RIGHT:			return ATARI_KBD_SCANCODE_RIGHT;
		case SDL_SCANCODE_LEFT:				return ATARI_KBD_SCANCODE_LEFT;
		case SDL_SCANCODE_DOWN:				return ATARI_KBD_SCANCODE_DOWN;
		case SDL_SCANCODE_UP:				return ATARI_KBD_SCANCODE_UP;

		case SDL_SCANCODE_NUMLOCKCLEAR:		return 0;

		case SDL_SCANCODE_KP_DIVIDE:		return ATARI_KBD_SCANCODE_KP_DIVIDE;
		case SDL_SCANCODE_KP_MULTIPLY:		return ATARI_KBD_SCANCODE_KP_MULTIPLY;
		case SDL_SCANCODE_KP_MINUS:			return ATARI_KBD_SCANCODE_KP_MINUS;
		case SDL_SCANCODE_KP_PLUS:			return ATARI_KBD_SCANCODE_KP_PLUS;
		case SDL_SCANCODE_KP_ENTER:			return ATARI_KBD_SCANCODE_KP_ENTER;
		case SDL_SCANCODE_KP_1:				return ATARI_KBD_SCANCODE_KP_1;
		case SDL_SCANCODE_KP_2:				return ATARI_KBD_SCANCODE_KP_2;
		case SDL_SCANCODE_KP_3:				return ATARI_KBD_SCANCODE_KP_3;
		case SDL_SCANCODE_KP_4:				return ATARI_KBD_SCANCODE_KP_4;
		case SDL_SCANCODE_KP_5:				return ATARI_KBD_SCANCODE_KP_5;
		case SDL_SCANCODE_KP_6:				return ATARI_KBD_SCANCODE_KP_6;
		case SDL_SCANCODE_KP_7:				return ATARI_KBD_SCANCODE_KP_7;
		case SDL_SCANCODE_KP_8:				return ATARI_KBD_SCANCODE_KP_8;
		case SDL_SCANCODE_KP_9:				return ATARI_KBD_SCANCODE_KP_9;
		case SDL_SCANCODE_KP_0:				return ATARI_KBD_SCANCODE_KP_0;
		case SDL_SCANCODE_KP_PERIOD:		return ATARI_KBD_SCANCODE_KP_PERIOD;

		case SDL_SCANCODE_NONUSBACKSLASH:	return ATARI_KBD_SCANCODE_LTGT;	// <> on German keyboard

		case SDL_SCANCODE_APPLICATION:		return 0;
		case SDL_SCANCODE_POWER:			return 0;

		case SDL_SCANCODE_KP_EQUALS:		return 0;

		case SDL_SCANCODE_LCTRL:			return ATARI_KBD_SCANCODE_CONTROL;
		case SDL_SCANCODE_LSHIFT:			return ATARI_KBD_SCANCODE_LSHIFT;
		case SDL_SCANCODE_LALT:				return ATARI_KBD_SCANCODE_ALT;
		case SDL_SCANCODE_LGUI:				return 0;
		case SDL_SCANCODE_RCTRL:			return ATARI_KBD_SCANCODE_CONTROL;
		case SDL_SCANCODE_RSHIFT:			return ATARI_KBD_SCANCODE_RSHIFT;
		case SDL_SCANCODE_RALT:				return ATARI_KBD_SCANCODE_ALTGR;
		case SDL_SCANCODE_RGUI:				return 0;

		default: return 0;
	}
}


/**********************************************************************
*
* Scancode umrechnen
*
**********************************************************************/

unsigned char CMagiCKeyboard::GetScanCode(UInt32 message)
{
	unsigned char scan;

	scan = (unsigned char) (message >> 8);		// Mac-Scancode
	if	(scan > 127)
		scan = 0;
	scan = s_tabScancodeMac2Atari[scan];
	return(scan);
}


/**********************************************************************
*
* "modifier" umrechnen
*
**********************************************************************/

unsigned char CMagiCKeyboard::GetModifierScanCode(UInt32 modifiers, bool *bAutoBreak)
{
	UInt32 diffmod;
	unsigned char ret;
	const UInt32 *pmask;
	UInt32 mask = 0;


	// €nderungen zu bisher gesendetem Status durch XOR berechnen

	diffmod = modifiers ^ m_modifiers;
	if	(!diffmod)
		return(0);

	// Alle Bits auf Unterschiede prŸfen

	for	(pmask = s_modconvtab; pmask < s_modconvtab+16; pmask += 2)
	{
		if	(diffmod & pmask[0])
		{
			mask = pmask[0];
			ret = (unsigned char) pmask[1];
			break;
		}
	}

	if	(!mask)
	{
		// nichts relevantes gefunden
		m_modifiers = modifiers;
		return(0);
	}

	*bAutoBreak = (ret == 58);		// Atari-CapsLock

	if	(modifiers & mask)
		{
		m_modifiers |= mask;
		}
	else	{
		if	(!(*bAutoBreak))	// FŸr CapsLock wird der Break-Code automatisch generiert
			ret |= 0x80;
		m_modifiers &= ~mask;
		}

	return(ret);
}