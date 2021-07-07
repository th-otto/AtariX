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
 * MacXFS.h
 * ========
 *
 * Enthält die Definition der Übergabestrukturen vom MacOS
 * an das Mac-XFS von MagiCMacX
 *
 */

#ifndef _MACXFS_H_INCLUDED_
#define _MACXFS_H_INCLUDED_

// System-Header
#include <dirent.h>
// Programm-Header
#include "Atari.h"
#include "MAC_XFS.H"
// Definitionen
#define NDRVS  32
//#define SPECIALDRIVE_AB
#ifndef ELINK
#define ELINK -300
#endif

#include "typemapper.h"

typedef uint32_t memptr;

#pragma options align=packed
typedef struct {
	uint64_t _st_dev;
	uint32_t _st_ino;
	uint32_t _st_mode;
	uint32_t _st_nlink;
	uint32_t _st_uid;
	uint32_t _st_gid;
	uint64_t _st_rdev;
	uint64_t _st_atime;
	uint32_t _st_atime_ns;
	uint64_t _st_mtime;
	uint32_t _st_mtime_ns;
	uint64_t _st_ctime;
	uint32_t _st_ctime_ns;
	uint64_t _st_size;
	uint64_t _st_blocks;
	uint32_t _st_blksize;
	uint32_t _st_flags;
	uint32_t _st_gen;
	uint32_t _st_reserved[7];
} MINT_STAT64;
#pragma options align=reset

class CMacXFS
{
   public:

	typedef enum
	{
		NoMacXFS,
		MacDir,
		MacDrive,
		MacRoot
	} MacXFSDrvType;

#if defined(_DEBUG)
	const char *xfsDrvTypeToStr(MacXFSDrvType type)
	{
		switch(type)
		{
			case NoMacXFS: return "NoMacXFS"; break;
			case MacDir: return "MacDir"; break;
			case MacDrive: return "MacDrive"; break;
			case MacRoot: return "MacRoot"; break;
			default: return "UNKNOWN"; break;
		}
	}
#endif

	CMacXFS();
	~CMacXFS();
	void Set68kAdressRange(memptr AtariMemSize);
	int32_t XFSFunctions(memptr params, unsigned char *AdrOffset68k);
	int32_t XFSDevFunctions(memptr params, unsigned char *AdrOffset68k);
	int32_t Drv2DevCode(memptr params, unsigned char *AdrOffset68k);
	int32_t RawDrvr(memptr params, unsigned char *AdrOffset68k);
	void SetXFSDrive (
			unsigned short dev,
			MacXFSDrvType drvType,
			CFURLRef path,
			unsigned int flags,
			unsigned char *AdrOffset68k);
	void ChangeXFSDriveFlags(unsigned short dev, unsigned int flags);

   private:

	memptr m_AtariMemSize;
	typedef void PD;

#pragma options align=packed

typedef struct _mx_dhd {
     memptr	    dhd_dmd;			// struct _mx_dmd *dhd_dmd;
} MX_DHD;

typedef struct _mx_dev {
     int32_t    dev_close;
     int32_t    dev_read;
     int32_t    dev_write;
     int32_t    dev_stat;
     int32_t    dev_seek;
     int32_t    dev_datime;
     int32_t    dev_ioctl;
     int32_t    dev_getc;
     int32_t    dev_getline;
     int32_t    dev_putc;
} MX_DEV;

typedef struct _mx_dd {
     memptr     dd_dmd;				// struct _mx_dmd *dd_dmd;
     uint16_t   dd_refcnt;
} MX_DD;

typedef struct _mx_dta {
     char       dta_res[20];
     char       dta_drive;
     char       dta_attribute;
     uint16_t   dta_time;
     uint16_t   dta_date;
     uint32_t   dta_len;
     char       dta_name[14];
} MX_DTA;

typedef struct _mx_dmd {
     memptr	    d_xfs;				// struct _mx_xfs *d_xfs;
     uint16_t   d_drive;
     memptr	    d_root;				// MX_DD     *d_root;
     uint16_t   biosdev;
     memptr     driver;
     uint32_t   devcode;
} MX_DMD;

typedef struct _mx_fd {
     memptr	    fd_dmd;				// struct _mx_dmd *fd_dmd;
     uint16_t   fd_refcnt;
     uint16_t   fd_mode;
     memptr	    fd_dev;				// MX_DEV    *fd_dev;
} MX_FD;

/* Open- Modus von Dateien (Mag!X- intern)                                 */
/* NOINHERIT wird nicht unterstuetzt, weil nach TOS- Konvention nur die     */
/* Handles 0..5 vererbt werden                                             */
/* HiByte wie unter MiNT verwendet                                         */

#define   OM_RPERM       1
#define   OM_WPERM       2
#define   OM_EXEC        4
#define   OM_APPEND      8
#define   OM_RDENY       16
#define   OM_WDENY       32
#define   OM_NOCHECK     64

