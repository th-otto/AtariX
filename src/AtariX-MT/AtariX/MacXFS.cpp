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
#include <Carbon/Carbon.h>
#include <machine/endian.h>
#include <stdlib.h>
#include <string.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "MacXFS.h"
#include "Atari.h"
#include "TextConversion.h"
#include "PascalStrings.h"
extern "C" {
#include "MyMoreFiles.h"
#include "FullPath.h"
}
#include "s_endian.h"

#if defined(_DEBUG)
//#define DEBUG_VERBOSE
#endif

#define EOS '\0'

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


/*****************************************************************
*
*  Konstruktor
*
******************************************************************/

CMacXFS::CMacXFS()
{
	int i;

	xfs_drvbits = 0;
	for (i = 0; i < NDRVS; i++)
	{
		drv_fsspec[i].vRefNum = 0;    // ungueltig
//		drv_must_eject[i] = 0;
		drv_changed[i] = 0;
		drv_longnames[i] = false;
	}

	// Mac-Wurzelverzeichnis machen
	drv_type['M'-'A'] = MacRoot;
	drv_valid['M'-'A'] = true;
	drv_longnames['M'-'A'] = true;
	drv_rvsDirOrder['M'-'A'] = true;
	drv_dirID['M'-'A'] = 0;
	drv_fsspec['M'-'A'].vRefNum = -32768;		// ungültig
	drv_fsspec['M'-'A'].parID = 0;
	drv_fsspec['M'-'A'].name[0] = EOS;

	DebugInfo("CMacXFS::CMacXFS() -- Drive %c: %s dir order, %s names", 'A' + ('M'-'A'), drv_rvsDirOrder['M'-'A'] ? "reverse" : "normal", drv_longnames['M'-'A'] ? "long" : "8+3");
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
static void __dump(const unsigned char *p, int len)
{
//	char buf[256];

	while (len >= 4)
	{
		DebugInfo(" mem = %02x %02x %02x %02x", p[0], p[1], p[2], p[3]);
		p += 4;
		len -= 4;
	}

	while (len)
	{
		DebugInfo(" mem = %02x", p[0]);
		p += 1;
		len -= 1;
	}
}
#endif


/*****************************************************************
*
*  68k Adreßbereich festlegen
*
******************************************************************/

void CMacXFS::Set68kAdressRange(UInt32 AtariMemSize)
{
	m_AtariMemSize = AtariMemSize;
}


/*****************************************************************
*
*  (statisch) Wandelt Klein- in Großchrift um
*
******************************************************************/

/* äöüçé(a°)(ae)(oe)à(ij)(n˜)(a˜)(o/)(o˜) */
static char const lowers[] = {'\x84','\x94','\x81','\x87','\x82','\x86','\x91','\xb4','\x85','\xc0','\xa4','\xb0','\xb3','\xb1',0};
static char const uppers[] = {'\x8e','\x99','\x9a','\x80','\x90','\x8f','\x92','\xb5','\xb6','\xc1','\xa5','\xb7','\xb2','\xb8',0};
char CMacXFS::ToUpper( char c )
{
	char *found;

	if (c >= 'a' && c <= 'z')
		return((char) (c & '\x5f'));
	if ((unsigned char) c >= 128 &&((found = strchr(lowers, c)) != NULL))
		return(uppers[found - lowers]);
	return(c);
}

char CMacXFS::ToLower( char c )
{
	/* äöüçé(a°)(ae)(oe)à(ij)(n˜)(a˜)(o/)(o˜) */
	char *found;

	if (c >= 'A' && c <= 'Z')
		return((char) (c | '\x20'));
	if ((unsigned char) c >= 128 &&((found = strchr(uppers, c)) != NULL))
		return(lowers[found - uppers]);
	return(c);
}


/*****************************************************************
*
*  (statisch) konvertiert Atari-Dateiname in Mac-Dateinamen und "vice versa"
*
******************************************************************/

void CMacXFS::AtariFnameToMacFname(const unsigned char *src, unsigned char *dst)
{
	while (*src)
		*dst++ = CTextConversion::Atari2MacFilename(*src++);
	*dst = EOS;
}
void CMacXFS::MacFnameToAtariFname(const unsigned char *src, unsigned char *dst)
{
	while (*src)
		*dst++ = CTextConversion::Mac2AtariFilename(*src++);
	*dst = EOS;
}


/*****************************************************************
*
*  (statisch) ermittelt Dateitypen
*
******************************************************************/

static void GetTypeAndCreator(const char *name, OSType *pType, OSType *pCreator)
{
	name = strrchr(name, '.');

#if TARGET_RT_MAC_MACHO
	if ((name) && (!strcmp(name, ".PRG") || !strcmp(name, ".prg") || !strcmp(name, ".TOS") || !strcmp(name, ".tos") || !strcmp(name, ".TTP") || !strcmp(name, ".ttp")))
#else
	if ((name) && (!stricmp(name, ".PRG") || !stricmp(name, ".TOS") || !stricmp(name, ".TTP")))
#endif
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

void CMacXFS::date_mac2dos( unsigned long macdate, uint16_t *time, uint16_t *date)
{
	DateTimeRec dt;

	SecondsToDate(macdate, &dt);
	if (time)
		*time = (uint16_t) ((dt.second >> 1) + (dt.minute << 5) + (dt.hour << 11));
	if (date)
		*date = (uint16_t) ((dt.day) + (dt.month  << 5) + ((dt.year - 1980) << 9));
}

/*****************************************************************
*
*  (statisch) konvertiert DOS-Datumsangaben in Mac-Angaben
*
******************************************************************/

void CMacXFS::date_dos2mac(uint16_t time, uint16_t date, unsigned long *macdate)
{
	DateTimeRec dt;
	dt.second	= (short) ((time & 0x1f) << 1);
	dt.minute	= (short) ((time >> 5 ) & 0x3f);
	dt.hour	= (short) ((time >> 11) & 0x1f);
	dt.day	= (short) (date & 0x1f);
	dt.month	= (short) ((date >> 5 ) & 0x0f);
	dt.year	= (short) (((date >> 9 ) & 0x7f) + 1980);
	DateToSeconds(&dt, macdate);
}


/*****************************************************************
*
*  (statisch) Testet, ob ein Dateiname gültig ist. Verboten sind Dateinamen, die
*  nur aus '.'-en bestehen.
*
******************************************************************/

int CMacXFS::fname_is_invalid( char *name)
{
	while ((*name) == '.')		// führende Punkte weglassen
		name++;
	return(!(*name));			// Name besteht nur aus Punkten
}


/*****************************************************************
*
*  (statisch) konvertiert Mac-Fehlercodes in MagiC- Fehlercodes
*
******************************************************************/

int32_t CMacXFS::cnverr(OSErr err)
{
	switch(err)
	{
          case noErr:         return(E_OK);       /* 0      no error  */
          case nsvErr:        return(EDRIVE);     /* -35    no such volume */
          case fnfErr:        return(EFILNF);     /* -43    file not found */
          case dirNFErr:      return(EPTHNF);     // -120   directory not found
          case ioErr:         return(EREADF);     /* -36    IO Error */
          case gfpErr:                            /* -52    get file pos error */
          case rfNumErr:                          /* -51    Refnum error */
          case paramErr:                          /* -50    error in user parameter list */
          case fnOpnErr:      return(EINTRN);     /* -38    file not open */
          case bdNamErr:                          /* -37    bad name in file system (>31 Zeichen)*/
          case posErr:                            /* -40    pos before start of file */
          case eofErr:        return(ATARIERR_ERANGE);     /* -39    end of file */
          case mFulErr:       return(ENSMEM);     /* -41    memory full */
          case tmfoErr:       return(ENHNDL);     /* -42    too many open files */
          case wPrErr:        return(EWRPRO);     /* -44    disk is write protected */
          case notAFileErr:                       /* -1302  is directory, no file! */
          case wrPermErr:                         /* -61    write permission error */

          case afpAccessDenied:                   // -5000  AFP access denied
          case afpDenyConflict:                   // -5006  AFP
          case afpVolLocked:                      // -5031  AFP

          case permErr:                           /* -54    permission error */
          case opWrErr:                           /* -49    file already open with write permission */
          case dupFNErr:                          /* -48    duplicate filename */
          case fBsyErr:                           /* -47    file is busy */
          case vLckdErr:                          /* -46    volume is locked */
          case fLckdErr:      return(EACCDN);     /* -45    file is locked */
          case volOffLinErr:  return(EDRVNR);     /* -53    volume off line (ejected) */
          case badMovErr:     return ENSAME;      // -122   not same volumes or folder move from/to file
          case dataVerErr:                        // read verify error
          case verErr:        return EBADSF;      // track format verify error
	}

	return(ERROR);
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
		return((muster[0] == '?') || (muster[0] == fname[0]));

	/* vergleiche 11 Zeichen */

	for	(i = 10; i >= 0; i--)
	{
		c1 = *muster++;
		c2 = *fname++;
		if (c1 != '?')
		{
			if (ToUpper(c1) != ToUpper(c2))
				return(false);
		}
	}

	/* vergleiche Attribut */

	c1 = *muster;
	c2 = *fname;

//	if (c1 == F_ARCHIVE)	
//		return(c1 & c2);
	c2 &= F_SUBDIR + F_SYSTEM + F_HIDDEN + F_VOLUME;
	if (!c2)
		return(true);
	if ((c2 & F_SUBDIR) && !(c1 & F_SUBDIR))
		return(false);
	if ((c2 & F_VOLUME) && !(c1 & F_VOLUME))
		return(false);
	if (c2 & F_HIDDEN+F_SYSTEM)
	{
		c2 &= F_HIDDEN+F_SYSTEM;
		c1 &= F_HIDDEN+F_SYSTEM;
	}

	return((bool) (c1 & c2));
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

	for	(i = 0; (i < 8) && (*path) &&
		 (*path != '\\') && (*path != '*') && (*path != '.') && (*path != ' '); i++)
	{
		*name++ = ToUpper(*path++);
	}

	/* Zeichen fuer das Auffuellen ermitteln */

	if (i == 8)
	{
		while ((*path) && (*path != ' ') && (*path != '\\') && (*path != '.'))
		{
			path++;
			truncated = true;
		}
	}

	c = (*path == '*') ? '?' : ' ';
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

	for	(i = 0; (i < 3) && (*path) &&
		 (*path != '\\') && (*path != '*') && (*path != '.') && (*path != ' '); i++)
	{
		*name++ = ToUpper(*path++);
	}

	if ((*path) && (*path != '\\') && (*path != '*'))
		truncated = true;

	/* Zeichen fuer das Auffuellen ermitteln */

	c = (*path == '*') ? '?' : ' ';

	/* Rest bis 3 Zeichen auffuellen */

	for	(;i < 3; i++)
	{
		*name++ = c;
	}

	return(truncated);
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

bool CMacXFS::nameto_8_3 (const unsigned char *macname,
				unsigned char *dosname,
				int convmode, bool toAtari)
{
	short i;
	bool truncated = false;


 	/* max. 8 Zeichen fuer Dateinamen kopieren */
 	i = 0;
	while ((i < 8) && (*macname) && (*macname != '.'))
	{
		if ((*macname == ' ') || (*macname == '\\'))
		{
			macname++;
			continue;
		}
		if (toAtari)
		{
			if (convmode == 0)
				*dosname++ = CTextConversion::Mac2AtariFilename(*macname++);
			else if (convmode == 1)
				*dosname++ = (unsigned char) ToUpper((char) CTextConversion::Mac2AtariFilename(*macname++));
			else
				*dosname++ = (unsigned char) ToLower((char) CTextConversion::Mac2AtariFilename(*macname++));
		}
		else
		{
			if (convmode == 0)
				*dosname++ = *macname++;
			else if (convmode == 1)
				*dosname++ = (unsigned char) ToUpper((char) (*macname++));
			else
				*dosname++ = (unsigned char) ToLower((char) (*macname++));
		}
		i++;
	}

	while ((*macname) && (*macname != '.'))
	{
		macname++;		// Rest vor dem '.' ueberlesen
		truncated = true;
	}
	if (*macname == '.')
		macname++;		// '.' ueberlesen
	*dosname++ = '.';			// '.' in DOS-Dateinamen einbauen

	/* max. 3 Zeichen fuer Typ kopieren */
	i = 0;
	while ((i < 3) && (*macname) && (*macname != '.'))
	{
		if ((*macname == ' ') || (*macname == '\\'))
		{
			macname++;
			continue;
		}
		if (toAtari)
		{
			if (convmode == 0)
				*dosname++ = CTextConversion::Mac2AtariFilename(*macname++);
			else if (convmode == 1)
				*dosname++ = (unsigned char) ToUpper((char) CTextConversion::Mac2AtariFilename(*macname++));
			else
				*dosname++ = (unsigned char) ToLower((char) CTextConversion::Mac2AtariFilename(*macname++));
		}
		else
		{
			if (convmode == 0)
				*dosname++ = *macname++;
			else if (convmode == 1)
				*dosname++ = (unsigned char) ToUpper((char) (*macname++));
			else
				*dosname++ = (unsigned char) ToLower((char) (*macname++));
		}
		i++;
	}

	if (dosname[-1] == '.')		// trailing '.'
		dosname[-1] = EOS;		//   entfernen
	else
		*dosname = EOS;

	if (*macname)
		truncated = true;

	return(truncated);
}


/*************************************************************
*
* (statisch)
* wandelt einen Pascal-String um in eine Zeichenkette.
* Benutzt dabei eine statischen Puffer!!
*
*************************************************************/

char *CMacXFS::ps(char *s)
{
	static char cs[512];

	strncpy(cs, s + 1, s[0]);
	cs[(int) s[0]] = EOS;
	return(cs);
}


/*************************************************************
*
* (statisch)
* wandelt eine Zeichenkette um in einen Pascal-String.
* D.h. initialisiert das Laengenbyte.
*
*************************************************************/

void CMacXFS::sp(char *s)
{
	s[0] = (char) strlen(s+1);
}


/*************************************************************
*
* Wandelt dirID, drv und Dateinamen in einen FSSpec um.
* Wenn das Dateisystem keine langen Namen unterstuetzt, wird
* der Name gekuerzt.
* Wenn <fromAtari> == TRUE, wird der Dateiname, d.h. Umlaute,
* konvertiert
*
*************************************************************/

long CMacXFS::cfss
(
	int drv,
	long dirID,
	short vRefNum,
	unsigned char *name,
	FSSpec *fs,
	bool fromAtari
)
{
	if (!drv_fsspec[drv].vRefNum)		// Abfrage eigentlich unnoetig
		return(EDRIVE);

	fs->vRefNum = vRefNum;
	fs->parID = dirID;
	if (drv_longnames[drv])
	{
		// copy up to 62 characters and nul-terminate the destination
		strlcpy((char *) fs->name + 1, (char *) name, sizeof(fs->name) - 1);
	}
	else
	{
		// bzw. in 8+3 wandeln
		nameto_8_3(name, fs->name + 1, 2, false);
	}

	if (fromAtari)
	{
		// convert character set from Atari to MacOS
		AtariFnameToMacFname(fs->name + 1, fs->name + 1);
	}

	sp((char *) fs->name);

	return(E_OK);
}


/*************************************************************
*
* initialisiert die Strukturen fuer Laufwerk <n> des XFS.
* Ein kompletter Pfad wird umgewandelt in eine DirID.
* Das erste Zeichen des Pfads muss frei sein.
*
*************************************************************/

OSErr CMacXFS::fsspec2DirID(int drv)
{
	OSErr err;
	FSSpec *fs;
	CInfoPBPtr pb;

	fs = drv_fsspec+drv;
	pb = drv_pbrec+drv;

	pb -> hFileInfo.ioVRefNum = fs -> vRefNum;
	pb -> hFileInfo.ioNamePtr = fs -> name;
	pb -> hFileInfo.ioFDirIndex = 0;
	pb -> hFileInfo.ioDirID = fs -> parID;
	if (pb->hFileInfo.ioNamePtr[0] == 0) pb->hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync (pb);
	if (err)
		return(err);
	if (!(pb -> hFileInfo.ioFlAttrib & 0x10))
		return(-1);    /* kein SubDir */
	drv_dirID[drv] = pb -> dirInfo.ioDrDirID;
	return(0);
}

/*
OSErr CMacXFS::cpath2DirID( int drv, char *cpath )
{
	OSErr err;
	FSSpec *fs;

	c2pstr(cpath);
	fs = drv_fsspec+drv;
	err = FSMakeFSSpec(0, 0L, (unsigned char*)cpath, fs);
	if (err)
		{
		fs->vRefNum = 0;
		return(err);
		}
	return fsspec2DirID (drv);
}
*/

/*************************************************************
*
* Löst einen MagiC-Alias auf.
* Eingabe:
*		fs		FSSpec des Alias
*		buflen	max. Laenge des Symlink inkl. EOS
*		buf		NULL:
*					Der Symlink wird dereferenziert,
*					der Ziel-FSSpec wieder nach <fs>
*					geschrieben. Ggf. wird ELINK
*					geliefert und der mx_symlink
*					initialisiert.
*				Puffer fuer Symlink: (fuer Freadlink)
*					Der Symlink wird gelesen, wenn
*					er vom Finder erstellt war, wird
*					EACCDN geliefert.
*
* Rueckgabe:
*		fs		ggf. FSSpec der Datei
*		retcode	E_OK:	fs ist gueltig
*				ELINK: xfs_symlink ist gueltig
*				EACCDN: Datei ist kein Alias oder ein
*					(ungueltiger) Finder-Alias.
*
* Zurueckgegeben wird ein MagiC-Fehlercode
*
*************************************************************/

int32_t CMacXFS::resolve_symlink( FSSpec *fs, uint16_t buflen, char *buf )
{
	short refnum;
	Handle myhandle;
	AliasHandle myalias;
	Boolean wasChanged;
	OSType AliasUserType;
	Size AliasSize;
	char *s;
	uint16_t len;
	OSErr err;
	int32_t doserr;

	short saveRefNum = CurResFile();


	refnum = FSpOpenResFile(fs, fsRdPerm);
	if (refnum == -1)
		return(EACCDN);

	/* Die Ressource #0 des Typs 'alis' holen	*/
	/* -------------------------------------	*/
	
	myhandle = Get1Resource('alis', 0);
	if (myhandle)
	{
		myalias = (AliasHandle) myhandle;

		if (!buf)
		{
			/* Der Mac soll den Alias dereferenzieren	*/
			/* ------------------------------------	*/
			err = ResolveAlias (NULL, myalias, fs, &wasChanged);
			doserr = cnverr(err);
		}
		else
			doserr = EFILNF;

		if (doserr)
		{
			/* Der Mac kann den Alias nicht dereferenzieren,	*/
			/* Wir holen deshalb den DOS-Pfad und geben ihn an	*/
			/* den MagiC-Kernel zurueck.					*/
			/* ------------------------------------------------	*/

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
			// Compilat für i86 erfordert als Minimal-OS-Version 10.4.
			// Im SDK 10.4 ist die Alias-Struktur "opaque", d.h. man
			// kommt nicht mehr an den <userType> dran.
			AliasUserType = GetAliasUserType(myalias);
			AliasSize = GetAliasSize(myalias);
#else
			AliasUserType = (*myalias)->userType;
			AliasSize = (*myalias)->aliasSize;
#endif
			// ntohl: konvertiere network (= big endian) nach host (aktuelle CPU)
			if (ntohl(AliasUserType) == 'MgMc')
			{
				s = *myhandle + AliasSize;
				len = (unsigned short) (strlen(s) + 1);
				if (!buf)
				{
					if (len & 1)
						len++;
					buflen = 256;
					mx_symlink.len = cpu_to_be16(len);
					buf = mx_symlink.data;
				/*	doserr = ELINK;	*/
					doserr = EFILNF;
				}
				else
				{
					doserr = E_OK;
				}

				if (len <= buflen)
				{
					strcpy(buf, s);
				}
				else
				{
					doserr = ATARIERR_ERANGE;
				}
			}
			else
			{
				doserr = EACCDN;
			}
		}
	}
	else
	{
		doserr = EACCDN;		/* keine 'alis'-Ressource */
	}

	CloseResFile(refnum);
	UseResFile (saveRefNum);
	return(doserr);
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
	OSErr err;
	ParamBlockRec pb;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_sync(drv = %d)", (int) drv);
#endif

	if (!(pb.ioParam.ioRefNum = drv_fsspec[drv].vRefNum))
		return(EDRIVE);

	pb.ioParam.ioNamePtr = NULL;		// !!!
//	if (UseAsynchronousDiskIO)
//		err = PBFlushVolAsync (pb);
//	else
		err = PBFlushVolSync (&pb);
	return(cnverr(err));
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

OSErr CMacXFS::getInfo (CInfoPBPtr pb, FSSpec *fs)
{
	pb->hFileInfo.ioVRefNum = fs->vRefNum;
	pb->hFileInfo.ioNamePtr = fs->name;
	pb->hFileInfo.ioFDirIndex = 0;
	pb->hFileInfo.ioDirID = fs->parID;
	if (pb->hFileInfo.ioNamePtr[0] == 0) pb->hFileInfo.ioFDirIndex = -1;
	return PBGetCatInfoSync (pb);
}

OSErr CMacXFS::resAlias(AliasHandle alias, 	FSSpec *gSpec, bool gOnlyMountedVols)
{
	Boolean b;
	CInfoPBRec pb;
	short saveRefNum, fRefNum;
	AliasHandle ori_alias = alias;
	OSErr gErr;


	do
	{
		// Schleife: Falls gewählte Datei wieder ein Alias ist, das auch auflösen
		if (gOnlyMountedVols)
		{
			short matches = 1;
			unsigned long mode = kARMSearch | kARMSearchRelFirst | kARMMultVols | kARMNoUI;
		 	gErr = MatchAlias (&Globals.s_ProcDir, mode, alias, &matches, gSpec, &b, nil, nil);
		}
		else
		{
			gErr = ResolveAlias (&Globals.s_ProcDir, alias, gSpec, &b);
		}
		if (alias != ori_alias) DisposeHandle ((Handle)alias);
		if (gErr || getInfo (&pb, gSpec) || (pb.hFileInfo.ioFlAttrib&16) || !(pb.hFileInfo.ioFlFndrInfo.fdFlags&0x8000))
		{
			break;
		}
		saveRefNum = CurResFile ();
		fRefNum = FSpOpenResFile (gSpec, fsRdPerm);
		if (fRefNum == -1) { gErr = dirNFErr; break; }
		UseResFile (fRefNum);
		alias = (AliasHandle)GetResource ('alis', 0);
		DetachResource ((Handle)alias);
		CloseResFile (fRefNum);
		UseResFile (saveRefNum);
	}
	while (alias);

	return(gErr);
}


int32_t CMacXFS::drv_open (uint16_t drv, bool onlyMountedVols)
{
	HVolumeParam pbh;
	OSErr err;

//	checkForNewDrive (drv);

	if (drv == 'M'-'A')
		return(E_OK);

	if (!drv_valid[drv])
		return(EDRIVE);

	// convert modern FSRef to ancient FFSpec
	err = FSGetCatalogInfo(
					&xfs_path[drv],
					kFSCatInfoNone,		// FSCatalogInfoBitmap whichInfo,
					NULL,				// FSCatalogInfo *catalogInfo,
					NULL,				// HFSUniStr255 *outName,
					&drv_fsspec[drv],
					NULL				// FSRef *parentRef
					);
//	if (resAlias(xfs_alias[drv], &drv_fsspec[drv], onlyMountedVols))
	if (err)
	{
		DebugError("CMacXFS::drv_open(): Cannot convert FSRef to FSSpec");
		return(EDRVNR);
	}

	fsspec2DirID (drv);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	// Ermitteln, ob Volume "locked" ist.
	pbh.ioVolIndex = 0;
	pbh.ioNamePtr = nil;
	pbh.ioVRefNum = drv_fsspec[drv].vRefNum;
	pbh.ioVAtrb = 0;
	PBHGetVInfoSync ((HParmBlkPtr)&pbh);
	drv_readOnly[drv] = (pbh.ioVAtrb & 0x8080) != 0;

	return E_OK;
}

int32_t CMacXFS::xfs_drv_open (uint16_t drv, MXFSDD *dd, int32_t flg_ask_diskchange)
{
	//static void *oldsp;
	int32_t	err;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_drv_open(drv = %d, flag = %d)", (int) drv, (int) flg_ask_diskchange);
#endif

	if (flg_ask_diskchange)		// Diskchange- Status ermitteln
	{
		if (drv_changed[drv])
			return(E_CHNG);
		else
			return(E_OK);
	}

	drv_changed[drv] = false;		// Diskchange reset

	err = drv_open(drv, false);
	if (err)
		return err;

	// note that dirID and vRefNum remain in CPU natural byte order, i.e. little endian on x86

	dd->dirID = drv_dirID[drv];
	dd->vRefNum = drv_fsspec[drv].vRefNum;

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_drv_open => (dd.dirID = %ld, vRefNum = %d)", dd->dirID, (int) dd->vRefNum);
#endif

	return(E_OK);
}


/*************************************************************
*
* Rechnet eine Volume-Nummer in ein Atari-Laufwerk um.
* Rückgabe EDRIVE, wenn nicht gefunden
*
* Nur aufgerufen von Path2DD.
* drv zeigt auf Atari-Speicher, also bigendian
*
*************************************************************/

int32_t CMacXFS::vRefNum2drv(short vRefNum, uint16_t *drv)
{
	uint16_t i;

	for	(i = 0; i < NDRVS; i++)
		if (drv_fsspec[i].vRefNum == vRefNum)
		{
			*drv = cpu_to_be16(i);
			return(E_OK);
		}
	return(EDRIVE);
}


/*************************************************************
*
* "Schließt" ein Laufwerk.
* mode =	0: Laufwerk schliessen oder EACCDN liefern
*		1: Laufwerk schliessen, immer E_OK liefern.
*
*************************************************************/

int32_t CMacXFS::xfs_drv_close(uint16_t drv, uint16_t mode)
{
	if (drv == 'M'-'A')
		return((mode) ? E_OK : EACCDN);
	if (mode == 0 && drv_fsspec[drv].vRefNum == 0)    /* ungueltig */
		return(EDRIVE);

	if (mode == 0 && drv_fsspec[drv].vRefNum == -1)
	{
		// Mac-Boot-Volume darf nicht ungemountet werden.
		return(EACCDN);
	}
	else
	{
		if (drv_type[drv] == MacDrive) drv_valid[drv] = false;	// macht Alias ungültig
		drv_fsspec[drv].vRefNum = 0;
		return(E_OK);
	}
}


/*************************************************************
*
* Berechnet manuell einen FSSpec Schritt fuer Schritt, um auch
* Aliase aufloesen zu koennen.
* Der Pfad ist immer relativ und beginnt mit einem ':', sowohl
* das Laengenbyte ist eingetragen als auch der Pfad
* nullterminiert.
* Bei einem Befehl "chdir mist\schrott" wird der Mac-Pfad
* "mist:schrott" uebergeben. Wenn nun "schrott" ein Alias
* ist, wird der Alias von "schrott" zurueckgegeben.
*
*************************************************************/

int32_t CMacXFS::MakeFSSpecManually( short vRefNum, long reldir,
					char *macpath,
					FSSpec *fs)
{
	Str63 dirname;
	char *s,*t;		// Laufzeiger
	unsigned long len;
	OSErr err;
	int32_t doserr;
	CInfoPBRec pb;


	s = macpath+2;	// Laengenbyte und ':' ueberlesen
	while ((t = strchr(s, ':')) != NULL)
	{
		len = (unsigned long) (t - s);
		memcpy(dirname+1, s, len);		// ein Pfadelement
		dirname[0] = (unsigned char) len;
		s = t+1;

		/* Eine Verzeichnisebene durchsuchen */
		/* ---------------------------------- */

		err = FSMakeFSSpec(vRefNum, reldir, dirname, fs);
		if (err)
			return(cnverr(err));		// darf eigentlich nicht passieren
		pb.hFileInfo.ioVRefNum = fs->vRefNum;
		pb.hFileInfo.ioNamePtr = fs->name;
		pb.hFileInfo.ioFDirIndex = 0;
		pb.hFileInfo.ioDirID = fs->parID;
		if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
		err = PBGetCatInfoSync (&pb);
		if (err)
			return(cnverr(err));			// nix gefunden

		/* ggf. Alias aufloesen */
		/* ------------------- */

		if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
		{
			doserr = resolve_symlink( fs, 0, NULL );
	
			if (!doserr)		/* Der Mac hat dereferenziert! */
			{
				pb.hFileInfo.ioVRefNum = fs->vRefNum;
				pb.hFileInfo.ioNamePtr = fs->name;
				pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
				pb.hFileInfo.ioDirID = fs->parID;
				if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
				err = PBGetCatInfoSync (&pb);
				if (err)
					return(cnverr(err));			// nix gefunden
			}
			else	return(doserr);		/* Fehler */
		}
		reldir = pb.hFileInfo.ioDirID;
		vRefNum = fs->vRefNum;
	}

	len = strlen(s);				// Restpfad
	memcpy(dirname+1, s, len);
	dirname[0] = (unsigned char) len;
	err = FSMakeFSSpec(vRefNum, reldir, dirname, fs);
	return (cnverr(err));
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
*    "AWS 95:magix:MAGIX_A:subdir1:magx_acc.zip"
*
* Dabei ist
*    "AWS 95:magix:MAGIX_A"
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
	uint16_t drv, MXFSDD *rel_dd, char *pathname,
	char **restpfad, MXFSDD *symlink_dd, char **symlink,
	MXFSDD *dd,
	uint16_t *dir_drive
)
{
#pragma unused(symlink_dd)
	OSErr err;
	int32_t doserr;
	FSSpec fs;
	CInfoPBRec pb;
	unsigned char macpath[256];
	unsigned char *s,*t,*u;
	unsigned char c;
	bool get_parent;
	short vRefNum;
	long reldir = rel_dd->dirID;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_path2DD(drv = %d, pathname = \"%s\")", (int) drv, pathname);
#endif

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);

	vRefNum = rel_dd->vRefNum;

	/* Hier schon einmal das Laufwerk setzen, das wir zurückliefern	*/
	/* Wenn wir einen Alias verfolgen, kann sich das noch ändern	*/
	/* -------------------------------------------------------------	*/

	*dir_drive = cpu_to_be16(drv);

	/* Der relative DOS-Pfad wird umgewandelt in einen relativen	*/
	/* Mac-Pfad. Eintraege "." und ".." werden beruecksichtigt.		*/
	/* ------------------------------------------------------------ */

	s = macpath + 1;		// 1 Byte fuer Laenge lassen
	*s++ = ':';				// alle Pfade sind Mac-relativ
	t = (unsigned char *) pathname;
	u = s;
	while (*pathname)
	{
		c = CTextConversion::Atari2MacFilename((unsigned char) (*pathname++));	// 9.6.96
		if (c == '\\')
		{
			if ((t[0] == '.') && ((unsigned char *) pathname == t + 2))
				s -= 2;	// war Element "." => entfernen
			else
			if ((t[0] == '.') && (t[1] == '.') && ((unsigned char *) pathname == t + 3))
			{
				// war Element ".."
				s -= 3;	// zunaechst aus Mac-Pfad entfernen
				if (s > macpath + 2)
				{
					// ich kann zurueckgehen
					s--;
					do	
						s--;
					while (*s != ':');	// gehe zurueck
				}
				else
				{
					// ich kann nicht zurueckgehen
					if (reldir == drv_dirID[drv])
					{
						// bin schon root
						*restpfad = (char *) t;
						*symlink = NULL;
						return(ELINK);
					}
					err = FSMakeFSSpec(vRefNum, reldir, NULL, &fs);
					if (err)
						goto was_doserr;
					reldir = fs.parID;		// parent anwaehlen
				}
			}
			t = (unsigned char *) pathname;	// letztes Element Atari
			u = s;						// letztes Element Mac ohne ':'
			*s++ = ':';
		}
		else
		if (c == ':')
			return(EPTHNF);				// Doppelpunkt nicht erlaubt
		else
			*s++ = c;
	}
	*s = EOS;

	if (!mode) 			/* Dateinamen noch nicht bearbeiten */
		*u = EOS;			/* Dateinamen abtrennen vom Mac-Pfad */
	else
	{
		get_parent = false;			// nicht den parent!
		if ((u[0] == ':') && (u[1] == '.') && (!u[2]))
			*u = EOS;				// letztes Element "." => abtrennen
		else
		if ((u[0] == '.') && (!u[1]))
			*u = EOS;				// einziges Element "." => abtrennen
		else
		if ((u[0] == ':') && (u[1] == '.') && (u[2] == '.') && (!u[3]))
			goto parent;			// letztes Element ".." => abtrennen
		else
		if ((u[0] == '.') && (u[1] == '.') && (!u[2]))
		{
			parent:
			*u = EOS;				// einziges Element ".." => abtrennen
			get_parent = true;
		}
	}

	sp((char *) macpath);

	/* Jetzt ist der Pfad in einen Mac-Pfad umgewandelt.	*/
	/* Wir versuchen zunaechst, die dirID mit nur		*/
	/* einem Aufruf zu bestimmen						*/
	/* ---------------------------------------------------	*/

	if ((rel_dd->vRefNum == -32768) && (rel_dd->dirID == 0))	// Mac-Root
	{
		if (!strcmp((char *) (macpath + 1), ":"))					// noch Mac-Root
		{
			pb.hFileInfo.ioFlAttrib = 0x10;
			reldir = rel_dd->dirID;
			vRefNum = rel_dd->vRefNum;
			goto giveback;
		}

		// Pfad absolut machen
		memmove(macpath + 1, macpath + 2, strlen((char *) (macpath + 2)));		// ":" eliminieren
		macpath[strlen((char *) macpath) - 1] = EOS;
		macpath[0]--;

		// Macke in FSMakeMSSpec ausbügeln für Wurzelverzeichnis eines Volumes
		if (!strchr((char *) (macpath + 1), ':'))
		{
			strcat((char *) (macpath + 1), ":");
			macpath[0]++;
		}
//		DebugStr(macpath);
	}

	err = FSMakeFSSpec(vRefNum, reldir, (unsigned char *) macpath, &fs);

	/* Wenn das Verzeichnis nicht gefunden wurde, kann	*/
	/* es sein, dass eines der Verzeichnisse ein Alias ist.*/
	/* ---------------------------------------------------	*/

	if (err == dirNFErr)
	{
		doserr = MakeFSSpecManually(vRefNum, reldir, (char *) macpath, &fs);
		if (doserr)
			goto was_doserr2;
		if (vRefNum2drv(fs.vRefNum, dir_drive))
			err = nsvErr;
		else
			err = noErr;
	}

	if (err)
		goto was_doserr;

	vRefNum = fs.vRefNum;

	/* Wir holen uns jetzt die Informationen ueber das	*/
	/* letzte Pfadelement.							*/
	/* ---------------------------------------------------	*/

	pb.hFileInfo.ioVRefNum = fs.vRefNum;
	pb.hFileInfo.ioNamePtr = fs.name;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = fs.parID;
	if (!pb.hFileInfo.ioNamePtr[0])
		pb.hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync (&pb);

	if (err)
	{
		was_doserr:
		doserr = cnverr(err);
		was_doserr2:
		if (doserr == EFILNF)
		     doserr = EPTHNF;
		return(doserr);
	}

	/* ggf. Alias aufloesen */
	/* ------------------- */

	if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
	{
		doserr = resolve_symlink(&fs, 0, NULL );

		if (!doserr)		/* Der Mac hat dereferenziert! */
		{
			pb.hFileInfo.ioVRefNum = fs.vRefNum;
			pb.hFileInfo.ioNamePtr = fs.name;
			pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb.hFileInfo.ioDirID = fs.parID;
			if (!pb.hFileInfo.ioNamePtr[0])
				pb.hFileInfo.ioFDirIndex = -1;
			err = PBGetCatInfoSync (&pb);
			if ((!err) && (vRefNum2drv(fs.vRefNum, dir_drive)))
				err = nsvErr;
			if (err)
				return(cnverr(err));			// nix gefunden
		}
		else
			goto was_doserr2;		/* Fehler */
	}

	reldir = pb.hFileInfo.ioDirID;

	giveback:
	if (mode)                                       /* Verzeichnis */
	{
		if (!(pb.hFileInfo.ioFlAttrib & 0x10))
			return(EPTHNF);                         /* kein SubDir */
		*restpfad = pathname + strlen(pathname);     /* kein Restpfad */
		if (get_parent)
		{

			if (reldir == drv_dirID[drv])
			{	// bin schon root
				*symlink = NULL;
				return(ELINK);
			}

			err = FSMakeFSSpec(vRefNum, reldir, NULL, &fs);
			if (err)
				goto was_doserr;
			reldir = fs.parID;		// parent anwaehlen

			if (reldir == fsRtParID)
			{
				vRefNum = -32768;
				reldir = 0;
			}
		}
	}
	else
	{
		*restpfad = (char *) t;                               /* Restpfad */
	}

	dd->dirID = reldir;
	dd->vRefNum = vRefNum;

//	return(reldir);
	return(E_OK);
}


char CMacXFS::getArchiveMask (CInfoPBRec *pb)
{
	if ((pb->hFileInfo.ioFlAttrib & ioDirMask) ||			// falls es ein Dir ist …
		 (pb->hFileInfo.ioFlBkDat >= pb->hFileInfo.ioFlMdDat))	// oder seit der letzten Änderung gesichert wurde …
	{
		return 0;
	}
	else
	{
		return F_ARCHIVE;		// Archiv-Bit wird bei ungesicherten Dateien gesetzt.
	}
}

Byte CMacXFS::mac2DOSAttr (CInfoPBRec *pb)
// Attribute von Mac nach DOS konvertieren
// 17.6.96: wertet nun auch hidden-flag aus
{
	Byte attr;
	attr = (Byte) (pb->hFileInfo.ioFlAttrib & (F_RDONLY + F_SUBDIR));
	attr |= getArchiveMask (pb);
	if (pb->hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)	// 17.6.96 invisible?
		attr |= F_HIDDEN;
	return attr;
}

/*************************************************************
*
* Wird von xfs_sfirst und xfs_snext verwendet
* Unabhaengig von drv_longnames[drv] werden die Dateien
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

int32_t CMacXFS::_snext(uint16_t drv, MAC_DTA *dta)
{
	Str255 VolumeName;
	HVolumeParam pbh;
	FSSpec fs;		/* fuer Aliase */
	CInfoPBRec pb;
	int32_t doserr;
	OSErr err;
	unsigned char macname[256];		// Pascalstring Mac-Dateiname
	char atariname[256];			// C-String Atari-Dateiname (lang)
	unsigned char dosname[14], cmpname[14];	// intern, 8+3
	bool first = !drv_rvsDirOrder[drv] && !drv_readOnly[drv];


	DebugInfo("CMacXFS::_snext() -- pattern = %.11s", dta->macdta.sname);

	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);

	if ((dta->macdta.vRefNum == -32768) && (dta->macdta.dirID == 0))	// Mac-Root
	{
		pbh.ioNamePtr = VolumeName;

		for	(;;)
		{
			pbh.ioVolIndex = (short) dta->macdta.index;
			err = PBHGetVInfoSync ((HParmBlkPtr)&pbh);
			if (err)
				break;	// keine weiteren Volumes

			dta->macdta.index++;
			pstrcpy(macname, pbh.ioNamePtr);
			/* Datei gefunden, passen Name und Attribut ? */
			macname[macname[0]+1] = EOS;
			MacFnameToAtariFname(macname + 1,
							(unsigned char *) atariname);	// Umlaute wandeln
			if (conv_path_elem(atariname, (char *) dosname))		// Konvertier. fuer Vergleich
				continue;			/* Dateiname zu lang */

			dosname[11] = F_SUBDIR;

			if (filename_match(dta->macdta.sname, (char *) dosname))
			{
				pb.hFileInfo.ioFlMdDat = pbh.ioVLsMod;
				goto doit;
			}
		}

		dta->macdta.sname[0] = EOS;                  /* DTA ungueltig machen */
		return(ENMFIL);
	}

	/* suchen */
	/* ------ */

	pb.hFileInfo.ioNamePtr = macname;
	do
	{
		again:

		/* Ende des Verzeichnisses erreicht (index == 0):	*/
		/* --------------------------------------------	*/

		if (dta->macdta.index == 0)
		{
			dta->macdta.sname[0] = EOS;                  /* DTA ungueltig machen */
			return(EFILNF);
		}

		/* Verzeichniseintrag (PBREC) lesen.		*/
		/* --------------------------------	*/ 
    
		pb.hFileInfo.ioVRefNum = dta->macdta.vRefNum;
		pb.hFileInfo.ioFDirIndex = (short) dta->macdta.index;
		pb.hFileInfo.ioDirID = dta->macdta.dirID;
		/* notwendige Initialisierung fuer Verzeichnisse (sonst Muell!) */
		pb.hFileInfo.ioFlLgLen = 0;
		pb.hFileInfo.ioFlPyLen = 0;
		err = PBGetCatInfoSync(&pb);
		if (err)
		{
			DebugInfo("CMacXFS::_snext() -- PBGetCatInfoSync() => %d", err);
			dta->macdta.sname[0] = EOS;                  /* DTA ungueltig machen */
			return(cnverr(err));
		}

		if (drv_rvsDirOrder[drv])
		{
			dta->macdta.index--;
		}
		else
		{
			dta->macdta.index++;
		}

		/* Datei gefunden, passen Name und Attribut ? */
		/* geht nicht wegen Compilerfehler in 1.2.2: */
		/* conv_path_elem(cs(macname), dosname); */

		macname[macname[0] + 1] = EOS;
		MacFnameToAtariFname(macname + 1, (unsigned char *) atariname);	// Umlaute wandeln
/*
		if (atariname[0] == '.')
			DebugInfo("CMacXFS::_snext() -- name = %s", atariname);
*/
		DebugInfo("CMacXFS::_snext() -- directory entry found: \"%s\"", atariname);

		if (conv_path_elem(atariname, (char *) dosname))		// Konvertier. fuer Vergleich
		{
			DebugInfo("CMacXFS::_snext() -- skip long filename \"%s\"", atariname);
			goto again;			/* Dateiname zu lang */
		}

		/* Schon hier muessen wir eventuelle Aliase	*/
		/* dereferenzieren, damit der Attributvergleich	*/
		/* moeglich ist (Attr aus Datei, nicht Alias!)	*/
		/* -----------------------------------------	*/

		if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
		{
			DebugInfo("CMacXFS::_snext() -- dereference Alias %s", atariname);

			/* FSSpec erstellen und Alias dereferenzieren	*/
			/* ----------------------------------------	*/
			cfss(drv, dta->macdta.dirID, dta->macdta.vRefNum, macname + 1, &fs, false);
			doserr = resolve_symlink( &fs, 0, NULL );

			if (!doserr)		/* Der Mac hat dereferenziert! */
			{
				pb.hFileInfo.ioVRefNum = fs.vRefNum;
				pb.hFileInfo.ioNamePtr = fs.name;
				pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
				pb.hFileInfo.ioDirID = fs.parID;
				if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
				err = PBGetCatInfoSync (&pb);
				if (err)
				{
					dta->macdta.sname[0] = EOS;                  /* DTA ungueltig machen */
					return cnverr(err);
				}
				pb.hFileInfo.ioNamePtr = (unsigned char*)macname;
			}
		}

		dosname[11] = mac2DOSAttr(&pb);
		
		if (first)
		{
			first = false;
			if (nameto_8_3(macname + 1, cmpname, 1, true))
				goto again;		// mußte Dateinamen kürzen!

			if (!strncmp (dta->mxdta.dta_name, (char *) cmpname, 12))
				goto again;
		}
	}
	while (!filename_match(dta->macdta.sname, (char *) dosname));

	/* erfolgreich: DTA initialisieren. Daten liegen nur in der Data fork */
	/* ------------------------------------------------------------------ */

//	keine Ahnung, was das sollte:
//	if (!drv_rvsDirOrder[drv] && !drv_readOnly[drv])
//		--dta->macdta.index;		// letzten Eintrag das nächste Mal nochmal lesen!

doit:
	dta->mxdta.dta_attribute = (char) dosname[11];
	dta->mxdta.dta_len = cpu_to_be32((uint32_t) ((dosname[11] & F_SUBDIR) ? 0L : pb.hFileInfo.ioFlLgLen));
	/* Datum ist ioFlMdDat bzw. ioDrMdDat */
	date_mac2dos(pb.hFileInfo.ioFlMdDat,	(uint16_t *) &(dta->mxdta.dta_time),
								(uint16_t *) &(dta->mxdta.dta_date));

	dta->mxdta.dta_time = cpu_to_be16(dta->mxdta.dta_time);
	dta->mxdta.dta_date = cpu_to_be16(dta->mxdta.dta_date);

	nameto_8_3 (macname + 1, (unsigned char *) dta->mxdta.dta_name, 1, true);
	return(E_OK);
}


/*************************************************************
*
* Durchsucht ein Verzeichnis und merkt den Suchstatus
* für Fsnext.
* Der Mac benutzt für F_SUBDIR und F_RDONLY dieselben Bits
* wie der Atari.
*
*************************************************************/

int32_t CMacXFS::xfs_sfirst(uint16_t drv, MXFSDD *dd, char *name,
                    MAC_DTA *dta, uint16_t attrib)
{
	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

//	DebugInfo("CMacXFS::xfs_sfirst(%s, attrib = %d)", name, (int) attrib);

	/* Unschoenheit im Kernel ausbügeln: */
	dta->mxdta.dta_drive = (char) drv;

	conv_path_elem(name, dta->macdta.sname);	// Suchmuster -> DTA
	dta->mxdta.dta_name[0] = 0;		//  gefundenen Namen erstmal löschen
	dta->macdta.sattr = (char) attrib;			// Suchattribut
	dta->macdta.dirID = dd->dirID;
	dta->macdta.vRefNum = dd->vRefNum;

	if (drv_rvsDirOrder[drv])
	{
		CInfoPBRec pb;
		//Str255 name2;
		OSErr err;

		pb.hFileInfo.ioNamePtr = nil;
		pb.hFileInfo.ioVRefNum = drv_fsspec[drv].vRefNum;
		pb.hFileInfo.ioFDirIndex = -1; // get info of directory
		pb.hFileInfo.ioDirID = dd->dirID;
		err = PBGetCatInfoSync (&pb);
		if (err)
		{
			dta->macdta.sname[0] = EOS;                  // DTA ungueltig machen
			return cnverr (err);
		}
		dta->macdta.index = pb.dirInfo.ioDrNmFls;      // letzte Datei zuerst, dann weiter rückwärts
	}
	else
	{
		dta->macdta.index = 1;				// erste Datei
	}
	
	return(_snext(drv, dta));
}


/*************************************************************
*
* Durchsucht ein Verzeichnis weiter und merkt den Suchstatus
* fuer Fsnext.
*
*************************************************************/

int32_t CMacXFS::xfs_snext(uint16_t drv, MAC_DTA *dta)
{
	int32_t err;


	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	if (!dta->macdta.sname[0])
		return(ENMFIL);
	err = _snext(drv, dta);
	if (err == EFILNF)
		err = ENMFIL;
	return(err);
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

int32_t CMacXFS::xfs_fopen(char *name, uint16_t drv, MXFSDD *dd,
			uint16_t omode, uint16_t attrib)
{
	FSSpec fs;
	FInfo finfo;
	OSErr err;
	int32_t doserr;
	SignedByte perm;
	/* HParamBlockRec pb; */
	short refnum;
	unsigned char dosname[20];


#if DEBUG_68K_EMU
	if (!strcmp(name, ATARI_PRG_TO_TRACE))
		trigger = 1;
#endif
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_fopen(„%s“, drv = %d, omode = %d)", name, (int) drv, (int) omode);
	__dump((const unsigned char *) dd, sizeof(*dd));
#endif

#ifdef DEMO
	if (omode & O_CREAT)
	{
		(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
		return(EWRPRO);
	}
#endif

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	if (fname_is_invalid(name))
		return(EACCDN);

	if (!drv_longnames[drv])
	{
		// keine langen Namen: nach Großschrift und 8+3
		nameto_8_3 ((unsigned char *) name, dosname, 2, false);
		name = (char *) dosname;
	}

	// Note that dirID and vRefNum always are in the host's natural byte order,
	// so we do not have to endian-convert here
	cfss(drv, dd->dirID, dd->vRefNum, (unsigned char *) name, &fs, true);

	/* Datei erstellen, wenn noetig */
	/* ---------------------------- */

	if (omode & O_CREAT)
	{
		OSType creator, type;
		creator = MyCreator;
		type = 'TEXT';
		GetTypeAndCreator ((char*) fs.name + 1, &type, &creator);
		err = FSpCreate(&fs, creator, type, smSystemScript);

		if (err == dupFNErr)        /* Datei existiert schon */
		{
			if (omode & O_EXCL)
				return(EACCDN);
			err = FSpDelete(&fs);
			if (!err)
				err = FSpCreate(&fs, creator, type, smSystemScript);
		}

		/* Dateiattribute aendern, wenn noetig */
		/* ----------------------------------- */

		if ((!err) && (attrib & (F_RDONLY+F_HIDDEN)))
		{
			doserr = xfs_attrib(drv, dd, name, 1 /*write*/, attrib);
			if (doserr >= 0)
				doserr = 0;
			if (doserr)
				err = permErr;
		}

		if (err)
			return(cnverr(err));
	}


	/* Open-Modus festlegen. Problem: Der Mac hat unzureichende	*/
	/* Moeglichkeiten, den Modus zu spezifizieren. Deshalb z.B.:	*/

	/* exklusiv lesen, andere duerfen weder lesen noch schreiben	*/
	/* (OM_RPERM + OM_RDENY + OM_WDENY) => fsRdPerm			*/
	/*							(exklusiv lesen)		*/

	/* exklusiv schreiben, andere weder lesen noch schreiben		*/
	/* (OM_WPERM + OM_RDENY + OM_WDENY) => fsWrPerm		*/
	/*							(exclusive write)		*/

	/* gemeinsames Lesen, andere duerfen nicht schreiben		*/
	/* (OM_RPERM + OM_WDENY) => fsRdWrShPerm				*/
	/*	(!! ACHTUNG)				(shared read+write)	*/

	/*
		Genauer: Wenn der Atari nicht WDENY _und_ RDENY angibt, darf
		die Datei z.B. von anderen Prozessen gelesen werden.
		Deshalb wird dann immer fsRdWrShPerm ausgefuehrt. Damit sind
		saemtliche Sicherheitsmechanismen zum Teufel, da andere Prozesse
		nun auch schreiben duerfen.
	*/

	if ((omode & (OM_RDENY+OM_WDENY)) == (OM_RDENY+OM_WDENY))
		// niemand sonst darf lesen oder schreiben
	{
		if ((omode & (OM_WPERM+OM_RPERM)) == OM_WPERM+OM_RPERM)
			perm = fsRdWrPerm;	// Lesen und Schreiben
		else
		if (omode & OM_WPERM)
			perm = fsWrPerm;		// kein Lesen, nur Schreiben
		else
			perm = fsRdPerm;		// nur Lesen, kein Schreiben
	}
	else
	// andere dürfen auch Lesen und/oder Schreiben
	{
		perm = fsRdWrShPerm;
	}

	/* Bevor wir irgendetwas treiben, muessen wir feststellen,	*/
	/* ob es sich bei der Datei um einen Alias handelt!		*/
	/* ------------------------------------------------------ */

	err = FSpGetFInfo (&fs, &finfo);
	if (err)
		return(cnverr(err));
	if (finfo.fdFlags & kIsAlias)
	{
		doserr = resolve_symlink( &fs, 0, NULL );
		if (doserr)
			return(doserr);
	}

	err = FSpOpenDF(&fs, perm, &refnum);

	if ((cnverr (err) == EACCDN) && (perm == fsRdWrShPerm) && ((omode & OM_WPERM) == 0))
		// Datei ist entw. schreibgeschützt oder bereits zum exkl. Lesen/Schreiben geöffnet.
		// Da der User nur lesen wollte, probieren wir´s deshalb mal mit exkl. Lesen
		// für den Fall, daß die Datei lediglich schreibgeschützt ist (mehrere exkl.
		// Leser erlaubt das Mac-FS nämlich).
	{
		if (FSpOpenDF(&fs, fsRdPerm, &refnum) == noErr)
			err = 0;	// hat geklappt!
	}

/*
	pb.ioParam.ioVRefNum = drv_fsspec[drv].vRefNum;
	pb.ioParam.ioNamePtr = fs.name;
	pb.ioParam.ioPermssn = perm;
	pb.ioParam.ioMisc = 0;
	pb.fileParam.ioDirID = dirID;
	err = PBHOpen(&pb, FALSE);
*/
	if (err)
		return(cnverr(err));

	if (omode & O_TRUNC)
	{
		err = SetEOF(refnum, 0L);
		if (err)
		{
			FSClose(refnum);
			return(cnverr(err));
		}
	}

#if DEBUG_68K_EMU
	if (trigger == 1)
	{
		trigger_refnum = refnum;
		trigger++;
	}
#endif

	return cpu_to_be16((unsigned short) (refnum));
}


/*************************************************************
*
* Löscht eine Datei.
*
* Aliase werden NICHT dereferenziert, d.h. es wird der Alias
* selbst gelöscht.
*
*************************************************************/

int32_t CMacXFS::xfs_fdelete(uint16_t drv, MXFSDD *dd, char *name)
{
#ifdef DEMO
	#pragma unused(drv, dd, name)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);
#else
	FSSpec fs;
	OSErr err;

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

//	DebugInfo("CMacXFS::xfs_fdelete(name = %s)", name);
	cfss(drv, dd->dirID, dd->vRefNum, (unsigned char *) name, &fs, true);
	err = FSpDelete(&fs);
	return(cnverr(err));
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

int32_t CMacXFS::xfs_link(uint16_t drv, char *nam1, char *nam2,
               MXFSDD *dd1, MXFSDD *dd2, uint16_t mode, uint16_t dst_drv)
{
#ifdef DEMO
	#pragma unused(drv, nam1, nam2, dd1, dd2, mode, dst_drv)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);
#else
	long doserr;
	FSSpec fs1,fs2;
	FInfo fi;
	Str63 name;
	OSErr err;
	int diffnames;
	unsigned char tempname[256];


	memcpy(tempname, "\pmmac&&&", 9);

	if ((drv_changed[drv]) || (drv_changed[dst_drv]))
		return(E_CHNG);
	if ((!drv_fsspec[drv].vRefNum) || (!drv_fsspec[dst_drv].vRefNum))
		return(EDRIVE);
	if (drv_fsspec[drv].vRefNum != drv_fsspec[dst_drv].vRefNum)
		return(ENSAME);		// !AK: 23.1.97
	// auf demselben Volume?
	if (dd1->vRefNum != dd2->vRefNum)
		return(ENSAME);
	if (fname_is_invalid(nam2))
		return(EACCDN);

	if (mode)
		return(EINVFN);          // keine Hardlinks

	/* Beide Namen ins Mac-Format wandeln */

	doserr = cfss(drv, dd1->dirID, dd1->vRefNum, (unsigned char *) nam1, &fs1, true);
	if (doserr)
		return doserr;
	doserr = cfss(dst_drv, dd2->dirID, dd2->vRefNum, (unsigned char *) nam2, &fs2, true);
	if (doserr)
		return doserr;

	diffnames = strcmp((char *) fs1.name + 1, (char *) fs2.name + 1);

	again:
	fs1.parID = dd1->dirID;		// Quellname

	if (dd1->dirID == dd2->dirID)			// rename
	{
		err = FSpRename(&fs1, fs2.name);

		// T&C setzen:
		if (!err)
		{
			FSpGetFInfo (&fs2, &fi);
			GetTypeAndCreator ((char *) fs2.name + 1, &fi.fdType, &fi.fdCreator);
			fi.fdCreator = MyCreator ;
			fi.fdType = 'TEXT';
			FSpSetFInfo (&fs2, &fi);
		}

	}
	else
	{					// move
		CMovePBRec r;


		// Bei unterschiedlichen Namen muessen wir erst testen,
		// ob die Zieldatei im Zielverzeichnis existiert
		// ----------------------------------------------------

		if (diffnames)
		{
			err = FSpGetFInfo (&fs2, &fi);
			if (err != fnfErr)		// file not found ??
			{
				if (!err)			// Datei existiert => EACCDN
					err = permErr;
				goto ende;			// Aufruf abbrechen
			}
		}

		/* Wir verschieben jetzt die Quelldatei mit demselben Namen	*/
		/* ins neue Verzeichnis. Das kann natürlich schief gehen. 		*/
		/* -----------------------------------------------------------	*/

		r.ioNamePtr = fs1.name;
		r.ioVRefNum = fs1.vRefNum;
		r.ioDirID = fs1.parID;
		r.ioNewName = nil;		// Zielverzeichnis: ioNewDirID
		r.ioNewDirID = dd2->dirID;
		err = PBCatMoveSync(&r);

		if (!diffnames)		// umbenennen ?
			goto ende;			// nein, wir sind fertig

		// Wenn beim Verschieben der Quellname im Zielverzeichnis
		// schon existiert, muessen wir erst die Quelldatei in
		// einen Temporaernamen umbenennen, sie dann in das
		// Zielverzeichnis schieben und dort umbenennen
		// ------------------------------------------------------

		if (err == dupFNErr)
		{
			err = FSpRename (&fs1, tempname);
			if (!err)
			{
				r.ioNamePtr = tempname;
				err = PBCatMoveSync(&r);

				if (!err)
				{
					// Datei liegt mit Temporaernamen im Zielverzeichnis
					dd1->dirID = dd2->dirID;
					strcpy((char *) fs1.name, (char *) tempname);	// Quellname
					goto again;
				}
				else
				{

					// Verschieben ging nicht.
					// Datei wieder in fs1.name umbenennen
					// -----------------------------------

					strcpy((char *) name, (char *) fs1.name);
					strcpy((char *) fs1.name, (char *) tempname);
					FSpRename(&fs1, name);
				}
			}
		}

		// Wenn das Verschieben geklappt hat und die Zieldatei
		// einen anderen Namen als die Quelldatei hat, muessen wir
		// sie nach dem Verschieben noch umbenennen.
		// -------------------------------------------------------

		if (!err)				// move and rename
		{
			dd1->dirID = dd2->dirID;		// Quelldatei jetzt in dirID2
			goto again;			// zum Umbenennen
		}
	}
ende:
	return(cnverr(err));
#endif
}


/*************************************************************
*
* Wandelt CInfoPBRec => XATTR
*
* (Fuer Fxattr und Dxreaddir)
*
*************************************************************/

void CMacXFS::cinfo_to_xattr(CInfoPBRec * pb, XATTR *xattr, uint16_t drv)
{
#pragma unused(drv)
	xattr->attr = cpu_to_be16(mac2DOSAttr(pb));

	if (pb->hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
		xattr->mode = cpu_to_be16(_ATARI_S_IFLNK);		// symlink
	else
	if (xattr->attr & cpu_to_be16(F_SUBDIR))
		xattr->mode = cpu_to_be16(_ATARI_S_IFDIR);		// subdir
	else
		xattr->mode = cpu_to_be16(_ATARI_S_IFREG);		// regular file

		

	xattr->mode |= cpu_to_be16(0777);				// rwx fuer user/group/world
	if (xattr->attr & cpu_to_be16(F_RDONLY))
		xattr->mode &= cpu_to_be16(~(0222));		// Schreiben verboten

	xattr->index = cpu_to_be32((uint32_t) pb->hFileInfo.ioDirID);
	xattr->dev = cpu_to_be16((uint16_t) pb->hFileInfo.ioVRefNum);
	xattr->reserved1 = 0;
	xattr->nlink = cpu_to_be16(1);
	xattr->uid = xattr->gid = 0;
	// F_SUBDIR-Abfrage ist unbedingt nötig!
	xattr->size = cpu_to_be32((uint32_t) ((be16_to_cpu(xattr->attr) & F_SUBDIR) ?
								0 : pb->hFileInfo.ioFlLgLen));	// Log. Laenge Data Fork
	xattr->blksize = cpu_to_be32(512);			// ?????
	xattr->nblocks = cpu_to_be32((uint32_t) (pb->hFileInfo.ioFlPyLen / 512));	// Phys. Länge / Blockgröße
	date_mac2dos(pb->hFileInfo.ioFlMdDat, &(xattr->mtime), &(xattr->mdate));
	xattr->mtime = cpu_to_be16(xattr->mtime);
	xattr->mdate = cpu_to_be16(xattr->mdate);
	xattr->atime = xattr->mtime;
	xattr->adate = xattr->mdate;
	date_mac2dos(pb->hFileInfo.ioFlCrDat, &(xattr->ctime), &(xattr->cdate));
	xattr->ctime = cpu_to_be16(xattr->ctime);
	xattr->cdate = cpu_to_be16(xattr->cdate);
	xattr->reserved2 = 0;
	xattr->reserved3[0] = xattr->reserved3[1] = 0;
}


/*************************************************************
*
* Fuer Fxattr
*
* MODE == 0: Folge Symlinks
*
*************************************************************/

int32_t CMacXFS::xfs_xattr(uint16_t drv, MXFSDD *dd, char *name,
				XATTR *xattr, uint16_t mode)
{
	FSSpec fs;
	CInfoPBRec pb;
	OSErr err;
	int32_t doserr;
	unsigned char fname[64];


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_xattr(drv = %d, „%s“, mode = 0x%04x)", (int) drv, name, (int) mode);
	__dump((const unsigned char *) dd, sizeof(*dd));
//	__dump((const unsigned char *) xattr, sizeof(*xattr));
#endif

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	if (drv_longnames[drv])
	{
		if ((name[0] == '.') && (!name[1]))
			fname[1] = EOS;		// "." wie leerer Name
		else
			AtariFnameToMacFname((unsigned char *) name, fname + 1);
	}
	else
	{
		nameto_8_3((unsigned char *) name, fname + 1, 1, false);
		AtariFnameToMacFname(fname + 1, fname + 1);
	}
	sp((char *) fname);

	if ((dd->vRefNum == -32768) && (dd->dirID == 0))	// Mac-Root
	{
//		DebugStr(fname);
		if (fname[1])
		{
			strcat((char *) (fname + 1), ":");		// MacOS-Unschönheit ausbügeln
			fname[0]++;
			err = FSMakeFSSpec(0, 0, fname, &fs);	// abs. Pfad
			if (err)
				return(cnverr(err));
			xattr->attr = cpu_to_be16(F_SUBDIR);			// Volume-Root
			xattr->mode = cpu_to_be16(S_IFDIR + 0777);	// rwx fuer user/group/world
			xattr->dev = cpu_to_be16((uint16_t) fs.vRefNum);
			xattr->index = cpu_to_be32(fsRtDirID);		// Volume-Root
		}
		else
		{
			xattr->attr = cpu_to_be16(F_SUBDIR + F_RDONLY);
			xattr->mode = cpu_to_be16(S_IFDIR + 0555);	// r-x fuer user/group/world
			xattr->dev = cpu_to_be16((uint16_t) dd->vRefNum);
			xattr->index = cpu_to_be32(fsRtParID);		// Wurzel
		}

		xattr->reserved1 = 0;
		xattr->nlink = cpu_to_be16(1);
		xattr->uid = xattr->gid = 0;
		xattr->size = 0;
		xattr->blksize = cpu_to_be16(512);
		xattr->nblocks = 0;
		xattr->mtime = 0;		// kein Datum/Uhrzeit
		xattr->mdate = 0;
		xattr->atime = xattr->mtime;
		xattr->adate = xattr->mdate;
		xattr->ctime = 0;
		xattr->cdate = 0;
		xattr->reserved2 = 0;
		xattr->reserved3[0] = xattr->reserved3[1] = 0;
		return(E_OK);
	}

	pb.hFileInfo.ioNamePtr = fname;
	pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
	pb.hFileInfo.ioDirID = dd->dirID;
	pb.hFileInfo.ioVRefNum = dd->vRefNum;
	/* notwendige Initialisierung fuer Verzeichnisse (sonst Muell!) */
	pb.hFileInfo.ioFlLgLen = 0;
	pb.hFileInfo.ioFlPyLen = 0;
	if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync (&pb);
	if (err)
		return(cnverr(err));

	/* Im Modus 0 muessen Aliase dereferenziert werden	*/
	/* ------------------------------------------------	*/

	doserr = E_OK;
	if ((!mode) && (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias))
	{

		/* FSSpec erstellen und Alias dereferenzieren	*/
		/* ----------------------------------------	*/

		cfss(drv, dd->dirID, dd->vRefNum, fname + 1, &fs, false);
		doserr = resolve_symlink( &fs, 0, NULL );

		if (!doserr)		/* Der Mac hat dereferenziert! */
		{
			pb.hFileInfo.ioVRefNum = fs.vRefNum;
			pb.hFileInfo.ioNamePtr = fs.name;
			pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb.hFileInfo.ioDirID = fs.parID;
			if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
			err = PBGetCatInfoSync (&pb);
			if (err)
				return cnverr(err);
		}
	}

	if (!doserr)
		cinfo_to_xattr(&pb, xattr, drv);
	return(doserr);
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

int32_t CMacXFS::xfs_attrib(uint16_t drv, MXFSDD *dd, char *name, uint16_t rwflag, uint16_t attr)
{
	FSSpec fs;
	CInfoPBRec pb;
	OSErr err;
	int32_t doserr;
	unsigned char fname[64];
	int oldattr;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_attrib(„%s“, drv = %d, wrmode = %d, attr = 0x%04x)", name, (int) drv, (int) rwflag, (int) attr);
#endif
	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

#ifdef DEMO
	if (rwflag)
	{
		(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
		return(EWRPRO);
	}
#endif
	
	if (drv_longnames[drv])
	{
		AtariFnameToMacFname((unsigned char *) name, fname + 1);
	}
	else
	{
		nameto_8_3 ((unsigned char *) name, fname + 1, 1, false);
		AtariFnameToMacFname(fname + 1, fname + 1);
	}

	sp((char *) fname);
	pb.hFileInfo.ioVRefNum = dd->vRefNum;
	pb.hFileInfo.ioNamePtr = (unsigned char *) fname;
	pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
	pb.hFileInfo.ioDirID = dd->dirID;
	if (pb.hFileInfo.ioNamePtr[0] == 0)
		pb.hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync(&pb);
	if (err)
		return(cnverr(err));

	/* Aliase muessen dereferenziert werden	*/
	/* -------------------------------------	*/

	if (pb.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
	{
		cfss(drv, dd->dirID, dd->vRefNum, fname+1, &fs, false);
		doserr = resolve_symlink( &fs, 0, NULL );

		if (!doserr)		/* Der Mac hat dereferenziert! */
		{
			pb.hFileInfo.ioVRefNum = fs.vRefNum;
			pb.hFileInfo.ioNamePtr = fs.name;
			pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb.hFileInfo.ioDirID = fs.parID;
			if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
			err = PBGetCatInfoSync (&pb);
			if (err)
				return cnverr(err);
		}
		else
		{
			return(doserr);		/* Fehler */
		}
	}

	/* Normale Datei, oder Alias ist dereferenziert	*/
	/* ------------------------------------------	*/

	oldattr = mac2DOSAttr (&pb);
	if (rwflag)
	{
		if (pb.hFileInfo.ioFlAttrib & ioDirMask)
		{			// !AK 4.10.98
			/* Bei Verzeichnissen ist nur HIDDEN änderbar */
			if ((attr & ~F_HIDDEN) != (oldattr & ~F_HIDDEN))
				err = permErr;
		}

		pb.hFileInfo.ioDirID = pb.hFileInfo.ioFlParID;
		if (!err && (oldattr & F_RDONLY) != (attr & F_RDONLY))
		{
			if (attr & F_RDONLY)
				err = PBHSetFLockSync((HParmBlkPtr)&pb);
			else
				err = PBHRstFLockSync((HParmBlkPtr)&pb);
		}
		
		// Archiv-Bit:
		if (!err && (oldattr & F_ARCHIVE) != (attr & F_ARCHIVE))
		{
			if (attr & F_ARCHIVE)
			{
				if (pb.hFileInfo.ioFlBkDat == pb.hFileInfo.ioFlMdDat)
					pb.hFileInfo.ioFlBkDat = 0;
			}
			else
				pb.hFileInfo.ioFlBkDat = pb.hFileInfo.ioFlMdDat;
			err = PBSetCatInfoSync (&pb);
		}
		
		// !AK 6.2.99: Hidden-Bit auch für Ordner
		if (!err && (oldattr & F_HIDDEN) != (attr & F_HIDDEN))
		{
			if (attr & F_HIDDEN)
				pb.hFileInfo.ioFlFndrInfo.fdFlags |= kIsInvisible;
			else
				pb.hFileInfo.ioFlFndrInfo.fdFlags &= ~kIsInvisible;
			err = PBSetCatInfoSync (&pb);
		}
	}

	if (err)
		return(cnverr(err));
	return((int32_t) oldattr);
}


/*************************************************************
*
* Fuer Fchown
*
*************************************************************/

int32_t CMacXFS::xfs_fchown(uint16_t drv, MXFSDD *dd, char *name,
                    uint16_t uid, uint16_t gid)
{
#pragma unused(drv, dd, name, uid, gid)
	return(EINVFN);
}


/*************************************************************
*
* Fuer Fchmod
*
*************************************************************/

int32_t CMacXFS::xfs_fchmod(uint16_t drv, MXFSDD *dd, char *name, uint16_t fmode)
{
#pragma unused(drv, dd, name, fmode)
	return(EINVFN);
}


/*************************************************************
*
* Fuer Dcreate
*
*************************************************************/

int32_t CMacXFS::xfs_dcreate(uint16_t drv, MXFSDD *dd, char *name)
{
	FSSpec fs;
	OSErr err;
	long ndirID;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_dcreate(„%s“, drv = %d)", name, (int) drv);
#endif
	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);
	if (fname_is_invalid(name))
		return(EACCDN);

#ifdef DEMO

	#pragma unused(fs, err, ndirID)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);

#else

	cfss(drv, dd->dirID, dd->vRefNum, (unsigned char *) name, &fs, true);
	err = FSpDirCreate(&fs, -1, &ndirID);

	// Versuch, das Inkonsistenzproblem bei OS X 10.2 zu beheben
/*
	if (!err)
		FlushVol(NULL, dd->vRefNum);
*/
	return(cnverr(err));

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

int32_t CMacXFS::xfs_ddelete(uint16_t drv, MXFSDD *dd)
{
#ifdef DEMO

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);
	
	#pragma unused(dd)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);
	
#else

	FSSpec fs;
	OSErr err;

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	err = FSMakeFSSpec(dd->vRefNum, dd->dirID, NULL, &fs);
	if (!err)
		err = HDelete(fs.vRefNum, fs.parID, fs.name);
	return(cnverr(err));

#endif
}


/*************************************************************
*
* Fuer Dgetpath
*
*************************************************************/

int32_t CMacXFS::xfs_DD2name(uint16_t drv, MXFSDD *dd, char *buf, uint16_t bufsiz)
{
	FSSpec fs;
	OSErr err;
	int32_t doserr;
	uint16_t len;
	MXFSDD par_dd;

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	if (drv == 'M'-'A')
	{
// DebugStr("\phallo");
		if (dd->vRefNum == -32768)	// Mac-Root
			goto root;
		if (dd->dirID == fsRtDirID)			// Wurzelverzeichnis eines Volumes
		{
			HVolumeParam pb;

			pb.ioVolIndex = 0;
			pb.ioVRefNum = dd->vRefNum;
			pb.ioNamePtr = fs.name;
			err = PBHGetVInfoSync ((HParamBlockRec*)&pb);
			if (err)
				return(cnverr(err));
			len = fs.name[0];
			if (len > bufsiz)
				return(ATARIERR_ERANGE);
			goto doit;
		}
	}
	if (dd->dirID == drv_dirID[drv])          /* root ? */
	{
		root:
		len = 1;
		if (len > bufsiz)
			return(ATARIERR_ERANGE);
		buf[0] = EOS;
		return(E_OK);
	}
	err = FSMakeFSSpec(dd->vRefNum, dd->dirID, NULL, &fs);
	if (err)
		return(cnverr(err));
	len = fs.name[0];
	if (len > bufsiz)
		return(ATARIERR_ERANGE);
	par_dd.vRefNum = dd->vRefNum;
	par_dd.dirID = fs.parID;
	doserr = xfs_DD2name(drv, &par_dd, buf, (uint16_t) (bufsiz - len));
	if (doserr)
		return(doserr);

	doit:
	if (buf[strlen(buf)-1] != '\\')
		strcat(buf, "\\");
	fs.name[fs.name[0]+1] = EOS;

// Umlaute konvertieren 9.6.96

	while (*buf)
		buf++;
	MacFnameToAtariFname(fs.name+1, (unsigned char *) buf);

//	alte Version: strcat(buf, (char*)fs.name+1);

	return(E_OK);
}


/*************************************************************
*
* Fuer Dopendir.
*
* Aliase brauchen hier nicht dereferenziert zu werden, weil
* dies bereits bei path2DD haette passieren muessen.
*
*************************************************************/

int32_t CMacXFS::xfs_dopendir(MAC_DIRHANDLE *dirh, uint16_t drv, MXFSDD *dd,
				uint16_t tosflag)
{
	dirh -> dirID = dd->dirID;
	dirh -> vRefNum = dd->vRefNum;
	dirh -> tosflag = tosflag;

	if ((dd->vRefNum == -32768) && (dd->dirID == 0))	// Mac-Root
		dirh->index = 1;
	else
	if (drv_rvsDirOrder[drv])
	{
		CInfoPBRec pb;
		//Str255 name2;
		OSErr err;
		
		pb.hFileInfo.ioNamePtr = nil;
		pb.hFileInfo.ioVRefNum = dd->vRefNum;
		pb.hFileInfo.ioDirID = dd->dirID;
		pb.hFileInfo.ioFDirIndex = -1; // get info of directory
		err = PBGetCatInfoSync(&pb);
		if (err)
		{
			return cnverr(err);
		}
		dirh -> index = pb.dirInfo.ioDrNmFls;      // letzte Datei zuerst, dann weiter rückwärts
	}
	else
	{
		dirh -> index = 1;
	}

	return(E_OK);
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
	Str255 VolumeName;
	HVolumeParam pbh;
	CInfoPBRec pb;
	OSErr err;
	unsigned char macname[256];


	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);

	again:
	if (dirh->index == 0)
	{
		return(ENMFIL);
	}

	if ((dirh->vRefNum == -32768) && (dirh->dirID == 0))	// Mac-Root
	{
		pbh.ioNamePtr = VolumeName;
		for	(;;)
		{
			pbh.ioVolIndex = (short) dirh->index;
			err = PBHGetVInfoSync ((HParmBlkPtr)&pbh);
			if (err)
				break;	// keine weiteren Volumes
			pstrcpy(macname, pbh.ioNamePtr);
			if (xattr)
			{
				xattr->attr = cpu_to_be16(F_SUBDIR);
				xattr->mode = cpu_to_be16(S_IFDIR);				// subdir
				xattr->mode |= cpu_to_be16(0777);				// rwx fuer user/group/world
				xattr->index = 0;
				xattr->dev = cpu_to_be16((uint16_t) pbh.ioVRefNum);
				xattr->reserved1 = 0;
				xattr->nlink = cpu_to_be16(1);
				xattr->uid = xattr->gid = 0;
				xattr->size = 0;
				xattr->blksize = cpu_to_be32((uint32_t) pbh.	ioVAlBlkSiz);
				xattr->nblocks = cpu_to_be32(pbh.ioVNmAlBlks);
				date_mac2dos(pbh.ioVLsMod, &(xattr->mtime), &(xattr->mdate));
				xattr->mtime = cpu_to_be16(xattr->mtime);
				xattr->mdate = cpu_to_be16(xattr->mdate);
				xattr->atime = xattr->mtime;
				xattr->adate = xattr->mdate;
				date_mac2dos(pbh.ioVCrDate, &(xattr->ctime), &(xattr->cdate));
				xattr->ctime = cpu_to_be16(xattr->ctime);
				xattr->cdate = cpu_to_be16(xattr->cdate);
				xattr->reserved2 = 0;
				xattr->reserved3[0] = xattr->reserved3[1] = 0;
				*xr = cpu_to_be32(E_OK);
				xattr = NULL;
			}
			dirh->index++;
			goto doit;
		}
		dirh->index = 0;	// ungültig machen
		return(ENMFIL);
	}

	/* suchen */
	/* ------ */

	pb.hFileInfo.ioNamePtr = (unsigned char *) macname;
	pb.hFileInfo.ioVRefNum = dirh->vRefNum;
	if (drv_rvsDirOrder[drv])
	{
		// scan index num, num - 1, num - 2, ..., 1
		pb.hFileInfo.ioFDirIndex = (short) dirh->index--;
	}
	else
	{
		// scan index 1, 2, ..., num
		pb.hFileInfo.ioFDirIndex = (short) dirh->index++;
	}
	pb.hFileInfo.ioDirID = dirh->dirID;
	/* notwendige Initialisierung fuer Verzeichnisse (sonst Muell!) */
	pb.hFileInfo.ioFlLgLen = 0;
	pb.hFileInfo.ioFlPyLen = 0;
	err = PBGetCatInfoSync(&pb);		// hier später mal asynchron

	// Carbon-Fehler von MacOS X bei Wurzelverzeichnis einer Netzwerkverbindung
	// umgehen (Anzahl Dateien ist um 1 zu hoch!)
	if ((err == fnfErr) && (drv_rvsDirOrder[drv]) && (dirh->index > 0))
	{
		DebugError("CMacXFS::xfs_dreaddir() -- Bug im MacOS. Versuche zu umgehen (1. Wdh)");
		// wir machen es einfach nochmal
		pb.hFileInfo.ioFDirIndex = (short) dirh->index--;
		err = PBGetCatInfoSync (&pb);		// hier später mal asynchron
	}
	if ((err == fnfErr) && (drv_rvsDirOrder[drv]) && (dirh->index > 0))
	{
		DebugError("CMacXFS::xfs_dreaddir() -- Bug im MacOS. Versuche zu umgehen (2. Wdh)");
		// wir machen es einfach ein drittes Mal
		pb.hFileInfo.ioFDirIndex = (short) dirh->index--;
		err = PBGetCatInfoSync (&pb);		// hier später mal asynchron
	}
	if ((err == fnfErr) && (drv_rvsDirOrder[drv]) && (dirh->index > 0))
	{
		DebugError("CMacXFS::xfs_dreaddir() -- Bug im MacOS. Versuche zu umgehen (3. Wdh)");
		// wir machen es einfach ein viertes Mal
		pb.hFileInfo.ioFDirIndex = (short) dirh->index--;
		err = PBGetCatInfoSync (&pb);		// hier später mal asynchron
	}

	if (err)
	{
		dirh->index = 0;                  /* dirh ungueltig machen */
		if (err == fnfErr)
			return(ENMFIL);		// special error code for D(x)readdir() Fsnext()
		else
			return(cnverr(err));
	}

	doit:
	macname[macname[0] + 1] = EOS;
	if (dirh->tosflag)
	{
		if (size < 13)
			return(ATARIERR_ERANGE);
		if (nameto_8_3(macname + 1, (unsigned char *) buf, 1, true))
			goto again;		// musste Dateinamen kuerzen
	}
	else
	{

		// OS X verwendet die Dateien .DS_Store, um irgendwelche Attribute
		// zu verwalten. Die Dateien sollten ausgeblendet werden.

		if (!memcmp(macname, "\p.DS_Store", 10))
			goto again;
		if (size < macname[0] + 5)
			return(ATARIERR_ERANGE);
		strncpy(buf, (char *) &(pb.hFileInfo.ioDirID), 4);
		buf += 4;
		MacFnameToAtariFname(macname + 1, (unsigned char *) buf);

/*
		if (buf[0] == '.')
			DebugInfo("CMacXFS::xfs_dreaddir() -- name = %s", buf);
*/
	}

	if (xattr)
	{
		cinfo_to_xattr( &pb, xattr, drv);
		*xr = cpu_to_be32(E_OK);
	}

	return(E_OK);
}


/*************************************************************
*
* Fuer Drewinddir
*
*************************************************************/

int32_t CMacXFS::xfs_drewinddir(MAC_DIRHANDLE *dirh, uint16_t drv)
{
	if (drv_rvsDirOrder[drv])
		return(xfs_dopendir(dirh, drv, (MXFSDD*) (&dirh->dirID), dirh->tosflag));
	dirh -> index = 1;
	return(E_OK);
}


/*************************************************************
*
* Fuer Dclosedir
*
*************************************************************/

int32_t CMacXFS::xfs_dclosedir(MAC_DIRHANDLE *dirh, uint16_t drv)
{
#pragma unused(drv)
	dirh -> dirID = -1L;
	return(E_OK);
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
*                  error value ERANGE is  returned  from  that  system
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
		case	DP_MAXREQ:	return(DP_XATTRFIELDS);
		case	DP_IOPEN:	return(100);	// ???
		case	DP_MAXLINKS:	return(1);
		case	DP_PATHMAX:	return(128);
		case	DP_NAMEMAX:	return((drv_longnames[drv]) ? 31 : 12);
		case	DP_ATOMIC:	return(512);	// ???
		case	DP_TRUNC:	return((drv_longnames[drv]) ? DP_AUTOTRUNC : DP_DOSTRUNC);
		case	DP_CASE:		return((drv_longnames[drv]) ? DP_CASEINSENS : DP_CASECONV);
		case	DP_MODEATTR:	return(F_RDONLY+F_SUBDIR+F_ARCHIVE+F_HIDDEN
							+DP_FT_DIR+DP_FT_REG+DP_FT_LNK);
		case	DP_XATTRFIELDS:return(DP_INDEX+DP_DEV+DP_NLINK+DP_BLKSIZE
							+DP_SIZE+DP_NBLOCKS
							+DP_CTIME+DP_MTIME);
	}
	return(EINVFN);
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

int32_t CMacXFS::xfs_dfree(uint16_t drv, int32_t dirID, uint32_t data[4])
{
#pragma unused(dirID)
	XVolumeParam xpb;		// für Platten > 2G
	union
	{
		unsigned long long bytes;
		UnsignedWide wbytes;
	} wbu;
	OSErr err;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_dfree(drv = %d, dirID = 0x%08x)", (int) drv, (int) dirID);
#endif

	if (drv_type[drv] == MacRoot)
	{
		// 2G frei, d.h. 4 M Blöcke à 512 Bytes
		data[0] = cpu_to_be32((2 * 1024) * 2 * 1024);	// # freie Blöcke
		data[1] = cpu_to_be32((2 * 1024) * 2 * 1024);	// # alle Blöcke
		data[2] = cpu_to_be32(512);					// Sektorgröße in Bytes
		data[3] = cpu_to_be32(1);						// Sektoren pro Cluster
		return(E_OK);
	}

	if (drv_changed[drv])
		return(E_CHNG);

	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	xpb.ioVolIndex = 0;       /* ioRefNum ist gueltig */
	xpb.ioVRefNum = drv_fsspec[drv].vRefNum;
	xpb.ioNamePtr = NULL;
	err = PBXGetVolInfo(&xpb, 0);			// hier später mal asynchron
	if (err)
		return(cnverr(err));

//		wbu.wbytes = xpb.ioVFreeBytes;		// CWPRO 3
	wbu.bytes = xpb.ioVFreeBytes;		// CWPRO 5
	data[0] = cpu_to_be32((uint32_t) (wbu.bytes / xpb.ioVAlBlkSiz));	// # freie Blöcke

//		wbu.wbytes = xpb.ioVTotalBytes;		// CWPRO 3
	wbu.bytes = xpb.ioVTotalBytes;		// CWPRO 5
	data[1] = cpu_to_be32((uint32_t) (wbu.bytes / xpb.ioVAlBlkSiz));	// # alle Blöcke
	data[2] = cpu_to_be32(xpb.ioVAlBlkSiz);				// Bytes pro Sektor
	data[3] = cpu_to_be32(1);								// Sektoren pro Cluster
	return(E_OK);
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
	return(EINVFN);
}

int32_t CMacXFS::xfs_rlabel(uint16_t drv, MXFSDD *dd, char *name, uint16_t bufsiz)
{
#pragma unused(dd)
	HVolumeParam pb;
	unsigned char buf[256];
	OSErr err;

	if (drv_type[drv] == MacRoot)
		return(EFILNF);
	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	pb.ioVolIndex = 0;       /* ioRefNum ist gueltig */
	pb.ioVRefNum = drv_fsspec[drv].vRefNum;
	pb.ioNamePtr = buf;
	err = PBHGetVInfoSync ((HParmBlkPtr) &pb);	// hier später mal asychron
	if (err)
		return(cnverr(err));
	buf[buf[0] + 1] = EOS;
	if (buf[0] + 1 > bufsiz)
		return(ATARIERR_ERANGE);
	MacFnameToAtariFname(buf + 1, (unsigned char *) name);
	return(E_OK);
}


/*************************************************************
*
* Wandelt einen DOS-Pfad in das Mac-Aequivalent um. Diese
* Funktion wird verwendet, um dem Finder zu ermoeglichen,
* Aliase zu verwenden, die von MagicMac erstellt wurden.
*
* Der Mac-Name wird ganz normal als NUL-terminierter String
* erstellt, nicht als Pascal-String.
*
*************************************************************/

OSErr CMacXFS::PathNameFromDirID( long dirid, short vrefnum,
	char *fullpathname)
{
	DirInfo block;
	Str255 dirname;
	char dirnamec[255];
	OSErr err;

	fullpathname[0] = EOS;
	block.ioDrParID = dirid;
	block.ioNamePtr = dirname;
	do
	{
		block.ioVRefNum = vrefnum;
		block.ioFDirIndex = -1;
		block.ioDrDirID = block.ioDrParID;
		err = PBGetCatInfoSync ((CInfoPBPtr) &block);
		if (err)
			return(err);
		memcpy(dirnamec, dirname+1, dirname[0]);
		dirnamec[dirname[0]] = EOS;
		strcat(dirnamec, ":");
		memmove(fullpathname + dirname[0] + 1,
				fullpathname, strlen(fullpathname) + 1);
		memcpy(fullpathname, dirnamec, (unsigned long) (dirname[0] + 1));
	}
	while (block.ioDrDirID != 2);
	return(0);
}

int32_t CMacXFS::dospath2macpath( uint16_t drv, MXFSDD *dd,
		char *dospath, char *macname)
{
	MXFSDD rel_dd;
	MXFSDD my_dd;
	MXFSDD symlink_dd;
	int32_t reldir = dd->dirID;
	short vRefNum = dd->vRefNum;
	int32_t doserr;
	OSErr err;
	char *restpfad;
	char *symlink;

	FSSpec spec;
	Handle hFullPath;
	short fullPathLength;


	if (dospath[0] && (dospath[1] == ':'))
		dospath += 2;
	if (*dospath == '\\')			// absoluter Pfad
	{
		reldir = drv_dirID[drv];
		vRefNum = drv_fsspec[drv].vRefNum;
		dospath++;
	}

	rel_dd.dirID = reldir;
	rel_dd.vRefNum = vRefNum;
	doserr = xfs_path2DD(0, drv, &rel_dd, dospath,
			&restpfad, &symlink_dd, &symlink, &my_dd, &drv );

	if (doserr < E_OK)
		return(doserr);

	// neu:

	// erstelle aus der DirID ein FSSpec
	err = FSMakeFSSpec(my_dd.vRefNum, my_dd.dirID, nil, &spec);
	if (err)
		return(cnverr(err));

	// erstelle aus der DirID einen Pfad

	err = FSpGetFullPath(&spec, &fullPathLength, &hFullPath);
	if (err)
		return(cnverr(err));

	if (fullPathLength > 200)
		return(EPTHOV);

	memcpy(macname, *hFullPath, fullPathLength);
	macname[fullPathLength] = '\0';
	if (*restpfad)
		strcat(macname, restpfad);

	if (hFullPath)
		DisposeHandle(hFullPath);
/*
	err = PathNameFromDirID( my_dd.dirID, my_dd.vRefNum, macname);
	if (!err && *restpfad)
		strcat(macname, restpfad);
*/
	return(cnverr(err));
}


/*************************************************************
*
* Fuer Fsymlink
*
* Unter dem Namen <name> wird im Ordner <dirID> ein
* Alias erstellt, der die Datei <to> repraesentiert.
*
*************************************************************/

int32_t CMacXFS::xfs_symlink(uint16_t drv, MXFSDD *dd, char *name, char *to)
{
#ifdef DEMO

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);
	if (fname_is_invalid(name))
		return(EACCDN);
	
	#pragma unused(dd, to)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);

#else

	bool symlink_ok;
	uint16_t dst_drv;
	bool is_macpath;
	char fullpath[256];
	AliasHandle newalias;
	OSErr err;
	short refnum;
	FSSpec fs;
	CInfoPBRec pb;
	OSType creator, type;
	unsigned char dosname[20];
	Str255 resname;
	unsigned char macname[65];
	char *onlyname;
	FInfo	fi;
	short	saveRefNum;


	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);
	if (fname_is_invalid(name))
		return(EACCDN);

	/* pruefen, ob der Symlink auf ein Mac-Laufwerk zeigt */
	/* -------------------------------------------------- */

	symlink_ok = false;		/* wir sind erstmal pessimistisch */
	dst_drv = (uint16_t) (ToUpper(*to)-'A');
	is_macpath = true;
	if (/*(dst_drv >= 0) &&*/ (dst_drv < NDRVS) && (to[1] == ':'))
	{	// Laufwerk angegeben
		if (!drv_fsspec[dst_drv].vRefNum)    /* ungueltig */
			is_macpath = false;
	}

	onlyname = strrchr(to, '\\');
	if (onlyname)
	{
		onlyname++;
	}
	else
	{
		if (to[0] && to[1] == ':')
			onlyname = to + 2;
		else
			onlyname = to;
	}

	if (is_macpath)
	{
		symlink_ok = (! dospath2macpath( dst_drv, dd,
				to, fullpath));
	}

	/* Wenn das Ziellaufwerk kein Mac-Volume ist oder der Pfad irgendwie	*/
	/* sonst nicht in Ordnung ist, setzen wir einen Dummy-Namen als		*/
	/* Alias fuer den Finder ein.								*/
	/* -----------------------------------------------------------------	*/

	if (!symlink_ok)
		strcpy(fullpath, "dummy:dummy");

	err = NewAliasMinimalFromFullPath((short) strlen(fullpath),
							fullpath,
							NULL,
							NULL,
							&newalias);
	if (err)
		return(cnverr(err));

	/* Der Alias braucht nicht umkopiert zu werden, wir vergroessern	*/
	/* einfach den Speicherblock, damit wir genug Platz fuer die	*/
	/* Zeichenkette <to> haben.							*/
	/* Wir setzen unsere Kennung und kopieren den DOS-Pfad 1:1	*/
	/* hinter den Alias.								*/
	/* ------------------------------------------------------------	*/

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	// Compilat für i86 erfordert als Minimal-OS-Version 10.4.
	// Im SDK 10.4 ist die Alias-Struktur "opaque", d.h. man
	// kommt nicht mehr an den <userType> dran.
	{
		Size AliasSize;

		AliasSize = GetAliasSize(newalias);
		SetHandleSize((Handle) newalias, (long) (AliasSize + strlen(to) + 1));
		SetAliasUserType(newalias, htonl('MgMc'));
		AliasSize = GetAliasSize(newalias);
		memcpy(((char *) (*newalias)) + AliasSize,
				to, strlen(to) + 1);
	}
#else
	SetHandleSize((Handle) newalias, (long) ((*newalias)->aliasSize + strlen(to) + 1));
	(*newalias)->userType = htonl('MgMc');
	memcpy(((char *) (*newalias)) + (*newalias)->aliasSize, to, strlen(to) + 1);
#endif

	/* FFSpec erstellen */
	/* --------------- */

	if (!drv_longnames[drv])
	{	// keine langen Namen: nach Gross-Schrift und 8+3
		nameto_8_3 ((unsigned char *) name, dosname, 2, false);
		name = (char *) dosname;
	}
	cfss(drv, dd->dirID, dd->vRefNum, (unsigned char *) name, &fs, true);

	/* Testen, ob die Datei schon existiert. Nach MagiC-Konvention */
	/* darf Fsymlink() niemals eine existierende Datei beeinflussen */
	/* ---------------------------------------------------------- */

	err = FSpGetFInfo (&fs, &fi);
	if (err != fnfErr)		// file not found ??
	{
		if (!err)			// Datei existiert => EACCDN
			err = permErr;
		goto ende;			// Aufruf abbrechen
	}

	/* Datei erstellen */
	/* -------------- */

	if (!(*onlyname))		// "to" ist Verzeichnis
	{
		type = 'fdrp';
		creator = 'MACS';
	}
	else
	{
		GetTypeAndCreator (name, &type, &creator);		// Datei
		type = 'TEXT';
		creator = MyCreator;
	}
	FSpCreateResFile(&fs, creator, type, smSystemScript);

	/* Resource 'alis' erstellen */
	/* ---------------------- */

	saveRefNum = CurResFile ();
	refnum = FSpOpenResFile(&fs, fsWrPerm);
	if (refnum == -1)
	{
		err = permErr;
		goto ende;
	}
//	UseResFile(refnum);
	strlcpy((char *) (resname + 1), onlyname, sizeof(resname) - 1 - 6);
	strcat((char *) (resname + 1), " Alias");
	sp((char *) resname);
	AddResource((Handle) newalias, 'alis', 0, resname);
	CloseResFile(refnum);
//	FlushVol(nil, drv_fsspec[drv].vRefNum);

	/* Finder-Flags fuer Alias setzen (umstaendlich!!!) */

	strlcpy((char *) (macname + 1), name, sizeof(macname) - 1);
	AtariFnameToMacFname(macname + 1, macname + 1);
	sp((char *) macname);
	pb.hFileInfo.ioNamePtr = macname;
	pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
	pb.hFileInfo.ioDirID = dd->dirID;
	pb.hFileInfo.ioVRefNum = dd->vRefNum;
	if (pb.hFileInfo.ioNamePtr[0] == 0) pb.hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync (&pb);
	if (err)
		goto ende;
	pb.hFileInfo.ioFlFndrInfo.fdFlags |= kIsAlias;	// symlink
	pb.hFileInfo.ioDirID = dd->dirID;		// Boese Falle !
	err = PBSetCatInfoSync (&pb);

ende:
	DisposeHandle((Handle) newalias);
	UseResFile (saveRefNum);
	return(cnverr(err));

#endif
}


/*************************************************************
*
* Fuer Freadlink
*
*************************************************************/

int32_t CMacXFS::xfs_readlink(uint16_t drv, MXFSDD *dd, char *name,
				char *buf, uint16_t bufsiz)
{
	FSSpec fs;


#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::xfs_readlink(„%s“, drv = %d, dirID = %d, buf = 0x%04x, bufsize = %d)", name, (int) drv, (int) dd->dirID, (int) buf, bufsiz);
#endif

	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)    /* ungueltig */
		return(EDRIVE);

	/* FSSpec erstellen und Alias auslesen	*/
	/* ---------------------------------	*/

	cfss(drv, dd->dirID, dd->vRefNum, (unsigned char *) name, &fs, true);

	return(resolve_symlink(&fs, bufsiz, buf));
}


/*************************************************************
*
* Fuer Dcntl.
*
* Da der Parameter <arg> in allen Fällen auf eine Struktur im 68k-Adreßraum
* zeigt, wird hier ein Zeiger übergeben.
*
*************************************************************/

int32_t CMacXFS::getCatInfo (uint16_t drv, CInfoPBRec *pb, bool resolveAlias)
{
#pragma unused(resolveAlias)
	FSSpec fs;
	OSErr  err;
	int32_t   doserr;

	err = PBGetCatInfoSync (pb);
	if (err)
		return cnverr(err);

	/* Aliase dereferenzieren	*/
	/* ----------------------	*/
	doserr = E_OK;
	if (pb->hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
	{
		pb->hFileInfo.ioNamePtr[pb->hFileInfo.ioNamePtr[0] + 1] = 0;
		cfss(drv, pb->hFileInfo.ioFlParID, pb->hFileInfo.ioVRefNum, pb->hFileInfo.ioNamePtr+1, &fs, true);
		doserr = resolve_symlink( &fs, 0, NULL );
		if (!doserr)		/* Der Mac hat dereferenziert! */
		{
			pb->hFileInfo.ioVRefNum = fs.vRefNum;
			pstrcpy (pb->hFileInfo.ioNamePtr, fs.name);
			pb->hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb->hFileInfo.ioDirID = fs.parID;
			if (pb->hFileInfo.ioNamePtr[0] == 0) pb->hFileInfo.ioFDirIndex = -1;
			err = PBGetCatInfoSync(pb);
			if (err)
				return cnverr(err);
		}
	}
	return doserr;
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
	uint16_t drv,
	MXFSDD *dd,
	char *name,
	uint16_t cmd,
	void *pArg,
	unsigned char *AdrOffset68k
)
{
	CInfoPBRec pb;
	OSErr err;
	int32_t doserr;
	unsigned char fname[64];


	if (drv_changed[drv])
		return(E_CHNG);
	if (!drv_fsspec[drv].vRefNum)
		return(EDRIVE);

	if (drv_longnames[drv])
	{
		if ((name[0] == '.') && (!name[1]))
			fname[1] = EOS;		// "." wie leerer Name
		else
			AtariFnameToMacFname((unsigned char *) name, fname + 1);
	}
	else
	{
		nameto_8_3 ((unsigned char *) name, fname + 1, 2, false);
		AtariFnameToMacFname(fname+1, fname + 1);
	}

	sp((char *) fname);
	pb.hFileInfo.ioVRefNum = dd->vRefNum;
	pb.hFileInfo.ioDirID = dd->dirID;
	pb.hFileInfo.ioNamePtr = (unsigned char *) fname;
	pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
	if (pb.hFileInfo.ioNamePtr[0] == 0)
		pb.hFileInfo.ioFDirIndex = -1;

	switch(cmd)
	{
	  case FUTIME:
		if (!pArg)
		    return EINVFN;
		doserr = getCatInfo(drv, &pb, true);
		if (doserr)
			return doserr;
		date_dos2mac(be16_to_cpu(((mutimbuf *) pArg)->modtime),
						be16_to_cpu(((mutimbuf *) pArg)->moddate), &pb.hFileInfo.ioFlMdDat);
		pb.hFileInfo.ioDirID = pb.hFileInfo.ioFlParID;
		if ((err = PBSetCatInfoSync(&pb)) != noErr)
		    return cnverr(err);
	  	return 0;

	  case FSTAT:
		if (!pArg)
		    return EINVFN;
		doserr = getCatInfo(drv, &pb, true);
		if (doserr)
			return doserr;
		cinfo_to_xattr(&pb, (XATTR *) pArg, drv);
		return(E_OK);

	// "type" und "creator" einer Datei ermitteln

	  case FMACGETTYCR:
		if (!pArg)
		    return EINVFN;
		doserr = getCatInfo (drv, &pb, true);
		if (doserr)
			return doserr;
		*(FInfo *) pArg = pb.hFileInfo.ioFlFndrInfo;
		return(E_OK);

	// "type" und "creator" einer Datei ändern

	  case FMACSETTYCR:
		if (!pArg)
		    return EINVFN;
#ifdef DEMO

		(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
		return(EWRPRO);

#else
			
		doserr = getCatInfo (drv, &pb, true);
		if (doserr)
			return doserr;
		pb.hFileInfo.ioFlFndrInfo = *(FInfo *) pArg;
		pb.hFileInfo.ioDirID = pb.hFileInfo.ioFlParID;
		if ((err = PBHSetFInfoSync((HParmBlkPtr)&pb)) != noErr)
		{
		    return cnverr(err);
		}
		return(E_OK);

#endif

	// spezielle MagiC-Funktionen

	  case FMACMAGICEX:
		{
		MMEXRec *mmex = (MMEXRec *) pArg;
		switch(be16_to_cpu(mmex->funcNo))
		{
			case MMEX_INFO:
				mmex->longVal = cpu_to_be32(1);
				mmex->destPtr = NULL;	// MM_VersionPtr;
				return 0;

			case MMEX_GETFSSPEC:
				if (be32_to_cpu((uint32_t) (mmex->destPtr)) >= m_AtariMemSize)
				{
					DebugError("CMacXFS::xfs_dcntl(FMACMAGICEX, MMEX_GETFSSPEC) - invalid dest ptr");
					return(ERROR);
				}
				doserr = getCatInfo (drv, &pb, true);
				if (doserr)
					return doserr;
				mmex->longVal = cpu_to_be32(FSMakeFSSpec(
									pb.hFileInfo.ioVRefNum,
									pb.hFileInfo.ioFlParID,
									pb.hFileInfo.ioNamePtr,
									(FSSpec *) (AdrOffset68k + be32_to_cpu((uint32_t) (mmex->destPtr)))));
				return cnverr ((OSErr) be32_to_cpu(mmex->longVal));

			case MMEX_GETRSRCLEN:
				// Mac-Rsrc-Länge liefern
				doserr = getCatInfo (drv, &pb, true);
				if (doserr)
					return doserr;
				if (pb.hFileInfo.ioFlAttrib & ioDirMask)
					return EACCDN;
				mmex->longVal = cpu_to_be32((long) pb.hFileInfo.ioFlRLgLen);
				return 0;

			case FMACGETTYCR:
				doserr = getCatInfo (drv, &pb, true);
				if (doserr)
					return doserr;
				*(CInfoPBRec*) pArg = pb;
				return(E_OK);

			case FMACSETTYCR:
#ifdef DEMO
				
				(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
				return(EWRPRO);
				
#else
				doserr = getCatInfo (drv, &pb, true);
				if (doserr)
					return doserr;
				((HFileInfo *) pArg)->ioVRefNum = pb.hFileInfo.ioVRefNum;
				((HFileInfo *) pArg)->ioDirID = pb.hFileInfo.ioFlParID;
				((HFileInfo *) pArg)->ioNamePtr = pb.hFileInfo.ioNamePtr;
				err = PBSetCatInfoSync ((CInfoPBRec *) pArg);
				return cnverr(err);

#endif
		}
		}
	}
	return(EINVFN);
}



/*************************************************************/
/******************* Dateitreiber ****************************/
/*************************************************************/

OSErr CMacXFS::f_2_cinfo( MAC_FD *f, CInfoPBRec *pb, char *fname)
{
	FCBPBRec fpb;
	OSErr err;

	fpb.ioFCBIndx = 0;
	fpb.ioVRefNum = 0;
	fpb.ioNamePtr = (unsigned char*)fname;
     	fpb.ioRefNum = f->refnum;
	err = PBGetFCBInfoSync (&fpb);  /* refnum => parID und Name */

	if (err)
		return(err);

	pb->hFileInfo.ioVRefNum = fpb.ioFCBVRefNum;
	pb->hFileInfo.ioNamePtr = (unsigned char*)fname;
	pb->hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
	pb->hFileInfo.ioDirID = fpb.ioFCBParID;
	if (pb->hFileInfo.ioNamePtr[0] == 0) pb->hFileInfo.ioFDirIndex = -1;
	err = PBGetCatInfoSync (pb);
	return(err);
}


int32_t CMacXFS::dev_close( MAC_FD *f )
{
	FCBPBRec fpb;
	OSErr err;
	char fname[64];
	CInfoPBRec pb;
	IOParam ipb;
	UInt16 refcnt;


	refcnt = be16_to_cpu(f->fd.fd_refcnt);
	if (refcnt <= 0)
		return(EINTRN);

	// FCB der Datei ermitteln
	fpb.ioFCBIndx = 0;
	fpb.ioVRefNum = 0;
	fpb.ioNamePtr = (unsigned char*)fname;
	fpb.ioRefNum = f->refnum;
	err = PBGetFCBInfoSync (&fpb);  /* refnum => parID und Name */
	if (err)
	{
		FSClose(f->refnum);
		return(cnverr(err));
	}

	refcnt--;
	f->fd.fd_refcnt = cpu_to_be16(refcnt);
	if (!refcnt)
	{

	/* Datum und Uhrzeit koennen erst nach Schliessen gesetzt werden */
	/* ------------------------------------------------------------- */

		if (f->mod_time_dirty)
		{
			err = FSClose(f->refnum);
			if (err)
				return(cnverr(err));

			pb.hFileInfo.ioVRefNum = fpb.ioFCBVRefNum;
			pb.hFileInfo.ioNamePtr = (unsigned char*) fname;
			pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb.hFileInfo.ioDirID = fpb.ioFCBParID;

			if (pb.hFileInfo.ioNamePtr[0] == 0)
				pb.hFileInfo.ioFDirIndex = -1;

			err = PBGetCatInfoSync (&pb);

    		if (err)
				return(cnverr(err));

			// !TT Archiv-Bit: falls Datei gerade neu angelegt, dann Backup-Datum auf Null setzen
			if (pb.hFileInfo.ioFlCrDat == pb.hFileInfo.ioFlMdDat)
			{
				pb.hFileInfo.ioFlBkDat = 0;	// -> Archiv-Bit löschen
			}

			pb.hFileInfo.ioFDirIndex = 0;      /* Namen suchen */
			pb.hFileInfo.ioDirID = fpb.ioFCBParID;
			date_dos2mac(f->mod_time[0], f->mod_time[1], &pb.hFileInfo.ioFlMdDat);
			pb.hFileInfo.ioFlCrDat = pb.hFileInfo.ioFlMdDat;

			// Archiv-Bit: sicherstellen, daß Backup-Datum verschieden von Modif.-Datum ist.
			if (pb.hFileInfo.ioFlBkDat == pb.hFileInfo.ioFlMdDat)
			{
				pb.hFileInfo.ioFlBkDat = 0;
			}

			err = PBSetCatInfoSync (&pb);    /* synchron */
		}
		else
			err = FSClose(f->refnum);
	}
	else
	{
		ipb.ioRefNum = f->refnum;
		err = PBFlushFileSync ((ParmBlkPtr) &ipb);
	}

//	if (!err && FlushWhenClosing)
//		FlushVol (nil, fpb.ioVRefNum);

	return(cnverr(err));
}

/*
*
* pread() und pwrite() für "Hintergrund-DMA".
*
* Der Atari-Teil des XFS legt den ParamBlockRec (50 Bytes) auf dem Stapel
* an und initialisiert <ioCompletion>, <ioBuffer> und <ioReqCount>.
* Die Completion-Routine befindet sich
* auf der "Atari-Seite" in "macxfs.s", wird jedoch im Mac-Modus aufgerufen;
* diesen Umstand habe ich berücksichtigt.
* Die Completion-Routine erhält in a0 einen Zeiger auf den ParamBlockRec und
* in d0 (== ParamBlockrec.ioResult) den Fehlercode. Die Routine darf d0-d2 und
* a0-a1 verändern (PureC-Konvention) und ist "void". a5 ist undefiniert.
* Mit dem Trick:
*
*	int32_t geta0 ( void )
*		= 0x2008;			// MOVE.L	A0,D0
*
*	static pascal void dev_p_complete( void )
*	{
*		ParamBlockRec *pb = (ParamBlockRec *) geta0();
*	}
*
* könnte man die Routine auch in C schreiben. Den ParamBlockRec kann man
* beliebig für eigene Zwecke erweitern (z.B. a5 ablegen).
*
* Rückgabe von dev_pread() und dev_pwrite():
*
* >=0		Transfer läuft und ist beendet, wenn ioComplete den
*		Fehlercode enthält.
* <0		Fehler
*
*/

/*
static int32_t CMacXFS::dev_pwrite( MAC_FD *f, ParamBlockRec *pb )
{
	OSErr err;


	pb->ioParam.ioRefNum = f->refnum;		// Datei-Handle
	pb->ioParam.ioPosMode = 0;				// ???

	pb->ioParam.ioResult = 1;					// warte, bis <= 0
	err = PBWriteAsync (pb);				// asynchron!
	if (err)
		return(cnverr(err));
	return(pb->ioParam.ioActCount);
}


static int32_t CMacXFS::dev_pread( MAC_FD *f, ParamBlockRec *pb )	
{
	OSErr err;


	pb->ioParam.ioRefNum = f->refnum;		// Datei-Handle
	pb->ioParam.ioPosMode = 0;				// ???

	pb->ioParam.ioResult = 1;					// warte, bis <= 0
	err = PBReadAsync (pb);				// asynchron!
	if (err == eofErr)
		err = 0;					// nur Teil eingelesen, kein Fehler!
	if (err)
		return(cnverr(err));
	return(pb->ioParam.ioActCount);
}
*/

int32_t CMacXFS::dev_read( MAC_FD *f, int32_t count, char *buf )
{
	OSErr err;
	long lcount;

#if DEBUG_68K_EMU
	if ((trigger == 2) && (count > 0x1e) && (trigger_refnum == f->refnum))
	{
		DebugInfo("#### DEBUG TRIGGER #### Ladeadresse ist 0x%08x", buf);
		trigger_ProcessStartAddr = buf;
		trigger_ProcessFileLen = (unsigned) count;
		trigger = 0;
	}
#endif
	lcount = count;
	err = FSRead(f->refnum, &lcount, buf);
	if (err == eofErr)
		err = 0;				/* nur Teil eingelesen, kein Fehler! */
	else
	if (err)
		return(cnverr(err));
	return((int32_t) lcount);
}


int32_t CMacXFS::dev_write( MAC_FD *f, int32_t count, char *buf )
{
#ifdef DEMO
	#pragma unused(f, count, buf)
	(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
	return(EWRPRO);
#else
	OSErr err;
	long lcount;

	lcount = count;
	err = FSWrite(f->refnum, &lcount, buf);
	if (err)
		return(cnverr(err));
	return((int32_t) lcount);
#endif
}


int32_t CMacXFS::dev_stat(MAC_FD *f, void *unsel, uint16_t rwflag, int32_t apcode)
{
#pragma unused(unsel, apcode)
	OSErr err;
	long pos;
	long laenge;


	if (rwflag)
		return(1L);         /* Schreiben immer bereit */
	err = GetEOF(f->refnum, &laenge);
	if (err)
		return(cnverr(err));
	err = GetFPos(f->refnum, &pos);
	if (err)
		return(cnverr(err));
	if (pos < laenge)
		return(1L);
	return(0L);
}


int32_t CMacXFS::dev_seek(MAC_FD *f, int32_t pos, uint16_t mode)
{
	OSErr err;
	short macmode;
	long lpos;

	switch(mode)
	{
		case 0:   macmode = 1;break;
		case 1:   macmode = 3;break;
		case 2:   macmode = 2;break;
		default:  return(EINVFN);
	}
	lpos = pos;
	err = SetFPos(f->refnum, macmode, lpos);
	if (err)
		return(cnverr(err));
	err = GetFPos(f->refnum, &lpos);
	if (err)
		return(cnverr(err));
	return(lpos);
}


int32_t CMacXFS::dev_datime(MAC_FD *f, uint16_t d[2], uint16_t rwflag)
{
	OSErr err;
	char fname[64];
	CInfoPBRec pb;


#ifdef DEMO
	
	if (rwflag)
	{
		(void) MyAlert(ALRT_DEMO, kAlertNoteAlert);
		return(EWRPRO);
	}
	
#endif
	
	if (rwflag)			/* schreiben */
	{
		f->mod_time[0] = be16_to_cpu(d[0]);
		f->mod_time[1] = be16_to_cpu(d[1]);
		f->mod_time_dirty = 1;		/* nur puffern */
		err = E_OK;				/* natürlich kein Fehler */
	}
	else
	{
		if (f->mod_time_dirty)	/* war schon geaendert */
		{
			d[0] = cpu_to_be16(f->mod_time[0]);
			d[1] = cpu_to_be16(f->mod_time[1]);
			err = E_OK;				/* natuerlich kein Fehler */
		}
		else
		{
			err = f_2_cinfo(f, &pb, fname);
   			if (err)
       			return(cnverr(err));

          	/* Datum ist ioFlMdDat bzw. ioDrMdDat */
          	date_mac2dos(pb.hFileInfo.ioFlMdDat, &(d[0]), &(d[1]));
			d[0] = cpu_to_be16(d[0]);
			d[1] = cpu_to_be16(d[1]);
		}
	}

	return(cnverr(err));
}

OSErr CMacXFS::getFSSpecByFileRefNum (short fRefNum, FSSpec *spec, FCBPBRec *pb)
{
	OSErr err;
	pb->ioFCBIndx = 0;
	pb->ioRefNum = fRefNum;
	pb->ioNamePtr = spec->name;
	err = PBGetFCBInfoSync ((FCBPBPtr)pb);
	spec->parID = pb->ioFCBParID;
	spec->vRefNum = pb->ioFCBVRefNum;
	return err;
}


int32_t CMacXFS::dev_ioctl(MAC_FD *f, uint16_t cmd, void *buf)
{
	OSErr err;
	char fname[64];
	CInfoPBRec pb;


	switch(cmd)
	{
	  case FSTAT:
		if (buf == 0)
		    return EINVFN;
		err = f_2_cinfo(f, &pb, fname);
		if (err)
			return(cnverr(err));
		/*
		struct _mx_dmd *fd_dmd = (struct _mx_dmd *) (be32_to_cpu(f->fd.fd_dmd) + AdrOffset68k);
		cinfo_to_xattr(&pb, (XATTR *) buf, be16_to_cpu(fd_dmd->d_drive));
		*/
		cinfo_to_xattr(&pb, (XATTR *) buf, 0 /*dummy*/);
		return(E_OK);
	  	break;
	  case FTRUNCATE:
	  	err = SetEOF(f->refnum, cpu_to_be32(*((int32_t *) buf)));
		return(cnverr(err));
	  	break;

	  case FMACOPENRES:
	  {
      		HParamBlockRec pb;
      		FSSpec	spec;
      		char	perm;
      		FCBPBRec	fcb;
      		
			// Get filename & dirID
			err = getFSSpecByFileRefNum (f->refnum, &spec, &fcb);
			if (err) return cnverr (err);
			
			// get the file's access permission
			switch (fcb.ioFCBFlags & 0x1100)
			{
			  case 0x0100:
				perm = fsRdWrPerm;
				break;
			  case 0x0000:
				perm = fsRdPerm;
				break;
			  case 0x1100:
				perm = fsRdWrShPerm;
				break;
			  case 0x1000:
				perm = fsRdPerm;
				break;
			}

			/* close the data fork */
			pb.ioParam.ioRefNum = f->refnum;
			if ((err = PBCloseSync((ParmBlkPtr)&pb)) != noErr)
			    return cnverr(err);

			/* now open the data fork */
			pb.ioParam.ioVRefNum = spec.vRefNum;
			pb.fileParam.ioDirID = spec.parID;
			pb.ioParam.ioNamePtr = spec.name;
			pb.ioParam.ioPermssn = perm;
			pb.ioParam.ioMisc = 0;
			if ((err = PBHOpenRFSync(&pb)) != noErr)
			    return cnverr(err);
			f->refnum = pb.ioParam.ioRefNum;
			if (be16_to_cpu(f->fd.fd_mode) & O_TRUNC)
			{
			    pb.ioParam.ioMisc = 0;
			    if ((err = PBSetEOFSync((ParmBlkPtr)&pb)) != noErr)
			        return cnverr(err);
			}
			return 0;
		}

	  case FMACGETTYCR:
		{
      		FSSpec	spec;
      		HFileParam pbf;
      		FCBPBRec	fcb;

			if (buf == 0)
			    return EINVFN;

			// Get filename & dirID
			err = getFSSpecByFileRefNum (f->refnum, &spec, &fcb);
			if (err) return cnverr (err);

			pbf.ioVRefNum = spec.vRefNum;
			pbf.ioDirID = spec.parID;
			pbf.ioNamePtr = spec.name;
			pbf.ioFDirIndex = 0;
			if ((err = PBHGetFInfoSync((HParmBlkPtr)&pbf)) != noErr)
			    return cnverr(err);

			*(FInfo *) buf = pbf.ioFlFndrInfo;
			return 0;
		}

	  case FMACSETTYCR:
		{
      		FSSpec	spec;
      		HFileParam pbf;
      		FCBPBRec	fcb;

			if (buf == 0)
			    return EINVFN;

			// Get filename & dirID
			err = getFSSpecByFileRefNum (f->refnum, &spec, &fcb);
			if (err)
				return cnverr(err);

			pbf.ioVRefNum = spec.vRefNum;
			pbf.ioDirID = spec.parID;
			pbf.ioNamePtr = spec.name;
			pbf.ioFDirIndex = 0;
			if ((err = PBHGetFInfoSync((HParmBlkPtr)&pbf)) != noErr)
			    return cnverr(err);

			pbf.ioFlFndrInfo = *(FInfo *)buf;
			pbf.ioDirID = spec.parID;
			if ((err = PBHSetFInfoSync((HParmBlkPtr)&pbf)) != noErr)
			    return cnverr(err);
			return 0;
		}

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
      			mmex->longVal = f->refnum;
      			return 0;
      		}
		}
	}
	return(EINVFN);
}

int32_t CMacXFS::dev_getc(MAC_FD *f, uint16_t mode)
{
#pragma unused(mode)
	unsigned char c;
	int32_t ret;

	ret = dev_read(f, 1L, (char *) &c);
	if (ret < 0L)
		return(ret);			// Fehler
	if (!ret)
		return(0x0000ff1a);		// EOF
	return(c & 0x000000ff);
}


int32_t CMacXFS::dev_getline( MAC_FD *f, char *buf, int32_t size, uint16_t mode )
{
#pragma unused(mode)
	char c;
	int32_t gelesen,ret;

	for	(gelesen = 0L; gelesen < size;)
		{
		ret = dev_read(f, 1L, (char *) &c);
		if (ret < 0L)
			return(ret);			// Fehler
		if (ret == 0L)
			break;			// EOF
		if (c == 0x0d)
			continue;
		if (c == 0x0a)
			break;
		gelesen++;
		*buf++ = c;
		}
	return(gelesen);
}


int32_t CMacXFS::dev_putc( MAC_FD *f, uint16_t mode, int32_t val )
{
#pragma unused(mode)
	char c;

	c = (char) val;
	return(dev_write(f, 1L, (char *) &c));
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

int32_t CMacXFS::XFSFunctions(uint32_t param, unsigned char *AdrOffset68k)
{
#pragma options align=packed
	uint16_t fncode;
	int32_t doserr;
	unsigned char *params = AdrOffset68k + param;

	fncode = be16_to_cpu(*((uint16_t *) params));
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::XFSFunctions(%d)", (int) fncode);
	if (fncode == 7)
	{
/*
		if (!m68k_trace_trigger)
		{
			// ab dem ersten fopen() tracen
			m68k_trace_trigger = 1;
			_DumpAtariMem("AtariMemOnFirstXfsCall.data");
		}
*/
	}
#endif
	params += 2;
	switch(fncode)
	{
		case 0:
		{
			struct syncparm
			{
				uint16_t drv;
			};
			syncparm *psyncparm = (syncparm *) params;
			doserr = xfs_sync(be16_to_cpu(psyncparm->drv));
			break;
		}

		case 1:
		{
			struct ptermparm
			{
				uint32_t pd;		// PD *
			};
			ptermparm *pptermparm = (ptermparm *) params;
			xfs_pterm((PD *) (AdrOffset68k + be32_to_cpu(pptermparm->pd)));
			doserr = E_OK;
			break;
		}
            
		case 2:
		{
			struct drv_openparm
			{
				uint16_t drv;
				uint32_t dd;		// MXFSDD *
				int32_t flg_ask_diskchange;	// in fact: DMD->D_XFS (68k-Pointer or NULL)
			};
			drv_openparm *pdrv_openparm = (drv_openparm *) params;
			doserr = xfs_drv_open(
					be16_to_cpu(pdrv_openparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdrv_openparm->dd)),
					be32_to_cpu(pdrv_openparm->flg_ask_diskchange));
			break;
		}
            
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
			break;
		}

		case 4:
		{
			struct path2DDparm
			{
				uint16_t mode;
				uint16_t drv;
				uint32_t rel_dd;	// MXFSDD *
				uint32_t pathname;		// char *
				uint32_t restpfad;		// char **
				uint32_t symlink_dd;	// MXFSDD *
				uint32_t symlink;		// char **
				uint32_t dd;		// MXFSDD *
				uint32_t dir_drive;
			};
			char *restpath;
			char *symlink;

			path2DDparm *ppath2DDparm = (path2DDparm *) params;
#ifdef DEBUG_VERBOSE
			__dump((const unsigned char *) ppath2DDparm, sizeof(*ppath2DDparm));
#endif
			doserr = xfs_path2DD(
					be16_to_cpu(ppath2DDparm->mode),
					be16_to_cpu(ppath2DDparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->rel_dd)),
					(char *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->pathname)),
					&restpath,
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->symlink_dd)),
					&symlink,
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->dd)),
					(uint16_t *) (AdrOffset68k + be32_to_cpu(ppath2DDparm->dir_drive))
					);

			*((char **) (AdrOffset68k + be32_to_cpu(ppath2DDparm->restpfad))) = (char *) cpu_to_be32(((uint32_t) restpath) - ((uint32_t) AdrOffset68k));
			*((char **) (AdrOffset68k + be32_to_cpu(ppath2DDparm->symlink))) = (char *) cpu_to_be32(((uint32_t) symlink) - ((uint32_t) AdrOffset68k));
#ifdef DEBUG_VERBOSE
			__dump((const unsigned char *) ppath2DDparm, sizeof(*ppath2DDparm));
			if (doserr >= 0)
				DebugInfo(" restpath = „%s“", restpath);
#endif
			break;
		}
			
		case 5:
		{
			struct sfirstparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint32_t dta;		// MAC_DTA *
				uint16_t attrib;
			};
			sfirstparm *psfirstparm = (sfirstparm *) params;
			doserr = xfs_sfirst(
					be16_to_cpu(psfirstparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(psfirstparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(psfirstparm->name)),
					(MAC_DTA *) (AdrOffset68k + be32_to_cpu(psfirstparm->dta)),
					be16_to_cpu(psfirstparm->attrib)
					);
			break;
		}

		case 6:
		{
			struct snextparm
			{
				uint16_t drv;
				uint32_t dta;		// MAC_DTA *
			};
			snextparm *psnextparm = (snextparm *) params;
			doserr = xfs_snext(
					be16_to_cpu(psnextparm->drv),
					(MAC_DTA *) (AdrOffset68k + be32_to_cpu(psnextparm->dta))
					);
			break;
		}
	
		case 7:
		{
			struct fopenparm
			{
				uint32_t name;	// char *
				uint16_t drv;
				uint32_t dd;		//MXFSDD *
				uint16_t omode;
				uint16_t attrib;
			};
			fopenparm *pfopenparm = (fopenparm *) params;
			doserr = xfs_fopen(
					(char *) (AdrOffset68k + be32_to_cpu(pfopenparm->name)),
					be16_to_cpu(pfopenparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pfopenparm->dd)),
					be16_to_cpu(pfopenparm->omode),
					be16_to_cpu(pfopenparm->attrib)
					);
			break;
		}
			
		case 8:
		{
			struct fdeleteparm
			{
				uint16_t drv;
				uint32_t dd;		//MXFSDD *
				uint32_t name;	// char *
			};
			fdeleteparm *pfdeleteparm = (fdeleteparm *) params;
			doserr = xfs_fdelete(
					be16_to_cpu(pfdeleteparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pfdeleteparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pfdeleteparm->name))
					);
			break;
		}
			
		case 9:
		{
			struct flinkparm
			{
				uint16_t drv;
				uint32_t nam1;	// char *
				uint32_t nam2;	// char *
				uint32_t dd1;		// MXFSDD *
				uint32_t dd2;		// MXFSDD *
				uint16_t mode;
				uint16_t dst_drv;
			};
			flinkparm *pflinkparm = (flinkparm *) params;
			doserr = xfs_link(
					be16_to_cpu(pflinkparm->drv),
					(char *) (AdrOffset68k + be32_to_cpu(pflinkparm->nam1)),
					(char *) (AdrOffset68k + be32_to_cpu(pflinkparm->nam2)),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pflinkparm->dd1)),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pflinkparm->dd2)),
					be16_to_cpu(pflinkparm->mode),
					be16_to_cpu(pflinkparm->dst_drv)
					);
			break;
		}
			
		case 10:
		{
			struct xattrparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint32_t xattr;	// XATTR *
				uint16_t mode;
			};
			xattrparm *pxattrparm = (xattrparm *) params;
			doserr = xfs_xattr(
					be16_to_cpu(pxattrparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pxattrparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pxattrparm->name)),
					(XATTR *) (AdrOffset68k + be32_to_cpu(pxattrparm->xattr)),
					be16_to_cpu(pxattrparm->mode)
					);
			break;
		}
			
		case 11:
		{
			struct attribparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint16_t rwflag;
				uint16_t attr;
			};
			attribparm *pattribparm = (attribparm *) params;
			doserr = xfs_attrib(
					be16_to_cpu(pattribparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pattribparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pattribparm->name)),
					be16_to_cpu(pattribparm->rwflag),
					be16_to_cpu(pattribparm->attr)
					);
			break;
		}
			
		case 12:
		{
			struct chownparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint16_t uid;
				uint16_t gid;
			};
			chownparm *pchownparm = (chownparm *) params;
			doserr = xfs_fchown(
					be16_to_cpu(pchownparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pchownparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pchownparm->name)),
					be16_to_cpu(pchownparm->uid),
					be16_to_cpu(pchownparm->gid)
					);
			break;
		}
			
		case 13:
		{
			struct chmodparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint16_t fmode;
			};
			chmodparm *pchmodparm = (chmodparm *) params;
			doserr = xfs_fchmod(
					be16_to_cpu(pchmodparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pchmodparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pchmodparm->name)),
					be16_to_cpu(pchmodparm->fmode)
					);
			break;
		}
			
		case 14:
		{
			struct dcreateparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
			};
			dcreateparm *pdcreateparm = (dcreateparm *) params;
			if (be32_to_cpu((uint32_t) (pdcreateparm->name)) >= m_AtariMemSize)
			{
				DebugError("CMacXFS::xfs_dcreate() - invalid name ptr");
				return(ERROR);
			}

			doserr = xfs_dcreate(
					be16_to_cpu(pdcreateparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdcreateparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pdcreateparm->name))
					);
			break;
		}
			
		case 15:
		{
			struct ddeleteparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
			};
			ddeleteparm *pddeleteparm = (ddeleteparm *) params;
			doserr = xfs_ddelete(
					be16_to_cpu(pddeleteparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pddeleteparm->dd))
					);
			break;
		}
			
		case 16:
		{
			struct dd2nameparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t buf;		// char *
				uint16_t bufsiz;
			};
			dd2nameparm *pdd2nameparm = (dd2nameparm *) params;
			doserr = xfs_DD2name(
					be16_to_cpu(pdd2nameparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdd2nameparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pdd2nameparm->buf)),
					be16_to_cpu(pdd2nameparm->bufsiz)
					);
			break;
		}
			
		case 17:
		{
			struct dopendirparm
			{
				uint32_t dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint16_t tosflag;
			};
			dopendirparm *pdopendirparm = (dopendirparm *) params;
			doserr = xfs_dopendir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdopendirparm->dirh)),
					be16_to_cpu(pdopendirparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdopendirparm->dd)),
					be16_to_cpu(pdopendirparm->tosflag)
					);
			break;
		}
	
		case 18:
		{
			struct dreaddirparm
			{
				uint32_t dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
				uint16_t size;
				uint32_t buf;		// char *
				uint32_t xattr;	// XATTR * oder NULL
				uint32_t xr;		// int32_t * oder NULL
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
			break;
		}
			
		case 19:
		{
			struct drewinddirparm
			{
				uint32_t dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
			};
			drewinddirparm *pdrewinddirparm = (drewinddirparm *) params;
			doserr = xfs_drewinddir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdrewinddirparm->dirh)),
					be16_to_cpu(pdrewinddirparm->drv)
					);
			break;
		}
			
		case 20:
		{
			struct dclosedirparm
			{
				uint32_t dirh;		// MAC_DIRHANDLE *
				uint16_t drv;
			};
			dclosedirparm *pdclosedirparm = (dclosedirparm *) params;
			doserr = xfs_dclosedir(
					(MAC_DIRHANDLE *) (AdrOffset68k + be32_to_cpu(pdclosedirparm->dirh)),
					be16_to_cpu(pdclosedirparm->drv)
					);
			break;
		}
			
		case 21:
		{
			struct dpathconfparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint16_t which;
			};
			dpathconfparm *pdpathconfparm = (dpathconfparm *) params;
			doserr = xfs_dpathconf(
					be16_to_cpu(pdpathconfparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdpathconfparm->dd)),
					be16_to_cpu(pdpathconfparm->which)
					);
			break;
		}

		case 22:
		{
			struct dfreeparm
			{
				uint16_t drv;
				int32_t dirID;
				uint32_t data;	// uint32_t data[4]
			};
			dfreeparm *pdfreeparm = (dfreeparm *) params;
			doserr = xfs_dfree(
					be16_to_cpu(pdfreeparm->drv),
					pdfreeparm->dirID,
					(uint32_t *) (AdrOffset68k + be32_to_cpu(pdfreeparm->data))
					);
#ifdef DEBUG_VERBOSE
			__dump((const unsigned char *) (AdrOffset68k + be32_to_cpu(pdfreeparm->data)), 16);
#endif
			break;
		}

		case 23:
		{
			struct wlabelparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
			};
			wlabelparm *pwlabelparm = (wlabelparm *) params;
			doserr = xfs_wlabel(
					be16_to_cpu(pwlabelparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pwlabelparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pwlabelparm->name))
					);
			break;
		}
			
		case 24:
		{
			struct rlabelparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint16_t bufsiz;
			};
			rlabelparm *prlabelparm = (rlabelparm *) params;
			doserr = xfs_rlabel(
					be16_to_cpu(prlabelparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(prlabelparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(prlabelparm->name)),
					be16_to_cpu(prlabelparm->bufsiz)
					);
			break;
		}
			
		case 25:
		{
			struct symlinkparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint32_t to;		// char *
			};
			symlinkparm *psymlinkparm = (symlinkparm *) params;
			doserr = xfs_symlink(
					be16_to_cpu(psymlinkparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(psymlinkparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(psymlinkparm->name)),
					(char *) (AdrOffset68k + be32_to_cpu(psymlinkparm->to))
					);
			break;
		}
			
		case 26:
		{
			struct readlinkparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint32_t buf;		// char *
				uint16_t bufsiz;
			};
			readlinkparm *preadlinkparm = (readlinkparm *) params;
			doserr = xfs_readlink(
					be16_to_cpu(preadlinkparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(preadlinkparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(preadlinkparm->name)),
					(char *) (AdrOffset68k + be32_to_cpu(preadlinkparm->buf)),
					be16_to_cpu(preadlinkparm->bufsiz)
					);
			break;
		}
			
		case 27:
		{
			struct dcntlparm
			{
				uint16_t drv;
				uint32_t dd;	// MXFSDD *
				uint32_t name;	// char *
				uint16_t cmd;
				int32_t arg;
			};
			dcntlparm *pdcntlparm = (dcntlparm *) params;
			doserr = xfs_dcntl(
					be16_to_cpu(pdcntlparm->drv),
					(MXFSDD *) (AdrOffset68k + be32_to_cpu(pdcntlparm->dd)),
					(char *) (AdrOffset68k + be32_to_cpu(pdcntlparm->name)),
					be16_to_cpu(pdcntlparm->cmd),
					AdrOffset68k + be32_to_cpu(pdcntlparm->arg),
					AdrOffset68k
					);
			break;
		}
			

		default:
			doserr = EINVFN;
			break;
	}

