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
** (TAB-Setting: 5)
** (Font = Chicago 12)
**
**
** Dies ist der Mac-Teil des Mac-XFS fuer MagiCMacX
**
** (C) Andreas Kromke 1994-2007
**
**
** Übergabestrukturen:
**   ataridos       enthält die Mac-Seite des Dateisystems (XFS)
**   macdev         enthält die Mac-Seite des Dateitreibers (MX_FD)
**
*/

#include "config.h"
// System-Header
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <string.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "MacXFS.h"
#include "Atari.h"
#include "maptab.h"
#include "TextConversion.h"
#include "s_endian.h"
#include <sys/stat.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/ioctl.h>

// please remember to leave this define _after_ the required system headers!!!
// some systems does define this to some important value for them....
#ifndef O_BINARY
# define O_BINARY 0
#endif


#if defined(_DEBUG)
#define DEBUG_VERBOSE
#endif

#if DEBUG_68K_EMU
// Mechanismus zum Herausfinden der Ladeadresse eines Programms
static int trigger = 0;
#define ATARI_PRG_TO_TRACE "PD.PRG"
short trigger_refnum;
//int m68k_trace_trigger = 0;
char *trigger_ProcessStartAddr = 0;
unsigned trigger_ProcessFileLen = 0;
extern void _DumpAtariMem(const char *filename);
#endif


#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/* directory separator, atari */
#define IS_DIR_SEP(c) ((c) == '\\' || (c) == '/')
/* directory separator, host */
#define DIRSEPARATOR "/"


#define HOSTFS_XFS_VERSION   0
#define HOSTFS_NFAPI_VERSION 1

/*
 * Should actually be checked by autoconf
 */
#define HAVE_REALPATH
#define HAVE_STRUCT_TM_TM_GMTOFF
#if defined(MAC_OS_X_VERSION_10_13) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_13
#define HAVE_FUTIMENS
#endif


#ifdef HAVE_SYS_STATVFS_H
#  define STATVFS struct statvfs
#else
#  define STATVFS struct statfs
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/stavfs.h>
#else
#include <sys/mount.h>
#endif


/*****************************************************************
*
*  Support functions
*
******************************************************************/

static char *strd2upath(char *dest)
{
	char *result = dest;
	while (*dest)
	{
		if (*dest == '\\' || *dest == '/')
			*dest = DIRSEPARATOR[0];
		dest++;
	}
	return result;
}

static char *stru2dpath(char *dest)
{
	char *result = dest;
	while (*dest)
	{
		if (*dest == '/')
			*dest = '\\';
		dest++;
	}
	return result;
}

#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? (d - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? (d - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? (d - '1' + 26) : \
	 -1)
#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')

static void datetime2tm(uint32_t dtm, struct tm* ttm)
{
	ttm->tm_mday = dtm & 0x1f;
	ttm->tm_mon	 = ((dtm>>5) & 0x0f) - 1;
	ttm->tm_year = ((dtm>>9) & 0x7f) + 80;
	ttm->tm_sec	 = ((dtm>>16) & 0x1f) << 1;
	ttm->tm_min	 = (dtm>>21) & 0x3f;
	ttm->tm_hour = (dtm>>27) & 0x1f;
	ttm->tm_isdst = -1;
}

static time_t datetime2utc(uint32_t dtm)
{
	struct tm ttm;
	datetime2tm(dtm, &ttm);
	return mktime(&ttm);
}

static long gmtoff(time_t t)
{
	struct tm *tp;

	if ((tp = localtime(&t)) == NULL)
		return 0;
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	return tp->tm_gmtoff;
#else
	{
		struct tm gtm;
		struct tm ltm;
		time_t lt;
		long a4, b4, a100, b100, a400, b400, intervening_leap_days, years, days;
		ltm = *tp;
		lt = mktime(&ltm);
	
		if (lt == (time_t) -1)
		{
			/* mktime returns -1 for errors, but -1 is also a
			   valid time_t value.  Check whether an error really
			   occurred.  */
			struct tm tm;
	
			if ((tp = localtime(&lt)) == NULL)
				return 0;
			tm = *tp;
			if ((ltm.tm_sec ^ tm.tm_sec) ||
				(ltm.tm_min ^ tm.tm_min) ||
				(ltm.tm_hour ^ tm.tm_hour) ||
				(ltm.tm_mday ^ tm.tm_mday) ||
				(ltm.tm_mon ^ tm.tm_mon) ||
				(ltm.tm_year ^ tm.tm_year))
				return 0;
		}
	
		if ((tp = gmtime(&lt)) == NULL)
			return 0;
		gtm = *tp;
		
		a4 = (ltm.tm_year / 4) + (1900 / 4) - !(ltm.tm_year & 3);
		b4 = (gtm.tm_year / 4) + (1900 / 4) - !(gtm.tm_year & 3);
		a100 = a4 / 25 - (a4 % 25 < 0);
		b100 = b4 / 25 - (b4 % 25 < 0);
		a400 = a100 / 4;
		b400 = b100 / 4;
		intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
		years = ltm.tm_year - gtm.tm_year;
		days = (365 * years + intervening_leap_days + (ltm.tm_yday - gtm.tm_yday));
	
		return (60 * (60 * (24 * days + (ltm.tm_hour - gtm.tm_hour)) + (ltm.tm_min - gtm.tm_min)) + (ltm.tm_sec - gtm.tm_sec));
	}
#endif
}

CMacXFS::XfsFsFile::XfsFsFile(CMacXFS &xfs, const char *root)
	: parent(NULL),
	  refCount(1),
	  childCount(0),
	  created(false),
	  locks(0),
	  name(strdup(root)),
	  childs(NULL),
	  next(NULL),
	  m_xfs(xfs)
{
	mapped_value = MAPNEWVOIDP(this);
}

CMacXFS::XfsFsFile::~XfsFsFile()
{
	free(name);
	MAPDELVOIDP(this);
}

CMacXFS::XfsFsFile *CMacXFS::XfsFsFile::insert(CMacXFS &xfs, const char *name)
{
	CMacXFS::XfsFsFile *f;
	CMacXFS::XfsFsFile **prev;
	
	for (prev = &this->childs; *prev != NULL; prev = &(*prev)->next)
		if (strcmp((*prev)->name, name) == 0)
			return *prev;
	f = new CMacXFS::XfsFsFile(xfs, name);
	if (f == NULL)
	{
		DebugError("MacXFS: malloc() failed!");
		return NULL;
	}
	*prev = f;
	this->childCount++;
	f->parent = this;
	return f;
}

void CMacXFS::XfsFsFile::remove(const char *name)
{
	CMacXFS::XfsFsFile **prev;
	CMacXFS::XfsFsFile *f;
	
	for (prev = &this->childs; *prev != NULL; prev = &(*prev)->next)
		if (strcmp((*prev)->name, name) == 0)
		{
			f = *prev;
			*prev = f->next;
			this->childCount--;
			delete f;
			return;
		}
	DebugInfo("CMacXFS: name %s not found in %s", name, this->name);
}

/*****************************************************************
*
*  Konstruktor
*
******************************************************************/

CMacXFS::CMacXFS()
{
	int i;
	struct mount_info *drv;
	unsigned int dev;
	
	for (i = 0; i < NDRVS; i++)
	{
		drives[i].drv_valid = false;    // ungueltig
		drives[i].drv_must_eject = false;
		drives[i].drv_changed = false;
		drives[i].drv_flags = M_DRV_DOSNAMES;
		drives[i].drv_type = NoMacXFS;
		drives[i].host_root = NULL;
	}

	// Mac-Wurzelverzeichnis machen
	dev = DriveFromLetter('M');
	xfs_drvbits = 1L << dev;
	drv = &drives[dev];
	drv->drv_type = MacRoot;
	drv->drv_valid = true;
	drv->drv_flags = M_DRV_REVERSE_DIR_ORDER | M_DRV_READONLY; /* XXX */
	// drive_m->halfSensitive = true;
	drv->host_root = new XfsFsFile(*this, "/");
	strcpy(drv->mount_point, "A:\\");
	drv->mount_point[0] = DriveToLetter(dev);

	DebugInfo("CMacXFS::%s() -- Drive %c: '%s', root DD=%08lx, dir order=%s, names=%s",
		__FUNCTION__,
		DriveToLetter(dev),
		drv->host_root ? drv->host_root->name : "",
		(unsigned long)(drv->host_root ? MAPVOIDPTO32(drv->host_root) : 0),
		drv->drv_flags & M_DRV_REVERSE_DIR_ORDER ? "reverse" : "normal",
		drv->drv_flags & M_DRV_DOSNAMES ? "8+3" : "long");
}


/*****************************************************************
*
*  Destruktor
*
******************************************************************/

CMacXFS::~CMacXFS()
{
}


#ifdef DEBUG_VERBOSE
static void __dump(const char *what, const unsigned char *p, int len)
{
	DebugInfo("%s", what);
	while (len >= 8)
	{
		DebugInfo(" mem = %02x %02x %02x %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		len -= 8;
	}

	if (len >= 7)
	{
		DebugInfo(" mem = %02x %02x %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4], p[5], p[6]);
	} else if (len >= 6)
	{
		DebugInfo(" mem = %02x %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4], p[5]);
	} else if (len >= 5)
	{
		DebugInfo(" mem = %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4]);
	} else if (len >= 4)
	{
		DebugInfo(" mem = %02x %02x %02x %02x", p[0], p[1], p[2], p[3]);
	} else if (len >= 3)
	{
		DebugInfo(" mem = %02x %02x %02x", p[0], p[1], p[2]);
	} else if (len >= 2)
	{
		DebugInfo(" mem = %02x %02x", p[0], p[1]);
	} else if (len >= 1)
	{
		DebugInfo(" mem = %02x", p[0]);
	}
}
#endif


/*****************************************************************
*
*  68k Adreßbereich festlegen
*
******************************************************************/

void CMacXFS::Set68kAdressRange(memptr AtariMemSize)
{
	m_AtariMemSize = AtariMemSize;
}


/*****************************************************************
*
*  (statisch) Wandelt Klein- in Großchrift um
*
******************************************************************/

                             /* ä       ö      ü       ç       é        å       æ      œ       à       (ij)    (n˜)    (a˜)    ø       (o˜) */
static char const lowers[] = { '\x84', '\x94', '\x81', '\x87', '\x82', '\x86', '\x91', '\xb4', '\x85', '\xc0', '\xa4', '\xb0', '\xb3', '\xb1',0};
static char const uppers[] = { '\x8e', '\x99', '\x9a', '\x80', '\x90', '\x8f', '\x92', '\xb5', '\xb6', '\xc1', '\xa5', '\xb7', '\xb2', '\xb8',0};
unsigned char CMacXFS::ToUpper(unsigned char c)
{
	char *found;

	if (c >= 'a' && c <= 'z')
		return c & 0x5f;
	if (c >= 128 && ((found = strchr(lowers, c)) != NULL))
		return uppers[found - lowers];
	return c;
}

unsigned char CMacXFS::ToLower(unsigned char c)
{
	char *found;

	if (c >= 'A' && c <= 'Z')
		return c | 0x20;
	if ((unsigned char) c >= 128 && (found = strchr(uppers, c)) != NULL)
		return lowers[found - uppers];
	return c;
}


/*****************************************************************
*
*  (statisch) ermittelt Dateitypen
*
******************************************************************/

static void GetTypeAndCreator(const char *name, OSType *pType, OSType *pCreator)
{
	name = strrchr(name, '.');

	if ((name) && (!strcmp(name, ".PRG") || !strcmp(name, ".prg") || !strcmp(name, ".TOS") || !strcmp(name, ".tos") || !strcmp(name, ".TTP") || !strcmp(name, ".ttp")))
	{
		*pType = 'Gem1';
	}
	else
	{
		*pType = '\0\0\0\0';
		*pCreator = '\0\0\0\0';
	}
/*
	char **list = CGlobals::s_Preferences.m_DocTypes;
	int i = CGlobals::s_Preferences.m_DocTypesNum;
	char *s;

	name = strrchr(name, '.');
	if (!name)
		return;		// keine Datei-Endung

	while (i)
	{
		s = *list++;
		t = name;
		while ((toupper(*s) != '.') && (toupper(*t)) && (toupper(*s) == toupper(*t)))
		{
			
		}
	}
*/
}


/*****************************************************************
*
*  (statisch) konvertiert Mac-Datumsangaben in DOS- Angaben
*
******************************************************************/

void CMacXFS::date_mac2dos(time_t macdate, uint16_t *time, uint16_t *date)
{
	struct tm *x;
	x = localtime(&macdate);

	if (time)
		*time = ((x->tm_sec & 0x3f) >> 1) | ((x->tm_min & 0x3f) << 5) | ((x->tm_hour & 0x1f) << 11);
	if (date)
		*date = x->tm_mday | ((x->tm_mon + 1) << 5) | (MAX(x->tm_year - 80, 0) << 9);
}

/*****************************************************************
*
*  (statisch) konvertiert DOS-Datumsangaben in Mac-Angaben
*
******************************************************************/

time_t CMacXFS::date_dos2mac(uint16_t time, uint16_t date)
{
	struct tm tm;
	
	tm.tm_sec	= (short) ((time & 0x1f) << 1);
	tm.tm_min	= (short) ((time >> 5) & 0x3f);
	tm.tm_hour	= (short) ((time >> 11) & 0x1f);
	tm.tm_mday	= (short) (date & 0x1f);
	tm.tm_mon	= (short) ((date >> 5) & 0x0f) - 1;
	tm.tm_year	= (short) (((date >> 9) & 0x7f) + 80);
	tm.tm_isdst = -1;
	return mktime(&tm);
}


/*****************************************************************
*
*  (statisch) Testet, ob ein Dateiname gültig ist. Verboten sind Dateinamen, die
*  nur aus '.'-en bestehen.
*
******************************************************************/

int CMacXFS::fname_is_invalid(const char *name)
{
	while (*name == '.')		// führende Punkte weglassen
		name++;
	return *name == '\0';		// Name besteht nur aus Punkten
}


/*****************************************************************
*
*  (statisch) konvertiert Posix-Fehlercodes in MagiC- Fehlercodes
*
******************************************************************/

int32_t CMacXFS::errnoHost2Mint(int unixerrno, int defaulttoserrno)
{
	int retval = defaulttoserrno;

	switch (unixerrno)
	{
		case 0:       retval = TOS_E_OK; break;
		case EACCES:
		case EPERM:
		case ENFILE:  retval = TOS_EACCDN; break;
		case EBADF:	  retval = TOS_EIHNDL; break;
		case ENOTDIR: retval = TOS_EPTHNF; break;
		case EROFS:   retval = TOS_EWRPRO; break;
		case ENOENT: /* GEMDOS most time means it should be a file */
		case ECHILD:  retval = TOS_EFILNF; break;
		case ESRCH:   retval = TOS_ESRCH; break;
		case ENXIO:	  retval = TOS_EDRIVE; break;
		case EIO:	  retval = TOS_EIO; break;	 /* -90 I/O error */
		case ENOEXEC: retval = TOS_EPLFMT; break;
		case ENOMEM:  retval = TOS_ENSMEM; break;
		case EFAULT:  retval = TOS_EIMBA; break;
		case EEXIST:  retval = TOS_EEXIST; break; /* -85 file exist, try again later */
		case EXDEV:	  retval = TOS_ENSAME; break;
		case ENODEV:  retval = TOS_EUNDEV; break;
		case EINVAL:  retval = TOS_EINVFN; break;
		case EMFILE:  retval = TOS_ENHNDL; break;
		case ENOSPC:  retval = TOS_ENOSPC; break; /* -91 disk full */
		case EBUSY:   retval = TOS_EDRVNR; break;
		case EISDIR:  retval = TOS_EISDIR; break;
		case ENAMETOOLONG: retval = TOS_ERANGE; break;
	}

	return retval;
}

/**
 * Convert the unix file stat.st_mode value into a MagiC/MiNT's one.
 *
 * File permissions and mode bitmask according
 * to the FreeMiNT 1.16.x stat.h header file.
 *
 *  Name     Mask     Permission
 * S_IXOTH  0000001  Execute permission for all others.
 * S_IWOTH  0000002  Write permission for all others.
 * S_IROTH  0000004  Read permission for all others.
 * S_IXGRP  0000010  Execute permission for processes with same group ID.
 * S_IWGRP  0000020  Write permission for processes with same group ID.
 * S_IRGRP  0000040  Read permission for processes with same group ID.
 * S_IXUSR  0000100  Execute permission for processes with same user ID.
 * S_IWUSR  0000200  Write permission for processes with same user ID.
 * S_IRUSR  0000400  Read permission for processes with same user ID.

 * S_ISVTX  0001000  Sticky bit
 * S_ISGID  0002000  Alter effective group ID when executing this file.
 * S_ISUID  0004000  Alter effective user ID when executing this file.
 *
 * S_IFSOCK 0010000  File is a FreeMiNTNet socket file.
 * S_IFCHR  0020000  File is a BIOS (character) special file.
 * S_IFDIR  0040000  File is a directory.
 * S_IFBLK  0060000  File is a block special file.
 * S_IFREG  0100000  File is a regular file.
 * S_IFIFO  0120000  File is a FIFO (named pipe).
 * S_IMEM   0140000  File is a memory region.
 * S_IFLNK  0160000  File is a symbolic link.
 */
uint16_t CMacXFS::modeHost2Mint(mode_t m)
{
	uint16_t result = 0;

	// permissions
	if ( m & S_IXOTH ) result |= 00001;
	if ( m & S_IWOTH ) result |= 00002;
	if ( m & S_IROTH ) result |= 00004;
	if ( m & S_IXGRP ) result |= 00010;
	if ( m & S_IWGRP ) result |= 00020;
	if ( m & S_IRGRP ) result |= 00040;
	if ( m & S_IXUSR ) result |= 00100;
	if ( m & S_IWUSR ) result |= 00200;
	if ( m & S_IRUSR ) result |= 00400;
	if ( m & S_ISVTX ) result |= 01000;
	if ( m & S_ISGID ) result |= 02000;
	if ( m & S_ISUID ) result |= 04000;

	if ( S_ISSOCK(m) ) result |= _ATARI_S_IFSOCK;
	if ( S_ISCHR(m)  ) result |= _ATARI_S_IFCHR;
	if ( S_ISDIR(m)  ) result |= _ATARI_S_IFDIR;
	if ( S_ISBLK(m)  ) result |= _ATARI_S_IFBLK;
	if ( S_ISREG(m)  ) result |= _ATARI_S_IFREG;
	if ( S_ISFIFO(m) ) result |= _ATARI_S_IFIFO;
	// Linux doesn't have this! if ( S_ISMEM(m)  ) result |= 0140000;
	if ( S_ISLNK(m)  ) result |= _ATARI_S_IFLNK;

	return result;
}

/*****************************************************************
*
*  (static) convert mint file modes to host
*
******************************************************************/

mode_t CMacXFS::modeMint2Host(uint16_t m)
{
	mode_t result = 0;

	// permissions
	if ( m & 00001 ) result |= S_IXOTH;
	if ( m & 00002 ) result |= S_IWOTH;
	if ( m & 00004 ) result |= S_IROTH;
	if ( m & 00010 ) result |= S_IXGRP;
	if ( m & 00020 ) result |= S_IWGRP;
	if ( m & 00040 ) result |= S_IRGRP;
	if ( m & 00100 ) result |= S_IXUSR;
	if ( m & 00200 ) result |= S_IWUSR;
	if ( m & 00400 ) result |= S_IRUSR;
	if ( m & 01000 ) result |= S_ISVTX;
	if ( m & 02000 ) result |= S_ISGID;
	if ( m & 04000 ) result |= S_ISUID;

	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFSOCK) result |= S_IFSOCK;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFCHR) result |= S_IFCHR;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFDIR) result |= S_IFDIR;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFBLK) result |= S_IFBLK;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFREG) result |= S_IFREG;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFIFO) result |= S_IFIFO;
	// Linux doesn't have this!	if ( m & 0140000 ) result |= S_IFMEM;
	if ( (m & _ATARI_S_IFMT) == _ATARI_S_IFLNK) result |= S_IFLNK;

	return result;
}


