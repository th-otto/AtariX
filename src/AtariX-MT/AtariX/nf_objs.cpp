/*
 * This file was taken from the ARAnyM project, and adapted to AtariX:
 *
 * nf_objs.c - Collection of NatFeature objects
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
#include "m68kcpu.h"
#include "natfeat.h"
#include "nf_basicset.h"
#include "nf_debugprintf.h"



extern const NF_Base *const nf_objects[] = {
	/* NF basic set */
	&nf_name,
	&nf_version,
	&nf_shutdown,
	&nf_exit,
	&nf_stderr,
	/* add your NatFeat object below */

	/* */
	&nf_debugprintf
};

extern unsigned int const nf_objs_cnt = sizeof(nf_objects) / sizeof(nf_objects[0]);

void NFReset(void)
{
	unsigned int i;
	
	for (i = 0; i < nf_objs_cnt; i++)
	{
		nf_objects[i]->reset();
	}
}

void NFCreate(void)
{
	unsigned int i;
	
	for (i = 0; i < nf_objs_cnt; i++)
	{
		nf_objects[i]->init();
	}
}

void NFDestroy(void)
{
	unsigned int i;
	
	for (i = 0; i < nf_objs_cnt; i++)
	{
		nf_objects[i]->exit();
	}
}