#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::XFSFunctions => %d (= 0x%08x)", (int) doserr, (int) doserr);
#endif
	return(doserr);
}


/*************************************************************
*
* Dispatcher für Dateitreiber
*
*************************************************************/

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
	DebugInfo("CMacXFS::XFSDevFunctions(%d)", (int) fncode);
	__dump((const unsigned char *) f, sizeof(*f));
#endif
	switch(fncode)
	{
		case 0:
		{
			doserr = dev_close(f);
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
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
					(void *) (AdrOffset68k + be32_to_cpu(pdevioctlparm->buf))
					);
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
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
			break;
		}
			
		default:
			doserr = EINVFN;
			break;
	}
#ifdef DEBUG_VERBOSE
	DebugInfo("CMacXFS::XFSDevFunctions => %d", (int) doserr);
#endif
	return(doserr);
}
   	#pragma options align=reset


/*************************************************************
*
* Tauscht in der Atari-Systemvariablen <_drvbits> alle Bits aus,
* die das MacXFS betreffen.
*
*************************************************************/

void CMacXFS::setDrivebits(long newbits, unsigned char *AdrOffset68k)
{
	long val;

	val = be32_to_cpu(*(long*)(&AdrOffset68k[_drvbits]));
	newbits |= (1L << ('m'-'a'));	// virtuelles Laufwerk M: immer präsent
	val &= -1L-xfs_drvbits;		// alte löschen
	val |= newbits;			// neue setzen
	*(long*)(&AdrOffset68k[_drvbits]) = cpu_to_be32(val);
	xfs_drvbits = newbits;
}


