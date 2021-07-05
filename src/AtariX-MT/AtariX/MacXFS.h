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

typedef uint32_t memptr;

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
	int32_t XFSFunctions(memptr params, unsigned char *AdrOffset68k );
	int32_t XFSDevFunctions(memptr params, unsigned char *AdrOffset68k );
	int32_t Drv2DevCode(memptr params, unsigned char *AdrOffset68k );
	int32_t RawDrvr(memptr params, unsigned char *AdrOffset68k );
	void SetXFSDrive (
			short drv,
			MacXFSDrvType drvType,
			CFURLRef path,
			bool longnames,
			bool reverseDirOrder,
			unsigned char *AdrOffset68k);
	void ChangeXFSDriveFlags (
			short drv,
			bool longnames,
			bool reverseDirOrder);

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

	typedef struct
	{
	     char      sname[11];		/* Suchname */
	     char      sattr;			/* Suchattribut */
	     int32_t   dirID;			/* Verzeichnis */
	     int16_t   vRefNum;			/* Mac-Volume */
	     uint16_t  index;			/* Index innerhalb des Verzeichnis */

	     DIR       *hostDir;		/* used DIR (host one) */
	} _MAC_DTA;

	/*
	 * Note: the host_fd member here is assigned from an int
	 * as obtained from open() etc. If we ever get numbers that
	 * don't fit in 16 bit, we may have to use a NativeTypeMapper<int, short>
	 */
	typedef struct
	{
	     MX_FD	fd;			/* allgemeiner Teil (big endian) */
	     short	host_fd;	/* Mac-Teil: Handle (host native endian, but written in macxfs.s) */
	     uint16_t	mod_time_dirty;	/* Mac-Teil: Fdatime war aufgerufen (host native endian) */
	     uint16_t	mod_time[2];	/* Mac-Teil: Zeit fuer Fdatime (DOS-Codes) (host native endian) */
	} MAC_FD;

	struct MXFSDD
	{
		int32_t dirID;			/* Verzeichniskennung (host native endian) */
		int16_t vRefNum;		/* Mac-Volume (host native endian) */
	};

	typedef struct
	{
	     MX_DHD	dhd;			/* allgemeiner Teil */
	     struct MXFSDD dhdd;
	     uint16_t	index;		/* Position des Lesezeigers (host native endian) */
	     uint16_t	tosflag;	/* TOS-Modus, d.h. 8+3 und ohne Inode (host native endian) */
	} MAC_DIRHANDLE;

	typedef union
	{
	     MX_DTA    mxdta;
	     _MAC_DTA  macdta;
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
		FSSpec drv_fsspec;		// => macSys, damit MagiC die volume-ID ermitteln kann.
		FSRef xfs_path;			// nur auswerten, wenn drv_valid = true
		CInfoPBRec drv_pbrec;
		long drv_dirID;
		bool drv_longnames;		// initialisiert auf 0en
		bool drv_rvsDirOrder;
		bool drv_readOnly;
		MacXFSDrvType drv_type;

		char *host_root;
	};
	
	struct XfsFsFile {
		XfsFsFile *parent;
		uint32_t  refCount;
		uint32_t  childCount;
		bool      created;      // only xfs_creat() was issued (no dev_open yet)

		memptr locks;
		char	  *name;
	};

	uint32_t DriveToDeviceCode (short drv);
	long EjectDevice (short opcode, long device);

	// lokale Variablen:
	// -----------------
	struct mount_info drives[NDRVS];

	uint32_t xfs_drvbits;

	/* Zur Rueckgabe an den MagiC-Kernel: */
	MX_SYMLINK mx_symlink;

	// statische Funktionen

	static char ToUpper(char c);
	static char ToLower(char c);
	static void AtariFnameToMacFname(const unsigned char *src, unsigned char *dst);
	static void MacFnameToAtariFname(const unsigned char *src, unsigned char *dst);
	static void date_mac2dos( unsigned long macdate, uint16_t *time, uint16_t *date);
	static void date_dos2mac( uint16_t time, uint16_t date, unsigned long *macdate);
	static int fname_is_invalid(const char *name);
	static int32_t cnverr (OSErr err);
	static int32_t errnoHost2Mint(int unixerrno, int defaulttoserrno);
	static bool filename_match(char *muster, char *fname);
	static bool conv_path_elem(const char *path, char *name);
	static bool nameto_8_3 (const unsigned char *macname,
				unsigned char *dosname,
				int convmode, bool toAtari);
	static char *ps(char *s);
	static void sp(char *s);
	static OSErr getInfo (CInfoPBPtr pb, FSSpec *fs);

	// XFS-Aufrufe

	int32_t xfs_sync(uint16_t drv);
	void xfs_pterm (PD *pd);
	int32_t xfs_drv_open (uint16_t drv, MXFSDD *dd, int32_t flg_ask_diskchange);
	int32_t xfs_drv_close(uint16_t drv, uint16_t mode);
	int32_t xfs_path2DD(uint16_t mode, uint16_t drv, MXFSDD *rel_dd, char *pathname,
                  char **restpfad, MXFSDD *symlink_dd, char **symlink,
                   MXFSDD *dd,
                   uint16_t *dir_drive );
	int32_t xfs_sfirst(uint16_t drv, MXFSDD *dd, char *name, MAC_DTA *dta, uint16_t attrib);
	int32_t xfs_snext(uint16_t drv, MAC_DTA *dta);
	int32_t xfs_fopen(char *name, uint16_t drv, MXFSDD *dd,
				uint16_t omode, uint16_t attrib);
	int32_t xfs_fdelete(uint16_t drv, MXFSDD *dd, char *name);
	int32_t xfs_link(uint16_t drv, char *nam1, char *nam2,
	               MXFSDD *dd1, MXFSDD *dd2, uint16_t mode, uint16_t dst_drv);
	int32_t xfs_xattr(uint16_t drv, MXFSDD *dd, char *name,
					XATTR *xattr, uint16_t mode);
	int32_t xfs_attrib(uint16_t drv, MXFSDD *dd, char *name, uint16_t rwflag, uint16_t attr);
	int32_t xfs_fchown(uint16_t drv, MXFSDD *dd, char *name, uint16_t uid, uint16_t gid);
	int32_t xfs_fchmod(uint16_t drv, MXFSDD *dd, char *name, uint16_t fmode);
	int32_t xfs_dcreate(uint16_t drv, MXFSDD *dd, char *name);
	int32_t xfs_ddelete(uint16_t drv, MXFSDD *dd);
	int32_t xfs_DD2name(uint16_t drv, MXFSDD *dd, char *buf, uint16_t bufsiz);
	int32_t xfs_dopendir(MAC_DIRHANDLE *dirh, uint16_t drv, MXFSDD *dd, uint16_t tosflag);
	int32_t xfs_dreaddir(MAC_DIRHANDLE *dirh, uint16_t drv,
			uint16_t size, char *buf, XATTR *xattr, int32_t *xr);
	int32_t xfs_drewinddir(MAC_DIRHANDLE *dirh, uint16_t drv);
	int32_t xfs_dclosedir(MAC_DIRHANDLE *dirh, uint16_t drv);
	int32_t xfs_dpathconf(uint16_t drv, MXFSDD *dd, uint16_t which);
	int32_t xfs_dfree(uint16_t drv, int32_t dirID, uint32_t data[4]);
	int32_t xfs_wlabel(uint16_t drv, MXFSDD *dd, char *name);
	int32_t xfs_rlabel(uint16_t drv, MXFSDD *dd, char *name, uint16_t bufsiz);
	int32_t xfs_readlink(uint16_t drv, MXFSDD *dd, char *name,
					char *buf, uint16_t bufsiz);
	int32_t xfs_dcntl(uint16_t drv, MXFSDD *dd, char *name, uint16_t cmd, void *pArg, unsigned char *AdrOffset68k);

	// Gerätetreiber

	int32_t dev_close( MAC_FD *f );
	int32_t dev_read( MAC_FD *f, int32_t count, char *buf );
	int32_t dev_write( MAC_FD *f, int32_t count, char *buf );
	int32_t dev_stat( MAC_FD *f, void *unsel, uint16_t rwflag, int32_t apcode );
	int32_t dev_seek( MAC_FD *f, int32_t pos, uint16_t mode );
	int32_t dev_datime( MAC_FD *f, uint16_t d[2], uint16_t rwflag );
	int32_t dev_ioctl( MAC_FD *f, uint16_t cmd, void *buf );
	int32_t dev_getc( MAC_FD *f, uint16_t mode );
	int32_t dev_getline( MAC_FD *f, char *buf, int32_t size, uint16_t mode );
	int32_t dev_putc( MAC_FD *f, uint16_t mode, int32_t val );

	// Hilfsfunktionen

	long cfss(int drv, long dirID, short vRefNum, unsigned char *name, FSSpec *fs,
			bool fromAtari);
	OSErr fsspec2DirID (int drv);
	int32_t resolve_symlink( FSSpec *fs, uint16_t buflen, char *buf );
	int32_t drv_open (uint16_t drv, bool onlyMountedVols);
	int32_t vRefNum2drv(short vRefNum, uint16_t *drv);
	int32_t MakeFSSpecManually( short vRefNum, long reldir,
					char *macpath,
					FSSpec *fs);
	char getArchiveMask (CInfoPBRec *pb);
	Byte mac2DOSAttr (CInfoPBRec *pb);
	int32_t _snext(uint16_t drv, MAC_DTA *dta);
	void cinfo_to_xattr( CInfoPBRec * pb, XATTR *xattr, uint16_t drv);
	OSErr PathNameFromDirID( long dirid, short vrefnum,
		char *fullpathname);
	int32_t dospath2macpath( uint16_t drv, MXFSDD *dd,
			char *dospath, char *macname);
	int32_t xfs_symlink(uint16_t drv, MXFSDD *dd, char *name, char *to);
	int32_t getCatInfo (uint16_t drv, CInfoPBRec *pb, bool resolveAlias);

	OSErr f_2_cinfo( MAC_FD *f, CInfoPBRec *pb, char *fname);
	OSErr getFSSpecByFileRefNum (short fRefNum, FSSpec *spec, FCBPBRec *pb);

	void setDrivebits (uint32_t newbits, unsigned char *AdrOffset68k);
};

#endif
