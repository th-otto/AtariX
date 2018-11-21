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
*
* Enth�lt alle Konstanten und Strukturen f�r den Atari
*
*/

#ifndef ATARI_H_INCLUDED
#define ATARI_H_INCLUDED

   	#pragma options align=packed

/* File Attributes */

#define F_RDONLY 0x01
#define F_HIDDEN 0x02
#define F_SYSTEM 0x04
#define F_VOLUME 0x08
#define F_SUBDIR 0x10
#define F_ARCHIVE 0x20

/* GEMDOS (MiNT) Fopen modes */

#define _ATARI_O_RDONLY       0
#define _ATARI_O_WRONLY       1
#define _ATARI_O_RDWR         2
#define _ATARI_O_APPEND       8
#define _ATARI_O_COMPAT       0
#define _ATARI_O_DENYRW       0x10
#define _ATARI_O_DENYW        0x20
#define _ATARI_O_DENYR        0x30
#define _ATARI_O_DENYNONE     0x40
#define _ATARI_O_CREAT        0x200
#define _ATARI_O_TRUNC        0x400
#define _ATARI_O_EXCL         0x800

/* unterstuetzte Dcntl- Modi (Mag!X- spezifisch!) */
#define   KER_GETINFO    0x0100
#define   KER_INSTXFS    0x0200
#define   KER_SETWBACK   0x0300
#define   DFS_GETINFO    0x1100
#define   DFS_INSTDFS    0x1200
#define   DEV_M_INSTALL  0xcd00

/* unterstuetzte Fcntl- Modi */
#define   FSTAT          0x4600
#define   FIONREAD       0x4601
#define   FIONWRITE      0x4602
#define   FUTIME         0x4603
#define   FTRUNCATE      0x4604

#define	TIOCGPGRP		(('T'<< 8) | 6)
#define	TIOCSPGRP		(('T'<< 8) | 7)
#define	TIOCFLUSH		(('T'<< 8) | 8)
#define	TIOCIBAUD		(('T'<< 8) | 18)
#define	TIOCOBAUD		(('T'<< 8) | 19)
#define	TIOCGFLAGS		(('T'<< 8) | 22)
#define	TIOCSFLAGS		(('T'<< 8) | 23)
#define TIOCBUFFER (('T'<<8) | 128)
#define TIOCCTLMAP (('T'<<8) | 129)
#define TIOCCTLGET (('T'<<8) | 130)
#define TIOCCTLSET (('T'<<8) | 131)



/* Modi und Codes fuer Dpathconf() (-> MiNT) */

#define DP_MAXREQ      0xffff
#define DP_IOPEN       0
#define DP_MAXLINKS    1
#define DP_PATHMAX     2
#define DP_NAMEMAX     3
#define DP_ATOMIC      4
#define DP_TRUNC       5
#define    DP_NOTRUNC    0
#define    DP_AUTOTRUNC  1
#define    DP_DOSTRUNC   2
#define DP_CASE        6
#define    DP_CASESENS   0
#define    DP_CASECONV   1
#define    DP_CASEINSENS 2
/* Ab hier Julians geniale Neuerungen */
#define DP_MODEATTR 	7
#define  DP_ATTRBITS   0x000000ff
#define  DP_MODEBITS   0x000fff00
#define  DP_FILETYPS   0xfff00000
#define   DP_FT_DIR    0x00100000
#define   DP_FT_CHR    0x00200000
#define   DP_FT_BLK    0x00400000
#define   DP_FT_REG    0x00800000
#define   DP_FT_LNK    0x01000000
#define   DP_FT_SOCK   0x02000000
#define   DP_FT_FIFO   0x04000000
#define   DP_FT_MEM    0x08000000
#define DP_XATTRFIELDS 8
#define  DP_INDEX      0x0001
#define  DP_DEV        0x0002
#define  DP_RDEV       0x0004
#define  DP_NLINK      0x0008
#define  DP_UID        0x0010
#define  DP_GID        0x0020
#define  DP_BLKSIZE    0x0040
#define  DP_SIZE       0x0080
#define  DP_NBLOCKS    0x0100
#define  DP_ATIME      0x0200
#define  DP_CTIME      0x0400
#define  DP_MTIME      0x0800

/* D/Fcntl(FUTIME,...) */

struct mutimbuf
{
	uint16_t		actime;          /* Zugriffszeit */
	uint16_t		acdate;
	uint16_t		modtime;         /* letzte �nderung */
	uint16_t		moddate;
};

/* structure for getxattr (-> MiNT) */

struct XATTR
{
	uint16_t	mode;
	/* file types */
	#define _ATARI_S_IFMT 	   0170000        /* mask to select file type */
	#define _ATARI_S_IFCHR     0020000        /* BIOS special file */
	#define _ATARI_S_IFDIR     0040000        /* directory file */
	#define _ATARI_S_IFREG     0100000        /* regular file */
	#define _ATARI_S_IFIFO	   0120000        /* FIFO */
	#define _ATARI_S_IMEM	   0140000        /* memory region or process */
	#define _ATARI_S_IFLNK     0160000        /* symbolic link */

	/* special bits: setuid, setgid, sticky bit */
	#define _ATARI_S_ISUID     04000
	#define _ATARI_S_ISGID 02000
	#define _ATARI_S_ISVTX     01000

	/* file access modes for user, group, and other*/
	#define _ATARI_S_IRUSR     0400
	#define _ATARI_S_IWUSR 0200
	#define _ATARI_S_IXUSR 0100
	#define _ATARI_S_IRGRP 0040
	#define _ATARI_S_IWGRP     0020
	#define _ATARI_S_IXGRP     0010
	#define _ATARI_S_IROTH     0004
	#define _ATARI_S_IWOTH     0002
	#define _ATARI_S_IXOTH     0001
	#define _ATARI_DEFAULT_DIRMODE (0777)
	#define _ATARI_DEFAULT_MODE     (0666)
	uint32_t	index;
	uint16_t	dev;
	uint16_t	reserved1;
	uint16_t	nlink;
	uint16_t	uid;
	uint16_t	gid;
	uint32_t	size;
	uint32_t	blksize, nblocks;
	uint16_t	mtime, mdate;
	uint16_t	atime, adate;
	uint16_t	ctime, cdate;
	uint16_t	attr;
	uint16_t	reserved2;
	uint32_t	reserved3[2];
};

struct BasePage
{
	uint32_t p_lowtpa;				//   0 (0x00)
	uint32_t p_hitpa;				//   4 (0x04)
	uint32_t p_tbase;				//   8 (0x08)
	uint32_t p_tlen;				//   12 (0x0c)
	uint32_t p_dbase;				//   16 (0x10)
	uint32_t p_dlen;				//   20 (0x14)
	uint32_t p_bbase;				//   24 (0x18)
	uint32_t p_blen;
	uint32_t p_dta;
	uint32_t p_parent;
	uint32_t unused1;
	uint32_t p_env;
	char p_devx[6];
	char unused2;
	char p_defdrv;
	uint32_t unused[2+16];
	unsigned char p_cmdline[128];
};

struct ExeHeader
{
	uint16_t code;
	uint32_t tlen;
	uint32_t dlen;
	uint32_t blen;
	uint32_t slen;
	uint32_t dum1;
	uint32_t dum2;
	uint16_t relmod;
};

/* Systemvariable _sysbase (0x4F2L) zeigt auf: */

