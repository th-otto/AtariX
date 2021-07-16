/*
 * This file was taken from the ARAnyM project, and adapted to AtariX:
 *
 * Copyright 2002-2004 Petr Stehlik of the Aranym dev team
 *
 * printf routines Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include "Globals.h"
#include "m68kcpu.h"
#include "natfeat.h"
#include "nf_debugprintf.h"

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void NF_Debugprintf_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Debugprintf_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Debugprintf_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static long debugprintf_puts(FILE *f, uint32 s, int width)
{
	long put = 0;
    unsigned char c;
    
	if (s == 0)
    {
		fputs("(null)", f);
        width -= (int)sizeof("(null)") - 1;
        put += (int)sizeof("(null)") - 1;
    } else
    {
        while ((c = m68ki_read_8(s)) != 0)
        {
            fputc(c, f);
            s++;
            put++;
            width--;
        }
    }
    
	while (width-- > 0)
	{
		fputc(' ', f);
		put++;
	}
	
	return put;
}

/*** ---------------------------------------------------------------------- ***/

static long debugprintf_putl(FILE *f, uint32 u, int base, int width, int fill_char)
{
	char obuf[32];
	char *t = obuf;
	long put = 0;
	
	do {
		*t++ = "0123456789ABCDEF"[u % base];
		u /= base;
		width--;
	} while (u > 0);
	
	while (width-- > 0)
	{
		fputc(fill_char, f);
		put++;
	}
	
	while (t != obuf)
	{
		fputc(*--t, f);
		put++;
	}
	
	return put;
}

/*** ---------------------------------------------------------------------- ***/

static long debugprintf_putc(FILE *f, int c, int width)
{
	long put = 1;
	
	fputc(c, f);
	while (--width > 0)
	{
		fputc(' ', f);
		put++;
	}
	
	return put;
}

/*** ---------------------------------------------------------------------- ***/

# define TIMESTEN(x)	((((x) << 2) + (x)) << 1)

static long debugprintf(FILE *f, const char *fmt, uint32 args, int param)
{
	char c;
	char fill_char;

	long len = 0;
	
	int width;
	int long_flag;
	
	uint32 s_arg;
	int   i_arg;
	long  l_arg;

	while ((c = *fmt++) != 0)
	{
		if (c != '%')
		{
			len += debugprintf_putc(f, c, 1);
			continue;
		}
		
		c = *fmt++;
		width = 0;
		long_flag = 0;
		fill_char = ' ';
		
		if (c == '0')
		{
			fill_char = '0';
			c = *fmt++;
		}
		
		while (c >= '0' && c <= '9')
		{
			width = TIMESTEN(width) + (c - '0');
			c = *fmt++;
		}
		
		if (c == 'l' || c == 'L')
		{
			long_flag = 1;
			if (c == 'l' && *fmt == 'l')
				fmt++;
			c = *fmt++;
		}
		
		if (!c)
			break;
		
		switch (c)
		{
		case '%':
			len += debugprintf_putc(f, c, width);
			break;
		case 'c':
			i_arg = (int)nf_getparameter(args, param++);
			len += debugprintf_putc(f, i_arg, width);
			break;
		case 's':
			s_arg = nf_getparameter(args, param);
			param++;
			len += debugprintf_puts(f, s_arg, width);
			break;
		case 'i':
		case 'd':
			if (long_flag)
				l_arg = (long)nf_getparameter(args, param++);
			else
				l_arg = (int)nf_getparameter(args, param++);
			if (l_arg < 0)
			{
				len += debugprintf_putc(f, '-', 1);
				width--;
				l_arg = -l_arg;
			}
			len += debugprintf_putl(f, (uint32_t)l_arg, 10, width, fill_char);
			break;
		case 'o':
			if (long_flag)
				l_arg = (unsigned long)nf_getparameter(args, param++);
			else
				l_arg = (unsigned int)nf_getparameter(args, param++);
			len += debugprintf_putl(f, (uint32_t)l_arg, 8, width, fill_char);
			break;
		case 'x':
			if (long_flag)
				l_arg = (unsigned long)nf_getparameter(args, param++);
			else
				l_arg = (unsigned int)nf_getparameter(args, param++);
			len += debugprintf_putl(f, (uint32_t)l_arg, 16, width, fill_char);
			break;
		case 'b':
			if (long_flag)
				l_arg = (unsigned long)nf_getparameter(args, param++);
			else
				l_arg = (unsigned int)nf_getparameter(args, param++);
			len += debugprintf_putl(f, (uint32_t)l_arg, 2, width, fill_char);
			break;
		case 'u':
			if (long_flag)
				l_arg = (unsigned long)nf_getparameter(args, param++);
			else
				l_arg = (unsigned int)nf_getparameter(args, param++);
			len += debugprintf_putl(f, (uint32_t)l_arg, 10, width, fill_char);
			break;
		}
	}
	
	return len;
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Debugprintf_dispatch(uint32 fncode, uint32 args)
{
	char buffer[2048];
	FILE *output = stderr;
	uint32 str_ptr;
	int ret = 0;
	
	switch (fncode)
	{
	case 0:
		str_ptr = nf_getparameter(args, 0);
	
		atari2HostSafeStrncpy(buffer, str_ptr, sizeof(buffer));
	
		ret = (int)debugprintf(output, buffer, args, 1);
		fflush(output);
		break;
	}
	return ret;
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_debugprintf = {
	NF_Debugprintf_init,
	NF_Debugprintf_exit,
	NF_Debugprintf_reset,
	"DEBUGPRINTF",
	FALSE,
	NF_Debugprintf_dispatch
};
