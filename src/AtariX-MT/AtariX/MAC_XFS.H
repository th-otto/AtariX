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
 * MAC_XFS.H  -  MagiC-XFS extensions for Macintosh file system
 */

#ifndef _MAC_XFS_H_INCLUDED_
#define _MAC_XFS_H_INCLUDED_

/*
 * Some Mac OS types:
 */

#ifndef __MWERKS__

typedef unsigned long OSType;

typedef struct
{
	short	v;
	short	h;
} _ATARI_Point;

typedef struct
{
	OSType			fdType;			/* file's Type */
	OSType			fdCreator;		/* file's Creator */
	unsigned short	fdFlags;		/* file's flags */
	_ATARI_Point	fdLocation;		/* file's location in folder */
	short			fdFldr;			/* (unsued) */
} _ATARI_FInfo;

#endif

/*
 * MacMiNT functions:
 */

#define FMACOPENRES     (('F' << 8) | 72)	/* open resource fork */
#define FMACGETTYCR     (('F' << 8) | 73)	/* get Type & Creator */
#define FMACSETTYCR     (('F' << 8) | 74)	/* set Type & Creator */

/*
 * MagiCMac specific functions:
 */

   	#pragma options align=packed
typedef struct
{
	UINT16	funcNo;
	UINT32	longVal;
	UINT32	srcPtr;			// 68k pointer
	UINT32	destPtr;			// 68k pointer
} MMEXRec;
   	#pragma options align=reset

#define FMACMAGICEX     (('F' << 8) | 75)
#define MMEX_INFO       0	/* Dcntl & Fcntl */
#define MMEX_GETFREFNUM 1	/* Fcntl         */
#define MMEX_GETFSSPEC  2	/* Dcntl         */
#define MMEX_GETRSRCLEN 3	/* Dcntl         */
#define MMEX_GETCATINFO 4	/* Dcntl & Fcntl */
#define MMEX_SETCATINFO 5	/* Dcntl & Fcntl */

#endif