struct SYSHDR
{
	UInt16		os_entry;		/* $00 BRA to reset handler             */
	UInt16		os_version;		/* $02 TOS version number               */
	UInt32		os_start;		/* $04 -> reset handler                 */
	UInt32		os_base;		/* $08 -> baseof OS                     */
	UInt32		os_membot;		/* $0c -> end BIOS/GEMDOS/VDI ram usage */
	UInt32		os_rsv1;		/* $10 << unused,reserved >>            */
	UInt32		os_magic;		/* $14 -> GEM memory usage parm. block   */
	UInt32		os_gendat;		/* $18 Date of system build($MMDDYYYY)  */
	UInt16		os_palmode;		/* $1c OS configuration bits            */
	UInt16		os_gendatg;		/* $1e DOS-format date of system build   */
	/*
	Die folgenden Felder gibt es ab TOS 1.2
	*/
	UInt32		_root;			/* $20 -> base of OS pool               */
	UInt32		kbshift;		/* $24 -> 68-Adresse der Atari-Variablen "kbshift" und "kbrepeat" */
	UInt32		_run;			/* $28 -> GEMDOS PID of current process */
	UInt32		p_rsv2;			/* $2c << unused, reserved >>           */
};

/* Interruptvektoren */

#define	INTV_0_RESET_SSP        0x000	/* ssp bei Reset */
#define	INTV_1_RESET_PC         0x004	/* pc bei Reset */
#define	INTV_2_BUS_ERROR        0x008	/* Busfehler */
#define	INTV_3_ADDRESS_ERROR	0x00c	/* Adre�fehler */
#define	INTV_4_ILLEGAL          0x010	/* Illegaler Opcode */
#define	INTV_5_DIV_BY_ZERO      0x014	/* Division durch 0 */
#define	INTV_6_CHK              0x018	/* chk Opcode */
#define	INTV_7_TRAPV            0x01c	/* trapv Befehl */
#define	INTV_8_PRIV_VIOL        0x020	/* Privilegverletzung */
#define	INTV_9_TRACE            0x024	/* trace */
#define	INTV_10_LINE_A          0x028	/* LineA-Opcode */
#define	INTV_11_LINE_F          0x02c	/* LineF-Opcode */
#define	INTV_12                 0x030	/* reserviert */
#define	INTV_13                 0x034	/* reserviert */
#define	INTV_14                 0x038	/* reserviert */
#define	INTV_15                 0x03c	/* reserviert */
#define	INTV_16                 0x040	/* reserviert */
#define	INTV_17                 0x044	/* reserviert */
#define	INTV_18                 0x048	/* reserviert */
#define	INTV_19                 0x04c	/* reserviert */
#define	INTV_20                 0x050	/* reserviert */
#define	INTV_21                 0x054	/* reserviert */
#define	INTV_22                 0x058	/* reserviert */
#define	INTV_23                 0x05c	/* reserviert */
#define	INTV_24_SPURIOUS        0x060	/* Interrupt unbekannter Herkunft */
#define	INTV_25_AUTV_1          0x064	/* beim ST unbenutzt */
#define	INTV_26_AUTV_2          0x068	/* ST: Hblank */
#define	INTV_27_AUTV_3          0x06c	/* beim ST unbenutzt */
#define	INTV_28_AUTV_4          0x070	/* ST: VBlank */
#define	INTV_29_AUTV_5          0x074	/* beim ST unbenutzt */
#define	INTV_30_AUTV_6          0x078	/* beim ST unbenutzt */
#define	INTV_31_AUTV_7          0x07c	/* beim ST unbenutzt */
#define	INTV_32_TRAP_0          0x080	/* Trap #0 */
#define	INTV_33_TRAP_1          0x084	/* Trap #1 */
#define	INTV_34_TRAP_2          0x088	/* Trap #2 */
#define	INTV_35_TRAP_3          0x08c	/* Trap #3 */
#define	INTV_36_TRAP_4          0x090	/* Trap #4 */
#define	INTV_37_TRAP_5          0x094	/* Trap #5 */
#define	INTV_38_TRAP_6          0x098	/* Trap #6 */
#define	INTV_39_TRAP_7          0x09c	/* Trap #7 */
#define	INTV_40_TRAP_8          0x0a0	/* Trap #8 */
#define	INTV_41_TRAP_9          0x0a4	/* Trap #9 */
#define	INTV_42_TRAP_10         0x0a8	/* Trap #10 */
#define	INTV_43_TRAP_11         0x0ac	/* Trap #11 */
#define	INTV_44_TRAP_12         0x0b0	/* Trap #12 */
#define	INTV_45_TRAP_13         0x0b4	/* Trap #13 */
#define	INTV_46_TRAP_14         0x0b8	/* Trap #14 */
#define	INTV_47_TRAP_15         0x0bc	/* Trap #15 */
#define	INTV_48                 0x0c0	/* reserviert */
#define	INTV_49                 0x0c4	/* reserviert */
#define	INTV_50                 0x0c8	/* reserviert */
#define	INTV_51                 0x0cc	/* reserviert */
#define	INTV_52                 0x0d0	/* reserviert */
#define	INTV_53                 0x0d4	/* reserviert */
#define	INTV_54                 0x0d8	/* reserviert */
#define	INTV_55                 0x0dc	/* reserviert */
#define	INTV_56                 0x0e0	/* reserviert */
#define	INTV_57                 0x0e4	/* reserviert */
#define	INTV_58                 0x0e8	/* reserviert */
#define	INTV_59                 0x0ec	/* reserviert */
#define	INTV_60                 0x0f0	/* reserviert */
#define	INTV_61                 0x0f4	/* reserviert */
#define	INTV_62                 0x0f8	/* reserviert */
#define	INTV_63                 0x0fc	/* reserviert */
#define	INTV_MFP0_CENTBUSY      0x100	/* centronics busy */
#define	INTV_MFP1_DCD           0x104	/* rs232 carrier detect */
#define	INTV_MFP2_CTS           0x108	/* rs232 clear to send */
#define	INTV_MFP3_GPU_DONE      0x10c	/* blitter */
#define	INTV_MFP4_BAUDGEN       0x110	/* Baudratengenerator */
#define	INTV_MFP5_HZ200         0x114	/* 200Hz Timer */
#define	INTV_MFP6_IKBD_MIDI     0x118	/* IKBD/MIDI */
#define	INTV_MFP7_FDC_ACSI      0x11c	/* FDC/ACSI */
#define	INTV_MFP8               0x120	/* display enable (?) */
#define	INTV_MFP9_TX_ERR        0x124	/* Sendefehler RS232 */
#define	INTV_MFP10_SND_EMPT     0x128	/* RS232 Sendepuffer leer*/
#define	INTV_MFP11_RX_ERR       0x12c	/* Empfangsfehler RS232 */
#define	INTV_MFP12_RCV_FULL     0x130	/* RS232 Empfangspuffer voll */
#define	INTV_MFP13              0x134	/* unbenutzt */
#define	INTV_MFP14_RING_IND     0x138	/* RS232: ankommender Anruf */
#define	INTV_MFP15_MNCHR        0x13c	/* monochrome monitor detect */

/* System variables */

