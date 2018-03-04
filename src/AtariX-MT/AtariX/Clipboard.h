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
* Clipboard-Umsetzung für MagicMacX
*
*/

class CClipboard
{
   public:
	static void Mac2Atari(const uint8_t *pData);	// put Clipboard data in UTF-8 format
	static void Atari2Mac(uint8_t **pBuffer);

   private:
	// Typen
	typedef struct
	{
		uint8_t atariChar;
		const char *utf8Char;
	} atariCharEntry;
	// Funktionen
	static int OpenAtariScrapFile(int unixPerm);
	static const atariCharEntry *FindUtf8(const uint8_t *utf8);
	static const atariCharEntry *FindAtari(uint8_t c);
//	static OSErr CheckAtariScrapFile( bool *bChanged );
//	static void DeleteAtariScrapFile( const char *fname );
	// Attribute
	static const CClipboard::atariCharEntry atariCharConvTable[];
//	static unsigned long s_lastCrDat;
//	static unsigned long s_lastMdDat;
//	static long s_lastFILgLen;
//	static char s_AtariScrapPath[256];
};