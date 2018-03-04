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
* Zeichensatz-Umsetzung für MagicMacX
*
*/

class CTextConversion
{
   public:
	// Initialisierung
	static int Init( void );
	static void Atari2MacFilename(unsigned char *s);
	static unsigned char Atari2MacFilename(unsigned char c);
	static unsigned char Atari2MacText(unsigned char c);
	static void Mac2AtariFilename(unsigned char *s);
	static unsigned char Mac2AtariFilename(unsigned char c);
	static unsigned char Mac2AtariText(unsigned char c);

   private:
	// Funktionen
	// Attribute
	static unsigned const char s_tabAtari2MacText[256];
	static unsigned const char s_tabMac2AtariText[256];
	static unsigned const char s_tabAtari2MacFilename[256];
	static unsigned const char s_tabMac2AtariFilename[256];
};