#define	proc_lives              0x380
#define	proc_regs               0x384
#define	proc_pc                 0x3c4
#define	proc_usp                0x3c8
#define	proc_stk                0x3cc
#define	etv_timer               0x400
#define	etv_critic              0x404
#define	etv_term                0x408
#define	etv_xtra                0x40c
#define	memvalid                0x420
#define	memctrl                 0x424
#define	resvalid                0x426
#define	resvector               0x42a
#define	phystop                 0x42e
#define	_membot                 0x432
#define	_memtop                 0x436
#define	memval2                 0x43a
#define	flock                   0x43e
#define	seekrate                0x440
#define	_timer_ms               0x442
#define	_fverify                0x444
#define	_bootdev                0x446
#define	palmode                 0x448	//unbenutzt
#define	defshiftmd              0x44a
#define	sshiftmd                0x44c
#define	_v_bas_ad               0x44e
#define	vblsem                  0x452
#define	nvbls                   0x454
#define	_vblqueue               0x456
#define	colorptr                0x45a
#define	screenpt                0x45e
#define	_vbclock                0x462
#define	_frclock                0x466
#define	hdv_init                0x46a
#define	swv_vec                 0x46e
#define	hdv_bpb                 0x472
#define	hdv_rw                  0x476
#define	hdv_boot                0x47a
#define	hdv_mediach             0x47e
#define	_cmdload                0x482
#define	conterm                 0x484
#define	trp14ret                0x486	//hier unbenutzt
#define	os_chksum               trp14ret	//in Mag!X Checksumme des Systems
#define	criticret               0x48a	//MagiC 6.01: DOS-Event-Critic aktiv
#define	themd                   0x48e
#define	____md                  0x49e	//hier unbenutzt
#define	fstrm_beg               ____md	//in Mag!X Beginn des TT-RAMs
#define	savptr                  0x4a2
#define	_nflops                 0x4a6
#define	con_state               0x4a8	//hier unbenutzt
#define	save_row                0x4ac	//hier unbenutzt
#define	sav_context             0x4ae	//hier unbenutzt
#define	_bufl                   0x4b2	//hier unbenutzt
#define	_hz_200                 0x4ba
#define	the_env                 0x4be	//hier unbenutzt
#define	_drvbits                0x4c2
#define	_dskbufp                0x4c6
#define	_autopath               0x4ca	//hier unbenutzt
#define	_vbl_list               0x4ce
#define	_dumpflg                0x4ee
#define	_prtabt                 0x4f0	//hier unbenutzt
#define	_sysbase                0x4f2
#define	_shell_p                0x4f6	//hier unbenutzt
#define	end_os                  0x4fa
#define	exec_os                 0x4fe
#define	scr_dump                0x502
#define	prv_lsto                0x506
#define	prv_lst                 0x50a
#define	prv_auxo                0x50e
#define	prv_aux                 0x512
#define	pun_ptr                 0x516	//hier unbenutzt
#define	memval3                 0x51a
#define	dev_vecs                0x51e	//long dev_vecs[8*4]
#define	cpu_typ                 0x59e	//int cpu_typ
#define	_p_cookies              0x5a0	//long *cookie
#define	fstrm_top               0x5a4
#define	fstrm_valid             0x5a8
#define	bell_hook               0x5ac	//long *pointer auf pling
#define	kcl_hook                0x5b0	//long *pointer auf keyklick

/* BIOS level errors */

#define E_OK	  0L	/* OK, no error 		*/
#define ERROR	 -1L	/* basic, fundamental error	*/
#define EDRVNR	 -2L	/* drive not ready		*/
#define EUNCMD	 -3L	/* unknown command		*/
#define E_CRC	 -4L	/* CRC error			*/
#define EBADRQ	 -5L	/* bad request			*/
#define E_SEEK	 -6L	/* seek error			*/
#define EMEDIA	 -7L	/* unknown media		*/
#define ESECNF	 -8L	/* sector not found		*/
#define EPAPER	 -9L	/* no paper				*/
#define EWRITF	-10L	/* write fault			*/
#define EREADF	-11L	/* read fault			*/
#define EGENRL	-12L	/* general error		*/
#define EWRPRO	-13L	/* write protect		*/
#define E_CHNG	-14L	/* media change 		*/
#define EUNDEV	-15L	/* unknown device		*/
#define EBADSF	-16L	/* bad sectors on format	*/
#define EOTHER	-17L	/* insert other disk	*/

/* BDOS level errors */

#define EINVFN	-32L	/* invalid function number		 1 */
#define EFILNF	-33L	/* file not found				 2 */
#define EPTHNF	-34L	/* path not found	(0xffde)		 3 */
#define ENHNDL	-35L	/* no handles left				 4 */
#define EACCDN	-36L	/* access denied				 5 */
#define EIHNDL	-37L	/* invalid handle				 6 */
#define ENSMEM	-39L	/* insufficient memory			 8 */
#define EIMBA	-40L	/* invalid memory block address	 9 */
#define EDRIVE	-46L	/* invalid drive was specified	15 */
#define ENSAME	-48L	/* MV between two different drives 17 */
#define ENMFIL	-49L	/* no more files				18 */
#define ATARIERR_ERANGE	-64L	/* range error					33 */
#define EINTRN	-65L	/* internal error				34 */
#define EPLFMT	-66L	/* invalid program load format	35 */
#define EGSBF	-67L	/* setblock failure 			36 */
#define EBREAK	-68L	/* user break (^C)				37 */
#define EXCPT	-69L	/* 68000- exception ("bombs")	38 */
#define EPTHOV	-70L	/* path overflow                          MAG!X    */

// Keyboard Scancodes