int CMacXFS::flagsMagic2Host(uint16_t flags)
{
	int res = O_RDONLY;

	switch (flags & (OM_RPERM|OM_WPERM))
	{
	default:
	case 0:
	case OM_RPERM:
		res = O_RDONLY;
		break;
	case OM_WPERM:
		res = O_WRONLY;
		break;
	case OM_RPERM|OM_WPERM:
		res = O_RDWR;
		break;
	}
	if (flags & _ATARI_O_CREAT)
		res |= O_CREAT;
	if (flags & _ATARI_O_TRUNC)
		res |= O_TRUNC;
	if (flags & _ATARI_O_EXCL)
		res |= O_EXCL;
	if (flags & _ATARI_O_APPEND)
		res |= O_APPEND;
	if (flags & _ATARI_O_NONBLOCK)
		res |= O_NONBLOCK;
#if 0
	if (flags & _ATARI_O_NOCTTY) /* not handled by kernel */
		res |= O_NOCTTY;
#endif

	return res;
}


int16_t CMacXFS::flagsHost2Magic(int flags)
{
	int16_t res = 0; /* default read only */

	switch (flags & O_ACCMODE)
	{
	case O_RDONLY:
	default:
		res = OM_RPERM;
		break;
	case O_WRONLY:
		res = OM_WPERM;
		break;
	case O_RDWR:
		res = OM_RPERM | OM_WPERM;
		break;
	}

	if (flags & O_CREAT)
		res |= _ATARI_O_CREAT;
	if (flags & O_TRUNC)
		res |= _ATARI_O_TRUNC;
	if (flags & O_EXCL)
		res |= _ATARI_O_EXCL;
	if (flags & O_APPEND)
		res |= _ATARI_O_APPEND;
	if (flags & O_NONBLOCK)
		res |= _ATARI_O_NONBLOCK;
#if 0
	if (flags & O_NOCTTY)
		res |= _ATARI_O_NOCTTY; /* not handled by kernel */
#endif

	return res;
}


uint16_t CMacXFS::modeHost2TOS(mode_t m)
{
	return S_ISDIR(m) ? F_SUBDIR : 0;	/* FIXME */
}


/*********************************************************************
*
* (statisch)
* Vergleicht ein 12- stelliges Dateimuster mit einem Dateinamen.
* Die Stelle 11 ist das Datei- Attribut(-muster).
* Rueckgabe : 1 = MATCH, sonst 0
*
* Regeln zum Vergleich der Attribute:
*    1) ReadOnly und Archive werden bei dem Vergleich NIEMALS
*       beruecksichtigt.
*    2) Ist das Suchattribut 8, werden genau alle Dateien mit gesetztem
*       Volume- Bit gefunden (auch versteckte usw.).
*    3) Ist das Suchattribut nicht 8, werden normale Dateien IMMER
*       gefunden.
*    4) Ist das Suchattribut nicht 8, werden Ordner nur bei gesetztem
*       Bit 4 gefunden.
*    5) Ist das Suchattribut nicht 8, werden Volumes nur bei gesetztem
*       Bit 3 gefunden.
*    6) Ist das Suchattribut nicht 8, werden versteckte oder System-
*       dateien (auch Ordner oder Volumes) NUR gefunden, wenn das
*       entsprechende Bit im Suchattribut gesetzt ist.
*
* Beispiele (die Bits ReadOnly und Archive sind ohne Belang):
*    8    alle Dateien mit gesetztem Bit 3 (Volumes)
*    0    nur normale Dateien
*    2    normale und versteckte Dateien
*    6    normale, versteckte und System- Dateien
*  $10    normale Dateien, normale Ordner
*  $12    normale und versteckte Dateien und Ordner
*  $16    normale und versteckte und System- Dateien und -Ordner
*   $a    normale und versteckte Dateien und Volumes
*   $e    normale, versteckte und System- Dateien und -Volumes
*  $1e    alles
*
***************************************************************************/

bool CMacXFS::filename_match(char *muster, char *fname)
{
	int i;
	char c1,c2;


	if (fname[0] == '\xe5')     /* Suche nach geloeschter Datei */
		return muster[0] == '?' || muster[0] == fname[0];

	/* vergleiche 11 Zeichen */

	for	(i = 10; i >= 0; i--)
	{
		c1 = *muster++;
		c2 = *fname++;
		if (c1 != '?')
		{
			if (ToUpper(c1) != ToUpper(c2))
				return false;
		}
	}

	/* vergleiche Attribut */

	c1 = *muster;
	c2 = *fname;

//	if (c1 == F_ARCHIVE)	
//		return c1 & c2;
	c2 &= F_SUBDIR | F_SYSTEM | F_HIDDEN | F_VOLUME;
	if (!c2)
		return true;
	if ((c2 & F_SUBDIR) && !(c1 & F_SUBDIR))
		return false;
	if ((c2 & F_VOLUME) && !(c1 & F_VOLUME))
		return false;
	if (c2 & (F_HIDDEN|F_SYSTEM))
	{
		c2 &= F_HIDDEN | F_SYSTEM;
		c1 &= F_HIDDEN | F_SYSTEM;
	}

	return (bool) (c1 & c2);
}


/*********************************************************************
*
* (statisch)
*  Das in <pathname> stehende, erste Pfadelement (vor '\' oder EOS)
*  wird ins interne Format gewandelt und nach <name> kopiert
*
* 7.9.95:  Funktion liefert TRUE zurück, wenn das Pfadelement gekürzt
*  wurde.
*
**********************************************************************/

bool CMacXFS::conv_path_elem(const char *path, char *name)
{
	int i;
	char c;
	bool truncated = false;

	/* max. 8 Zeichen fuer Dateinamen kopieren */

	for	(i = 0; i < 8 && *path &&
		 !IS_DIR_SEP(*path) && *path != '*' && *path != '.' && *path != ' '; i++)
	{
		*name++ = ToUpper(*path++);
	}

	/* Zeichen fuer das Auffuellen ermitteln */

	if (i == 8)
	{
		while (*path && *path != ' ' && !IS_DIR_SEP(*path) && *path != '.')
		{
			path++;
			truncated = true;
		}
	}

	c = *path == '*' ? '?' : ' ';
	if (*path == '*')
		path++;
	if (*path == '.')
		path++;

	/* Rest bis 8 Zeichen auffuellen */

	for	(;i < 8; i++)
	{
		*name++ = c;
	}

	/* max. 3 Zeichen fuer Typ kopieren */

	for	(i = 0; i < 3 && *path &&
		 !IS_DIR_SEP(*path) && *path != '*' && *path != '.' && *path != ' '; i++)
	{
		*name++ = ToUpper(*path++);
	}

	if (*path && !IS_DIR_SEP(*path) && *path != '*')
		truncated = true;

	/* Zeichen fuer das Auffuellen ermitteln */

	c = *path == '*' ? '?' : ' ';

	/* Rest bis 3 Zeichen auffuellen */

	for	(;i < 3; i++)
	{
		*name++ = c;
	}

	return truncated;
}


/*************************************************************
*
* (statisch)
* Wandelt Mac-Dateinamen nach 8.3-Namen fuer GEMDOS.
* Wenn <convmode> == 0 ist, wird nicht nach Gross-Schrift
* konvertiert, das Format ist aber in jedem Fall 8+3.
* Zeichen ' ' (Leerstelle) und '\' (Backslash) werden weggelassen.
*
* Neu (6.9.95): Rückgabe TRUE, wenn der Dateiname gekürzt
* werden mußte. Fsfirst/next sowie D(x)readdir können dann
* die entsprechende Datei ignorieren.
*
*************************************************************/

bool CMacXFS::nameto_8_3(const char *atariname, char *dosname, int convmode, bool toatari)
{
	short i;
	bool truncated = false;
	unsigned char c;
	unsigned short ch;

 	/* max. 8 Zeichen fuer Dateinamen kopieren */
 	i = 0;
	while (i < 8 && *atariname && *atariname != '.')
	{
		if (*atariname == ' ')
		{
			atariname++;
			continue;
		}
		if (IS_DIR_SEP(*atariname))
		{
			break;
		}
		c = *atariname++;
		if (convmode == 0)
			;
		else if (convmode == 1)
			c = ToUpper(c);
		else
			c = ToLower(c);
		if (toatari)
		{
			*dosname++ = c;
		} else
		{
			ch = atari_to_utf16[c];
			if (ch < 0x80)
			{
				*dosname++ = ch;
			} else if (ch < 0x800)
			{
				*dosname++ = ((ch >> 6) & 0x3f) | 0xc0;
				*dosname++ = (ch & 0x3f) | 0x80;
			} else
			{
				*dosname++ = ((ch >> 12) & 0x0f) | 0xe0;
				*dosname++ = ((ch >> 6) & 0x3f) | 0x80;
				*dosname++ = (ch & 0x3f) | 0x80;
			}
		}
		i++;
	}

	while (*atariname && *atariname != '.')
	{
		atariname++;		// Rest vor dem '.' ueberlesen
		truncated = true;
	}
	if (*atariname == '.')
		atariname++;		// '.' ueberlesen
	*dosname++ = '.';			// '.' in DOS-Dateinamen einbauen

	/* max. 3 Zeichen fuer Typ kopieren */
	i = 0;
	while (i < 3 && *atariname && *atariname != '.')
	{
		if (*atariname == ' ')
		{
			atariname++;
			continue;
		}
		if (IS_DIR_SEP(*atariname))
		{
			break;
		}
		c = *atariname++;
		if (convmode == 0)
			;
		else if (convmode == 1)
			c = ToUpper(c);
		else
			c = ToLower(c);
		if (toatari)
		{
			*dosname++ = c;
		} else
		{
			ch = atari_to_utf16[c];
			if (ch < 0x80)
			{
				*dosname++ = ch;
			} else if (ch < 0x800)
			{
				*dosname++ = ((ch >> 6) & 0x3f) | 0xc0;
				*dosname++ = (ch & 0x3f) | 0x80;
			} else
			{
				*dosname++ = ((ch >> 12) & 0x0f) | 0xe0;
				*dosname++ = ((ch >> 6) & 0x3f) | 0x80;
				*dosname++ = (ch & 0x3f) | 0x80;
			}
		}
		i++;
	}

	if (dosname[-1] == '.')		// trailing '.'
		dosname[-1] = '\0';		//   entfernen
	else
		*dosname = '\0';

	if (*atariname)
		truncated = true;

	return truncated;
}


/*************************************************************
*
* Laufwerk synchronisieren
*
* Der Aufrufer trägt in den ParamBlockRec bereits den Eintrag
* ioCompletion ein.
* Der Aufrufer stellt bereits sicher, daß die Routine nur einmal
* pro Mac-Volume aufgerufen wird.
*
* Rückgabe:		0	OK
*			<0	Fehler
*			>0	in Arbeit.
* AK 25.3.98
*
*************************************************************/

int32_t CMacXFS::xfs_sync(uint16_t drv)
{
	DebugInfo("CMacXFS::xfs_sync(drv=%d)", drv);

	if (drv >= NDRVS || !drives[drv].drv_valid)
		return TOS_EDRIVE;
		
	/* NYI */
	return TOS_EINVFN;
}


/*************************************************************
*
* Ein Prozeß ist terminiert.
*
*************************************************************/

void CMacXFS::xfs_pterm (PD *pd)
{
#pragma unused(pd)
}


/*************************************************************
* xfs_drv_open
* ============
*
* "Oeffnet" ein Laufwerk.
* D.h. sieht nach, ob es als Mac-Pfad existiert. Wenn ja,
* wird ein Deskriptor geliefert (d.h. die DirID der "root").
*
* Für Calamus: Für M: wird immer 0 geliefert.
*
*************************************************************/

int32_t CMacXFS::drv_open(uint16_t drv)
{
	struct stat st;
	
	if (drives[drv].host_root == NULL ||
		stat(drives[drv].host_root->name, &st) != 0 || !S_ISDIR(st.st_mode))
	{
		DebugError("CMacXFS::drv_open(): Cannot stat %s", drives[drv].host_root ? drives[drv].host_root->name : "");
		return TOS_EDRVNR;
	}

	return TOS_E_OK;
}

int32_t CMacXFS::xfs_drv_open(uint16_t drv, MXFSDD *dd, uint32_t flg_ask_diskchange)
{
	int32_t	err;

	DebugInfo("CMacXFS::xfs_drv_open(drv=%d, flag=%08lx)", drv, (unsigned long)flg_ask_diskchange);

	if (drv >= NDRVS || !drives[drv].drv_valid)
		return TOS_EDRIVE;

	if (flg_ask_diskchange)		// Diskchange- Status ermitteln
	{
		if (drives[drv].drv_changed)
			return TOS_E_CHNG;
		else
			return TOS_E_OK;
	}

	drives[drv].drv_changed = false;		// Diskchange reset

	err = drv_open(drv);
	if (err)
		return err;

	dd->dirID = MAPVOIDPTO32(drives[drv].host_root);
	dd->vRefNum = 0;

	DebugInfo("CMacXFS::xfs_drv_open => (DD=%08lx, vRefNum=%d)", (unsigned long)dd->dirID, dd->vRefNum);

	return TOS_E_OK;
}


/*************************************************************
*
* "Schließt" ein Laufwerk.
* mode =	0: Laufwerk schliessen oder TOS_EACCDN liefern
*		1: Laufwerk schliessen, immer TOS_E_OK liefern.
*
*************************************************************/

int32_t CMacXFS::xfs_drv_close(uint16_t drv, uint16_t mode)
{
	DebugInfo("CMacXFS::xfs_drv_close(drv=%d, mode=%d)", drv, mode);

	if (drv >= NDRVS)
		return TOS_EDRIVE;
	if (mode == 0 && !drives[drv].drv_valid)    /* ungueltig */
		return TOS_EDRIVE;

	if (mode == 0 && drv == DriveFromLetter('C'))
	{
		// Mac-Boot-Volume darf nicht ungemountet werden.
		return TOS_EACCDN;
	} else
	{
		if (drives[drv].drv_type == MacDrive)
			drives[drv].drv_valid = false;	// macht Alias ungültig
		return TOS_E_OK;
	}
}


