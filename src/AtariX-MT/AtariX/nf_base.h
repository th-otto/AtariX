/*
 * This file was taken from the ARAnyM project, and adapted to AtariX:
 *
 * nf_base.h - NatFeat common base
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

#ifndef _NF_BASE_H
#define _NF_BASE_H

void a2fstrcpy(char *dest, uint32_t source);
void f2astrcpy(uint32_t dest, const char *source);
void atari2HostSafeStrncpy(char *dest, uint32_t source, size_t count);
void host2AtariSafeStrncpy(uint32_t dest, const char *source, size_t count);
size_t atari2HostSafeStrlen(uint32_t source);

/*
 * Host<->Atari mem & str functions
 */
typedef struct _nf_base NF_Base;

struct _nf_base
{
	void (*init)(void);
	void (*exit)(void);
	void (*reset)(void);
	const char *name;
	int isSuperOnly;
	int32_t (*dispatch)(uint32_t fncode, uint32_t args);
};

#endif /* _NF_BASE_H */