// key									scancode		// international
#define ATARI_KBD_SCANCODE_ESCAPE		1
#define ATARI_KBD_SCANCODE_1			2
#define ATARI_KBD_SCANCODE_2			3
#define ATARI_KBD_SCANCODE_3			4
#define ATARI_KBD_SCANCODE_4			5
#define ATARI_KBD_SCANCODE_5			6
#define ATARI_KBD_SCANCODE_6			7
#define ATARI_KBD_SCANCODE_7			8
#define ATARI_KBD_SCANCODE_8			9
#define ATARI_KBD_SCANCODE_9			10
#define ATARI_KBD_SCANCODE_0			11
#define ATARI_KBD_SCANCODE_MINUS		12				// de: '�'  us: '-'
#define ATARI_KBD_SCANCODE_EQUALS		13				// de: '`'  us: '='
#define ATARI_KBD_SCANCODE_BACKSPACE	14
#define ATARI_KBD_SCANCODE_TAB			15
#define ATARI_KBD_SCANCODE_Q			16
#define ATARI_KBD_SCANCODE_W			17
#define ATARI_KBD_SCANCODE_E			18
#define ATARI_KBD_SCANCODE_R			19
#define ATARI_KBD_SCANCODE_T			20
#define ATARI_KBD_SCANCODE_Y			21				// de: 'Z'  us: 'Y'
#define ATARI_KBD_SCANCODE_U			22
#define ATARI_KBD_SCANCODE_I			23
#define ATARI_KBD_SCANCODE_O			24
#define ATARI_KBD_SCANCODE_P			25
#define ATARI_KBD_SCANCODE_LEFTBRACKET	26				// de: '�' us: '['
#define ATARI_KBD_SCANCODE_RIGHTBRACKET	27				// de: '+' us: ']'
#define ATARI_KBD_SCANCODE_RETURN		28
#define ATARI_KBD_SCANCODE_CONTROL		29
#define ATARI_KBD_SCANCODE_A			30
#define ATARI_KBD_SCANCODE_S			31
#define ATARI_KBD_SCANCODE_D			32
#define ATARI_KBD_SCANCODE_F			33
#define ATARI_KBD_SCANCODE_G			34
#define ATARI_KBD_SCANCODE_H			35
#define ATARI_KBD_SCANCODE_J			36
#define ATARI_KBD_SCANCODE_K			37
#define ATARI_KBD_SCANCODE_L			38
#define ATARI_KBD_SCANCODE_SEMICOLON	39				// de: '�'  us: ';'
#define ATARI_KBD_SCANCODE_APOSTROPHE	40				// de: '�'  us: '''
#define ATARI_KBD_SCANCODE_GRAVE		41				// de: '#'
#define ATARI_KBD_SCANCODE_LSHIFT		42
#define ATARI_KBD_SCANCODE_BACKSLASH	43				// de: --   us: '\'
#define ATARI_KBD_SCANCODE_Z			44				// de: 'Y'  us: 'Z'
#define ATARI_KBD_SCANCODE_X			45
#define ATARI_KBD_SCANCODE_C			46
#define ATARI_KBD_SCANCODE_V			47
#define ATARI_KBD_SCANCODE_B			48
#define ATARI_KBD_SCANCODE_N			49
#define ATARI_KBD_SCANCODE_M			50
#define ATARI_KBD_SCANCODE_COMMA		51
#define ATARI_KBD_SCANCODE_PERIOD		52
#define ATARI_KBD_SCANCODE_SLASH		53				// de: '-'  us: '/'
#define ATARI_KBD_SCANCODE_RSHIFT		54
// 55
#define ATARI_KBD_SCANCODE_ALT			56
#define ATARI_KBD_SCANCODE_SPACE		57
#define ATARI_KBD_SCANCODE_CAPSLOCK		58
#define ATARI_KBD_SCANCODE_F1			59
#define ATARI_KBD_SCANCODE_F2			60
#define ATARI_KBD_SCANCODE_F3			61
#define ATARI_KBD_SCANCODE_F4			62
#define ATARI_KBD_SCANCODE_F5			63
#define ATARI_KBD_SCANCODE_F6			64
#define ATARI_KBD_SCANCODE_F7			65
#define ATARI_KBD_SCANCODE_F8			66
#define ATARI_KBD_SCANCODE_F9			67
#define ATARI_KBD_SCANCODE_F10			68
// 69
// 70
#define ATARI_KBD_SCANCODE_CLRHOME		71
#define ATARI_KBD_SCANCODE_UP			72
#define ATARI_KBD_SCANCODE_PAGEUP		73				// not on Atari keyboard
#define ATARI_KBD_SCANCODE_KP_MINUS		74
#define ATARI_KBD_SCANCODE_LEFT			75
#define ATARI_KBD_SCANCODE_ALTGR		76				// not on Atari keyboard
#define ATARI_KBD_SCANCODE_RIGHT		77
#define ATARI_KBD_SCANCODE_KP_PLUS		78
#define ATARI_KBD_SCANCODE_END			79				// not on Atari keyboard
#define ATARI_KBD_SCANCODE_DOWN			80
#define ATARI_KBD_SCANCODE_PAGEDOWN		81				// not on Atari keyboard
#define ATARI_KBD_SCANCODE_INSERT		82
#define ATARI_KBD_SCANCODE_DELETE		83
#define ATARI_KBD_SCANCODE_SHIFT_F1		84
#define ATARI_KBD_SCANCODE_SHIFT_F2		85
#define ATARI_KBD_SCANCODE_SHIFT_F3		86
#define ATARI_KBD_SCANCODE_SHIFT_F4		87
#define ATARI_KBD_SCANCODE_SHIFT_F5		88
#define ATARI_KBD_SCANCODE_SHIFT_F6		89
#define ATARI_KBD_SCANCODE_SHIFT_F7		90
#define ATARI_KBD_SCANCODE_SHIFT_F8		91
#define ATARI_KBD_SCANCODE_SHIFT_F9		92
#define ATARI_KBD_SCANCODE_SHIFT_F10	93
// 94
// 95
#define ATARI_KBD_SCANCODE_LTGT			96				// de: '<>'
#define ATARI_KBD_SCANCODE_UNDO			97
#define ATARI_KBD_SCANCODE_HELP			98
#define ATARI_KBD_SCANCODE_KP_LPAREN	99				// '('
#define ATARI_KBD_SCANCODE_KP_RPAREN	100				// ')'
#define ATARI_KBD_SCANCODE_KP_DIVIDE	101				// '/'
#define ATARI_KBD_SCANCODE_KP_MULTIPLY	102				// '*'
#define ATARI_KBD_SCANCODE_KP_7			103
#define ATARI_KBD_SCANCODE_KP_8			104
#define ATARI_KBD_SCANCODE_KP_9			105
#define ATARI_KBD_SCANCODE_KP_4			106
#define ATARI_KBD_SCANCODE_KP_5			107
#define ATARI_KBD_SCANCODE_KP_6			108
#define ATARI_KBD_SCANCODE_KP_1			109
#define ATARI_KBD_SCANCODE_KP_2			110
#define ATARI_KBD_SCANCODE_KP_3			111
#define ATARI_KBD_SCANCODE_KP_0			112
#define ATARI_KBD_SCANCODE_KP_PERIOD	113
#define ATARI_KBD_SCANCODE_KP_ENTER		114
// 115
// 116
// 117
// 118
// 119
#define ATARI_KBD_SCANCODE_ALT_1		120
#define ATARI_KBD_SCANCODE_ALT_2	 	121
#define ATARI_KBD_SCANCODE_ALT_3	 	122
#define ATARI_KBD_SCANCODE_ALT_4	 	123
#define ATARI_KBD_SCANCODE_ALT_5	 	124
#define ATARI_KBD_SCANCODE_ALT_6	 	125
#define ATARI_KBD_SCANCODE_ALT_7	 	126
#define ATARI_KBD_SCANCODE_ALT_8	 	127
#define ATARI_KBD_SCANCODE_ALT_9	 	128
#define ATARI_KBD_SCANCODE_ALT_0	 	129
#define ATARI_KBD_SCANCODE_ALT_MINUS	130
#define ATARI_KBD_SCANCODE_ALT_EQUAL	131
// 132


// XCmd-Kommandos:

enum eXCMD
{
	eXCMDVersion = 0,
	eXCMDMaxCmd = 1,
	eXCMDLoadByPath = 10,
	eXCMDLoadByLibName = 11,
	eXCMDGetSymbolByName = 12,
	eXCMDGetSymbolByIndex = 13,
	eUnLoad = 14
};

// Befehlsformat f�r XCmd-Kommandos:

struct strXCMD
{
	uint32_t	m_cmd;			// ->	Kommando
	uint32_t	m_LibHandle;	// <->	Connection-ID (je nach Kommando IN oder OUT)
	uint32_t	m_MacError;		// ->	Mac-Fehlercode
	union
	{
		struct
		{
			char m_PathOrName[256];	// ->	Pfad (Kommando 10) oder Name
			int32_t m_nSymbols;		// <-	Anzahl Symbole beim �ffnen
		} m_10_11;
		struct
		{
			uint32_t m_Index;		// ->	Index (Kommando 13)
			char m_Name[256];		// ->	Symbolname (Kommando 12)
									// <-	Symbolname (Kommando 13)
			uint32_t m_SymPtr;		// <-	Zeiger auf Symbol
			uint8_t m_SymClass;		// <-	Symboltyp
		} m_12_13;
	};
};