/*************************************************************
*
* Rechnet einen Atari-Pfad um in einen Mac-Verzeichnis-Deskriptor.
* Das Laufwerk ist bereits vom MagiX-Kernel bearbeitet worden,
* d.h. der Pfad ist z.B.
*
*    "subdir1\magx_acc.zip".
*
* Die Datei liegt tatsächlich in
*    "/AWS 95/magix/MAGIX_A/subdir1/magx_acc.zip"
*
* Dabei ist
*    "/AWS 95/magix/MAGIX_A"
* Das Mac-Äquivalent zu "A:\"
*
* <reldir> ist bereits die DirID des Mac-Verzeichnisses.
* mode	= 0: <name> ist Datei
*		= 1: <name> ist selbst Verzeichnis
*
*    Ausgabeparameter:
*
*     1. Fall: Es ist ein Fehler aufgetreten
*
*         Gib den Fehlercode als Funktionswert zurueck
*
*     2. Fall: Ein Verzeichnis (DirID) konnte ermittelt werden
*
*         Gib die DirID als Funktionswert zurueck.
*
*         Gib in <restpfad> den Zeiger auf den restlichen Pfad ohne
*            beginnenden '\' bzw. '/'
*
*     3. Fall: Das XFS ist bei der Pfadauswertung auf einen symbolischen
*             Link gestoßen
*
*         Gib als Funktionswert den internen Mag!X- Fehlercode ELINK
*
*         Gib in <restpfad> den Zeiger auf den restlichen Pfad ohne
*            beginnenden '\' bzw. '/'
*
*         Gib in <symlink_dir> dir DirID des Pfades, in dem der symbolische
*                 Link liegt.
*
*         Gib in <symlink> den Zeiger auf den Link selbst. Ein Link beginnt
*                 mit einem Wort (16 Bit) fuer die Laenge des Pfads,
*                 gefolgt vom Pfad selbst.
*
*                 Achtung: Die Länge muß INKLUSIVE abschließendes
*                                Nullbyte und außerdem gerade sein. Der Link
*                                muß auf einer geraden Speicheradresse
*                                liegen.
*
*                 Der Puffer für den Link kann statisch oder auch
*                 fluechtig sein, da der Kernel die Daten sofort
*                 umkopiert, ohne daß zwischendurch ein Kontextwechsel
*                 stattfinden kann.
*
*                 Wird <symlink> == NULL übergeben, wird dem Kernel
*                 signalisiert, daß der Parent des
*                 Wurzelverzeichnisses angewählt wurde. Befindet sich
*                 der Pfad etwa auf U:\A, kann der Kernel auf U:\
*                 zurückgehen.
*
*************************************************************/

int32_t CMacXFS::xfs_path2DD
(
	uint16_t mode,
	uint16_t dev, MXFSDD *rel_dd, char *pathname,
	char **restpfad, MXFSDD *symlink_dd, char **symlink,
	MXFSDD *dd,
	uint16_t *dir_drive
)
{
#pragma unused(symlink_dd)
	int32_t doserr;
	char macpath[MAXPATHNAMELEN];
	char *s, *t, *u;
	unsigned char c;
	XfsFsFile *reldir;
	XfsFsFile *newFsFile;
	XfsCookie fc;
	struct stat st;
	char mac_dirname[MAXPATHNAMELEN];

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_path2DD(drv=%d, DD=%08lx, pathname=\"%s\", mode=%d)", dev, (unsigned long)rel_dd->dirID, pathname, mode);
#endif

	fetchXFSC(&fc, dev, rel_dd);
	
	if (fc.drv == NULL)    /* ungueltig */
		return TOS_EDRIVE;
	if (fc.drv->drv_changed)
		return TOS_E_CHNG;

	reldir = fc.index;

	/* Hier schon einmal das Laufwerk setzen, das wir zurückliefern	*/
	/* Wenn wir einen Alias verfolgen, kann sich das noch ändern	*/
	/* -------------------------------------------------------------	*/

	*dir_drive = cpu_to_be16(dev);

	/* Der relative DOS-Pfad wird umgewandelt in einen relativen	*/
	/* Mac-Pfad. Eintraege "." und ".." werden beruecksichtigt.		*/
	/* ------------------------------------------------------------ */

	cookie2Pathname(fc.drv, reldir, NULL, macpath, true);
	s = macpath + strlen(macpath);
	if (s[-1] != *DIRSEPARATOR)
		*s++ = *DIRSEPARATOR;				// alle Pfade sind Mac-relativ
	t = pathname;
	u = s;
	while (*pathname)
	{
		c = *pathname++;	// 9.6.96
		
		/*
		 * Note: cannot use Atari2HostUtf8Copy here,
		 * because we need to pass back the pathname in *restpfad
		 */
		if (IS_DIR_SEP(c))
		{
			t = pathname;	// letztes Element Atari
			if (IS_DIR_SEP(*pathname))
			{
				// war leeres Element => ignorieren
				continue;
			} else if (t[0] == '.' && pathname == t + 2)
			{
				s -= 2;	// war Element "." => entfernen
				continue;
			} else if (t[0] == '.' && t[1] == '.' && pathname == t + 3)
			{
				// war Element ".."
				s -= 3;	// zunaechst aus Mac-Pfad entfernen
				if (s > macpath + 2 && reldir->parent != NULL)
				{
					// ich kann zurueckgehen
					s--;
					do	
						s--;
					while (*s != *DIRSEPARATOR);	// gehe zurueck
					reldir = reldir->parent;		// parent anwaehlen
					continue;
				} else
				{
					// ich kann nicht zurueckgehen
					// bin schon root
					*restpfad = t;
					*symlink = NULL;
					return ELINK;
				}
			}
			*s = '\0';
			if (fc.drv->drv_flags & M_DRV_DOSNAMES)
				nameto_8_3(u, mac_dirname, 1, false);
			else
				CTextConversion::Atari2HostUtf8Copy(mac_dirname, u, MAXPATHNAMELEN);
			if ((newFsFile = reldir->insert(*this, mac_dirname)) == NULL)
			{
				return TOS_ENSMEM;
			}
			reldir = newFsFile;
			*s++ = *DIRSEPARATOR;
			u = s;						// letztes Element Mac
		} else if (c == ':')
		{
			return TOS_EPTHNF;				// Doppelpunkt nicht erlaubt
		} else
		{
			*s++ = c;
		}
	}
	*s = '\0';

	newFsFile = reldir;
	if (!mode) 			/* Dateinamen noch nicht bearbeiten */
	{
		*u = '\0';
	} else
	{
		if (s[-1] != *DIRSEPARATOR)
		{
			if (fc.drv->drv_flags & M_DRV_DOSNAMES)
				nameto_8_3(u, mac_dirname, 1, false);
			else
				CTextConversion::Atari2HostUtf8Copy(mac_dirname, u, MAXPATHNAMELEN);
			if ((newFsFile = reldir->insert(*this, mac_dirname)) == NULL)
			{
				return TOS_ENSMEM;
			}
			reldir = newFsFile;
			*s++ = *DIRSEPARATOR;
			*s = '\0';
			u = s;						// letztes Element Mac
		}
	}

	/* Jetzt ist der Pfad in einen Mac-Pfad umgewandelt.	*/
	/* Wir versuchen zunaechst, die dirID mit nur		*/
	/* einem Aufruf zu bestimmen						*/
	/* ---------------------------------------------------	*/
	cookie2Pathname(fc.drv, reldir, u, mac_dirname, true);
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_path2DD(%s -> \"%s\")", macpath, mac_dirname);
#endif
	
	if (stat(mac_dirname, &st) != 0)
	{
		doserr = errnoHost2Mint(errno, TOS_EFILNF);
		if (doserr == TOS_EFILNF)
		     doserr = TOS_EPTHNF;
		return doserr;
	}		

	if (mode)                                       /* Verzeichnis */
	{
		if (!S_ISDIR(st.st_mode))
			return TOS_EPTHNF;                         /* kein SubDir */
		*restpfad = pathname + strlen(pathname);     /* kein Restpfad */
	} else
	{
		*restpfad = t;                               /* Restpfad */
	}

	dd->dirID = MAPVOIDPTO32(newFsFile);
	dd->vRefNum = 0;

	return TOS_E_OK;
}


// Attribute von Mac nach DOS konvertieren
// 17.6.96: wertet nun auch hidden-flag aus
unsigned char CMacXFS::mac2DOSAttr(struct stat *st)
{
	unsigned char attr;
	
	attr = 0;
	if ((st->st_mode & S_IWUSR) == 0)
		attr |= F_RDONLY;
	if (S_ISDIR(st->st_mode))
		attr |= F_SUBDIR;
	if (st->st_flags & UF_HIDDEN)
		attr |= F_HIDDEN;
	return attr;
}

/*************************************************************
*
* Wird von xfs_sfirst und xfs_snext verwendet
* Unabhaengig von drv_longnames werden die Dateien
* immer in Gross-Schrift und 8+3 konvertiert.
* MiNT konvertiert je nach Pdomain() in Gross-Schrift oder nicht.
*
* 6.9.95:
* Lange Dateinamen werden nicht gefunden.
*
* 19.11.95:
* Aliase werden dereferenziert.
*
*************************************************************/