/*************************************************************
*
* Definiert ein Atari-Laufwerk
*
*************************************************************/

void CMacXFS::SetXFSDrive
(
	short drv,
	MacXFSDrvType drvType,
	CFURLRef pathUrl,
	bool longnames,
	bool reverseDirOrder,
	unsigned char *AdrOffset68k
)
{
	FSRef fs;

	DebugInfo("CMacXFS::%s(%c: has type %s)", __FUNCTION__, 'A' + drv, xfsDrvTypeToStr(drvType));

	long newbits = xfs_drvbits;

#ifdef SPECIALDRIVE_AB
	if (drv >= 2)
#endif
	{
	if (drvType == NoMacXFS)
	{
		// Laufwerk ist kein MacXFS-Laufwerk mehr => abmelden
		newbits &= ~(1L << drv);
	}
	else
	{
		// Laufwerk ist MacXFS-Laufwerk
		newbits |= 1L << drv;

		// convert CFURLRef to FSRef
		if (!CFURLGetFSRef(pathUrl, &fs))
		{
			DebugError("CMacXFS::%s() -- conversion from URL to FSRef failed", __FUNCTION__);
			return;
		}
	}

	}

	drv_changed[drv] = true;
	xfs_path[drv] = fs;
	drv_type[drv] = drvType;

	if (drvType == MacDir)
	{
		drv_valid[drv] = true;
	}
	else
	{
		drv_valid[drv] = false;
		drv_fsspec[drv].vRefNum = 0;
	}

	drv_longnames[drv] = longnames;
	drv_rvsDirOrder[drv] = reverseDirOrder;

	DebugInfo("CMacXFS::SetXFSDrive() -- Drive %c: %s dir order, %s names", 'A' + drv, drv_rvsDirOrder[drv] ? "reverse" : "normal", drv_longnames[drv] ? "long" : "8+3");

#ifdef SPECIALDRIVE_AB
	if (drv>=2)
#endif
		setDrivebits (newbits, AdrOffset68k);
}

