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
	static void Atari2HostUtf8Copy(char *dst, const char *src, size_t count);
	static void Host2AtariUtf8Copy(char *dst, const char *src, size_t count);

   private:
	// Funktionen
	// Attribute
	static void charset_conv_error(unsigned short ch);
};