int32_t CMacXFS::_snext(MAC_DTA *dta)
{
	char atariname[MAXPATHNAMELEN];			// C-String Atari-Dateiname (lang)
	char dosname[14];
	struct dirent *dirEntry;
	DIR *dir;
	char fpathName[MAXPATHNAMELEN];
	_MAC_DTA *macdta;
	struct stat st;

	if (dta->macdta.magic != DTA_MAGIC)
		return TOS_EINTRN;

	macdta = dta->macdta.macdta;
	dir = macdta->hostDir;

	DebugInfo("CMacXFS::%s() -- pattern=%.11s", __FUNCTION__, macdta->sname);

	/* suchen */
	/* ------ */

	for (;;)
	{
		/* Verzeichniseintrag lesen. */
		if ((dirEntry = readdir(dir)) == NULL)
		{
			/* Ende des Verzeichnisses erreicht */
			/* --------------------------------------------	*/
			macdta->sname[0] = '\0';                  /* DTA ungueltig machen */
			return TOS_EFILNF;
		}

		/*
		 * Skip "." & ".." when at root dir;
		 * they are typically not present in Atari filesystems
		 */
		if (!macdta->fc.index->parent)
		{
			if (dirEntry->d_name[0] == '.' &&
				(dirEntry->d_name[1] == '\0' ||
				  (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0')))
				continue;
		}

		macdta->index++;

		/* Datei gefunden, passen Name und Attribut ? */

		CTextConversion::Host2AtariUtf8Copy(atariname, dirEntry->d_name, sizeof(atariname));
		DebugInfo("CMacXFS::%s() -- directory entry found: \"%s\"", __FUNCTION__, atariname);

		if (conv_path_elem(atariname, dosname))		// Konvertier. fuer Vergleich
		{
			if (strcmp(dirEntry->d_name), "..") != 0)
			{
				DebugInfo("CMacXFS::%s() -- skip long filename \"%s\"", __FUNCTION__, dirEntry->d_name);
				continue;			/* Dateiname zu lang */
			}
		}

		
		/* Schon hier muessen wir eventuelle Aliase	*/
		/* dereferenzieren, damit der Attributvergleich	*/
		/* moeglich ist (Attr aus Datei, nicht Alias!)	*/
		/* -----------------------------------------	*/
		if (dirEntry->d_type == DT_LNK)
		{
			DebugInfo("CMacXFS::%s() -- dereference Alias %s", __FUNCTION__, atariname);

			/* Alias dereferenzieren	*/
			/* ----------------------------------------	*/
			cookie2Pathname(&macdta->fc, NULL, fpathName, true);
			strcat(fpathName, DIRSEPARATOR);
			strcat(fpathName, dirEntry->d_name);
			if (stat(fpathName, &st) != 0)
				continue;
			dosname[11] = mac2DOSAttr(&st);
		} else
		{
			cookie2Pathname(&macdta->fc, NULL, fpathName, true);
			strcat(fpathName, DIRSEPARATOR);
			strcat(fpathName, dirEntry->d_name);
			if (stat(fpathName, &st) != 0)
			{
				DebugInfo("stats %s failed", fpathName);
				continue;
			}
			dosname[11] = mac2DOSAttr(&st);
		}
		if (filename_match(macdta->sname, dosname))
			break;
	}

	/* erfolgreich: DTA initialisieren. Daten liegen nur in der Data fork */
	/* ------------------------------------------------------------------ */

doit:
	dta->mxdta.dta_attribute = dosname[11];
	dta->mxdta.dta_len = cpu_to_be32((uint32_t) ((dosname[11] & F_SUBDIR) ? 0 : st.st_size));
	date_mac2dos(st.st_mtime, &dta->mxdta.dta_time, &dta->mxdta.dta_date);

	dta->mxdta.dta_time = cpu_to_be16(dta->mxdta.dta_time);
	dta->mxdta.dta_date = cpu_to_be16(dta->mxdta.dta_date);

	nameto_8_3(atariname, dta->mxdta.dta_name, 1, true);
	DebugInfo("CMacXFS::%s() -- return: \"%s\"", __FUNCTION__, dta->mxdta.dta_name);
	return TOS_E_OK;
}


/*************************************************************
*
* Durchsucht ein Verzeichnis und merkt den Suchstatus
* für Fsnext.
* Der Mac benutzt für F_SUBDIR und F_RDONLY dieselben Bits
* wie der Atari.
*
*************************************************************/

int32_t CMacXFS::xfs_sfirst(XfsCookie *fc, char *name,
                    MAC_DTA *dta, uint16_t attrib)
{
	char fpathName[MAXPATHNAMELEN];
	DIR *dir;
	int32_t err;

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	dta->macdta.magic = 0;
	/* Unschoenheit im Kernel ausbügeln: */
	dta->mxdta.dta_drive = fc->dev;
	dta->mxdta.dta_name[0] = 0;		//  gefundenen Namen erstmal löschen
	cookie2Pathname(fc, NULL, fpathName, true);
	DebugInfo("CMacXFS::xfs_sfirst(%s, attrib=0x%04x)", fpathName, attrib);
	dir = host_opendir(fpathName);
	if (dir == NULL)
		return errnoHost2Mint(errno, TOS_EPTHNF);
	dta->macdta.magic = DTA_MAGIC;
	dta->macdta.macdta = new _MAC_DTA;
	dta->macdta.macdta->hostDir = dir;
	dta->macdta.macdta->sattr = (char) attrib;			// Suchattribut
	conv_path_elem(name, dta->macdta.macdta->sname);	// Suchmuster -> DTA
	dta->macdta.macdta->fc = *fc;
	err = _snext(dta);
	/*
	 * when not looking for a pattern, we are done with this directory
	 */
	if (strchr(dta->macdta.macdta->sname, '?') == NULL)
	{
		closedir(dta->macdta.macdta->hostDir);
		delete dta->macdta.macdta;
		dta->macdta.macdta = NULL;
		dta->macdta.magic = 0;
	}
	return err;
}


/*************************************************************
*
* Durchsucht ein Verzeichnis weiter und merkt den Suchstatus
* fuer Fsnext.
*
*************************************************************/

int32_t CMacXFS::xfs_snext(uint16_t dev, MAC_DTA *dta)
{
	int32_t err;

	if (dta->macdta.magic != DTA_MAGIC)
		return TOS_EINTRN;
	if (dev != dta->macdta.macdta->fc.dev)
		return TOS_EDRIVE;
	if (dta->macdta.macdta->fc.drv->drv_changed)
		return TOS_E_CHNG;

	if (!dta->macdta.macdta->sname[0])
		err = TOS_ENMFIL;
	else
		err = _snext(dta);
	if (err == TOS_EFILNF)
		err = TOS_ENMFIL;
	if (err == TOS_ENMFIL)
	{
		closedir(dta->macdta.macdta->hostDir);
		delete dta->macdta.macdta;
		dta->macdta.macdta = NULL;
		dta->macdta.magic = 0;
	}
	return err;
}


/*************************************************************
*
* Öffnet eine Datei.
* Liefert Fehlercode oder RefNum (eine Art Handle).
*
* fd.mod_time_dirty wird dabei automatisch vom Kernel
* auf FALSE gesetzt.
*
* Aliase werden dereferenziert.
*
* Attention: There is a design flaw in the API. The vRefNum
* shall be stored in the host's native endian mode, but
* it will be automatically converted, because it is the
* function return value in D0. As a workaround the
* return value is here converted to big endian, so it
* will be re-converted to little-endian on a little-endian-CPU.
*
*************************************************************/

int32_t CMacXFS::xfs_fopen(XfsCookie *fc, const char *name, uint16_t omode, uint16_t attrib)
{
	int host_fd;
	char fpathName[MAXPATHNAMELEN];

#if DEBUG_68K_EMU
	if (!strcmp(name, ATARI_PRG_TO_TRACE))
		trigger = 1;
#endif
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_fopen('%s', drv=%d, omode=%d, DD=%08lx)", name, fc->dev, omode, (unsigned long)MAPVOIDPTO32(fc->index));
#endif

#ifdef DEMO
	if (omode & _ATARI_O_CREAT)
	{
		(void) MyAlert(ALRT_DEMO, 2);
		return TOS_EWRPRO;
	}
#endif

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	if (fname_is_invalid(name))
		return TOS_EACCDN;

	int flags = flagsMagic2Host(omode);
	if ((fc->drv->drv_flags & M_DRV_READONLY) && (flags & O_CREAT))
		return TOS_EWRPRO;

	cookie2Pathname(fc, name, fpathName, true);
	
#ifdef NOTYET
	if (!fc->index->created && (flags & O_CREAT))
	{
		// if xfs_creat()'ed file is now opened with O_CREAT
		// then set the special index->created to true
		fc->index->created = true;

		// clear the O_EXCL and O_CREAT flags
		flags &= ~(O_CREAT|O_EXCL);
	}
#endif

	host_fd = open(fpathName, flags|O_BINARY, 0644);
	if (host_fd < 0)
		return errnoHost2Mint(errno, TOS_EFILNF);
	/*
	 * The host_fd member in MAC_FD is signed 16 bit,
	 * make sure we can store the fd there
	 */
	if (host_fd > 0x7fff)
	{
		close(host_fd);
		return TOS_ENHNDL;
	}
#if DEBUG_68K_EMU
	if (trigger == 1)
	{
		trigger_refnum = host_fd;
		trigger++;
	}
#endif

	/* Datei erstellen, wenn noetig */
	/* ---------------------------- */

	if (omode & _ATARI_O_CREAT)
	{
#if 0
		/*
		 * don't do that. It will create a resource fork in the file,
		 * and Atari-Programs can't handle that
		 */
		OSType creator, type;
		creator = MyCreator;
		type = 'TEXT';
		GetTypeAndCreator (fpathName, &type, &creator);
		err = FSpCreate(&fs, creator, type, smSystemScript);
#endif

		/* Dateiattribute aendern, wenn noetig */
		/* ----------------------------------- */

		if (attrib & F_HIDDEN)
		{
			struct stat st;
			
			if (fstat(host_fd, &st) != 0 || fchflags(host_fd, st.st_flags | UF_HIDDEN) != 0)
			{
				close(host_fd);
				return errnoHost2Mint(errno, TOS_EACCDN);
			}
		}
	}

	/*
	 * has to be swapped, because it is written on the Atari-side to the FD
	 */
	return cpu_to_be16(host_fd);
}


/*************************************************************
*
* Löscht eine Datei.
*
* Aliase werden NICHT dereferenziert, d.h. es wird der Alias
* selbst gelöscht.
*
*************************************************************/

int32_t CMacXFS::xfs_fdelete(XfsCookie *dir, const char *name)
{
#ifdef DEMO
	#pragma unused(dir, name)
	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;
#else
	char fpathName[MAXPATHNAMELEN];

	if (dir->drv == NULL)
		return TOS_EDRIVE;
	if (dir->drv->drv_changed)
		return TOS_E_CHNG;
	if (dir->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;

	cookie2Pathname(dir, name, fpathName, true); // get the cookie filename

//	DebugInfo("CMacXFS::xfs_fdelete(name=%s)", fpathName);

	if (unlink(fpathName))
		return errnoHost2Mint(errno, TOS_EFILNF);

	return TOS_E_OK;
#endif
}


/*************************************************************
*
* Datei umbenennen und verschieben bzw. Hard Link erstellen.
*
*	mode == 1		Hardlink
*		== 0		Umbenennen ("move")
*
* Aliase werden NICHT dereferenziert, d.h. es wird der Alias
* selbst umbenannt.
*
* ACHTUNG: Damit <dst_drv> gültig ist, muß ein neuer MagiC-
* Kernel verwendet werden, die alten übergeben diesen
* Parameter nicht.
*
*************************************************************/

int32_t CMacXFS::xfs_link(XfsCookie *fromDir, char *fromname, XfsCookie *toDir, char *toname, uint16_t mode)
{
#ifdef DEMO
	#pragma unused(fromDir, fromname, toDir, toname, mode)
	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;
#else

	if (fromDir->drv == NULL || toDir->drv == NULL)
		return TOS_EDRIVE;
	if (fromDir->drv->drv_changed || toDir->drv->drv_changed)
		return TOS_E_CHNG;
	// auf demselben Volume?
	if (fromDir->dev != toDir->dev)
		return TOS_ENSAME;
	if (fname_is_invalid(toname))
		return TOS_EACCDN;
	if ((fromDir->drv->drv_flags & M_DRV_READONLY) || (toDir->drv->drv_flags & M_DRV_READONLY))
		return TOS_EWRPRO;

	char ffromName[MAXPATHNAMELEN];
	cookie2Pathname(fromDir, fromname, ffromName, true);

	char ftoName[MAXPATHNAMELEN];
	cookie2Pathname(toDir, toname, ftoName, true);

	if (mode)
	{
		if (link(ffromName, ftoName) != 0)
			return errnoHost2Mint(errno, TOS_EFILNF);
	} else
	{
		if (rename(ffromName, ftoName) != 0)
			return errnoHost2Mint(errno, TOS_EFILNF);
	}	
	return TOS_E_OK;
#endif
}


/*************************************************************
*
* Wandelt struct stat => XATTR
*
* (Fuer Fxattr und Dxreaddir)
*
*************************************************************/

void CMacXFS::convert_to_xattr(struct stat *st, XATTR *xattr)
{
	uint64_t blksize, blocks;

	xattr->attr = cpu_to_be16(mac2DOSAttr(st));
	xattr->mode = cpu_to_be16(modeHost2Mint(st->st_mode));

	xattr->index = cpu_to_be32((uint32_t) st->st_ino);
	xattr->dev = cpu_to_be16((uint16_t) st->st_dev);
	xattr->reserved1 = 0;
	xattr->nlink = cpu_to_be16(st->st_nlink);
	xattr->uid = cpu_to_be16(st->st_uid);
	xattr->gid = cpu_to_be16(st->st_gid);
	// F_SUBDIR-Abfrage ist unbedingt nötig!
	if (be16_to_cpu(xattr->attr) & F_SUBDIR)
		xattr->size = 0;
	else if ((uint64_t)st->st_size >= 0x100000000ULL)
		xattr->size = cpu_to_be32(0xFFFFFFFFUL); /* too large to be expressed in 32 bits */
	else
		xattr->size = cpu_to_be32((uint32_t)st->st_size);
	blksize = st->st_blksize;
	xattr->blksize = cpu_to_be32(blksize);
	/*
	 * st->st_blocks is always in number of 512 bytes,
	 * regardless of blksize
	 */
	blocks = (st->st_blocks * 512 + blksize - 1) / blksize;
	if (blocks >= 0x100000000ULL)
		blocks = 0xFFFFFFFFUL;
	xattr->nblocks = cpu_to_be32(blocks);	// Phys. Länge / Blockgröße
	date_mac2dos(st->st_mtime, &xattr->mtime, &xattr->mdate);
	xattr->mtime = cpu_to_be16(xattr->mtime);
	xattr->mdate = cpu_to_be16(xattr->mdate);
	date_mac2dos(st->st_atime, &xattr->atime, &xattr->adate);
	xattr->atime = cpu_to_be16(xattr->atime);
	xattr->adate = cpu_to_be16(xattr->adate);
	date_mac2dos(st->st_ctime, &xattr->ctime, &xattr->cdate);
	xattr->ctime = cpu_to_be16(xattr->ctime);
	xattr->cdate = cpu_to_be16(xattr->cdate);
	xattr->reserved2 = 0;
	xattr->reserved3[0] = xattr->reserved3[1] = 0;
}

#if (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#  define st_atimensec st_atimespec.tv_nsec
#  define st_mtimensec st_mtimespec.tv_nsec
#  define st_ctimensec st_ctimespec.tv_nsec
#endif

void CMacXFS::convert_to_stat64(struct stat *st, MINT_STAT64 *statp)
{
	statp->_st_dev = cpu_to_be64(st->st_dev); // FIXME: this is Linux's one
	statp->_st_ino = cpu_to_be32(st->st_ino); // FIXME: this is Linux's one
	statp->_st_mode = cpu_to_be32(modeHost2Mint(st->st_mode));
	statp->_st_nlink = cpu_to_be32(st->st_nlink);
	statp->_st_uid = cpu_to_be32(st->st_uid); // FIXME: this is Linux's one
	statp->_st_gid = cpu_to_be32(st->st_gid); // FIXME: this is Linux's one
	statp->_st_rdev = cpu_to_be64(st->st_rdev); // FIXME: this is Linux's one

	statp->_st_atime = cpu_to_be64(st->st_atime);
	statp->_st_atime_ns = cpu_to_be32(st->st_atimensec);
	statp->_st_mtime = cpu_to_be64(st->st_mtime);
	statp->_st_mtime_ns = cpu_to_be32(st->st_mtimensec);
	statp->_st_ctime = cpu_to_be64(st->st_ctime);
	statp->_st_ctime_ns = cpu_to_be32(st->st_ctimensec);

	statp->_st_size = cpu_to_be64(st->st_size);
	uint64_t blksize, blocks;
	blksize = st->st_blksize;
    if (blksize <= 512)
		blksize = 512;
	blocks = st->st_blocks;
	statp->_st_blocks = cpu_to_be64(blocks);
	statp->_st_blksize = cpu_to_be32(blksize);
	statp->_st_flags = cpu_to_be32(0);
	statp->_st_gen = cpu_to_be32(0);
	statp->_st_reserved[0] = cpu_to_be32(0);
	statp->_st_reserved[1] = cpu_to_be32(0);
	statp->_st_reserved[2] = cpu_to_be32(0);
	statp->_st_reserved[3] = cpu_to_be32(0);
	statp->_st_reserved[4] = cpu_to_be32(0);
	statp->_st_reserved[5] = cpu_to_be32(0);
	statp->_st_reserved[6] = cpu_to_be32(0);
}


/*************************************************************
*
* Fuer Fxattr
*
* MODE == 0: Folge Symlinks
*
*************************************************************/

int32_t CMacXFS::xfs_xattr(XfsCookie *fc, const char *name, XATTR *xattr, uint16_t mode)
{
	struct stat st;
	char fname[MAXPATHNAMELEN];
	int err;

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_xattr(drv=%d, '%s', mode=0x%04x, DD=%08lx)", fc->dev, name, mode, (unsigned long)MAPVOIDPTO32(fc->index));
#endif

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	if (!(fc->drv->drv_flags & M_DRV_DOSNAMES))
	{
		if (name[0] == '.' && !name[1])
			name = "";		// "." wie leerer Name
	}
	cookie2Pathname(fc, name, fname, true);

	/* Im Modus 0 muessen Aliase dereferenziert werden	*/
	/* ------------------------------------------------	*/
	if (mode == 0)
		err = stat(fname, &st);
	else
		err = lstat(fname, &st);
	if (err != 0)
		return errnoHost2Mint(errno, TOS_EFILNF);

	convert_to_xattr(&st, xattr);
	return TOS_E_OK;
}


int32_t CMacXFS::xfs_stat64(XfsCookie *fc, const char *name, MINT_STAT64 *statp)
{
	struct stat st;
	char fname[MAXPATHNAMELEN];

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	if (!(fc->drv->drv_flags & M_DRV_DOSNAMES))
	{
		if (name[0] == '.' && !name[1])
			name = "";		// "." wie leerer Name
	}
	cookie2Pathname(fc, name, fname, true);

	if (lstat(fname, &st) != 0)
		return errnoHost2Mint(errno, TOS_EFILNF);

	convert_to_stat64(&st, statp);
	return TOS_E_OK;
}

/*************************************************************
*
* Fuer Fattrib
*
* Wertet zur Zeit nur die DOS-Attribute
*	F_RDONLY
*	F_HIDDEN
*	F_ARCHIVE
* aus.
*
* Aliase werden dereferenziert.
*
*************************************************************/

int32_t CMacXFS::xfs_attrib(XfsCookie *fc, const char *name, uint16_t rwflag, uint16_t attr)
{
	struct stat st;
	int oldattr;
	char fpathName[MAXPATHNAMELEN];

	DebugInfo("CMacXFS::xfs_attrib('%s', drv=%d, wrmode=%d, attr=0x%04x)", name, fc->dev, rwflag, attr);
	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

#ifdef DEMO
	if (rwflag)
	{
		(void) MyAlert(ALRT_DEMO, 2);
		return TOS_EWRPRO;
	}
#endif
	if (rwflag && (fc->drv->drv_flags & M_DRV_READONLY))
		return TOS_EWRPRO;

	cookie2Pathname(fc, name, fpathName, true);
	
	/*
	 * Fattrib() is only supposed to change flags of
	 * symlink targets, nit th elink itself
	 */
	if (stat(fpathName, &st) != 0)
		return errnoHost2Mint(errno, TOS_EACCDN);
	
	/* Normale Datei, oder Alias ist dereferenziert	*/
	/* ------------------------------------------	*/

	oldattr = mac2DOSAttr(&st);
	if (rwflag)
	{
		if (oldattr & F_SUBDIR)
		{			// !AK 4.10.98
			/* Bei Verzeichnissen ist nur HIDDEN änderbar */
			if ((attr & ~F_HIDDEN) != (oldattr & ~F_HIDDEN))
				return TOS_EACCDN;
		}

		if ((oldattr & F_RDONLY) != (attr & F_RDONLY))
		{
			if (attr & F_RDONLY)
				st.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
			else
				st.st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
			if (chmod(fpathName, st.st_mode) != 0)
				return errnoHost2Mint(errno, TOS_EACCDN);
		}
		
		// Archiv-Bit:
		if ((oldattr & F_ARCHIVE) != (attr & F_ARCHIVE))
		{
			if (attr & F_ARCHIVE)
				st.st_flags &= ~SF_ARCHIVED;
			else
				st.st_flags |= SF_ARCHIVED;
			if (chflags(fpathName, st.st_flags) != 0)
				return errnoHost2Mint(errno, TOS_EACCDN);
		}
		
		// !AK 6.2.99: Hidden-Bit auch für Ordner
		if ((oldattr & F_HIDDEN) != (attr & F_HIDDEN))
		{
			if (attr & F_HIDDEN)
				st.st_flags |= UF_HIDDEN;
			else
				st.st_flags &= ~UF_HIDDEN;
			if (chflags(fpathName, st.st_flags) != 0)
				return errnoHost2Mint(errno, TOS_EACCDN);
		}
	}

	return oldattr;
}


/*************************************************************
*
* Fuer Fchown
*
*************************************************************/

int32_t CMacXFS::xfs_fchown(XfsCookie *fc, const char *name, uint16_t uid, uint16_t gid)
{
#pragma unused(name, uid, gid)
	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fc->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;
	return TOS_EINVFN;
}


/*************************************************************
*
* Fuer Fchmod
*
*************************************************************/

int32_t CMacXFS::xfs_fchmod(XfsCookie *fc, const char *name, uint16_t fmode)
{
#pragma unused(name, fmode)
	char fpathName[MAXPATHNAMELEN];

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fc->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;
	cookie2Pathname(fc, name, fpathName, true);

    if (chmod(fpathName, modeMint2Host(fmode)))
		return errnoHost2Mint(errno, TOS_EACCDN);
	return TOS_EINVFN;
}


/*************************************************************
*
* Fuer Dcreate
*
*************************************************************/

int32_t CMacXFS::xfs_dcreate(XfsCookie *fc, const char *name)
{
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_dcreate('%s', drv=%d)", name, fc->dev);
#endif
	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fname_is_invalid(name))
		return TOS_EACCDN;

#ifdef DEMO

	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;

#else

	char fpathName[MAXPATHNAMELEN];

	if (fc->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;

	cookie2Pathname(fc, name, fpathName, true);
	if (mkdir(fpathName, 0755) != 0)
		errnoHost2Mint(errno, TOS_EFILNF);
	return TOS_E_OK;

#endif
}


/*************************************************************
*
* Fuer Ddelete
*
* Aliase werden NICHT dereferenziert, d.h. es wird der Alias
* selbst geloescht.
*
*************************************************************/

int32_t CMacXFS::xfs_ddelete(XfsCookie *fc)
{
	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
#ifdef DEMO

	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;
	
#else

	char fpathName[MAXPATHNAMELEN];

	if (fc->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;
	cookie2Pathname(fc, NULL, fpathName, true);
	if (rmdir(fpathName) != 0)
		return errnoHost2Mint(errno, TOS_EFILNF);
	return TOS_E_OK;
#endif
}


/*************************************************************
*
* Fuer Dgetpath
*
*************************************************************/

int32_t CMacXFS::xfs_DD2name(XfsCookie *fc, char *buf, uint16_t bufsiz)
{
    char fpathName[MAXPATHNAMELEN];
    char pathName[MAXPATHNAMELEN];
	uint16_t len;

	DebugInfo("CMacXFS::xfs_DD2name(drv=%d, DD=%08lx)", fc->dev, (unsigned long)MAPVOIDPTO32(fc->index));
	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

    cookie2Pathname(fc, NULL, fpathName, false); // get the cookie filename

	CTextConversion::Host2AtariUtf8Copy(pathName + 1, fpathName, sizeof(pathName) - 1);
	pathName[0] = '\\';
	len = strlen(pathName) + 1;
	if (len > bufsiz)
		return TOS_ERANGE;
	stru2dpath(pathName);
	memcpy(buf, pathName, len);
	
	return TOS_E_OK;
}


/*************************************************************
*
* Fuer Dopendir.
*
* Aliase brauchen hier nicht dereferenziert zu werden, weil
* dies bereits bei path2DD haette passieren muessen.
*
*************************************************************/

int32_t CMacXFS::xfs_dopendir(MAC_DIRHANDLE *dirh, XfsCookie *fc, uint16_t tosflag)
{
	char fpathName[MAXPATHNAMELEN];

	DebugInfo("CMacXFS::%s(drv=%d, DD=%08lx, tosflag=%u)", __FUNCTION__, fc->dev, (unsigned long)MAPVOIDPTO32(fc->index), tosflag);
	if (fc->drv == NULL)
		return TOS_EDRIVE;

	dirh->fc = *fc;
	dirh->tosflag = tosflag;
	dirh->index = 0;
	cookie2Pathname(&dirh->fc, NULL, fpathName, true);
	DebugInfo("CMacXFS::%s(%s)", __FUNCTION__, fpathName);

	dirh->hostDir = host_opendir(fpathName);
	if (dirh->hostDir == NULL)
		return errnoHost2Mint(errno, TOS_EPTHNF);

	return TOS_E_OK;
}


/*************************************************************
*
* Fuer D(x)readdir.
* dirh		Verzeichnis-Deskriptor
* size		Groesse des Puffers fuer Namen + ggf. Inode
* buf		Puffer fuer Namen + ggf. Inode
* xattr	Ist != NULL, wenn Dxreaddir ausgefuehrt wird.
* xr		Fehlercode von Fxattr.
*
* 6.9.95:
* Ist das TOS-Flag im <dirh> gesetzt, werden lange Dateinamen
* nicht gefunden.
*
*************************************************************/

int32_t CMacXFS::xfs_dreaddir(MAC_DIRHANDLE *dirh, uint16_t drv,
		uint16_t size, char *buf, XATTR *xattr, int32_t *xr)
{
	struct dirent *dirEntry;
	struct stat st;
	char atariname[MAXPATHNAMELEN];			// C-String Atari-Dateiname (lang)
	uint16_t len;

	if (dirh->hostDir == NULL)
		return TOS_EIHNDL;

	for (;;)
	{
		if ((dirEntry = readdir(dirh->hostDir)) == NULL)
			return TOS_ENMFIL;
		if (dirEntry->d_name[0] == '.' &&
			(dirEntry->d_name[1] == '\0' ||
			  (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0')))
		{
			/*
			 * Skip "." & ".." when at root dir;
			 * they are typically not present in Atari filesystems
			 */
			if (!dirh->fc.index->parent)
				continue;
			/*
			 * Otherwise, return them as is, and don't go through 8+3 conversion
			 */
			if (dirh->tosflag)
			{
				if (size < 3)
					return TOS_ERANGE;
				strcpy(buf, dirEntry->d_name);
			} else
			{
				if (size < 3 + 4)
					return TOS_ERANGE;
				strcpy(buf + 4, dirEntry->d_name);
				(*(uint32_t *)buf) = cpu_to_be32(dirEntry->d_ino);
			}
			break;
		}

		// OS X verwendet die Dateien .DS_Store, um irgendwelche Attribute
		// zu verwalten. Die Dateien sollten ausgeblendet werden.
		if (strcmp(dirEntry->d_name, ".DS_Store") == 0)
			continue;
	
		if (dirh->tosflag)
		{
			if (size < 13)
				return TOS_ERANGE;
			CTextConversion::Host2AtariUtf8Copy(atariname, dirEntry->d_name, sizeof(atariname));
			if (nameto_8_3(atariname, buf, 1, true))
				continue;		// musste Dateinamen kuerzen
		} else
		{
			CTextConversion::Host2AtariUtf8Copy(atariname, dirEntry->d_name, sizeof(atariname));
			/*
			 * must truncate directory names, or path lookup will fail
			 */
			if (dirEntry->d_type == DT_DIR && (dirh->fc.drv->drv_flags & M_DRV_DOSNAMES))
			{
				if (size < 13 + 4)
					return TOS_ERANGE;
				nameto_8_3(atariname, buf + 4, 0, true);
			} else
			{
				len = strlen(atariname);
				if (size < len + 5)
					return TOS_ERANGE;
				strcpy(buf + 4, atariname);
			}
			(*(uint32_t *)buf) = cpu_to_be32(dirEntry->d_ino);
		}

		break;
	}

	DebugInfo("CMacXFS::%s() -- return: %s \"%s\" \"%s\"", __FUNCTION__, dirEntry->d_type == DT_DIR ? "DIR" : "FIL", dirEntry->d_name, dirh->tosflag ? buf : buf + 4);

	if (xattr)
	{
		if (fstatat(dirfd(dirh->hostDir), dirEntry->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
		{
			if (xr)
				*xr = cpu_to_be32(errnoHost2Mint(errno, TOS_EACCDN));
		} else
		{
			convert_to_xattr(&st, xattr);
			if (xr)
				*xr = cpu_to_be32(TOS_E_OK);
		}
	}

	return TOS_E_OK;
}


/*************************************************************
*
* Fuer Drewinddir
*
*************************************************************/

int32_t CMacXFS::xfs_drewinddir(MAC_DIRHANDLE *dirh, uint16_t drv)
{
#pragma unused(drv)
	if (dirh->hostDir == NULL)
		return TOS_EIHNDL;
	rewinddir(dirh->hostDir);
	dirh->index = 0;
	return TOS_E_OK;
}


/*************************************************************
*
* Fuer Dclosedir
*
*************************************************************/

int32_t CMacXFS::xfs_dclosedir(MAC_DIRHANDLE *dirh, uint16_t drv)
{
#pragma unused(drv)
	DebugInfo("CMacXFS::%s(drv=%d, DD=%08lx)", __FUNCTION__, dirh->fc.dev, (unsigned long)MAPVOIDPTO32(dirh->fc.index));
	if (dirh->hostDir == NULL)
		return TOS_EIHNDL;
	if (closedir(dirh->hostDir))
		return errnoHost2Mint(errno, TOS_EPTHNF);
	dirh->hostDir = NULL;
	return TOS_E_OK;
}


/*************************************************************
*
* Fuer Dpathconf
*
* mode = -1:   max. legal value for n in Dpathconf(n)
*         0:   internal limit on the number of open files
*         1:   max. number of links to a file
*         2:   max. length of a full path name
*         3:   max. length of an individual file name
*         4:   number of bytes that can be written atomically
*         5:   information about file name truncation
*              0 = File names are never truncated; if the file name in
*                  any system call affecting  this  directory  exceeds
*                  the  maximum  length (returned by mode 3), then the
*                  error value TOS_ERANGE is  returned  from  that  system
*                  call.
*
*              1 = File names are automatically truncated to the maxi-
*                  mum length.
*
*              2 = File names are truncated according  to  DOS  rules,
*                  i.e. to a maximum 8 character base name and a maxi-
*                  mum 3 character extension.
*         6:   0 = case-sensitiv
*              1 = nicht case-sensitiv, immer in Gross-Schrift
*              2 = nicht case-sensitiv, aber unbeeinflusst
*         7:   Information ueber unterstuetzte Attribute und Modi
*         8:   information ueber gueltige Felder in XATTR
*
*      If any  of these items are unlimited, then 0x7fffffffL is
*      returned.
*
* Aliase brauchen hier nicht dereferenziert zu werden, weil
* dies bereits bei path2DD haette passieren muessen.
*
*************************************************************/

int32_t CMacXFS::xfs_dpathconf(uint16_t drv, MXFSDD *dd, uint16_t which)
{
#pragma unused(dd)
	switch(which)
	{
	case DP_MAXREQ:
		return DP_XATTRFIELDS;
	case DP_IOPEN:
		return 0x7fff;	// ???
	case DP_MAXLINKS:
		return LINK_MAX;
	case DP_PATHMAX:
		return MAXPATHNAMELEN;
	case DP_NAMEMAX:
		return drv < NDRVS && drives[drv].drv_flags & M_DRV_DOSNAMES ? 12 : NAME_MAX;
	case DP_ATOMIC:
		return 512;	// ???
	case DP_TRUNC:
		return drv < NDRVS && drives[drv].drv_flags & M_DRV_DOSNAMES ? DP_DOSTRUNC : DP_AUTOTRUNC;
	case DP_CASE:
		return drv < NDRVS && drives[drv].drv_flags & M_DRV_DOSNAMES ? DP_CASECONV : DP_CASEINSENS;
	case DP_MODEATTR:
		return F_RDONLY | F_SUBDIR | F_ARCHIVE | F_HIDDEN | DP_FT_DIR | DP_FT_REG | DP_FT_LNK | DP_FT_BLK | DP_FT_CHR | DP_FT_SOCK | DP_FT_FIFO | DP_MODEBITS;
	case DP_XATTRFIELDS:
		return DP_INDEX | DP_DEV | DP_NLINK | DP_UID | DP_GID | DP_BLKSIZE | DP_SIZE | DP_NBLOCKS | DP_ATIME | DP_CTIME | DP_MTIME;
	case DP_VOLNAMEMAX:
		return NAME_MAX;
	}
	return TOS_EINVFN;
}


/*************************************************************
*
* Fuer Dfree
*
* data = free,total,secsiz,clsiz
*
* Aliase brauchen hier nicht dereferenziert zu werden, weil
* dies bereits bei path2DD haette passieren muessen.
*
*************************************************************/

int32_t CMacXFS::host_statvfs(const char *fpathName, void *buff)
{
	DebugInfo("CMacXFS::%s (%s)", __FUNCTION__, fpathName);

#ifdef HAVE_SYS_STATVFS_H
	if (statvfs(fpathName, (STATVFS *)buff))
#else
	if (statfs(fpathName, (STATVFS *)buff))
#endif
		return errnoHost2Mint(errno, TOS_EFILNF);

	return TOS_E_OK;
}


int32_t CMacXFS::xfs_dfree(XfsCookie *fc, uint32_t data[4])
{
	char fpathName[MAXPATHNAMELEN];

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s(drv=%d, DD=0x%08lx)", __FUNCTION__, fc->dev, (unsigned long)MAPVOIDPTO32(fc->index));
#endif

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fc->drv->drv_type == MacRoot)
	{
		// 2G frei, d.h. 4 M Blöcke à 512 Bytes
		data[0] = cpu_to_be32((2 * 1024) * 2 * 1024);	// # freie Blöcke
		data[1] = cpu_to_be32((2 * 1024) * 2 * 1024);	// # alle Blöcke
		data[2] = cpu_to_be32(512);					// Sektorgröße in Bytes
		data[3] = cpu_to_be32(1);						// Sektoren pro Cluster
		return TOS_E_OK;
	}
	cookie2Pathname(fc->drv, fc->drv->host_root, NULL, fpathName, true);

	STATVFS buff;
	int32_t res = host_statvfs(fpathName, &buff);
	if (res != TOS_E_OK)
		return res;

#if 0
	fprintf(stderr, "bfree:   %llx\n", buff.f_bavail);
	fprintf(stderr, "blocks:  %llx\n", buff.f_blocks);
	fprintf(stderr, "blksize: %x\n", buff.f_bsize);
#endif
	data[0] = cpu_to_be32(buff.f_bavail); /* b_free */
	data[1] = cpu_to_be32(buff.f_blocks); /* b_total */
	data[2] = cpu_to_be32(buff.f_bsize); /* b_secsize */
	data[3] = cpu_to_be32(1); /* b_clssize */
	return TOS_E_OK;
}


/*************************************************************
*
* Disklabel.
* Zurueckgeliefert wird der Macintosh-Name.
* Das Aendern des Labels ist z.Zt. nicht moeglich.
*
* Bei allen Operationen, die Dateinamen zurueckliefern, ist
* darauf zu achten, dass die Angabe der Puffergroesse immer
* INKLUSIVE End-of-String ist.
*
*************************************************************/

int32_t CMacXFS::xfs_wlabel(uint16_t drv, MXFSDD *dd, char *name)
{
#pragma unused(drv, dd, name)
	return TOS_EINVFN;
}

int32_t CMacXFS::xfs_rlabel(uint16_t dev, MXFSDD *dd, char *name, uint16_t bufsiz)
{
#pragma unused(dd)
	mount_info *drv;
	size_t hostrootlen;;
	
	if (dev >= NDRVS || !(drv = &drives[dev])->drv_valid)
		return TOS_EDRIVE;
	if (drv->drv_changed)
		return TOS_E_CHNG;
	if (drv->drv_type == MacRoot)
	{
		CFURLRef url;
		CFStringRef prop;
		
		url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, CFStringCreateWithCString(NULL, drv->host_root->name, kCFStringEncodingUTF8), kCFURLPOSIXPathStyle, 1);
		prop = NULL;
		if (CFURLCopyResourcePropertyForKey(url, kCFURLVolumeNameKey, &prop, NULL))
		{
			const char *volumename = CFStringGetCStringPtr(prop, kCFStringEncodingUTF8);
			CTextConversion::Host2AtariUtf8Copy(name, volumename, bufsiz);
			CFRelease(prop);
			CFRelease(url);
			return TOS_E_OK;
		}
		CFRelease(url);
	}

	/*
	 * use the last component of the mounted folder
	 */
	hostrootlen = strlen(drv->host_root->name);
	char *startchar = drv->host_root->name;	//	position on start of name
	char *poschar = &startchar[hostrootlen - 1];	//	position on last character
	if (poschar > startchar && *poschar == *DIRSEPARATOR)
	{
		//	ignore an ending slash
		--poschar;
	}
	if (poschar > startchar && *poschar == ':')
	{
		//	ignore an ending ":" from dos drive letters
		--poschar;
	}
	char *endchar = poschar;	//	remember this position as the end of hostRoot to copy
	//	search backwards for bounding slash
	while (poschar > startchar && *poschar != *DIRSEPARATOR)
		--poschar;
	if (*poschar == *DIRSEPARATOR)
		//	move to character behind that slash
		++poschar;
		
	if (poschar <= endchar)
	{
		// there are some characters inbetween to copy
		hostrootlen = endchar - poschar + 2;
		if (bufsiz == 0 || hostrootlen > bufsiz)
			return TOS_ERANGE;
		CTextConversion::Host2AtariUtf8Copy(name, poschar, hostrootlen);
		return TOS_E_OK;
	}

	if (bufsiz > 0)
	{
		//	there is no label name to extract
		//	fall back to a default label
		CTextConversion::Host2AtariUtf8Copy(name, "MACFS", bufsiz);
		return TOS_E_OK;
	}
	return TOS_ERANGE;
}


/*************************************************************
*
* Fuer Fsymlink
*
* Unter dem Namen <name> wird im Ordner <dirID> ein
* Alias erstellt, der die Datei <to> repraesentiert.
*
*************************************************************/

int32_t CMacXFS::xfs_symlink(XfsCookie *fc, const char *name, const char *toname)
{
#ifdef DEMO

	if (fc->drv == NULL)    /* ungueltig */
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fname_is_invalid(name))
		return TOS_EACCDN;
	
	#pragma unused(dd, to)
	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;

#else

	char ffromName[MAXPATHNAMELEN];
	char ftoName[MAXPATHNAMELEN];
	char ftoname[MAXPATHNAMELEN];

	if (fc->drv == NULL)    /* ungueltig */
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;
	if (fname_is_invalid(name))
		return TOS_EACCDN;
	if (fc->drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;

	cookie2Pathname(fc, name, ffromName, true);
	CTextConversion::Atari2HostUtf8Copy(ftoname, toname, sizeof(ftoname));
	strd2upath(ftoname);
	strcpy(ftoName, ftoname);

	if (ftoName[0] == '\0' || ffromName[0] == '\0')
		return TOS_EFILNF;
	
	if (ftoName[0] == '\0' || ffromName[0] == '\0')
		return TOS_EFILNF;
	
	if (ftoName[0] != '/' && ftoName[1] != ':')
	{
		// relative symlink. Use it as is
	} else
	{
		// search among the mount points to find suitable link...

		size_t nameLen = strlen(ftoname);
		/* convert U:/c/... to c:/... */
		if (nameLen >= 4 && strncasecmp(ftoname, "u:/", 3) == 0 &&
			DriveFromLetter(ftoname[3]) >= 0 &&
			(ftoname[4] == '\0' || ftoname[4] == '/'))
		{
			ftoname[0] = ftoname[3];
			memmove(ftoname + 2, ftoname + 4, nameLen - 3);
			nameLen -= 2;
		} else
		/* convert /c/... to c:/... */
		if (nameLen >= 2 && ftoname[0] == '/' &&
			DriveFromLetter(ftoname[1]) >= 0 &&
			(ftoname[2] == '\0' || ftoname[2] == '/'))
		{
			ftoname[0] = ftoname[1];
			ftoname[1] = ':';
		}
		if (nameLen == 2)
		{
			strcat(ftoname, "/");
			nameLen++;
		}
		
		{
			bool found = false;
			int i;
			for (i = 0; i < NDRVS; i++)
			{
				struct mount_info *drv = &drives[i];
				if (!drv->drv_valid)
					continue;
				if (ftoname[1] == ':' &&
					toupper(ftoname[0]) == DriveToLetter(i) &&
					ftoname[2] == *DIRSEPARATOR)
				{
					// target drive found; replace MiNTs mount point
					// with the hosts root directory
					int len = MAXPATHNAMELEN;
					strcpy(ftoName, drv->host_root->name);
					int hrLen = strlen(drv->host_root->name);
					if (hrLen < len)
						strncpy(ftoName + hrLen, ftoname + 3, len - hrLen);
					found = true;
					break;
				}
			}
			if (!found)
			{
				// undo a possible _unx2dos() conversion from MiNTlib
				if (toupper(ftoName[0]) == 'U' && ftoName[1] == ':')
					memmove(ftoName, ftoName + 2, strlen(ftoName + 2) + 1);
			}
		}
		if (ftoName[0] == '/' && DriveFromLetter(ftoname[1]) >= 0 &&
			(ftoName[2] == '\0' || ftoName[2] == '/'))
			ftoName[1] = tolower(ftoName[1]);
	}
	
	DebugInfo("xfs_symlink: \"%s\" --> \"%s\"", ffromName, ftoName);

#ifdef NOTYET
	/* Datei erstellen */
	/* -------------- */

	if (!(*onlyname))		// "to" ist Verzeichnis
	{
		type = 'fdrp';
		creator = 'MACS';
	} else
	{
		GetTypeAndCreator (name, &type, &creator);		// Datei
		type = 'TEXT';
		creator = MyCreator;
	}
	FSpCreateResFile(&fs, creator, type, smSystemScript);
#endif

	if (symlink(ftoName, ffromName))
		return errnoHost2Mint(errno, TOS_EFILNF);

	return TOS_E_OK;

#endif
}


/*************************************************************
*
* Fuer Freadlink
*
*************************************************************/

int32_t CMacXFS::xfs_readlink(XfsCookie *fc, const char *name,
				char *buf, uint16_t bufsiz)
{
	char fpathName[MAXPATHNAMELEN];
	char target[MAXPATHNAMELEN];

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s('%s', drv=%d, DD=%08lx, buf=%p, bufsize=%d)", __FUNCTION__, name, fc->dev, (unsigned long)MAPVOIDPTO32(fc->index), (void *) buf, bufsiz);
#endif

	if (fc->drv == NULL)    /* ungueltig */
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	/* Name erstellen und Alias auslesen	*/
	/* ---------------------------------	*/
	cookie2Pathname(fc, name, fpathName, true); // get the cookie filename

	if (!host_readlink(fpathName, target, sizeof(target)))
		return errnoHost2Mint(errno, TOS_EFILNF);

	CTextConversion::Host2AtariUtf8Copy(buf, target, bufsiz);
	return TOS_E_OK;
}


/*************************************************************
*
* XFS-Funktion 27 (Dcntl())
*
* Der Parameter <pArg> ist bereits umgerechnet worden von der Atari-Adresse
* in die Mac-Adresse.
*
*************************************************************/

int32_t CMacXFS::xfs_dcntl
(
	XfsCookie *fc,
	const char *name,
	uint16_t cmd,
	uint32_t arg,
	void *pArg
)
{
	char fname[MAXPATHNAMELEN];

	if (fc->drv == NULL)
		return TOS_EDRIVE;
	if (fc->drv->drv_changed)
		return TOS_E_CHNG;

	if (!(fc->drv->drv_flags & M_DRV_DOSNAMES))
	{
		if (name[0] == '.' && !name[1])
			name = "";		// "." wie leerer Name
	}

	switch(cmd)
	{
	case MX_KER_XFSNAME:
		CTextConversion::Host2AtariUtf8Copy((char *)pArg, "macfs", 8);
		return TOS_E_OK;

	case MINT_FS_INFO:
		if (pArg)
		{
			uint32_t *p32;
			CTextConversion::Host2AtariUtf8Copy((char *)pArg, "macfs-xfs", 32);
			p32 = (uint32_t *)((char *)pArg + 32);
			*p32 = cpu_to_be32(((int32_t)HOSTFS_XFS_VERSION << 16) | HOSTFS_NFAPI_VERSION);
			p32 = (uint32_t *)((char *)pArg + 36);
			*p32 = cpu_to_be32(MINT_FS_HOSTFS);
			CTextConversion::Host2AtariUtf8Copy((char *)pArg + 40, "host filesystem", 32);
		}
		return TOS_E_OK;

	case MINT_FS_USAGE:
		{
			STATVFS buff;

			cookie2Pathname(fc, NULL, fname, true);

			int32_t res = host_statvfs(fname, &buff);
			if (res != TOS_E_OK)
				return res;

			if (pArg)
			{
				uint32_t *p32;
				uint64_t *p64;
				p32 = (uint32_t *)pArg;
				/* LONG  blocksize */  *p32 = cpu_to_be32(buff.f_bsize);
				p64 = (uint64_t *)((char *)pArg + 4);
				/* LLONG    blocks */  *p64 = cpu_to_be64(buff.f_blocks);
				p64 = (uint64_t *)((char *)pArg + 12);
				/* LLONG    freebs */  *p64 = cpu_to_be64(buff.f_bavail);
				p64 = (uint64_t *)((char *)pArg + 20);
				/* LLONG    inodes */  *p64 = cpu_to_be64(buff.f_files);
				p64 = (uint64_t *)((char *)pArg + 28);
				/* LLONG    finodes*/  *p64 = cpu_to_be64(buff.f_ffree);
			}
			return TOS_E_OK;
		}

	case MINT_V_CNTR_WP:
		// FIXME: TODO!
		break;

	case MINT_FUTIME:
		if (fc->drv->drv_flags & M_DRV_READONLY)
			return TOS_EWRPRO;
	    {
	    	struct utimbuf t_set;
			if (pArg)
			{
				mutimbuf *tim = (mutimbuf *)pArg;
				t_set.actime = date_dos2mac(be16_to_cpu(tim->actime), be16_to_cpu(tim->acdate));
				t_set.modtime = date_dos2mac(be16_to_cpu(tim->modtime), be16_to_cpu(tim->moddate));
			} else
			{
				t_set.actime = t_set.modtime = time(NULL);
			}
			cookie2Pathname(fc, name, fname, true);
			if (utime(fname, &t_set))
				return errnoHost2Mint(errno, TOS_EFILNF);
		}
	  	return TOS_E_OK;

	case MINT_FUTIME_UTC:
		/* NYI */
		/* useless until kernel handles UTC timestamps */
		break;

	case MINT_FTRUNCATE:
		cookie2Pathname(fc, name, fname, true);

		DebugInfo("CMacXFS::%s: FTRUNCATE: %s, %08x", __FUNCTION__, fname, arg);
		if (fc->drv->drv_flags & M_DRV_READONLY)
			return TOS_EWRPRO;
		if (truncate(fname, arg))
			return errnoHost2Mint(errno, TOS_EFILNF);

		return TOS_E_OK;

	case MINT_FSTAT:
		if (!pArg)
		    return TOS_EINVAL;
		return xfs_xattr(fc, name, (XATTR *) pArg, 0);

	case MINT_FSTAT64:
		if (!pArg)
		    return TOS_EINVAL;
		return xfs_stat64(fc, name, (MINT_STAT64 *) pArg);

	// "type" und "creator" einer Datei ermitteln
	case FMACGETTYCR:
		if (!pArg)
		    return TOS_EINVAL;
		/* no longer supported */
		return TOS_EINVFN;

	// "type" und "creator" einer Datei ändern
	 case FMACSETTYCR:
		if (!pArg)
		    return TOS_EINVAL;
#ifdef DEMO
		(void) MyAlert(ALRT_DEMO, 2);
		return TOS_EWRPRO;
#else
		/* no longer supported */
		return TOS_EINVFN;
#endif

	// spezielle MagiC-Funktionen

	case FMACMAGICEX:
		{
			MMEXRec *mmex = (MMEXRec *) pArg;
			switch (be16_to_cpu(mmex->funcNo))
			{
			case MMEX_INFO:
				mmex->longVal = cpu_to_be32(1);
				mmex->destPtr = NULL;	// MM_VersionPtr;
				return 0;

			case MMEX_GETFSSPEC:
				/* no longer supported */
				return TOS_EINVAL;

			case MMEX_GETRSRCLEN:
				// Mac-Rsrc-Länge liefern
				cookie2Pathname(fc, name, fname, true);
				// strcat(fname, "/..namedfork/rsrc
#ifdef NOTYET
				doserr = getCatInfo (drv, &pb, true);
				if (doserr)
					return doserr;
				if (pb.hFileInfo.ioFlAttrib & ioDirMask)
					return TOS_EACCDN;
				mmex->longVal = cpu_to_be32(pb.hFileInfo.ioFlRLgLen);
				return 0;
#endif
				return TOS_EINVAL;

			case FMACGETTYCR:
				/* no longer supported */
				return TOS_EINVAL;

			case FMACSETTYCR:
#ifdef DEMO
				
				(void) MyAlert(ALRT_DEMO, 2);
				return TOS_EWRPRO;
			
#else
				/* no longer supported */
				return TOS_EINVAL;
#endif
			}
		}
	}

	return TOS_EINVFN;
}



/*************************************************************/
/******************* Dateitreiber ****************************/
/*************************************************************/

int32_t CMacXFS::dev_close(MAC_FD *f)
{
	uint16_t refcnt;

	DebugInfo("CMacXFS::%s(%d)", __FUNCTION__, f->host_fd);
	refcnt = be16_to_cpu(f->fd.fd_refcnt);
	if (refcnt == 0)
		return TOS_EINTRN;

	refcnt--;
	f->fd.fd_refcnt = cpu_to_be16(refcnt);
	if (!refcnt)
	{
		/* Datum und Uhrzeit setzen */
		/* ------------------------ */
		if (f->mod_time_dirty)
		{
			struct timeval tv[2];
			
			tv[0].tv_sec = date_dos2mac(f->mod_time[0], f->mod_time[1]);
			tv[1].tv_sec = tv[0].tv_sec;
			tv[0].tv_usec = tv[1].tv_usec = 0;
			if (futimes(f->host_fd, tv) != 0)
				return errnoHost2Mint(errno, TOS_EACCDN);
		}

		if (close(f->host_fd) != 0)
			return errnoHost2Mint(errno, TOS_EIHNDL);
		f->fc.drv = NULL;
	}

	return TOS_E_OK;
}

int32_t CMacXFS::dev_read(MAC_FD *f, int32_t count, char *buf)
{
	long lcount;

	DebugInfo("CMacXFS::%s(%d, %p, %d)", __FUNCTION__, f->host_fd, (void *)buf, count);
#if DEBUG_68K_EMU
	if (trigger == 2 && count > 0x1e && trigger_refnum == f->host_fd)
	{
		DebugInfo("#### DEBUG TRIGGER #### Ladeadresse ist %p", (void *)buf);
		trigger_ProcessStartAddr = buf;
		trigger_ProcessFileLen = (unsigned) count;
		trigger = 0;
	}
#endif
	lcount = read(f->host_fd, buf, count);
	if (lcount < 0)
		return errnoHost2Mint(errno, TOS_EACCDN);
	return (int32_t) lcount;
}


int32_t CMacXFS::dev_write(MAC_FD *f, int32_t count, char *buf)
{
#ifdef DEMO
	#pragma unused(f, count, buf)
	(void) MyAlert(ALRT_DEMO, 2);
	return TOS_EWRPRO;
#else
	long lcount;

	DebugInfo("CMacXFS::%s(%d, %p, %d)", __FUNCTION__, f->host_fd, (void *)buf, count);
	if (f->fc.drv->drv_flags & M_DRV_READONLY)
		return TOS_EWRPRO;
	lcount = write(f->host_fd, buf, count);
	if (lcount < 0)
		return errnoHost2Mint(errno, TOS_EACCDN);
	return (int32_t) lcount;
#endif
}


int32_t CMacXFS::dev_stat(MAC_FD *f, void *unsel, uint16_t rwflag, int32_t apcode)
{
#pragma unused(unsel, apcode)
	struct stat st;
	off_t pos;

	if (rwflag)
		return 1;         /* Schreiben immer bereit */
	if (fstat(f->host_fd, &st) != 0)
		return errnoHost2Mint(errno, TOS_EACCDN);
	pos = lseek(f->host_fd, 0, SEEK_CUR);
	if (pos < st.st_size)
		return 1;
	return 0;
}


int32_t CMacXFS::dev_seek(MAC_FD *f, int32_t pos, uint16_t mode)
{
	short macmode;
	off_t lpos;

	DebugInfo("CMacXFS::%s(%d, %d, %d)", __FUNCTION__, f->host_fd, pos, mode);
	switch (mode)
	{
		case 0:   macmode = SEEK_SET; break;
		case 1:   macmode = SEEK_CUR; break;
		case 2:   macmode = SEEK_END; break;
		default:  return TOS_EINVFN;
	}
	lpos = lseek(f->host_fd, pos, macmode);
	if (lpos == (off_t)-1)
		return errnoHost2Mint(errno, TOS_EACCDN);
	return lpos;
}


int32_t CMacXFS::dev_datime(MAC_FD *f, uint16_t d[2], uint16_t rwflag)
{
	struct stat st;

#ifdef DEMO
	
	if (rwflag)
	{
		(void) MyAlert(ALRT_DEMO, 2);
		return TOS_EWRPRO;
	}
	
#endif
	
	if (rwflag)			/* schreiben */
	{
		if (f->fc.drv->drv_flags & M_DRV_READONLY)
			return TOS_EWRPRO;
		f->mod_time[0] = be16_to_cpu(d[0]);
		f->mod_time[1] = be16_to_cpu(d[1]);
		f->mod_time_dirty = 1;		/* nur puffern */
	} else
	{
		if (f->mod_time_dirty)	/* war schon geaendert */
		{
			d[0] = cpu_to_be16(f->mod_time[0]);
			d[1] = cpu_to_be16(f->mod_time[1]);
		} else
		{
			if (fstat(f->host_fd, &st) != 0)
				return errnoHost2Mint(errno, TOS_EACCDN);

          	date_mac2dos(st.st_mtime, &d[0], &d[1]);
			d[0] = cpu_to_be16(d[0]);
			d[1] = cpu_to_be16(d[1]);
		}
	}

	return TOS_E_OK;
}


int32_t CMacXFS::dev_ioctl(MAC_FD *f, uint16_t cmd, void *buf)
{
	struct stat st;

	switch(cmd)
	{
	case MINT_FIONWRITE:
		*((int32_t *)buf) = cpu_to_be32(1);
		return TOS_E_OK;

	case MINT_FIONREAD:
	{
		int navail;
		
#ifdef FIONREAD
		if (ioctl(f->host_fd, FIONREAD, &navail) < 0)
#endif
		{
			int32_t pos = lseek(f->host_fd, 0, SEEK_CUR); // get position
			navail = lseek(f->host_fd, 0, SEEK_END) - pos;
			lseek(f->host_fd, pos, SEEK_SET); // set the position back
		}
		*((int32_t *)buf) = cpu_to_be32(navail);
		return TOS_E_OK;
	}

	case MINT_FIOEXCEPT:
		*((int32_t *)buf) = cpu_to_be32(0);
		return TOS_E_OK;

	case MINT_FSTAT:
		if (buf == 0)
		    return TOS_EINVAL;
		if (fstat(f->host_fd, &st) != 0)
			return errnoHost2Mint(errno, TOS_EFILNF);
		if (buf)
			convert_to_xattr(&st, (XATTR *) buf);
		return TOS_E_OK;

	case MINT_FSTAT64:
		if (buf == 0)
		    return TOS_EINVAL;
		if (fstat(f->host_fd, &st))
			return errnoHost2Mint(errno, TOS_EFILNF);
		if (buf)
		    convert_to_stat64(&st, (MINT_STAT64 *)buf);
		return TOS_E_OK;
		
	case MINT_FUTIME:
		if (f->fc.drv->drv_flags & M_DRV_READONLY)
			return TOS_EWRPRO;
#ifdef HAVE_FUTIMENS
		// Mintlib calls the dcntl(FUTIME_UTC, filename) first (below).
		// but other libs might not know.
		if (__builtin_available(macOS 10.13, iOS 11, tvOS 11, watchOS 4, *))
		{
			struct timespec ts[2];
			if (buf)
			{
				uint32_t *p32 = (uint32_t *)buf;
				ts[0].tv_sec = be32_to_cpu(p32[0]);
				ts[1].tv_sec = be32_to_cpu(p32[1]);
				ts[0].tv_sec = datetime2utc(ts[0].tv_sec) - gmtoff(ts[0].tv_sec);
				ts[1].tv_sec = datetime2utc(ts[1].tv_sec) - gmtoff(ts[1].tv_sec);
			} else
			{
				ts[0].tv_sec = ts[1].tv_sec = time(NULL);
			}
			ts[0].tv_nsec = ts[1].tv_nsec = 0;
			if (futimens(f->host_fd, ts))
			    return errnoHost2Mint(errno, TOS_EACCDN);
		} else
#endif
		{
			struct timeval tv[2];
			if (buf)
			{
				uint32_t *p32 = (uint32_t *)buf;
				tv[0].tv_sec = be32_to_cpu(p32[0]);
				tv[1].tv_sec = be32_to_cpu(p32[1]);
				tv[0].tv_sec = datetime2utc(tv[0].tv_sec) - gmtoff(tv[0].tv_sec);
				tv[1].tv_sec = datetime2utc(tv[1].tv_sec) - gmtoff(tv[1].tv_sec);
			} else
			{
				tv[0].tv_sec = tv[1].tv_sec = time(NULL);
			}
			tv[0].tv_usec = tv[1].tv_usec = 0;
			if (futimes(f->host_fd, tv))
			    return errnoHost2Mint(errno, TOS_EACCDN);
		}
		return TOS_E_OK;

	case MINT_FUTIME_UTC:
		/* NYI */
		/* useless until kernel handles UTC timestamps */
		break;

	case MINT_F_SETLK:
	case MINT_F_SETLKW:
	case MINT_F_GETLK:
		/*
		 * locking can't be handled here.
		 * It has to be done in macxfs.xfs on the Atari side
		 * We must maintain the root pointer of the file
		 * locks list, however.
		 * Note that the "buf" arguments to flock() is a pointer
		 * to a struct flock, and older kernels without implementing
		 * the call will pass that to us, while the new implementation
		 * just passes the address of the root of the locks list.
		 */
		if (!f->fc.index)
			return TOS_EIHNDL;
		if (cmd == MINT_F_GETLK)
			*((uint32_t *)buf) = cpu_to_be32(f->fc.index->locks);
		else
			f->fc.index->locks = be32_to_cpu(*((uint32_t *)buf));
		return TOS_E_OK;			

	case MINT_FTRUNCATE:
		if (!(f->fd.fd_mode & cpu_to_be16(OM_WPERM)))
			return TOS_EACCDN;
		if (f->fc.drv->drv_flags & M_DRV_READONLY)
			return TOS_EWRPRO;
	  	if (ftruncate(f->host_fd, be32_to_cpu(*((uint32_t *) buf))) != 0)
	  		return errnoHost2Mint(errno, TOS_EACCDN);
		return TOS_E_OK;

	case MX_KER_XFSNAME:
		if (buf)
		    CTextConversion::Host2AtariUtf8Copy((char *)buf, "macfs", 8);
		return TOS_E_OK;
		
	case FMACOPENRES:
		{
#ifdef NOTYET
	  		HParamBlockRec pb;
	  		FSSpec	spec;
	  		char	perm;
	  		FCBPBRec	fcb;
	  		
			// Get filename & dirID
			err = getFSSpecByFileRefNum (f->host_fd, &spec, &fcb);
			if (err) return cnverr (err);
			
			// get the file's access permission
			switch (fcb.ioFCBFlags & (kioFCBSharedWriteMask|kioFCBWriteMask))
			{
			  case kioFCBWriteMask:
				perm = fsRdWrPerm;
				break;
			  case 0x0000:
				perm = fsRdPerm;
				break;
			  case kioFCBSharedWriteMask|kioFCBWriteMask:
				perm = fsRdWrShPerm;
				break;
			  case kioFCBSharedWriteMask:
				perm = fsRdPerm;
				break;
			}
	
			/* close the data fork */
			pb.ioParam.ioRefNum = f->host_fd;
			if ((err = PBCloseSync((ParmBlkPtr)&pb)) != noErr)
			    return cnverr(err);
	
			/* now open the resource fork */
			pb.ioParam.ioVRefNum = spec.vRefNum;
			pb.fileParam.ioDirID = spec.parID;
			pb.ioParam.ioNamePtr = spec.name;
			pb.ioParam.ioPermssn = perm;
			pb.ioParam.ioMisc = 0;
			if ((err = PBHOpenRFSync(&pb)) != noErr)
			    return cnverr(err);
			f->host_fd = pb.ioParam.ioRefNum;
			if (f->fd.fd_mode & cpu_to_be16(_ATARI_O_TRUNC))
			{
			    pb.ioParam.ioMisc = 0;
			    if ((err = PBSetEOFSync((ParmBlkPtr)&pb)) != noErr)
			        return cnverr(err);
			}
#endif
			return 0;
		}

	case FMACGETTYCR:
		/* no longer supported */
		return TOS_EINVAL;

	case FMACSETTYCR:
		/* no longer supported */
		return TOS_EINVAL;

	case FMACMAGICEX:
	  	{
		  	MMEXRec *mmex = (MMEXRec*)buf;

			switch (mmex->funcNo)
			{
      		case MMEX_INFO:
      			mmex->longVal = 1;
//      			mmex->destPtr = MM_VersionPtr;
      			mmex->destPtr = NULL;
      			return 0;

      		case MMEX_GETFREFNUM:
      			// Mac-Datei-Handle liefern
      			mmex->longVal = f->host_fd;
      			return 0;
      		}
		}
	}
	return TOS_EINVFN;
}

int32_t CMacXFS::dev_getc(MAC_FD *f, uint16_t mode)
{
#pragma unused(mode)
	unsigned char c;
	int32_t ret;

	ret = dev_read(f, 1L, (char *) &c);
	if (ret < 0)
		return ret;			// Fehler
	if (ret == 0)
		return 0x0000ff1a;		// EOF
	return c;
}


int32_t CMacXFS::dev_getline(MAC_FD *f, char *buf, int32_t size, uint16_t mode)
{
#pragma unused(mode)
	char c;
	int32_t gelesen,ret;

	for	(gelesen = 0; gelesen < size;)
	{
		ret = dev_read(f, 1, &c);
		if (ret < 0)
			return ret;			// Fehler
		if (ret == 0)
			break;			// EOF
		if (c == 0x0d)
			continue;
		if (c == 0x0a)
			break;
		gelesen++;
		*buf++ = c;
	}
	return gelesen;
}


int32_t CMacXFS::dev_putc(MAC_FD *f, uint16_t mode, int32_t val)
{
#pragma unused(mode)
	char c;

	c = (char) val;
	return dev_write(f, 1, &c);
}


/*************************************************************
*
* Dispatcher für XFS-Funktionen
*
* params		Zeiger auf Parameter (68k-Adresse)
* AdrOffset68k	Offset für 68k-Adresse
*
* Note that here is no endian conversion of the return
* value, because this is already done inside the 68k emulator.
*
*************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s */
int32_t CMacXFS::XFSFunctions(uint32_t param, unsigned char *AdrOffset68k)
{
#pragma options align=packed
	uint16_t fncode;
	int32_t doserr;
	unsigned char *params = AdrOffset68k + param;

	fncode = be16_to_cpu(*((uint16_t *) params));
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s(%d)", __FUNCTION__, fncode);
	if (fncode == 7)
	{
#if 0
		if (!m68k_trace_trigger)
		{
			// ab dem ersten fopen() tracen
			m68k_trace_trigger = 1;
			_DumpAtariMem("AtariMemOnFirstXfsCall.data");
		}
#endif
	}
#endif
	params += 2;
	switch(fncode)
	{
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L250 */
	case 0:
		{
			struct syncparm
			{
				uint16_t drv;
			};
			syncparm *psyncparm = (syncparm *) params;
			doserr = xfs_sync(be16_to_cpu(psyncparm->drv));
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L266 */
	case 1:
		{
			struct ptermparm
			{
				memptr pd;		// PD *
			};
			ptermparm *pptermparm = (ptermparm *) params;
			xfs_pterm((PD *) (AdrOffset68k + be32_to_cpu(pptermparm->pd)));
			doserr = TOS_E_OK;
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L323 */
	case 2:
		{
			struct drv_openparm
			{
				uint16_t drv;
				memptr dd;		// MXFSDD *
				int32_t flg_ask_diskchange;	// in fact: DMD->D_XFS (68k-Pointer or NULL)
			};
			drv_openparm *pdrv_openparm = (drv_openparm *) params;
			doserr = xfs_drv_open(
					be16_to_cpu(pdrv_openparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdrv_openparm->dd)),
					be32_to_cpu(pdrv_openparm->flg_ask_diskchange));
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L366 */
	case 3:
		{
			struct drv_closeparm
			{
				uint16_t drv;
				uint16_t mode;
			};
			drv_closeparm *pdrv_closeparm = (drv_closeparm *) params;
			doserr = xfs_drv_close(be16_to_cpu(pdrv_closeparm->drv),
										be16_to_cpu(pdrv_closeparm->mode));
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L438 */
	case 4:
		{
			struct path2DDparm
			{
				uint16_t mode;
				uint16_t drv;
				memptr rel_dd;	// MXFSDD *
				memptr pathname;		// char *
				memptr restpfad;		// char **
				memptr symlink_dd;	// MXFSDD *
				memptr symlink;		// char **
				memptr dd;		// MXFSDD *
				memptr dir_drive;
			};
			char *restpath;
			char *symlink;
			memptr *pp;
			
			path2DDparm *ppath2DDparm = (path2DDparm *) params;
			MXFSDD *dd = (MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->dd));
#ifdef DEBUG_VERBOSE
			__dump("path2DDParm:", (const unsigned char *) ppath2DDparm, sizeof(*ppath2DDparm));
#endif
			restpath = NULL;
			symlink = NULL;
			doserr = xfs_path2DD(
					be16_to_cpu(ppath2DDparm->mode),
					be16_to_cpu(ppath2DDparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->rel_dd)),
					(char *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->pathname)),
					&restpath,
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->symlink_dd)),
					&symlink,
					dd,
					(uint16_t *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->dir_drive))
					);

            /* Writing back Atari addresses into Atari RAM, we currently have Mac addresses */
            /* Need to write 32-bit pointers back in subtracting offset) */

			if (ppath2DDparm->restpfad != 0)
			{
				pp = (memptr *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->restpfad));
				if (restpath)
					*pp = cpu_to_be32(restpath - (char *)AdrOffset68k);
				else
					*pp = 0;
			}
			if (ppath2DDparm->symlink != 0)
			{
				pp = (memptr *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->symlink));
				if (symlink)
					*pp = cpu_to_be32(symlink - (char *)AdrOffset68k);
				else
					*pp = 0;
			}
#ifdef DEBUG_VERBOSE
			if (doserr >= 0)
				DebugInfo(" DD=%08lx, restpath='%s'", (unsigned long)dd->dirID, restpath);
#endif
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L540 */
	case 5:
		{
			struct sfirstparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				memptr dta;		// MAC_DTA *
				uint16_t attrib;
			};
			sfirstparm *psfirstparm = (sfirstparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(psfirstparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(psfirstparm->dd)));
			doserr = xfs_sfirst(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(psfirstparm->name)),
					(MAC_DTA *) (AdrOffset68k + be32_to_cpu(psfirstparm->dta)),
					be16_to_cpu(psfirstparm->attrib)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L568 */
	case 6:
		{
			struct snextparm
			{
				uint16_t drv;
				memptr dta;		// MAC_DTA *
			};
			snextparm *psnextparm = (snextparm *) params;
			doserr = xfs_snext(
					be16_to_cpu(psnextparm->drv),
					(MAC_DTA *) (AdrOffset68k + be32_to_cpu(psnextparm->dta))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L598 */
	case 7:
		{
			struct fopenparm
			{
				memptr name;	// char *
				uint16_t drv;
				memptr dd;		//MXFSDD *
				uint16_t omode;
				uint16_t attrib;
			};
			fopenparm *pfopenparm = (fopenparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pfopenparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pfopenparm->dd)));
			doserr = xfs_fopen(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pfopenparm->name)),
					be16_to_cpu(pfopenparm->omode),
					be16_to_cpu(pfopenparm->attrib)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L644 */
	case 8:
		{
			struct fdeleteparm
			{
				uint16_t drv;
				memptr dd;		//MXFSDD *
				memptr name;	// char *
			};
			fdeleteparm *pfdeleteparm = (fdeleteparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pfdeleteparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pfdeleteparm->dd)));
			doserr = xfs_fdelete(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pfdeleteparm->name))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L669 */
	case 9:
		{
			struct flinkparm
			{
				uint16_t drv;
				memptr nam1;	// char *
				memptr nam2;	// char *
				memptr dd1;		// MXFSDD *
				memptr dd2;		// MXFSDD *
				uint16_t mode;
				uint16_t dst_drv;
			};
			XfsCookie fromDir, toDir;
			flinkparm *pflinkparm = (flinkparm *) params;
			fetchXFSC(&fromDir, be16_to_cpu(pflinkparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pflinkparm->dd1)));
			fetchXFSC(&toDir, be16_to_cpu(pflinkparm->dst_drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pflinkparm->dd2)));
			doserr = xfs_link(&fromDir,
					(char *) (AdrOffset68k + be32_to_cpu(pflinkparm->nam1)),
					&toDir,
					(char *) (AdrOffset68k + be32_to_cpu(pflinkparm->nam2)),
					be16_to_cpu(pflinkparm->mode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L699 */
	case 10:
		{
			struct xattrparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				memptr xattr;	// XATTR *
				uint16_t mode;
			};
			xattrparm *pxattrparm = (xattrparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pxattrparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pxattrparm->dd)));
			doserr = xfs_xattr(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pxattrparm->name)),
					(XATTR *) (AdrOffset68k + be32_to_cpu(pxattrparm->xattr)),
					be16_to_cpu(pxattrparm->mode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L729 */
	case 11:
		{
			struct attribparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				uint16_t rwflag;
				uint16_t attr;
			};
			attribparm *pattribparm = (attribparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pattribparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pattribparm->dd)));
			doserr = xfs_attrib(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pattribparm->name)),
					be16_to_cpu(pattribparm->rwflag),
					be16_to_cpu(pattribparm->attr)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L756 */
	case 12:
		{
			struct chownparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				uint16_t uid;
				uint16_t gid;
			};
			chownparm *pchownparm = (chownparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pchownparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pchownparm->dd)));
			doserr = xfs_fchown(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pchownparm->name)),
					be16_to_cpu(pchownparm->uid),
					be16_to_cpu(pchownparm->gid)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L782 */
	case 13:
		{
			struct chmodparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				uint16_t fmode;
			};
			chmodparm *pchmodparm = (chmodparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pchmodparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pchmodparm->dd)));
			doserr = xfs_fchmod(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pchmodparm->name)),
					be16_to_cpu(pchmodparm->fmode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L809 */
	case 14:
		{
			struct dcreateparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
			};
			dcreateparm *pdcreateparm = (dcreateparm *) params;
			XfsCookie fc;
			if (be32_to_cpu(pdcreateparm->name) >= m_AtariMemSize)
			{
				DebugError("CMacXFS::xfs_dcreate() - invalid name ptr");
				return TOS_ERROR;
			}

			fetchXFSC(&fc, be16_to_cpu(pdcreateparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pdcreateparm->dd)));
			doserr = xfs_dcreate(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pdcreateparm->name))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L832 */
	case 15:
		{
			struct ddeleteparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
			};
			ddeleteparm *pddeleteparm = (ddeleteparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pddeleteparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pddeleteparm->dd)));
			doserr = xfs_ddelete(&fc);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L855 */
	case 16:
		{
			struct dd2nameparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr buf;		// char *
				uint16_t bufsiz;
			};
			dd2nameparm *pdd2nameparm = (dd2nameparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pdd2nameparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pdd2nameparm->dd)));
			doserr = xfs_DD2name(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pdd2nameparm->buf)),
					be16_to_cpu(pdd2nameparm->bufsiz)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L877 */
	case 17:
		{
			struct dopendirparm
			{
				memptr dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
				memptr dd;	// MXFSDD *
				uint16_t tosflag;
			};
			dopendirparm *pdopendirparm = (dopendirparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pdopendirparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pdopendirparm->dd)));
			doserr = xfs_dopendir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdopendirparm->dirh)),
					&fc,
					be16_to_cpu(pdopendirparm->tosflag)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L919 */
	case 18:
		{
			struct dreaddirparm
			{
				memptr dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
				uint16_t size;
				memptr buf;		// char *
				memptr xattr;	// XATTR * oder NULL
				memptr xr;		// int32_t * oder NULL
			};
			dreaddirparm *pdreaddirparm = (dreaddirparm *) params;
			doserr = xfs_dreaddir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdreaddirparm->dirh)),
					be16_to_cpu(pdreaddirparm->drv),
					be16_to_cpu(pdreaddirparm->size),
					(char *) (AdrOffset68k + be32_to_cpu(pdreaddirparm->buf)),
					(XATTR *) ((pdreaddirparm->xattr) ? AdrOffset68k + be32_to_cpu(pdreaddirparm->xattr) : NULL),
					(int32_t *) ((pdreaddirparm->xr) ? (AdrOffset68k + be32_to_cpu(pdreaddirparm->xr)) : NULL)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L943 */
	case 19:
		{
			struct drewinddirparm
			{
				memptr dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
			};
			drewinddirparm *pdrewinddirparm = (drewinddirparm *) params;
			doserr = xfs_drewinddir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdrewinddirparm->dirh)),
					be16_to_cpu(pdrewinddirparm->drv)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L963 */
	case 20:
		{
			struct dclosedirparm
			{
				memptr dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
			};
			dclosedirparm *pdclosedirparm = (dclosedirparm *) params;
			doserr = xfs_dclosedir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdclosedirparm->dirh)),
					be16_to_cpu(pdclosedirparm->drv)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1014 */
	case 21:
		{
			struct dpathconfparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				uint16_t which;
			};
			dpathconfparm *pdpathconfparm = (dpathconfparm *) params;
			doserr = xfs_dpathconf(
					be16_to_cpu(pdpathconfparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdpathconfparm->dd)),
					be16_to_cpu(pdpathconfparm->which)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1035 */
	case 22:
		{
			struct dfreeparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr data;	// uint32_t data[4]
			};
			dfreeparm *pdfreeparm = (dfreeparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pdfreeparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pdfreeparm->dd)));
			doserr = xfs_dfree(&fc,
					(uint32_t *) (AdrOffset68k + be32_to_cpu(pdfreeparm->data))
					);
