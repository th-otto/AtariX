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

#ifdef __cplusplus
extern "C" {
#endif

// C-Strings (hat MW in stdlib vergessen!)
void ultoa10(unsigned long val, char *str);
#define _ltoa(x, y, z) _itoa(x, y, z)
// Pascal-Strings
void pstrcpy(Str255 dst, ConstStr255Param src);
// Konvertierungen
void c2pstrcpy(Str255 dst, const char *src);
void p2cstrcpy(char *dst, ConstStr255Param src);
#define C2P(s) s[0] = (unsigned char) strlen((char *) (s+1))
#define P2C(s) s[s[0]+1] = '\0'

#ifdef __cplusplus
}
#endif