void CMacXFS::ChangeXFSDriveFlags
(
	short drv,
	bool longnames,
	bool reverseDirOrder
)
{
	drv_longnames[drv] = longnames;
	drv_rvsDirOrder[drv] = reverseDirOrder;

	DebugInfo("CMacXFS::ChangeXFSDriveFlags() -- Drive %c: %s dir order, %s names", 'A' + drv, drv_rvsDirOrder[drv] ? "reverse" : "normal", drv_longnames[drv] ? "long" : "8+3");
}


/*
bool CMacXFS::GetXFSRootDir (short drv, short *vRefNum, long *dirID)
// Liefert FSSpec des Root-Verzeichnisses eines MagiC-Laufwerks.
// Dazu muß das Laufwerk allerdings bereits in MagiC geöffnet/gemountet sein!
{
	*vRefNum = drv_fsspec[drv].vRefNum;
	*dirID = drv_dirID[drv];
	return *vRefNum != 0;
}

void CMacXFS::XFSVolUnmounted (ParmBlkPtr pb0)
// Hook in UnmountVol()
// Registriert, wenn ein von XFS benutztes Vol verschwindet, während MM im Vordergrund läuft
{
	HVolumeParam pb;
	short err;
	
	// Volume ermitteln
	pb.ioNamePtr = pb0->volumeParam.ioNamePtr;
	pb.ioVRefNum = pb0->volumeParam.ioVRefNum;
	pb.ioVolIndex = (pb.ioNamePtr)? (short)-1 : (short)0;
	err = PBHGetVInfoSync ((HParamBlockRec*)&pb);
	if (!err && pb.ioVDrvInfo > 2)
	{
		// nicht die Floppy-LWs vom Mac berücksichtigen
		short drv;
		for (drv = 0; drv < NDRVS; ++drv) {
			if (drv_fsspec[drv].vRefNum == pb.ioVRefNum)
			{
				drv_changed[drv] = true;
				drv_fsspec[drv].vRefNum = 0;    // ungueltig
				if (drv_type[drv] == MacDrive)
					drv_valid[drv] = false;
			}
		}
	}
}
*/