/*
struct CPPCCallback
{
	uint32_t (*Callback)(void *params1, void *params2, unsigned char *AdrOffset68k);
	void *params1;
};
*/

class CMagiC;
struct CMagiC_CPPCCallback
{
	typedef uint32_t (CMagiC::*CMagiC_PPCCallback)(uint32_t params, unsigned char *AdrOffset68k);
	CMagiC_PPCCallback m_Callback;
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CMagiC *m_thisptr;
};

class CMacXFS;
struct CMacXFS_CPPCCallback
{
	typedef int32_t (CMacXFS::*CMacXFS_PPCCallback)(uint32_t params, unsigned char *AdrOffset68k);
	CMacXFS_PPCCallback m_Callback;
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CMacXFS *m_thisptr;
};

class CXCmd;
struct CXCmd_CPPCCallback
{
	typedef int32_t (CXCmd::*CXCmd_PPCCallback)(uint32_t params, unsigned char *AdrOffset68k);
	CXCmd_PPCCallback m_Callback;		// gcc: 2 words, mwc: 3 words
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CXCmd *m_thisptr;
};

typedef uint32_t (*PPCCallback)(uint32_t params, unsigned char *AdrOffset68k);
//typedef uint32_t (CMagiC::*PPCCallback)(void *params, unsigned char *AdrOffset68k);

typedef struct
{
	uint8_t			*baseAddr;		/* pointer to pixels */
	uint16_t		rowBytes;		/* offset to next line */
//	Rect			bounds;			/* encloses bitmap */
	uint16_t bounds_top;			/* oberste Zeile */
	uint16_t bounds_left;			/* erste Spalte */
	uint16_t bounds_bottom;			/* unterste Zeile */
	uint16_t bounds_right;			/* letzte Spalte */
	uint16_t			pmVersion;	/* pixMap version number */
	uint16_t			packType;	/* defines packing format */
	uint32_t			packSize;	/* length of pixel data */
	int32_t			hRes;			/* horiz. resolution (ppi), in fact of type "Fixed" */
	int32_t			vRes;			/* vert. resolution (ppi), in fact of type "Fixed" */
	uint16_t			pixelType;	/* defines pixel type */
	uint16_t			pixelSize;	/* # bits in pixel */
	uint16_t			cmpCount;	/* # components in pixel */
	uint16_t			cmpSize;	/* # bits per component */
	uint32_t			planeBytes;	/* offset to next plane */
	uint8_t           *pmTable;		/* color map for this pixMap (definiert CtabHandle), in fact of type CTabHandle */
	uint32_t			pmReserved;	/* for future use. MUST BE 0 */
} MXVDI_PIXMAP;

struct OldMmSysHdr
{
	uint32_t	magic;				// ist 'MagC'
	uint32_t	syshdr;				// Adresse des Atari-Syshdr
	uint32_t	keytabs;			// 5*128 Bytes f�r Tastaturtabellen
	uint32_t	ver;				// Version
	uint16_t	cpu;				// CPU (30=68030, 40=68040)
	uint16_t	fpu;				// FPU (0=nix,4=68881,6=68882,8=68040)
	uint32_t	boot_sp;			// sp f�rs Booten
	uint32_t	biosinit;			// nach Initialisierung aufrufen
	uint32_t	pixmap;				// Daten f�rs VDI
	uint32_t	offs_32k;			// Adressenoffset f�r erste 32k im MAC
	uint32_t	a5;					// globales Register a5 f�r Mac-Programm
	uint32_t	tasksw;				// != NULL, wenn Taskswitch erforderlich
	uint32_t	gettime;			// Datum und Uhrzeit ermitteln
	uint32_t	bombs;				// Atari-Routine, wird vom MAC aufgerufen
	uint32_t	syshalt;			// "System halted", String in a0
	uint32_t	coldboot;
	uint32_t	debugout;			// f�rs Debugging
	uint32_t	prtis;				// 	F�r Drucker (PRT)
	uint32_t	prtos;				//
	uint32_t	prtin;				//
	uint32_t	prtout;				//
	uint32_t	serconf;			//	Rsconf f�r ser1
	uint32_t	seris;				//    F�r ser1 (AUX)
	uint32_t	seros;				//
	uint32_t	serin;				//
	uint32_t	serout;				//
	uint32_t	xfs;				// Routinen f�r das XFS
	uint32_t	xfs_dev;			//  Zugeh�riger Dateitreiber
	uint32_t	set_physbase;		// Bildschirmadresse bei Setscreen umsetzen (a0 zeigt auf den Stack von Setscreen())
	uint32_t	VsetRGB;			// Farbe setzen (a0 zeigt auf den Stack bei VsetRGB())
	uint32_t	VgetRGB;			// Farbe erfragen (a0 zeigt auf den Stack bei VgetRGB())
	uint32_t	error;				// Fehlermeldung in d0.l an das Mac-System zur�ckgeben
						     		//	Fehlermeldungen bei MacSys_error:
									//	-1: nicht unterst�tzte Grafikaufl�sung => kein VDI-Treiber
	uint32_t	init;				// Wird beim Warmstart des Atari aufgerufen
	uint32_t	drv2devcode;		// umrechnen Laufwerk->Devicenummer
	uint32_t	rawdrvr;			// Raw-Driver (Eject) f�r Mac
	uint32_t	floprd;
	uint32_t	flopwr;
	uint32_t	flopfmt;
	uint32_t	flopver;
	uint32_t	superstlen;			// Gr��e des Supervisorstack pro APP
	uint32_t	dos_macfn;			// DOS-Funktionen 0x60..0xfe
	uint32_t	settime;			// xbios Settime
	uint32_t	prn_wrts;			// String auf Drucker
	uint32_t	version;			// Versionsnummer der Struktur
	uint32_t	in_interrupt;		// Interruptz�hler f�r Mac-Seite
	uint32_t	drv_fsspec;			// Liste der FSSpec f�r Mac-Laufwerke
	uint32_t	cnverr;				// LONG cnverr( WORD mac_errcode )
	uint32_t	res1;				// reserviert
	uint32_t	res2;				// reserviert
	uint32_t	res3;				// reserviert
};

// Die Cookie-Struktur wird vom Emulator bereitgestellt. Ihre Adresse
// erh�lt der Kernel �ber die �bergabestruktur. Ausgef�llt werden die
// Felder vom Kernel, au�er den ersten dreien.

struct MgMxCookieData
{
	uint32_t	mgmx_magic;			// ist "MgMx"
	uint32_t	mgmx_version;		// Versionsnummer
	uint32_t	mgmx_len;			// Strukturl�nge
	uint32_t	mgmx_xcmd;			// PPC-Bibliotheken laden und verwalten
	uint32_t	mgmx_xcmd_exec;		// PPC-Aufruf aus PPC-Bibliothek
	uint32_t	mgmx_internal;		// 68k-Adresse der �bergabestruktur
	uint32_t	mgmx_daemon;		// Routine f�r den "mmx.prg"-Hintergrundproze�
};

