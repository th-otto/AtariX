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
* Bearbeitung von Pascal-Zeichenketten
*
*/

// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
#include <stdlib.h>
// Programm-Header
#include "PascalStrings.h"

// C-Strings (hat MW in stdlib vergessen!)

void ultoa10(unsigned long val, char *str)
{
	sprintf(str, "%lu", val);
}

// Pascal-Strings

void pstrcpy(Str255 dst, ConstStr255Param src)
{
	memcpy(dst, src, (size_t) (src[0]+1));
}

// Konvertierungen

#if !TARGET_RT_MAC_MACHO
void c2pstrcpy(Str255 dst, const char *src)
{
	size_t n = strlen(src);
	memcpy(dst+1, src, n);
	*dst = (unsigned char ) n;
}

void p2cstrcpy(char *dst, ConstStr255Param src)
{
	memcpy(dst, src+1, *src);
	dst[*src] = '\0';
}
#endif