/*************************************************************
*
* Rechnet einen Laufwerkbuchstaben um ein einen "device code".
* Wird vom Atari benötigt, um das richtige Medium auszuwerfen.
*
* Dies ist ein direkter Einsprungpunkt vom Emulator.
*
*************************************************************/

int32_t CMacXFS::Drv2DevCode(uint32_t params, unsigned char *AdrOffset68k)
{
	short vol;
	uint16_t drv = be16_to_cpu(*((uint16_t*) (AdrOffset68k + params)));


	if (drv <= 1)
	{
		// Floppy A: & B:
		return((int32_t) 0x00010000 | (drv + 1));	// liefert 1 bzw. 2
	}

	vol = drv_fsspec[drv].vRefNum;

	if (vol == 0)
	{
		// evtl. AHDI-Drive?
		if (false)
		{	// !!! erstmal nicht unterstützt.
			return((int32_t) (0x00020000 | drv));
		}
		else
		{
			return 0;
		}
	}
	else
	{
		// Es ist ein Mac-Volume - Drive-Nr. ermitteln
		HVolumeParam pb;
		short err, drvNum, driver;
		Str255 name;

		pb.ioVolIndex = 0;
		pb.ioVRefNum = vol;
		pb.ioNamePtr = name;
		err = PBHGetVInfoSync ((HParamBlockRec*)&pb);
		if (err)
		{
			return 0;
		}
		else
		{
			drvNum = pb.ioVDrvInfo;
			driver = pb.ioVDRefNum;
			if ((driver >= -39) && (driver <= -33))
			{	// ein SCSI-Device?
				drvNum = driver;
			}
			return((int32_t) (0x00010000 | (uint16_t) drvNum));
		}
	}
}


