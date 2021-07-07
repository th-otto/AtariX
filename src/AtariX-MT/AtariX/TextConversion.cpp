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

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Globals.h"
#include "resource.h"
#include "Debug.h"
#include "TextConversion.h"
#include "maptab.h"

// statische Attribute:



/**********************************************************************
*
* statisch: Initialisieren
*
**********************************************************************/

int CTextConversion::Init(void)
{
	return 0;
}


void CTextConversion::Atari2HostUtf8Copy(char *dst, const char *src, size_t count)
{
	unsigned short ch;
	
	while (count > 1)
	{
		ch = (unsigned char)*src++;
		if (ch == 0)
			break;
		ch = atari_to_utf16[ch];
		if (ch < 0x80)
		{
			*dst++ = ch;
			count--;
		} else if (ch < 0x800 || count < 3)
		{
			*dst++ = ((ch >> 6) & 0x3f) | 0xc0;
			*dst++ = (ch & 0x3f) | 0x80;
			count -= 2;
		} else 
		{
			*dst++ = ((ch >> 12) & 0x0f) | 0xe0;
			*dst++ = ((ch >> 6) & 0x3f) | 0x80;
			*dst++ = (ch & 0x3f) | 0x80;
			count -= 3;
		}
	}
	if (count > 0)
		*dst = '\0';
}


void CTextConversion::Host2AtariUtf8Copy(char *dst, const char *src, size_t count)
{
#ifdef __APPLE__
	/* MacOSX uses decomposed strings, normalize them first */
	CFMutableStringRef theString = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(theString, src, kCFStringEncodingUTF8);
	CFStringNormalize(theString, kCFStringNormalizationFormC);
	UniChar ch;
	unsigned short c;
	CFIndex idx;
	CFIndex len = CFStringGetLength(theString);
	
	idx = 0;
	while (count > 1 && idx < len)
	{
		ch = CFStringGetCharacterAtIndex(theString, idx);
		c = utf16_to_atari[ch];
		if (c >= 0x100)
		{
			charset_conv_error(ch);
			/* not convertible. return utf8-sequence to avoid producing duplicate filenames */
			if (ch < 0x80)
			{
				*dst++ = ch;
				count--;
			} else if (ch < 0x800 || count < 3)
			{
				*dst++ = ((ch >> 6) & 0x3f) | 0xc0;
				*dst++ = (ch & 0x3f) | 0x80;
				count -= 2;
			} else 
			{
				*dst++ = ((ch >> 12) & 0x0f) | 0xe0;
				*dst++ = ((ch >> 6) & 0x3f) | 0x80;
				*dst++ = (ch & 0x3f) | 0x80;
				count -= 3;
			}
		} else
		{
			*dst++ = c;
			count -= 1;
		}
		idx++;
	}
	if (count > 0)
	{
		*dst = 0;
	}
	CFRelease(theString);
#else
	unsigned short ch;
	unsigned short c;
	size_t bytes;
	
	while (count > 1 && *src)
	{
		c = (unsigned char) *src;
		ch = c;
		if (ch < 0x80)
		{
			bytes = 1;
		} else if ((ch & 0xe0) == 0xc0 || count < 3)
		{
			ch = ((ch & 0x1f) << 6) | (src[1] & 0x3f);
			bytes = 2;
		} else
		{
			ch = ((((ch & 0x0f) << 6) | (src[1] & 0x3f)) << 6) | (src[2] & 0x3f);
			bytes = 3;
		}
		c = utf16_to_atari[ch];
		if (c >= 0x100)
		{
			charset_conv_error(ch);
			/* not convertible. return utf8-sequence to avoid producing duplicate filenames */
			*dst++ = *src++;
			if (bytes >= 2)
				*dst++ = *src++;
			if (bytes >= 3)
				*dst++ = *src++;
			count -= bytes;
		} else
		{
			*dst++ = c;
			src += bytes;
			count -= 1;
		}
	}
	if (count > 0)
	{
		*dst = 0;
	}
#endif
}

void CTextConversion::charset_conv_error(unsigned short ch)
{
	fprintf(stderr, "cannot convert $%04x to atari codeset\n", ch);
}