/*
PTRLEN	EQU	4		; Zeiger auf Elementfunktion braucht 4 Zeiger

	OFFSET

;Atari -> Mac
MacSysX_magic:			DS.L 1		; ist 'MagC'
MacSysX_len:			DS.L 1		; L�nge der Struktur
MacSysX_syshdr:			DS.L 1		; Adresse des Atari-Syshdr
MacSysX_keytabs:		DS.L 1		; 5*128 Bytes f�r Tastaturtabellen
MacSysX_mem_root:		DS.L 1		; Speicherlisten
MacSysX_act_pd:			DS.L 1		; Zeiger auf aktuellen Proze�
MacSysX_act_appl:		DS.L 1		; Zeiger auf aktuelle Task
MacSysX_verAtari:		DS.L 1		; Versionsnummer MagicMacX.OS
;Mac -> Atari
MacSysX_verMac:			DS.L 1		; Versionsnummer der Struktur
MacSysX_cpu:			DS.W 1		; CPU (30=68030, 40=68040)
MacSysX_fpu:			DS.W 1		; FPU (0=nix,4=68881,6=68882,8=68040)
MacSysX_init:			DS.L PTRLEN	; Wird beim Warmstart des Atari aufgerufen
MacSysX_biosinit:		DS.L PTRLEN	; nach Initialisierung aufrufen
MacSysX_VdiInit:		DS.L PTRLEN	; nach Initialisierung des VDI aufrufen
MacSysX_pixmap:			DS.L 1		; Daten f�rs VDI
MacSysX_pMMXCookie:		DS.L 1		; 68k-Zeiger auf MgMx-Cookie
MacSysX_Xcmd:			DS.L PTRLEN	; XCMD-Kommandos
MacSysX_PPCAddr:		DS.L 1		; tats. PPC-Adresse von 68k-Adresse 0
MacSysX_VideoAddr:		DS.L 1		; tats. PPC-Adresse des Bildschirmspeichers
MacSysX_Exec68k:		DS.L PTRLEN	; hier kann der PPC-Callback 68k-Code ausf�hren
MacSysX_gettime:		DS.L 1		; LONG GetTime(void) Datum und Uhrzeit ermitteln
MacSysX_settime:		DS.L 1		; void SetTime(LONG *time) Datum/Zeit setzen
MacSysX_Setpalette:		DS.L 1		; void Setpalette( int ptr[16] )
MacSysX_Setcolor:		DS.L 1		; int Setcolor( int nr, int val )
MacSysX_VsetRGB:		DS.L 1		; void VsetRGB( WORD index, WORD count, LONG *array )
MacSysX_VgetRGB:		DS.L 1		; void VgetRGB( WORD index, WORD count, LONG *array )
MacSysX_syshalt:		DS.L 1		; SysHalt( char *str ) "System halted"
MacSysX_syserr:			DS.L 1		; SysErr( long val ) "a1 = 0 => Bomben"
MacSysX_coldboot:		DS.L 1		; ColdBoot(void) Kaltstart ausf�hren
MacSysX_exit:			DS.L 1		; Exit(void) beenden
MacSysX_debugout:		DS.L 1		; MacPuts( char *str ) f�rs Debugging
MacSysX_error:			DS.L 1		; d0 = -1: kein Grafiktreiber
MacSysX_prtos:			DS.L 1		; Bcostat(void) f�r PRT
MacSysX_prtin:			DS.L 1		; Cconin(void) f�r PRT
MacSysX_prtout:			DS.L 1		; Cconout( void *params ) f�r PRT
MacSysX_prn_wrts:		DS.L 1		; LONG PrnWrts({char *buf, LONG count}) String auf Drucker
MacSysX_serconf:		DS.L 1		; Rsconf( void *params ) f�r ser1
MacSysX_seris:			DS.L 1		; Bconstat(void) f�r ser1 (AUX)
MacSysX_seros:			DS.L 1		; Bcostat(void) f�r ser1
MacSysX_serin:			DS.L 1		; Cconin(void) f�r ser1
MacSysX_serout:			DS.L 1		; Cconout( void *params ) f�r ser1
MacSysX_SerOpen:		DS.L 1		; Serielle Schnittstelle �ffnen
MacSysX_SerClose:		DS.L 1		; Serielle Schnittstelle schlie�en
MacSysX_SerRead:		DS.L 1		; Mehrere Zeichen von seriell lesen
MacSysX_SerWrite:		DS.L 1		; Mehrere Zeichen auf seriell schreiben
MacSysX_SerStat:		DS.L 1		; Lese-/Schreibstatus f�r serielle Schnittstelle
MacSysX_SerIoctl:		DS.L 1		; Ioctl-Aufrufe f�r serielle Schnittstelle
MacSysX_GetKbOrMous:	DS.L PTRLEN	; Liefert Tastatur/Maus
MacSysX_dos_macfn:		DS.L 1		; DosFn({int,void*} *) DOS-Funktionen 0x60..0xfe
MacSysX_xfs_version:	DS.L 1		; Version des Mac-XFS
MacSysX_xfs_flags:		DS.L 1		; Flags f�r das Mac-XFS
MacSysX_xfs:			DS.L PTRLEN	; zentrale Routine f�r das XFS
MacSysX_xfs_dev:		DS.L PTRLEN	; zugeh�riger Dateitreiber
MacSysX_drv2devcode:	DS.L PTRLEN	; umrechnen Laufwerk->Devicenummer
MacSysX_rawdrvr:		DS.L PTRLEN	; LONG RawDrvr({int, long} *) Raw-Driver (Eject) f�r Mac
MacSysX_Daemon:			DS.L PTRLEN	; Aufruf f�r den mmx-Daemon
MacSysX_Yield:			DS.L 1		; Aufruf f�r Rechenzeit abgeben
MacSys_OldHdr:			DS.L 49		; Kompatibilit�t mit Behnes
MacSysX_sizeof:

	TEXT

; Prozedur aufrufen. a0 auf Zeiger, a1 ist Parameter.

MACRO	MACPPC
		DC.W $00c0
		ENDM

; Elementfunktion aufrufen. a0 auf 4 Zeiger, a1 ist Parameter

MACRO	MACPPCE
		DC.W $00c1
		ENDM
*/