	struct MXFSDD
	{
		int32_t dirID;			/* Verzeichniskennung (host native endian, a mapped XfsFsFile *) */
		int16_t vRefNum;		/* Mac-Volume (host native endian) (unused, but accessed by kernel) */
	};

	struct XfsFsFile {
		struct XfsFsFile *parent;
		uint32_t  refCount;
		uint32_t  childCount;
		bool      created;      // only xfs_creat() was issued (no dev_open yet)

		memptr locks;
		char *name;
	
		XfsFsFile(const char *root);
		virtual ~XfsFsFile(void);
	};

	struct mount_info;
	struct XfsCookie {
		uint16_t   dev;          // device number
		struct XfsFsFile *index;       // the filesystem implementation specific structure (host one)

		struct mount_info *drv;         // dev->drv mapping is found during the cookie fetch
	};

	typedef struct
	{
	     MX_DHD	dhd;			/* allgemeiner Teil */
	     uint16_t	index;		/* Position des Lesezeigers (host native endian) */
	     uint16_t	tosflag;	/* TOS-Modus, d.h. 8+3 und ohne Inode (host native endian) */
	     XfsCookie fc;
	     DIR       *hostDir;		/* used DIR (host one) */
	} MAC_DIRHANDLE;

	/*
	 * MacXFS specific part of DTA.
	 */
	typedef struct
	{
	     char      sname[11];		/* Suchname */
	     char      sattr;			/* Suchattribut */
	     uint16_t  index;			/* Index innerhalb des Verzeichnis */

	     XfsCookie fc;
	     DIR       *hostDir;		/* used DIR (host one) */
	} _MAC_DTA;

	/*
	 * Note: the host_fd member here is assigned from an int
	 * as obtained from open() etc. If we ever get numbers that
	 * don't fit in 16 bit, we may have to use a NativeTypeMapper<int, short>
	 * Note2: the size of this structure must not exceed FDSIZE (94 bytes)
	 */
	typedef struct
	{
	     MX_FD	fd;			/* allgemeiner Teil (big endian) */
	     short	host_fd;	/* Mac-Teil: Handle (host native endian, but written in macxfs.s) */
	     uint16_t	mod_time_dirty;	/* Mac-Teil: Fdatime war aufgerufen (host native endian) */
	     uint16_t	mod_time[2];	/* Mac-Teil: Zeit fuer Fdatime (DOS-Codes) (host native endian) */
	     XfsCookie fc;
	} MAC_FD;

	typedef union
	{
	     MX_DTA    mxdta;
	     struct {
#define DTA_MAGIC 0x4d674461 /* 'MgDa' */
	     	uint32_t magic;
	     	_MAC_DTA  *macdta;
	     } macdta;
	} MAC_DTA;

	typedef struct
	{
	  uint16_t	len;			/* Symlink-Laenge inklusive EOS, gerade */
	  char		data[256];
	} MX_SYMLINK;

#pragma options align=reset

	/*
	 * NDRVS Laufwerke werden vom XFS verwaltet
	 * Fuer jedes Laufwerk gibt es einen FSSpec, der
	 * das MAC-Verzeichnis repraesentiert, das fuer
	 * "x:\" steht.
	 * Ungültige FSSpec haben die Volume-Id 0.
	 */
	struct mount_info {
		bool drv_changed;
		bool drv_must_eject;
		bool drv_valid;			// zeigt an, ob alias gültig ist.
		unsigned int drv_flags;	// initialisiert auf 0en
		MacXFSDrvType drv_type;

		XfsFsFile *host_root;
		char mount_point[10];
	};
	
	uint32_t DriveToDeviceCode (short drv);
	long EjectDevice (short opcode, long device);

	// lokale Variablen:
	// -----------------
	struct mount_info drives[NDRVS];

	uint32_t xfs_drvbits;

	/* Zur Rueckgabe an den MagiC-Kernel: */
	MX_SYMLINK mx_symlink;

#if __SIZEOF_POINTER__ > 4 || DEBUG_NON32BIT
	NativeTypeMapper <void *, memptr> memptrMapper;
#endif

	// statische Funktionen

	static unsigned char ToUpper(unsigned char c);
	static unsigned char ToLower(unsigned char c);
	static void date_mac2dos(time_t macdate, uint16_t *time, uint16_t *date);
	static time_t date_dos2mac(uint16_t time, uint16_t date);
	static int fname_is_invalid(const char *name);
	static int32_t errnoHost2Mint(int unixerrno, int defaulttoserrno);
	mode_t modeMint2Host(uint16_t m);
	uint16_t modeHost2Mint(mode_t m);
	uint16_t modeHost2TOS(mode_t m);
	int flagsMagic2Host(uint16_t flags);
	int16_t flagsHost2Magic(int flags);
	static bool filename_match(char *muster, char *fname);
	static bool conv_path_elem(const char *path, char *name);
	static bool nameto_8_3 (const char *macname, char *dosname, int convmode);

	// XFS-Aufrufe