#ifdef DEBUG_VERBOSE
			__dump("dfreeparm:", (const unsigned char *) (AdrOffset68k + be32_to_cpu(pdfreeparm->data)), 16);
#endif
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1056 */
	case 23:
		{
			struct wlabelparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
			};
			wlabelparm *pwlabelparm = (wlabelparm *) params;
			doserr = xfs_wlabel(
					be16_to_cpu(pwlabelparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pwlabelparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pwlabelparm->name))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1078 */
	case 24:
		{
			struct rlabelparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				uint16_t bufsiz;
			};
			rlabelparm *prlabelparm = (rlabelparm *) params;
			doserr = xfs_rlabel(
					be16_to_cpu(prlabelparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(prlabelparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(prlabelparm->name)),
					be16_to_cpu(prlabelparm->bufsiz)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1102 */
	case 25:
		{
			struct symlinkparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				memptr to;		// char *
			};
			symlinkparm *psymlinkparm = (symlinkparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(psymlinkparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(psymlinkparm->dd)));
			doserr = xfs_symlink(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(psymlinkparm->name)),
					(char *) (AdrOffset68k + be32_to_cpu(psymlinkparm->to))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1127 */
	case 26:
		{
			struct readlinkparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				memptr buf;		// char *
				uint16_t bufsiz;
			};
			readlinkparm *preadlinkparm = (readlinkparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(preadlinkparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(preadlinkparm->dd)));
			doserr = xfs_readlink(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(preadlinkparm->name)),
					preadlinkparm->buf ? (char *) (AdrOffset68k + be32_to_cpu(preadlinkparm->buf)) : NULL,
					be16_to_cpu(preadlinkparm->bufsiz)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1135 */
	case 27:
		{
			struct dcntlparm
			{
				uint16_t drv;
				memptr dd;	// MXFSDD *
				memptr name;	// char *
				uint16_t cmd;
				int32_t arg;
			};
			dcntlparm *pdcntlparm = (dcntlparm *) params;
			XfsCookie fc;
			fetchXFSC(&fc, be16_to_cpu(pdcntlparm->drv), (MXFSDD *) (AdrOffset68k + be32_to_cpu(pdcntlparm->dd)));
			doserr = xfs_dcntl(&fc,
					(char *) (AdrOffset68k + be32_to_cpu(pdcntlparm->name)),
					be16_to_cpu(pdcntlparm->cmd),
					pdcntlparm->arg,
					pdcntlparm->arg ? AdrOffset68k + be32_to_cpu(pdcntlparm->arg) : NULL
					);
		}
		break;

	default:
		doserr = TOS_EINVFN;
		break;
	}

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s => %d (= 0x%08x)", __FUNCTION__, (int) doserr, (int) doserr);
#endif
	return doserr;
}