struct MacXSysHdr
{
	// Atari -> Mac
	uint32_t	MacSys_magic;		// ist 'MagC'
	uint32_t	MacSys_len;			// L�nge der Struktur
	uint32_t	MacSys_syshdr;		// Adresse des Atari-Syshdr
	uint32_t	MacSys_keytabs;		// 5*128 Bytes f�r Tastaturtabellen
	uint32_t	MacSys_mem_root;	// Speicherlisten
	uint32_t	MacSys_act_pd;		// Zeiger auf aktuellen Proze�
	uint32_t	MacSys_act_appl;	// Zeiger auf aktuelle Task
	uint32_t	MacSys_verAtari;	// Versionsnummer MagicMacX.OS
	//Mac -> Atari
	uint32_t	MacSys_verMac;		// Versionsnummer der Struktur
	uint16_t	MacSys_cpu;			// CPU (20 = 68020, 30=68030, 40=68040)
	uint16_t	MacSys_fpu;			// FPU (0=nix,4=68881,6=68882,8=68040)
	CMagiC_CPPCCallback	MacSys_init;	// Wird beim Warmstart des Atari aufgerufen
	CMagiC_CPPCCallback	MacSys_biosinit;	// nach Initialisierung aufrufen
	CMagiC_CPPCCallback	MacSys_VdiInit;	// nach Initialisierung des VDI aufrufen
	uint32_t	MacSys_pixmap;		// 68k-Zeiger, Daten f�rs VDI
	uint32_t	MacSys_pMMXCookie;	// 68k-Zeiger auf MgMx-Cookie
	CXCmd_CPPCCallback	MacSys_Xcmd;	// XCMD-Kommandos
	void		*MacSys_PPCAddr;	// tats. PPC-Adresse von 68k-Adresse 0
	void		*MacSys_VideoAddr;	// tats. PPC-Adresse des Bildschirmspeichers
	CMagiC_CPPCCallback	MacSys_Exec68k;	// hier kann der PPC-Callback 68k-Code ausf�hren
	void		*MacSys_gettime;	// LONG GetTime(void) Datum und Uhrzeit ermitteln
	void		*MacSys_settime;	// void SetTime(LONG *time) Datum/Zeit setzen
	void		*MacSys_Setpalette;	// void Setpalette( int ptr[16] )
	void		*MacSys_Setcolor;	// int Setcolor( int nr, int val )
	void		*MacSys_VsetRGB;	// void VsetRGB( WORD index, WORD count, LONG *array )
	void		*MacSys_VgetRGB;	// void VgetRGB( WORD index, WORD count, LONG *array )
	void		*MacSys_syshalt;	// SysHalt( char *str ) "System halted"
	void		*MacSys_syserr;		// SysErr( void ) Bomben
	void		*MacSys_coldboot;	// ColdBoot(void) Kaltstart ausf�hren
	void		*MacSys_exit;		// Exit(void) beenden
	void		*MacSys_debugout;	// MacPuts( char *str ) f�rs Debugging
	void		*MacSys_error;		// d0 = -1: kein Grafiktreiber
	void		*MacSys_prtos;		// Bcostat(void) f�r PRT
	void		*MacSys_prtin;		// Cconin(void) f�r PRT
	void		*MacSys_prtout;		// Cconout( void *params ) f�r PRT
	void		*MacSys_prtouts;	// LONG PrtOuts({char *buf, LONG count}) String auf Drucker
	void		*MacSys_serconf;	// Rsconf( void *params ) f�r ser1
	void		*MacSys_seris;		// Bconstat(void) f�r ser1 (AUX)
	void		*MacSys_seros;		// Bcostat(void) f�r ser1
	void		*MacSys_serin;		// Cconin(void) f�r ser1
	void		*MacSys_serout;		// Cconout( void *params ) f�r ser1
	void		*MacSys_SerOpen;	// Serielle Schnittstelle �ffnen
	void		*MacSys_SerClose;	// Serielle Schnittstelle schlie�en
	void		*MacSys_SerRead;	// Lesen(buffer, len) => gelesene Zeichen
	void		*MacSys_SerWrite;	// Schreiben(buffer, len) => geschriebene Zeichen
	void		*MacSys_SerStat;	// Lese-/Schreibstatus
	void		*MacSys_SerIoctl;	// Ioctl-Aufrufe f�r serielle Schnittstelle
	CMagiC_CPPCCallback	MacSys_GetKeybOrMouse;	// Wird im Interrupt 6 aufgerufen
	void		*MacSys_dos_macfn;	// DosFn({int,void*} *) DOS-Funktionen 0x60..0xfe
	uint32_t	MacSys_xfs_version;
	uint32_t	MacSys_xfs_flags;
	CMacXFS_CPPCCallback	MacSys_xfs;	// Routine f�r das XFS
	CMacXFS_CPPCCallback	MacSys_xfs_dev;	//  Zugeh�riger Dateitreiber
	CMacXFS_CPPCCallback	MacSys_drv2devcode;	// umrechnen Laufwerk->Devicenummer
	CMacXFS_CPPCCallback	MacSys_rawdrvr;	// LONG RawDrvr({int, long} *) Raw-Driver (Eject) f�r Mac
	CMagiC_CPPCCallback	MacSys_Daemon;	// Aufruf f�r den mmx-Daemon
	void		*MacSys_Yield;		// Rechenzeit abgeben
	OldMmSysHdr	MacSys_OldHdr;		// f�r Kompatibilit�t mit Behnes VDI
};

struct MagiC_SA
{
	uint32_t	_ATARI_sa_handler;	// 0x00: Signalhandler
	uint32_t	_ATARI_sa_sigextra;	// 0x04: OR-Maske bei Ausf�hrung des Signals
	uint16_t	_ATARI_sa_flags;
};

struct MagiC_FH
{
	uint32_t	fh_fd;
	uint16_t	fh_flag;
};

struct MagiC_ProcInfo
{
	uint32_t	pr_magic;			/* magischer Wert, �hnlich wie bei MiNT */
	uint16_t	pr_ruid;			/* "real user ID" */
	uint16_t	pr_rgid;			/* "real group ID" */
	uint16_t	pr_euid;			/* "effective user ID" */
	uint16_t	pr_egid;			/* "effective group ID" */
	uint16_t	pr_suid;			/* "saved user ID" */
	uint16_t	pr_sgid;			/* "saved group ID" */
	uint16_t	pr_auid;			/* "audit user ID" */
	uint16_t	pr_pri;				/* "base process priority" (nur dummy) */
	uint32_t	pr_sigpending;		/* wartende Signale */
	uint32_t	pr_sigmask;			/* Signalmaske */
	MagiC_SA	pr_sigdata[32];
	uint32_t	pr_usrva;			/* "User"-Wert (ab 9/96)	*/
	uint32_t	pr_memlist;			/* Tabelle der "shared memory blocks" */
	char		pr_fname[128];		/* Pfad der zugeh�rigen PRG-Datei */
	char		pr_cmdlin[128];		/* Urspr�ngliche Kommandozeile */
	uint16_t	pr_flags;			/* Bit 0: kein Eintrag in u:\proc */
									/* Bit 1: durch Pfork() erzeugt */
	uint8_t	pr_procname[10];		/* Proze�name f�r u:\proc\ ohne Ext. */
	uint16_t	pr_bconmap;			/* z.Zt. unbenutzt */
	MagiC_FH	pr_hndm6;			/* Handle -6: unbenutzt */
	MagiC_FH	pr_hndm5;			/* Handle -5: unbenutzt */
	MagiC_FH	pr_hndm4;			/* Handle -4: standardm��ig NUL: */
	MagiC_FH	pr_hndm3;			/* Handle -3: standardm��ig PRN: */
	MagiC_FH	pr_hndm2;			/* Handle -2: standardm��ig AUX: */
	MagiC_FH	pr_hndm1;			/* Handle -1: standardm��ig CON: */
	MagiC_FH	pr_handle[32];		/* Handles 0..31 */
};

