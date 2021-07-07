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
	short			fdFldr;			/* (unused) */
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
	uint16_t	funcNo;
	uint32_t	longVal;
	uint32_t	srcPtr;			// 68k pointer
	uint32_t	destPtr;			// 68k pointer
} MMEXRec;
   	#pragma options align=reset

#define FMACMAGICEX     (('F' << 8) | 75)
#define MMEX_INFO       0	/* Dcntl & Fcntl */
#define MMEX_GETFREFNUM 1	/* Fcntl         */
#define MMEX_GETFSSPEC  2	/* Dcntl         */
#define MMEX_GETRSRCLEN 3	/* Dcntl         */
#define MMEX_GETCATINFO 4	/* Dcntl & Fcntl */
#define MMEX_SETCATINFO 5	/* Dcntl & Fcntl */

/*
 * MagiC opcodes (all group 'm' opcodes are reserved for MagiC)
 */

# define MX_KER_GETINFO         (('m'<< 8) | 0)         /* mgx_dos.txt */
# define MX_KER_DOSLIMITS       (('m'<< 8) | 1)         /* mgx_dos.txt */
# define MX_KER_INSTXFS         (('m'<< 8) | 2)         /* mgx_dos.txt */
# define MX_KER_DRVSTAT         (('m'<< 8) | 4)         /* mgx_dos.txt */
# define MX_KER_XFSNAME         (('m'<< 8) | 5)         /* mgx_dos.txt */
# define MX_DEV_INSTALL         (('m'<< 8) | 0x20)      /* mgx_dos.txt */
# define MX_DFS_GETINFO         (('m'<< 8) | 0x40)      /* mgx_dos.txt */
# define MX_DFS_INSTDFS         (('m'<< 8) | 0x41)      /* mgx_dos.txt */

/*
 * from $FREEMINT_CVS/sys/mint/dcntl.h
 */

# define MINT_V_CNTR_WP	(('V'<< 8) | 100)		/* MiNT, write protect control */

# define MINT_FS_INFO        0xf100
# define MINT_FS_USAGE       0xf101

# define MINT_FS_HOSTFS   (15L << 16)

/*
 * from $FREEMINT_CVS/sys/mint/ioctl.h
 */

# define MINT_FSTAT		(('F'<< 8) | 0)		/* handled by kernel */
# define MINT_FIONREAD		(('F'<< 8) | 1)
# define MINT_FIONWRITE		(('F'<< 8) | 2)
# define MINT_FUTIME		(('F'<< 8) | 3)

# define MINT_FTRUNCATE		(('F'<< 8) | 4)
# define MINT_FIOEXCEPT		(('F'<< 8) | 5)

# define MINT_FSTAT64		(('F'<< 8) | 6)		/* 1.15.4 extension, optional */
# define MINT_FUTIME_UTC	(('F'<< 8) | 7)		/* 1.15.4 extension, optional */
# define MINT_FIBMAP		(('F'<< 8) | 10)

# define MINT_EXT2_IOC_GETFLAGS       (('f' << 8) | 1)
# define MINT_EXT2_IOC_SETFLAGS       (('f' << 8) | 2)
# define MINT_EXT2_IOC_GETVERSION_NEW (('f' << 8) | 3)
# define MINT_EXT2_IOC_SETVERSION_NEW (('f' << 8) | 4)

# define MINT_EXT2_IOC_GETVERSION     (('v' << 8) | 1)
# define MINT_EXT2_IOC_SETVERSION     (('v' << 8) | 2)

# define MINT_F_GETLK  5
# define MINT_F_SETLK  6
# define MINT_F_SETLKW 7

#endif