	int32_t xfs_sync(uint16_t drv);
	void xfs_pterm (PD *pd);
	int32_t xfs_drv_open (uint16_t drv, MXFSDD *dd, int32_t flg_ask_diskchange);
	int32_t xfs_drv_close(uint16_t drv, uint16_t mode);
	int32_t xfs_path2DD(uint16_t mode, uint16_t drv, MXFSDD *rel_dd, char *pathname,
                  char **restpfad, MXFSDD *symlink_dd, char **symlink,
                   MXFSDD *dd,
                   uint16_t *dir_drive);
	int32_t xfs_sfirst(XfsCookie *fc, char *name, MAC_DTA *dta, uint16_t attrib);
	int32_t xfs_snext(uint16_t dev, MAC_DTA *dta);
	int32_t xfs_fopen(XfsCookie *fc, const char *name, uint16_t omode, uint16_t attrib);
	int32_t xfs_fdelete(XfsCookie *dir, const char *name);
	int32_t xfs_link(XfsCookie *fromDir, char *fromname, XfsCookie *toDir, char *toname, uint16_t mode);
	int32_t xfs_xattr(XfsCookie *fc, const char *name, XATTR *xattr, uint16_t mode);
	int32_t xfs_stat64(XfsCookie *fc, const char *name, MINT_STAT64 *statp);
	int32_t xfs_attrib(XfsCookie *fc, const char *name, uint16_t rwflag, uint16_t attr);
	int32_t xfs_fchown(XfsCookie *fc, const char *name, uint16_t uid, uint16_t gid);
	int32_t xfs_fchmod(XfsCookie *fc, const char *name, uint16_t fmode);
	int32_t xfs_dcreate(XfsCookie *fc, const char *name);
	int32_t xfs_ddelete(XfsCookie *fc);
	int32_t xfs_DD2name(XfsCookie *fc, char *buf, uint16_t bufsiz);
	int32_t xfs_dopendir(MAC_DIRHANDLE *dirh, XfsCookie *fc, uint16_t tosflag);
	int32_t xfs_dreaddir(MAC_DIRHANDLE *dirh, uint16_t drv, uint16_t size, char *buf, XATTR *xattr, int32_t *xr);
	int32_t xfs_drewinddir(MAC_DIRHANDLE *dirh, uint16_t drv);
	int32_t xfs_dclosedir(MAC_DIRHANDLE *dirh, uint16_t drv);
	int32_t xfs_dpathconf(uint16_t drv, MXFSDD *dd, uint16_t which);
	int32_t xfs_dfree(XfsCookie *fc, uint32_t data[4]);
	int32_t xfs_wlabel(uint16_t drv, MXFSDD *dd, char *name);
	int32_t xfs_rlabel(uint16_t drv, MXFSDD *dd, char *name, uint16_t bufsiz);
	int32_t xfs_readlink(XfsCookie *fc, const char *name, char *buf, uint16_t bufsiz);
	int32_t xfs_symlink(XfsCookie *fc, const char *name, const char *to);
	int32_t xfs_dcntl(XfsCookie *fc, const char *name, uint16_t cmd, uint32_t arg, void *pArg);

	// Gerätetreiber

	int32_t dev_close(MAC_FD *f);
	int32_t dev_read(MAC_FD *f, int32_t count, char *buf);
	int32_t dev_write(MAC_FD *f, int32_t count, char *buf);
	int32_t dev_stat(MAC_FD *f, void *unsel, uint16_t rwflag, int32_t apcode);
	int32_t dev_seek(MAC_FD *f, int32_t pos, uint16_t mode);
	int32_t dev_datime(MAC_FD *f, uint16_t d[2], uint16_t rwflag);
	int32_t dev_ioctl(MAC_FD *f, uint16_t cmd, void *buf);
	int32_t dev_getc(MAC_FD *f, uint16_t mode);
	int32_t dev_getline(MAC_FD *f, char *buf, int32_t size, uint16_t mode);
	int32_t dev_putc(MAC_FD *f, uint16_t mode, int32_t val);

	// Hilfsfunktionen

	void fetchXFSC(XfsCookie *fc, uint16_t drv, MXFSDD *dd);
	char *cookie2Pathname(struct mount_info *drv, XfsFsFile *fs, const char *name, char *buf, bool insert_root);
	char *cookie2Pathname(XfsCookie *fc, const char *name, char *buf, bool insert_root);
	bool getHostFileName(char *result, struct mount_info *drv, const char *pathName, const char *name);

	static char *my_canonicalize_file_name(const char *filename, bool append_slash);


	DIR *host_opendir(const char *fpathName);
	char *host_readlink(const char *pathname, char *target, int len);
	int32_t host_statvfs(const char *fpathName, void *buff);

	int32_t drv_open (uint16_t drv);
	unsigned char mac2DOSAttr (struct stat *st);
	int32_t _snext(MAC_DTA *dta);
	void convert_to_xattr(struct stat *st, XATTR *xattr);
	void convert_to_stat64(struct stat *st, MINT_STAT64 *statp);

	void setDrivebits (uint32_t newbits, unsigned char *AdrOffset68k);
};

#endif
