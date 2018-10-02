/*
 * This file was taken from the ARAnyM project, and adapted to AtariX:
 *
 * nf_basicset.c - NatFeat Basic Set
 *
 * Copyright (c) 2002-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "Globals.h"
#include <stdlib.h>
#include "m68kcpu.h"
#include "natfeat.h"
#include "nf_basicset.h"
#include "maptab.h"

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void NF_Name_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Name_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Name_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Name_dispatch(uint32 fncode, uint32 args)
{
	uint32 name_ptr = nf_getparameter(args, 0);
	uint32 name_maxlen = nf_getparameter(args, 1);
	char buffer[1024];
	
	const char *text;

	switch (fncode)
	{
	case 0:								/* get_pure_name(char *name, uint32 max_len) */
		text = NAME_STRING;
		break;

	case 1:								/* get_complete_name(char *name, uint32 max_len) */
		sprintf(buffer, VERSION_STRING " (Host: " OS_TYPE "/" HOST_CPU_TYPE "/%s)", HOST_SCREEN_DRIVER);
		text = buffer;
		break;

	default:
		text = "Unimplemented NF_NAME sub-id";
		break;
	}

	host2AtariSafeStrncpy(name_ptr, text, name_maxlen);
	return strlen(text);
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_name = {
	NF_Name_init,
	NF_Name_exit,
	NF_Name_reset,
	"NF_NAME",
	FALSE,
	NF_Name_dispatch
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

/*
 * return version of the NF interface in the form HI.LO (currently 1.0)
 */

static void NF_Version_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Version_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Version_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Version_dispatch(uint32 fncode, uint32 args)
{
	(void)(args);
	switch (fncode)
	{
	case 0:
		return 0x00010000UL;
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_version = {
	NF_Version_init,
	NF_Version_exit,
	NF_Version_reset,
	"NF_VERSION",
	FALSE,
	NF_Version_dispatch
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void NF_Shutdown_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Shutdown_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Shutdown_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

void emu_shutdown(int mode, int exitcode)
{
	switch (mode)
	{
	case -1:
		exit(1);
	case 0:
		exit(exitcode);
		break;
	case 1:
		/* AtariWarmBoot(0, 0); */
		break;
	case 2:
		/* AtariColdBoot(0, 0); */
		break;
	case 3:
		exit(exitcode);
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Shutdown_dispatch(uint32 fncode, uint32 args)
{
	(void)(args);
	switch (fncode)
	{
	case 0:
		emu_shutdown(0, EXIT_SUCCESS);
		break;
	case 1:
		emu_shutdown(1, EXIT_SUCCESS);
		break;
	case 2:
		emu_shutdown(2, EXIT_SUCCESS);
		break;
	case 3:
		emu_shutdown(3, EXIT_SUCCESS);
		break;
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_shutdown = {
	NF_Shutdown_init,
	NF_Shutdown_exit,
	NF_Shutdown_reset,
	"NF_SHUTDOWN",
	TRUE,
	NF_Shutdown_dispatch
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void NF_Exit_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Exit_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Exit_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Exit_dispatch(uint32 fncode, uint32 args)
{
	uint32 exitval;
	
	switch (fncode)
	{
	case 0:
		exitval = nf_getparameter(args, 0);
		emu_shutdown(0, exitval);
		break;
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_exit = {
	NF_Exit_init,
	NF_Exit_exit,
	NF_Exit_reset,
	"NF_EXIT",
	FALSE,
	NF_Exit_dispatch
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

/*
 * print text on standard error stream
 * internally limited to 2048 characters for now
 */

static void NF_Stderr_init(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Stderr_exit(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static void NF_Stderr_reset(void)
{
}

/*** ---------------------------------------------------------------------- ***/

static sint32 NF_Stderr_dispatch(uint32 fncode, uint32 args)
{
	char buffer[2048];
	FILE *output = stderr;
	uint32 str_ptr;
	int i;
	unsigned short ch;
	
	switch (fncode)
	{
	case 0:
		str_ptr = nf_getparameter(args, 0);
	
		atari2HostSafeStrncpy(buffer, str_ptr, sizeof(buffer));
		for (i = 0; buffer[i] != 0; i++)
		{
			ch = atari_to_utf16[(unsigned char)buffer[i]];
			/* inplace variant of g_unichar_to_utf8, for speed */
			if (ch < 0x80)
			{
				putc(ch, output);
			} else if (ch < 0x800)
			{
				putc(((ch >> 6) & 0x3f) | 0xc0, output);
				putc((ch & 0x3f) | 0x80, output);
			} else
			{
				putc(((ch >> 12) & 0x0f) | 0xe0, output);
				putc(((ch >> 6) & 0x3f) | 0x80, output);
				putc((ch & 0x3f) | 0x80, output);
			}
		}
		fflush(output);
		return strlen(buffer);
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

NF_Base const nf_stderr = {
	NF_Stderr_init,
	NF_Stderr_exit,
	NF_Stderr_reset,
	"NF_STDERR",
	FALSE,
	NF_Stderr_dispatch
};