/*************************************************************
*
* Erledigt Gerätefunktionen, hier nur: Auswerfen eines
* Mediums
*
*************************************************************/

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
			ret = EDRIVE;
		else
		if ((params->device >> 16)  == 1)
			ret = EDRIVE;		// Mac-Medium
		else
			ret = EDRIVE;		// AHDI-Medium auswerfen
		break;

		default:
		ret = EINVFN;
		break;
	}
	return(ret);
}

/*
* Version von Thomas, 23.3.98
*/


/*
**	GetVolumeInfoNoName uses pathname and vRefNum to call PBHGetVInfoSync
**	in cases where the returned volume name is not needed by the caller.
**	The pathname and vRefNum parameters are not touched, and the pb
**	parameter is initialized by PBHGetVInfoSync except that ioNamePtr in
**	the parameter block is always returned as NULL (since it might point
**	to the local tempPathname).
**
**	I noticed using this code in several places, so here it is once.
**	This reduces the code size of MoreFiles.
*/
/*
static pascal OSErr GetVolumeInfoNoName(StringPtr pathname,
									short vRefNum,
									HParmBlkPtr pb)
{
	Str255 tempPathname;
	OSErr error;
	
	// Make sure pb parameter is not NULL
	if ( pb != NULL )
	{
		pb->volumeParam.ioVRefNum = vRefNum;
		if ( pathname == NULL )
		{
			pb->volumeParam.ioNamePtr = NULL;
			pb->volumeParam.ioVolIndex = 0;		// use ioVRefNum only
		}
		else
		{
			BlockMoveData(pathname, tempPathname, pathname[0] + 1);	// make a copy of the string and
			pb->volumeParam.ioNamePtr = (StringPtr)tempPathname;	// use the copy so original isn't trashed
			pb->volumeParam.ioVolIndex = -1;	// use ioNamePtr/ioVRefNum combination
		}
		error = PBHGetVInfoSync(pb);
		pb->volumeParam.ioNamePtr = NULL;	// ioNamePtr may point to local tempPathname, so don't return it
	}
	else
	{
		error = paramErr;
	}
	return error;
}

static pascal OSErr DetermineVRefNum(StringPtr pathname,
								 short vRefNum,
								 short *realVRefNum)
{
	HParamBlockRec pb;
	OSErr error;

	error = GetVolumeInfoNoName(pathname,vRefNum, &pb);
	*realVRefNum = pb.volumeParam.ioVRefNum;
	return error;
}

static pascal OSErr GetParentID(short vRefNum,
							long dirID,
							StringPtr name,
							long *parID)
{
	CInfoPBRec pb;
	Str31 tempName;
	OSErr error;
	short realVRefNum;
	
	// Protection against File Sharing problem
	if ( (name == NULL) || (name[0] == 0) )
	{
		tempName[0] = 0;
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioFDirIndex = -1;	// use ioDirID
	}
	else
	{
		pb.hFileInfo.ioNamePtr = name;
		pb.hFileInfo.ioFDirIndex = 0;	// use ioNamePtr and ioDirID
	}
	pb.hFileInfo.ioVRefNum = vRefNum;
	pb.hFileInfo.ioDirID = dirID;
	error = PBGetCatInfoSync(&pb);
	if ( error == noErr )
	{
		//
		//	There's a bug in HFS where the wrong parent dir ID can be
		//	returned if multiple separators are used at the end of a
		//	pathname. For example, if the pathname:
		//		'volumeName:System Folder:Extensions::'
		//	is passed, the directory ID of the Extensions folder is
		//	returned in the ioFlParID field instead of fsRtDirID. Since
		//	multiple separators at the end of a pathname always specifies
		//	a directory, we only need to work-around cases where the
		//	object is a directory and there are multiple separators at
		//	the end of the name parameter.
		//
		if ( (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0 )
		{
			// Its a directory
			
			// is there a pathname?
			if ( pb.hFileInfo.ioNamePtr == name )	
			{
				// could it contain multiple separators?
				if ( name[0] >= 2 )
				{
					// does it contain multiple separators at the end?
					if ( (name[name[0]] == ':') && (name[name[0] - 1] == ':') )
					{
						// OK, then do the extra stuff to get the correct parID
						
						// Get the real vRefNum (this should not fail)
						error = DetermineVRefNum(name, vRefNum, &realVRefNum);
						if ( error == noErr )
						{
							// we don't need the parent's name, but add protect against File Sharing problem
							tempName[0] = 0;
							pb.dirInfo.ioNamePtr = tempName;
							pb.dirInfo.ioVRefNum = realVRefNum;
							// pb.dirInfo.ioDrDirID already contains the
							// dirID of the directory object
							pb.dirInfo.ioFDirIndex = -1;	// get information about ioDirID
							error = PBGetCatInfoSync(&pb);
							// now, pb.dirInfo.ioDrParID contains the correct parID
						}
					}
				}
			}
		}
		
		// if no errors, then pb.hFileInfo.ioFlParID (pb.dirInfo.ioDrParID)
		// contains the parent ID
		*parID = pb.hFileInfo.ioFlParID;
	}
	return error;
}

static pascal OSErr GetCatInfoNoName(short vRefNum,
							   long dirID,
							   StringPtr name,
							   CInfoPBPtr pb)
{
	Str31 tempName;
	OSErr error;
	
	// Protection against File Sharing problem
	if ( (name == NULL) || (name[0] == 0) )
	{
		tempName[0] = 0;
		pb->dirInfo.ioNamePtr = tempName;
		pb->dirInfo.ioFDirIndex = -1;	// use ioDirID
	}
	else
	{
		pb->dirInfo.ioNamePtr = name;
		pb->dirInfo.ioFDirIndex = 0;	// use ioNamePtr and ioDirID
	}
	pb->dirInfo.ioVRefNum = vRefNum;
	pb->dirInfo.ioDrDirID = dirID;
	error = PBGetCatInfoSync(pb);
	pb->dirInfo.ioNamePtr = NULL;
	return error;
}


static pascal OSErr GetDirectoryID(short vRefNum,
							   long dirID,
							   StringPtr name,
							   long *theDirID,
							   Boolean *isDirectory)
{
	CInfoPBRec pb;
	OSErr error;

	error = GetCatInfoNoName(vRefNum, dirID ,name, &pb);
	*isDirectory = (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0;
	*theDirID = (*isDirectory) ? pb.dirInfo.ioDrDirID : pb.hFileInfo.ioFlParID;
	return ( error );
}
*/