struct MagiC_PD
{
	uint32_t	p_lowtpa;		/* 0x00: Beginn TPA, des BP selbst */
	uint32_t	p_hitpa;		/* 0x04: zeigt 1 Byte hinter TPA */
	uint32_t	p_tbase;		/* 0x08: Beginn des TEXT - Segments */
	uint32_t	p_tlen;			/* 0x0c: L�nge  des TEXT - Segments */
	uint32_t	p_dbase;		/* 0x10: Beginn des DATA - Segments */
	uint32_t	p_dlen;			/* 0x14: L�nge  des DATA - Segments */
	uint32_t	p_bbase;		/* 0x18: Beginn des BSS  - Segments */
	uint32_t	p_blen;			/* 0x1c: L�nge  des BSS  - Segments */
	uint32_t	p_dta;			/* 0x20: Aktueller DTA- Puffer */
	uint32_t	p_parent;		/* 0x24: Zeiger auf BP des Parent */
	uint16_t	p_procid;		/* 0x28: Proze�- ID */
	uint16_t	p_status;		/* 0x2a: ab MagiC 5.04 */
	uint32_t	p_env;			/* 0x2c: Zeiger auf Environment */
	uint8_t		p_devx[6];		/* 0x30: std-Handle <=> phs. Handle */
	uint8_t		p_flags;		/* 0x36: Bit 0: Pdomain (MiNT:1/TOS:0) */
	uint8_t		p_defdrv;		/* 0x37: Default- Laufwerk */
	uint8_t		p_res3[8];		/* 0x38: Terminierungskontext f�r ACC */
	uint8_t		p_drvx[32];		/* 0x40: Tabelle: Default-Path-Hdl. */
	uint32_t	p_procdata;		/* 0x60: Zeiger auf PROCDATA */
	uint16_t	p_umask;		/* 0x64: umask f�r Unix-Dateisysteme */
	uint16_t	p_procgroup;	/* 0x66: Proze�gruppe (ab 6.10.96) */
	uint32_t	p_mem;			/* 0x68: soviel Speicher darf ich holen */
	uint32_t	p_context;		/* 0x6c: unter MAGIX statt p_reg benutzt */
	uint32_t	p_mflags;		/* 0x70: Bit 2: Malloc aus AltRAM erlaubt */
	uint32_t	p_app;			/* 0x74: APPL, die den Proze� gestartet	hat (main thread) */
	uint32_t	p_ssp;			/* 0x78: ssp bei Start des Prozesses */
	uint32_t	p_reg;			/* 0x7c: f�r Kompatibilit�t mit TOS */
	uint8_t		p_cmdlin[128];	/* 0x80: Kommandozeile */
};

/* Werte f�r ap_status */

enum MagiC_APP_STATUS
{
	eAPSTAT_READY = 0,
	eAPSTAT_WAITING = 1,
	eAPSTAT_SUSPENDED = 2,
	eAPSTAT_ZOMBIE = 3,
	eAPSTAT_STOPPED = 4
};

struct MagiC_APP
{
	uint32_t  ap_next;			// Verkettungszeiger
	uint16_t  ap_id;			// Application-ID
	uint16_t  ap_parent;		// tats�chliche parent-ID
	uint16_t  ap_parent2;		// ggf. die ap_id des VT52, dorthin ->CH_EXIT
	uint16_t  ap_type;			// 0 = Main Thread/1 = Thread/2 = Signal Handler
	uint32_t  ap_oldsigmask;	// Alte Signalmaske (f�r Signal-Handler)
	uint32_t  ap_sigthr;		// Haupt-Thread: Zeiger auf aktiven Signalhandler
								// Signalhandler: Zeiger auf vorherigen oder NULL
	uint16_t  ap_srchflg;		// f�r appl_search
	uint32_t  ap_menutree;		// Men�baum
	uint32_t  ap_attached;		// NULL oder Liste f�r menu_attach()
	uint32_t  ap_desktree;		// Desktop-Hintergrund
	uint16_t  ap_1stob;			//  dazu erstes Objekt
	uint8_t   ap_dummy1[2];		// zwei Leerzeichen vor ap_name
	uint8_t   ap_name[8];		// Name (8 Zeichen mit trailing blanks)
	uint8_t   ap_dummy2[2];		// Leerstelle und ggf. Ausblendzeichen
	uint8_t   ap_dummy3;		// Nullbyte f�r EOS
	uint8_t   ap_status;		// APSTAT_...
	uint16_t  ap_hbits;			// eingetroffene Events
	uint16_t  ap_rbits;			// erwartete Events
	uint32_t  ap_evparm;		// Event-Daten, z.B. <pid> oder msg-Puffer
	uint32_t  ap_nxttim;		// N�chste auf Timer wartende APP
	uint32_t  ap_ms;			// Timer
	uint32_t  ap_nxtalrm;		// N�chste auf Alarm wartende APP
	uint32_t  ap_alrmms;		// Alarm
	uint16_t  ap_isalarm;		// Flag
	uint32_t  ap_nxtsem;		// N�chste auf Semaphore wartende APP
	uint32_t  ap_semaph;		// auf diese Semaphore warten wir
	uint16_t  ap_unselcnt;		// L�nge der Tabelle ap_unselx
	uint32_t  ap_unselx;		// Tabelle f�r evnt_(m)IO
	uint32_t  ap_evbut;			// f�r evnt_button
	uint32_t  ap_mgrect1;		// f�r evnt_mouse
	uint32_t  ap_mgrect2;		// f�r evnt_mouse
	uint16_t  ap_kbbuf[8];		// Puffer f�r 8 Tasten
	uint16_t  ap_kbhead;		// N�chstes zu lesendes Zeichen
	uint16_t  ap_kbtail;		// N�chstes zu schreibendes Zeichen
	uint16_t  ap_kbcnt;			// Anzahl Zeichen im Puffer
	uint16_t  ap_len;			// Message- Pufferl�nge
	uint8_t   ap_buf[0x300];	// Message- Puffer (768 Bytes = 48 Nachrichten)
	uint16_t  ap_critic;			// Z�hler f�r "kritische Phase"
	uint8_t   ap_crit_act;		// Bit 0: killed
								// Bit 1: stopped
								// Bit 2: Signale testen
	uint8_t   ap_stpsig;		// Flag "durch Signal gestoppt"
	uint32_t  ap_sigfreeze;		// Signalhandler f�r SIGFREEZE
	uint16_t  ap_recogn;		// Bit 0: verstehe AP_TERM
	uint32_t  ap_flags;			// Bit 0: will keinen prop. AES-Zeichensatz
	uint16_t  ap_doex;
	uint16_t  ap_isgr;
	uint16_t  ap_wasgr;
	uint16_t  ap_isover;
	uint32_t  ap_ldpd;			// PD des Loader-Prozesses
	uint32_t  ap_env;			// Environment oder NULL
	uint32_t  ap_xtail;			// Erw. Kommandozeile (> 128 Bytes) od. NULL
	uint32_t  ap_thr_usp;		// usp f�r Threads
	uint32_t  ap_memlimit;
	uint32_t  ap_nice;			// z.Zt. unbenutzt
	uint8_t   ap_cmd[128];		// Programmpfad
	uint8_t   ap_tai[128];		// Programmparameter
	uint16_t  ap_mhidecnt;		// lokaler Maus-Hide-Counter
	uint16_t  ap_svd_mouse[37];	// x/y/planes/bg/fg/msk[32]/moff_cnt
	uint16_t  ap_prv_mouse[37];
	uint16_t  ap_act_mouse[37];
	uint32_t  ap_ssp;
	uint32_t  ap_pd;
	uint32_t  ap_etvterm;
	uint32_t  ap_stkchk;		// magisches Wort f�r Stack�berpr�fung
	uint8_t   ap_stack[0];		// Stack
};

   	#pragma options align=reset
#endif