/*************************************************************
*
* Dispatcher für Dateitreiber
*
*************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1183 */
int32_t CMacXFS::XFSDevFunctions(uint32_t param, unsigned char *AdrOffset68k)
{
	uint16_t fncode;
	int32_t doserr;
	unsigned char *params = AdrOffset68k + param;
	uint32_t ifd;

	// first 2 bytes: function code
	fncode = be16_to_cpu(*((uint16_t *) params));
	params += 2;
	// next 4 bytes: pointer to MAC_FD
	ifd = *((uint32_t *) params);
	params += 4;
	MAC_FD *f = (MAC_FD *) (AdrOffset68k + be32_to_cpu(ifd));

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s(0x%04x)", __FUNCTION__, fncode);
	__dump("MAC_FD:", (const unsigned char *) f, sizeof(*f));
#endif
	switch(fncode)
	{
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1375 */
	case 0:
		doserr = dev_close(f);
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1183 */
	case 1:
		{
			struct devreadparm
			{
//				uint32_t f;	// MAC_FD *
				int32_t count;
				uint32_t buf;		// char *
			};
			devreadparm *pdevreadparm = (devreadparm *) params;
			doserr = dev_read(
					f,
					be32_to_cpu(pdevreadparm->count),
					(char *) (AdrOffset68k + be32_to_cpu(pdevreadparm->buf))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1202 */
	case 2:
		{
			struct devwriteparm
			{
//				uint32_t f;	// MAC_FD *
				int32_t count;
				uint32_t buf;		// char *
			};
			devwriteparm *pdevwriteparm = (devwriteparm *) params;
			doserr = dev_write(
					f,
					be32_to_cpu(pdevwriteparm->count),
					(char *) (AdrOffset68k + be32_to_cpu(pdevwriteparm->buf))
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1295 */
	case 3:
		{
			struct devstatparm
			{
//				uint32_t f;	// MAC_FD *
				uint32_t unsel;	//void *
				uint16_t rwflag;
				int32_t apcode;
			};
			devstatparm *pdevstatparm = (devstatparm *) params;
			doserr = dev_stat(
					f,
					(void *) (AdrOffset68k + pdevstatparm->unsel),
					be16_to_cpu(pdevstatparm->rwflag),
					be32_to_cpu(pdevstatparm->apcode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1315 */
	case 4:
		{
			struct devseekparm
			{
//				uint32_t f;	// MAC_FD *
				int32_t pos;
				uint16_t mode;
			};
			devseekparm *pdevseekparm = (devseekparm *) params;
			doserr = dev_seek(
					f,
					be32_to_cpu(pdevseekparm->pos),
					be16_to_cpu(pdevseekparm->mode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1353 */
	case 5:
		{
			struct devdatimeparm
			{
//				uint32_t f;	// MAC_FD *
				uint32_t d;		// uint16_t[2]
				uint16_t rwflag;
			};
			devdatimeparm *pdevdatimeparm = (devdatimeparm *) params;
			doserr = dev_datime(
					f,
					(uint16_t *) (AdrOffset68k + be32_to_cpu(pdevdatimeparm->d)),
					be16_to_cpu(pdevdatimeparm->rwflag)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1334 */
	case 6:
		{
			struct devioctlparm
			{
//				uint32_t f;	// MAC_FD *
				uint16_t cmd;
				uint32_t buf;		// void *
			};
			devioctlparm *pdevioctlparm = (devioctlparm *) params;
			doserr = dev_ioctl(
					f,
					be16_to_cpu(pdevioctlparm->cmd),
					pdevioctlparm->buf ? (void *) (AdrOffset68k + be32_to_cpu(pdevioctlparm->buf)) : NULL
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1227 */
	case 7:
		{
			struct devgetcparm
			{
//				uint32_t f;	// MAC_FD *
				uint16_t mode;
			};
			devgetcparm *pdevgetcparm = (devgetcparm *) params;
			doserr = dev_getc(
					f,
					be16_to_cpu(pdevgetcparm->mode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1251 */
	case 8:
		{
			struct devgetlineparm
			{
//				uint32_t f;	// MAC_FD *
				uint32_t buf;		// char *
				int32_t size;
				uint16_t mode;
			};
			devgetlineparm *pdevgetlineparm = (devgetlineparm *) params;
			doserr = dev_getline(
					f,
					(char *) (AdrOffset68k + be32_to_cpu(pdevgetlineparm->buf)),
					be32_to_cpu(pdevgetlineparm->size),
					be16_to_cpu(pdevgetlineparm->mode)
					);
		}
		break;

	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1275 */
	case 9:
		{
			struct devputcparm
			{
//				uint32_t f;	// MAC_FD *
				uint16_t mode;
				int32_t val;
			};
			devputcparm *pdevputcparm = (devputcparm *) params;
			doserr = dev_putc(
					f,
					be16_to_cpu(pdevputcparm->mode),
					be32_to_cpu(pdevputcparm->val)
					);
		}
		break;

	default:
		doserr = TOS_EINVFN;
		break;
	}
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::%s => %d", __FUNCTION__, (int) doserr);
#endif
	return doserr;
}
#pragma options align=reset


/*************************************************************
*
* Tauscht in der Atari-Systemvariablen <_drvbits> alle Bits aus,
* die das MacXFS betreffen.
*
*************************************************************/

void CMacXFS::setDrivebits(uint32_t newbits, unsigned char *AdrOffset68k)
{
	uint32_t val;
	uint32_t *p_drvbits = (uint32_t*)&AdrOffset68k[_drvbits];
	
	val = be32_to_cpu(*p_drvbits);
	val &= -1L - xfs_drvbits;		// alte löschen
	val |= newbits;			// neue setzen
	*p_drvbits = cpu_to_be32(val);
	xfs_drvbits = newbits;
}


/*************************************************************
*
* Definiert ein Atari-Laufwerk
*
*************************************************************/
void CMacXFS::SetXFSDrive
(
	unsigned short dev,
	MacXFSDrvType drvType,
	CFURLRef pathUrl,
	unsigned int flags,
	unsigned char *AdrOffset68k
)
{
	uint32_t newbits = xfs_drvbits;
	struct mount_info *drv;

	if (dev >= NDRVS)
	{
		DebugError("%s: invalid drv %u", __FUNCTION__, dev);
		return;
	}
	drv = &drives[dev];
	delete drv->host_root;
	drv->host_root = NULL;
	strcpy(drv->mount_point, "A:\\");
	drv->mount_point[0] = DriveToLetter(dev);

#ifdef SPECIALDRIVE_AB
	if (drv >= 2)
#endif
	{
		// Laufwerk ist kein MacXFS-Laufwerk mehr => abmelden
		newbits &= ~(1L << dev);
		if (drvType != NoMacXFS)
		{
			char dirname[MAXPATHNAMELEN];

			if (CFURLGetFileSystemRepresentation(pathUrl, true, (unsigned char *)dirname, sizeof(dirname)))
			{
				size_t len = strlen(dirname);
				if (len > 0 && dirname[len - 1] != *DIRSEPARATOR)
					strcat(dirname, DIRSEPARATOR);
				drv->host_root = new XfsFsFile(*this, dirname);
				// Laufwerk ist MacXFS-Laufwerk
				newbits |= 1L << dev;
			} else
			{
				drvType = NoMacXFS;
			}
		}
	}

	drv->drv_changed = true;
	drv->drv_type = drvType;

	if (drvType == MacDir)
	{
		drv->drv_valid = true;
	} else
	{
		drv->drv_valid = false;
	}

	flags |= M_DRV_READONLY; /* XXX */
	drv->drv_flags = flags;
	// drv->halfSensitive = true;

	DebugInfo("CMacXFS::%s() -- Drive %c: '%s', root DD=%08lx, type=%s, dir order=%s, names=%s",
		__FUNCTION__,
		DriveToLetter(dev),
		drv->host_root ? drv->host_root->name : "",
		(unsigned long)MAPVOIDPTO32(drv->host_root),
		xfsDrvTypeToStr(drvType),
		drv->drv_flags & M_DRV_REVERSE_DIR_ORDER ? "reverse" : "normal",
		drv->drv_flags & M_DRV_DOSNAMES ? "8+3" : "long");

#ifdef SPECIALDRIVE_AB
	if (dev >= 2)
#endif
		setDrivebits(newbits, AdrOffset68k);
}

void CMacXFS::ChangeXFSDriveFlags(unsigned short dev, unsigned int flags)
{
	struct mount_info *drv;
	if (dev >= NDRVS)
	{
		DebugError("%s: invalid drv %u", __FUNCTION__, dev);
		return;
	}
	drv = &drives[dev];
	flags |= M_DRV_READONLY; /* XXX */
	drv->drv_flags = flags;

	DebugInfo("CMacXFS::%s() -- Drive %c: '%s', dir order=%s, names=%s",
		__FUNCTION__,
		DriveToLetter(dev),
		drv->host_root ? drv->host_root->name : "",
		drv->drv_flags & M_DRV_REVERSE_DIR_ORDER ? "reverse" : "normal",
		drv->drv_flags & M_DRV_DOSNAMES ? "8+3" : "long");
}


/*************************************************************
*
* Rechnet einen Laufwerkbuchstaben um in einen "device code".
* Wird vom Atari benötigt, um das richtige Medium auszuwerfen.
*
* Dies ist ein direkter Einsprungpunkt vom Emulator.
*
*************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1772 */
int32_t CMacXFS::Drv2DevCode(uint32_t params, unsigned char *AdrOffset68k)
{
	uint16_t drv = be16_to_cpu(*((uint16_t*) (AdrOffset68k + params)));

	if (drv <= 1)
	{
		// Floppy A: & B:
		return (int32_t) 0x00010000 | (drv + 1);	// liefert 1 bzw. 2
	}

	if (drv >= NDRVS || !drives[drv].drv_valid)
	{
		// evtl. AHDI-Drive?
		if (false)
		{	// !!! erstmal nicht unterstützt.
			return (int32_t) (0x00020000 | drv);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		// Es ist ein Mac-Volume - Drive-Nr. ermitteln
		return (int32_t) (0x00030000 | (uint16_t) drv);
	}
}


/*************************************************************
*
* Erledigt Gerätefunktionen, hier nur: Auswerfen eines
* Mediums
*
*************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1753 */
int32_t CMacXFS::RawDrvr (uint32_t param, unsigned char *AdrOffset68k)
{
	int32_t ret;
	struct sparams
	{
		uint16_t opcode;
		uint32_t device;
	};


	sparams *params = (sparams *) (AdrOffset68k + param);

	switch(params->opcode)
	{
	case 0:
		if (params->device == 0)	// ??
			ret = TOS_EDRIVE;
		else
		if ((params->device >> 16)  == 1)
			ret = TOS_EDRIVE;		// Mac-Medium
		else
			ret = TOS_EDRIVE;		// AHDI-Medium auswerfen
		break;

	default:
		ret = TOS_EINVFN;
		break;
	}
	return ret;
}


/*
 * build a complete linux filepath
 * --> fs	 something like a handle to a known file
 *	   name	 a postfix to the path of fs or NULL, if no postfix
 *	   buf	 buffer for the complete filepath or NULL if static one
 *			 should be used.
 * <-- complete filepath or NULL if error
 */
char *CMacXFS::cookie2Pathname(struct mount_info *drv, XfsFsFile *fs, const char *name, char *buf, bool insert_root)
{
	static char sbuf[MAXPATHNAMELEN]; /* FIXME: size should told by unix */

	if (!buf)
		// use static buffer
		buf = sbuf;

	if (!fs)
	{
		// we are at root
		if (!name)
			return NULL;

		if (insert_root)
			strcpy(buf, drv->host_root->name);
		else
			*buf = '\0';
		return buf;
	}

	// recurse to deep
	if (!cookie2Pathname(drv, fs->parent, fs->name, buf, insert_root))
		return NULL;

	// returning from the recursion -> append the appropriate filename
	if (name && *name)
	{
		// make sure there's the right trailing dir separator
		int len = strlen(buf);
		if (len > 0)
		{
			char *last = buf + len - 1;
			if (*last == '\\' || *last == '/')
			{
				*last = '\0';
			}
			strcat(buf, DIRSEPARATOR);
		}
		getHostFileName(buf + strlen(buf), drv, buf, name);
	}

	return buf;
}

char *CMacXFS::cookie2Pathname(XfsCookie *fc, const char *name, char *buf, bool insert_root)
{
	return cookie2Pathname(fc->drv, fc->index, name, buf, insert_root);
}



DIR *CMacXFS::host_opendir(const char *fpathName)
{
	DIR *result = opendir(fpathName);
	if (result || errno != ENOTDIR)
		return result;

	// follow symlink when needed
	struct stat statBuf;
	if (!lstat(fpathName, &statBuf) && S_ISLNK(statBuf.st_mode))
	{
		char temp[MAXPATHNAMELEN];
		if (host_readlink(fpathName, temp, sizeof(temp) - 1))
			return host_opendir(temp);
	}

	return NULL;
}


char *CMacXFS::host_readlink(const char *pathname, char *target, int len)
{
	int rv;
	int i;

	target[0] = '\0';
	if ((rv = readlink(pathname, target, len)) < 0)
		return NULL;

	// put the trailing \0
	target[rv] = '\0';

	// relative host fs symlinks are left alone. The kernel will parse them
	if (target[0] != *DIRSEPARATOR && target[0] != '\\' && target[1] != ':')
		return target;
	// convert to real path (example: "/tmp/../file" -> "/file")
	{
		char *tmp = my_canonicalize_file_name(target, false);
		if (tmp == NULL)
			return target;
		size_t nameLen = strlen(tmp);
		for (i = 0; i < NDRVS; i++)
		{
			struct mount_info *drv = &drives[i];
			if (drv->drv_valid)
			{
				size_t hrLen = strlen(drv->host_root->name);
				if (hrLen == 0 || hrLen > nameLen)
					continue;
				if (strncmp(drv->host_root->name, tmp, hrLen) == 0)
				{
					// target drive found; replace the hosts root directory
					// with the mount point
					strncpy(target, drv->mount_point, len);
					int mLen = strlen(drv->mount_point);
					if (mLen < len)
						strncpy(target + mLen, tmp + hrLen, len - mLen);
					break;
				}
			}
		}
		free(tmp);
	}
	
	return target;
}


bool CMacXFS::getHostFileName(char *result, struct mount_info *drv, const char *pathName, const char *name)
{
	struct stat statBuf;

	// if the whole thing fails then take the requested name as is
	// it also completes the path
	if (drv->drv_flags & M_DRV_DOSNAMES)
		nameto_8_3(name, result, 2, false);
	else
		CTextConversion::Atari2HostUtf8Copy(result, name, MAXPATHNAMELEN);

	if (!strpbrk(name, "*?") && // if is it NOT a mask
		 stat(pathName, &statBuf)) // and if such file NOT really exists
	{
		// the TOS filename was adjusted (lettercase, length, ..)
		char testName[MAXPATHNAMELEN];
		char filenamepart[MAXPATHNAMELEN];
		struct dirent *dirEntry;
		bool nonexisting = false;

		// shorten the name from the pathName;
		strcpy(filenamepart, result);
		const char *finalName = filenamepart;
		*result = '\0';

		DIR *dh = host_opendir(pathName);
		if (dh == NULL)
		{
			goto lbl_final;	 // should never happen
		}

		for (;;)
		{
			if ((dirEntry = readdir(dh)) == NULL)
			{
				nonexisting = true;
				goto lbl_final;
			}

			// if (drv->halfSensitive)
				if (strcasecmp(filenamepart, dirEntry->d_name) == 0)
				{
					finalName = dirEntry->d_name;
					goto lbl_final;
				}

			nameto_8_3(dirEntry->d_name, testName, 1, false);

			if (strcmp(testName, filenamepart) == 0)
			{
				// FIXME isFile test (maybe?)
				// this follows one more argument to be passed

				finalName = dirEntry->d_name;
				goto lbl_final;
			}
		}

	lbl_final:
		strcpy(result, finalName);

		// in case of halfsensitive filesystem,
		// an upper case filename should be lowercase?
		if (nonexisting /* && (!drv || drv->halfSensitive) */)
		{
			bool isUpper = true;
			for (char *curr = result; *curr; curr++)
			{
				if (*curr != toupper(*curr))
				{
					isUpper = false;
					break;
				}
			}
			if (isUpper)
			{
				// lower case conversion
				// strlwr(result);
			}
		}
		if (dh != NULL)
			closedir(dh);
	}

	return true;
}


void CMacXFS::fetchXFSC(XfsCookie *fc, uint16_t drv, MXFSDD *dd)
{
	if (drv >= NDRVS || !drives[drv].drv_valid)
	{
		fc->dev = -1;
		fc->drv = NULL;
		fc->index = NULL;
		return;
	}
	fc->dev = drv;
	fc->drv = &drives[drv];
	fc->index = (XfsFsFile*)MAP32TOVOIDP(dd->dirID);
}


char *CMacXFS::my_canonicalize_file_name(const char *filename, bool append_slash)
{
	if (filename == NULL)
		return NULL;

#if (!defined HAVE_CANONICALIZE_FILE_NAME && defined HAVE_REALPATH) || defined __CYGWIN__
#ifdef PATH_MAX
	int path_max = PATH_MAX;
#else
	int path_max = pathconf(filename, _PC_PATH_MAX);
	if (path_max <= 0)
		path_max = 4096;
#endif
#endif

	char *resolved;
#if defined HAVE_CANONICALIZE_FILE_NAME
	resolved = canonicalize_file_name(filename);
#elif defined HAVE_REALPATH
	char *tmp = (char *)malloc(path_max);
	char *realp = realpath(filename, tmp);
	resolved = (realp != NULL) ? strdup(realp) : NULL;
	free(tmp);
#else
	resolved = NULL;
#endif
	if (resolved == NULL)
		resolved = strdup(filename);
	if (resolved)
	{
#ifdef __CYGWIN__
		char *tmp2 = (char *)malloc(path_max);
		strcpy(tmp2, resolved);
		cygwin_path_to_win32(tmp2, path_max);
		free(resolved);
		resolved = tmp2;
#endif
		if (append_slash)
		{
			size_t len = strlen(resolved);
			if (len > 1 && resolved[len - 1] != *DIRSEPARATOR)
			{
				resolved = (char *)realloc(resolved, len + 2);
				if (resolved)
					strcat(resolved, DIRSEPARATOR);
			}
		}
	}
	return resolved;
}