/*
 unbenutzt
bool CMacXFS::GetPathFromFSSpec (FSSpec *spec, char* path, short pathSize)
{
	// Um herauszufinden, ob sich ein Mac-Ordner auf einem der MagiC-Laufwerke
	// befindet, wird einfach geprüft, ob dessen DirID oder eine seiner Parent-Dirs
	// als Root-Dir der MagiC-Laufwerke benutzt wird.
	OSErr	err;
	uint16_t drv;
	long	theDir, theDir2;
	Boolean	isDir, ok = false;
	Boolean	tmpMount[NDRVS], was_changed[NDRVS];
	MXFSDD dd;
	unsigned long l;


	for	(drv = 0; drv < NDRVS; ++drv)
	{
		tmpMount[drv] = (drv_fsspec[drv].vRefNum == 0);
		if (tmpMount[drv] || drv_changed[drv])
		{
			drv_open(drv, true);
		}
		was_changed[drv] = drv_changed[drv];
		drv_changed[drv] = false;
	}
	
	err = GetDirectoryID(spec->vRefNum, spec->parID, (StringPtr)spec->name, &theDir, &isDir);
//	err = FSpGetDirectoryID(spec, &theDir, &isDir);
	theDir2 = theDir;
	while (!err && !ok)
	{
		for	(drv = 0; drv < NDRVS; ++drv)
		{
			if (drv_fsspec[drv].vRefNum == spec->vRefNum && drv_dirID[drv] == theDir)
			{
				// ein passendes Laufwerk gefunden - nun den Pfad zusammensetzen
				theDir = theDir2;
				// Laufwerk in Pfad einsetzen
				strcpy (path, "A:");
				path[0] += drv;
				l = strlen (path);
				// Pfad einsetzen
				dd.dirID = theDir;
				dd.vRefNum = drv_fsspec[drv].vRefNum;
				if (xfs_DD2name (drv, &dd, &path[l], (uint16_t) (pathSize-l)) == E_OK)
				{
					l = strlen (path);
					if ((l+1) < pathSize)
					{
						path[l++] = '\\';
						if (!isDir)
						{
							// Dateinamen anfügen
							short nameLen = spec->name[0];
							if (nameLen + l < pathSize)
							{
								strncpy (&path[l], (const char*)&spec->name[1], nameLen);
								path[l+nameLen] = 0;
								MacFnameToAtariFname ((StringPtr)path+l, (StringPtr)path+l);
								ok = true;
							}
						}
					}
				}
			}
		}
		if (!ok)
		{
			FSSpec temp;

			// get parent ID
			err = FSMakeFSSpec(spec->vRefNum, theDir, nil, &temp);
			if (!err)
				theDir = temp.parID;
//			err = GetParentID (spec->vRefNum, theDir, nil, &theDir);	// weiter mit Parent-Dir
		}
	}

	for	(drv = 0; drv < NDRVS; ++drv)
	{
		if (tmpMount[drv])
		{
			xfs_drv_close (drv, 0);
		}
		drv_changed[drv] = was_changed[drv];
	}
	
	return ok;
}
*/
// EOF
