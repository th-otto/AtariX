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
* Enthält alle Konstanten und Strukturen für den Atari
*
*/

#ifndef ATARI_H_INCLUDED
#define ATARI_H_INCLUDED

// define UINT32 et cetera
#include "osd_cpu.h"

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
	UINT16		actime;          /* Zugriffszeit */
	UINT16		acdate;
	UINT16		modtime;         /* letzte Änderung */
	UINT16		moddate;
};

/* structure for getxattr (-> MiNT) */

struct XATTR
{
	UINT16	mode;
	/* file types */
	#define _ATARI_S_IFMT 0170000        /* mask to select file type */
	#define _ATARI_S_IFCHR     0020000        /* BIOS special file */
	#define _ATARI_S_IFDIR     0040000        /* directory file */
	#define _ATARI_S_IFREG 0100000       /* regular file */
	#define _ATARI_S_IFIFO 0120000       /* FIFO */
	#define _ATARI_S_IMEM 0140000        /* memory region or process */
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
	UINT32	index;
	UINT16	dev;
	UINT16	reserved1;
	UINT16	nlink;
	UINT16	uid;
	UINT16	gid;
	UINT32	size;
	UINT32	blksize, nblocks;
	UINT16	mtime, mdate;
	UINT16	atime, adate;
	UINT16	ctime, cdate;
	UINT16	attr;
	UINT16	reserved2;
	UINT32	reserved3[2];
};

struct BasePage
{
	void *p_lowtpa;				//   0 (0x00)
	void *p_hitpa;				//   4 (0x04)
	void *p_tbase;				//   8 (0x08)
	unsigned long p_tlen;		//   12 (0x0c)
	void *p_dbase;				//   16 (0x10)
	unsigned long p_dlen;		//   20 (0x14)
	void *p_bbase;				//   24 (0x18)
	unsigned long p_blen;
	void *p_dta;
	void *p_parent;
	long unused1;
	void *p_env;
	char p_devx[6];
	char unused2;
	char p_defdrv;
	long unused[2+16];
	unsigned char p_cmdline[128];
};

struct ExeHeader
{
	short	code;
	unsigned long tlen;
	unsigned long dlen;
	unsigned long blen;
	unsigned long slen;
	long	dum1;
	long	dum2;
	short	relmod;
};

/* Systemvariable _sysbase (0x4F2L) zeigt auf: */

struct SYSHDR
{
	UInt16		os_entry;		/* $00 BRA to reset handler             */
	UInt16		os_version;		/* $02 TOS version number               */
	UInt32		os_start;		/* $04 -> reset handler                 */
	UInt32		os_base;		/* $08 -> baseof OS                     */
	UInt32		os_membot;	/* $0c -> end BIOS/GEMDOS/VDI ram usage */
	UInt32		os_rsv1;		/* $10 << unused,reserved >>            */
	UInt32		os_magic;		/* $14 -> GEM memory usage parm. block   */
	UInt32		os_gendat;		/* $18 Date of system build($MMDDYYYY)  */
	UInt16		os_palmode;	/* $1c OS configuration bits            */
	UInt16		os_gendatg;	/* $1e DOS-format date of system build   */
	/*
	Die folgenden Felder gibt es ab TOS 1.2
	*/
	UInt32		_root;		/* $20 -> base of OS pool               */
	UInt32		kbshift;		/* $24 -> 68-Adresse der Atari-Variablen "kbshift" und "kbrepeat" */
	UInt32		_run;			/* $28 -> GEMDOS PID of current process */
	UInt32		p_rsv2;		/* $2c << unused, reserved >>           */
};

/* Interruptvektoren */

#define	INTV_0_RESET_SSP        0x000	/* ssp bei Reset */
#define	INTV_1_RESET_PC         0x004	/* pc bei Reset */
#define	INTV_2_BUS_ERROR        0x008	/* Busfehler */
#define	INTV_3_ADDRESS_ERROR	0x00c	/* Adreßfehler */
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
#define ATARI_KBD_SCANCODE_MINUS		12				// de: 'ß'  us: '-'
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
#define ATARI_KBD_SCANCODE_LEFTBRACKET	26				// de: 'Ü' us: '['
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
#define ATARI_KBD_SCANCODE_SEMICOLON	39				// de: 'Ö'  us: ';'
#define ATARI_KBD_SCANCODE_APOSTROPHE	40				// de: 'Ä'  us: '''
#define ATARI_KBD_SCANCODE_NUMBER		41				// de: '#'
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

// Befehlsformat für XCmd-Kommandos:

struct strXCMD
{
	UINT32	m_cmd;		// ->	Kommando
	UINT32	m_LibHandle;	// <->	Connection-ID (je nach Kommando IN oder OUT)
	UINT32	m_MacError;	// ->	Mac-Fehlercode
	union
	{
		struct
		{
			char m_PathOrName[256];	// ->	Pfad (Kommando 10) oder Name
			INT32 m_nSymbols;		// <-	Anzahl Symbole beim Öffnen
		} m_10_11;
		struct
		{
			UINT32 m_Index;			// ->	Index (Kommando 13)
			char m_Name[256];		// ->	Symbolname (Kommando 12)
								// <-	Symbolname (Kommando 13)
			UINT32 m_SymPtr;		// <-	Zeiger auf Symbol
			UINT8 m_SymClass;		// <-	Symboltyp
		} m_12_13;
	};
};


/*
struct CPPCCallback
{
	UINT32 (*Callback)(void *params1, void *params2, unsigned char *AdrOffset68k);
	void *params1;
};
*/

class CMagiC;
struct CMagiC_CPPCCallback
{
	typedef UINT32 (CMagiC::*CMagiC_PPCCallback)(UINT32 params, unsigned char *AdrOffset68k);
	CMagiC_PPCCallback m_Callback;
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CMagiC *m_thisptr;
};

class CMacXFS;
struct CMacXFS_CPPCCallback
{
	typedef INT32 (CMacXFS::*CMacXFS_PPCCallback)(UINT32 params, unsigned char *AdrOffset68k);
	CMacXFS_PPCCallback m_Callback;
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CMacXFS *m_thisptr;
};

class CXCmd;
struct CXCmd_CPPCCallback
{
	typedef INT32 (CXCmd::*CXCmd_PPCCallback)(UINT32 params, unsigned char *AdrOffset68k);
	CXCmd_PPCCallback m_Callback;		// gcc: 2 words, mwc: 3 words
	#if defined(__GNUC__)
	UInt32 dummy;
	#endif
	CXCmd *m_thisptr;
};

typedef UINT32 (*PPCCallback)(UINT32 params, unsigned char *AdrOffset68k);
//typedef UINT32 (CMagiC::*PPCCallback)(void *params, unsigned char *AdrOffset68k);

typedef struct
{
	UINT8			*baseAddr;		/* pointer to pixels */
	UINT16			rowBytes;		/* offset to next line */
//	Rect			bounds;			/* encloses bitmap */
	UINT16 bounds_top;				/* oberste Zeile */
	UINT16 bounds_left;				/* erste Spalte */
	UINT16 bounds_bottom;			/* unterste Zeile */
	UINT16 bounds_right;			/* letzte Spalte */
	UINT16			pmVersion;		/* pixMap version number */
	UINT16			packType;		/* defines packing format */
	UINT32			packSize;		/* length of pixel data */
	INT32			hRes;			/* horiz. resolution (ppi), in fact of type "Fixed" */
	INT32			vRes;			/* vert. resolution (ppi), in fact of type "Fixed" */
	UINT16			pixelType;		/* defines pixel type */
	UINT16			pixelSize;		/* # bits in pixel */
	UINT16			cmpCount;		/* # components in pixel */
	UINT16			cmpSize;		/* # bits per component */
	UINT32			planeBytes;		/* offset to next plane */
	UINT8           *pmTable;		/* color map for this pixMap (definiert CtabHandle), in fact of type CTabHandle */
	UINT32			pmReserved;		/* for future use. MUST BE 0 */
} MXVDI_PIXMAP;

struct OldMmSysHdr
{
	UINT32	magic;			// ist 'MagC'
	UINT32	syshdr;			// Adresse des Atari-Syshdr
	UINT32	keytabs;			// 5*128 Bytes fÅr Tastaturtabellen
	UINT32	ver;				// Version
	UINT16	cpu;				// CPU (30=68030, 40=68040)
	UINT16	fpu;				// FPU (0=nix,4=68881,6=68882,8=68040)
	UINT32	boot_sp;			// sp fürs Booten
	UINT32	biosinit;			// nach Initialisierung aufrufen
	UINT32	pixmap;			// Daten fÅrs VDI
	UINT32	offs_32k;			// Adressenoffset für erste 32k im MAC
	UINT32	a5;				// globales Register a5 für Mac-Programm
	UINT32	tasksw;			// != NULL, wenn Taskswitch erforderlich
	UINT32	gettime;			// Datum und Uhrzeit ermitteln
	UINT32	bombs;			// Atari-Routine, wird vom MAC aufgerufen
	UINT32	syshalt;			// "System halted", String in a0
	UINT32	coldboot;
	UINT32	debugout;			// fürs Debugging
	UINT32	prtis;				// 	Für Drucker (PRT)
	UINT32	prtos;				//
	UINT32	prtin;				//
	UINT32	prtout;			//
	UINT32	serconf;			//	Rsconf für ser1
	UINT32	seris;				//    Für ser1 (AUX)
	UINT32	seros;			//
	UINT32	serin;				//
	UINT32	serout;			//
	UINT32	xfs;				// Routinen für das XFS
	UINT32	xfs_dev;			//  Zugehîriger Dateitreiber
	UINT32	set_physbase;		// Bildschirmadresse bei Setscreen umsetzen (a0 zeigt auf den Stack von Setscreen())
	UINT32	VsetRGB;			// Farbe setzen (a0 zeigt auf den Stack bei VsetRGB())
	UINT32	VgetRGB;			// Farbe erfragen (a0 zeigt auf den Stack bei VgetRGB())
	UINT32	error;				// Fehlermeldung in d0.l an das Mac-System zurückgeben
						          //	Fehlermeldungen bei MacSys_error:
						          //	-1: nicht unterstützte Grafikauflîsung => kein VDI-Treiber
	UINT32	init;				// Wird beim Warmstart des Atari aufgerufen
	UINT32	drv2devcode;		// umrechnen Laufwerk->Devicenummer
	UINT32	rawdrvr;			// Raw-Driver (Eject) für Mac
	UINT32	floprd;
	UINT32	flopwr;
	UINT32	flopfmt;
	UINT32	flopver;
	UINT32	superstlen;			// Größe des Supervisorstack pro APP
	UINT32	dos_macfn;			// DOS-Funktionen 0x60..0xfe
	UINT32	settime;			// xbios Settime
	UINT32	prn_wrts;			// String auf Drucker
	UINT32	version;			// Versionsnummer der Struktur
	UINT32	in_interrupt;		// Interruptzähler für Mac-Seite
	UINT32	drv_fsspec;			// Liste der FSSpec für Mac-Laufwerke
	UINT32	cnverr;			// LONG cnverr( WORD mac_errcode )
	UINT32	res1;				// reserviert
	UINT32	res2;				// reserviert
	UINT32	res3;				// reserviert
};

// Die Cookie-Struktur wird vom Emulator bereitgestellt. Ihre Adresse
// erhält der Kernel über die Übergabestruktur. Ausgefüllt werden die
// Felder vom Kernel, außer den ersten dreien.

struct MgMxCookieData
{
	UINT32	mgmx_magic;			// ist "MgMx"
	UINT32	mgmx_version;		// Versionsnummer
	UINT32	mgmx_len;			// Strukturlänge
	UINT32	mgmx_xcmd;			// PPC-Bibliotheken laden und verwalten
	UINT32	mgmx_xcmd_exec;		// PPC-Aufruf aus PPC-Bibliothek
	UINT32	mgmx_internal;		// 68k-Adresse der Übergabestruktur
	UINT32	mgmx_daemon;		// Routine für den "mmx.prg"-Hintergrundprozeß
};

/*
PTRLEN	EQU	4		; Zeiger auf Elementfunktion braucht 4 Zeiger

	OFFSET

;Atari -> Mac
MacSysX_magic:			DS.L 1		; ist 'MagC'
MacSysX_len:			DS.L 1		; Länge der Struktur
MacSysX_syshdr:			DS.L 1		; Adresse des Atari-Syshdr
MacSysX_keytabs:		DS.L 1		; 5*128 Bytes für Tastaturtabellen
MacSysX_mem_root:		DS.L 1		; Speicherlisten
MacSysX_act_pd:			DS.L 1		; Zeiger auf aktuellen Prozeß
MacSysX_act_appl:		DS.L 1		; Zeiger auf aktuelle Task
MacSysX_verAtari:		DS.L 1		; Versionsnummer MagicMacX.OS
;Mac -> Atari
MacSysX_verMac:			DS.L 1		; Versionsnummer der Struktur
MacSysX_cpu:			DS.W 1		; CPU (30=68030, 40=68040)
MacSysX_fpu:			DS.W 1		; FPU (0=nix,4=68881,6=68882,8=68040)
MacSysX_init:			DS.L PTRLEN	; Wird beim Warmstart des Atari aufgerufen
MacSysX_biosinit:		DS.L PTRLEN	; nach Initialisierung aufrufen
MacSysX_VdiInit:		DS.L PTRLEN	; nach Initialisierung des VDI aufrufen
MacSysX_pixmap:			DS.L 1		; Daten fürs VDI
MacSysX_pMMXCookie:		DS.L 1		; 68k-Zeiger auf MgMx-Cookie
MacSysX_Xcmd:			DS.L PTRLEN	; XCMD-Kommandos
MacSysX_PPCAddr:		DS.L 1		; tats. PPC-Adresse von 68k-Adresse 0
MacSysX_VideoAddr:		DS.L 1		; tats. PPC-Adresse des Bildschirmspeichers
MacSysX_Exec68k:		DS.L PTRLEN	; hier kann der PPC-Callback 68k-Code ausführen
MacSysX_gettime:		DS.L 1		; LONG GetTime(void) Datum und Uhrzeit ermitteln
MacSysX_settime:		DS.L 1		; void SetTime(LONG *time) Datum/Zeit setzen
MacSysX_Setpalette:		DS.L 1		; void Setpalette( int ptr[16] )
MacSysX_Setcolor:		DS.L 1		; int Setcolor( int nr, int val )
MacSysX_VsetRGB:		DS.L 1		; void VsetRGB( WORD index, WORD count, LONG *array )
MacSysX_VgetRGB:		DS.L 1		; void VgetRGB( WORD index, WORD count, LONG *array )
MacSysX_syshalt:		DS.L 1		; SysHalt( char *str ) "System halted"
MacSysX_syserr:			DS.L 1		; SysErr( long val ) "a1 = 0 => Bomben"
MacSysX_coldboot:		DS.L 1		; ColdBoot(void) Kaltstart ausführen
MacSysX_exit:			DS.L 1		; Exit(void) beenden
MacSysX_debugout:		DS.L 1		; MacPuts( char *str ) fürs Debugging
MacSysX_error:			DS.L 1		; d0 = -1: kein Grafiktreiber
MacSysX_prtos:			DS.L 1		; Bcostat(void) für PRT
MacSysX_prtin:			DS.L 1		; Cconin(void) für PRT
MacSysX_prtout:			DS.L 1		; Cconout( void *params ) für PRT
MacSysX_prn_wrts:		DS.L 1		; LONG PrnWrts({char *buf, LONG count}) String auf Drucker
MacSysX_serconf:		DS.L 1		; Rsconf( void *params ) für ser1
MacSysX_seris:			DS.L 1		; Bconstat(void) für ser1 (AUX)
MacSysX_seros:			DS.L 1		; Bcostat(void) für ser1
MacSysX_serin:			DS.L 1		; Cconin(void) für ser1
MacSysX_serout:			DS.L 1		; Cconout( void *params ) für ser1
MacSysX_SerOpen:		DS.L 1		; Serielle Schnittstelle öffnen
MacSysX_SerClose:		DS.L 1		; Serielle Schnittstelle schließen
MacSysX_SerRead:		DS.L 1		; Mehrere Zeichen von seriell lesen
MacSysX_SerWrite:		DS.L 1		; Mehrere Zeichen auf seriell schreiben
MacSysX_SerStat:		DS.L 1		; Lese-/Schreibstatus für serielle Schnittstelle
MacSysX_SerIoctl:		DS.L 1		; Ioctl-Aufrufe für serielle Schnittstelle
MacSysX_GetKbOrMous:	DS.L PTRLEN	; Liefert Tastatur/Maus
MacSysX_dos_macfn:		DS.L 1		; DosFn({int,void*} *) DOS-Funktionen 0x60..0xfe
MacSysX_xfs_version:	DS.L 1		; Version des Mac-XFS
MacSysX_xfs_flags:		DS.L 1		; Flags für das Mac-XFS
MacSysX_xfs:			DS.L PTRLEN	; zentrale Routine für das XFS
MacSysX_xfs_dev:		DS.L PTRLEN	; zugehöriger Dateitreiber
MacSysX_drv2devcode:	DS.L PTRLEN	; umrechnen Laufwerk->Devicenummer
MacSysX_rawdrvr:		DS.L PTRLEN	; LONG RawDrvr({int, long} *) Raw-Driver (Eject) für Mac
MacSysX_Daemon:			DS.L PTRLEN	; Aufruf für den mmx-Daemon
MacSysX_Yield:			DS.L 1		; Aufruf für Rechenzeit abgeben
MacSys_OldHdr:			DS.L 49		; Kompatibilität mit Behnes
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
	UINT32	MacSys_magic;		// ist 'MagC'
	UINT32	MacSys_len;		// Länge der Struktur
	UINT32	MacSys_syshdr;		// Adresse des Atari-Syshdr
	UINT32	MacSys_keytabs;		// 5*128 Bytes für Tastaturtabellen
	UINT32	MacSys_mem_root;	// Speicherlisten
	UINT32	MacSys_act_pd;		// Zeiger auf aktuellen Prozeû
	UINT32	MacSys_act_appl;		// Zeiger auf aktuelle Task
	UINT32	MacSys_verAtari;		// Versionsnummer MagicMacX.OS
	//Mac -> Atari
	UINT32	MacSys_verMac;		// Versionsnummer der Struktur
	UINT16	MacSys_cpu;		// CPU (20 = 68020, 30=68030, 40=68040)
	UINT16	MacSys_fpu;		// FPU (0=nix,4=68881,6=68882,8=68040)
	CMagiC_CPPCCallback	MacSys_init;	// Wird beim Warmstart des Atari aufgerufen
	CMagiC_CPPCCallback	MacSys_biosinit;	// nach Initialisierung aufrufen
	CMagiC_CPPCCallback	MacSys_VdiInit;	// nach Initialisierung des VDI aufrufen
	UINT32	MacSys_pixmap;		// 68k-Zeiger, Daten fürs VDI
	UINT32	MacSys_pMMXCookie;	// 68k-Zeiger auf MgMx-Cookie
	CXCmd_CPPCCallback	MacSys_Xcmd;	// XCMD-Kommandos
	void		*MacSys_PPCAddr;	// tats. PPC-Adresse von 68k-Adresse 0
	void		*MacSys_VideoAddr;	// tats. PPC-Adresse des Bildschirmspeichers
	CMagiC_CPPCCallback	MacSys_Exec68k;	// hier kann der PPC-Callback 68k-Code ausführen
	void		*MacSys_gettime;	// LONG GetTime(void) Datum und Uhrzeit ermitteln
	void		*MacSys_settime;		// void SetTime(LONG *time) Datum/Zeit setzen
	void		*MacSys_Setpalette;	// void Setpalette( int ptr[16] )
	void		*MacSys_Setcolor;	// int Setcolor( int nr, int val )
	void		*MacSys_VsetRGB;	// void VsetRGB( WORD index, WORD count, LONG *array )
	void		*MacSys_VgetRGB;	// void VgetRGB( WORD index, WORD count, LONG *array )
	void		*MacSys_syshalt;		// SysHalt( char *str ) "System halted"
	void		*MacSys_syserr;		// SysErr( void ) Bomben
	void		*MacSys_coldboot;	// ColdBoot(void) Kaltstart ausführen
	void		*MacSys_exit;		// Exit(void) beenden
	void		*MacSys_debugout;	// MacPuts( char *str ) fürs Debugging
	void		*MacSys_error;		// d0 = -1: kein Grafiktreiber
	void		*MacSys_prtos;		// Bcostat(void) für PRT
	void		*MacSys_prtin;		// Cconin(void) für PRT
	void		*MacSys_prtout;		// Cconout( void *params ) für PRT
	void		*MacSys_prtouts;		// LONG PrtOuts({char *buf, LONG count}) String auf Drucker
	void		*MacSys_serconf;		// Rsconf( void *params ) für ser1
	void		*MacSys_seris;		// Bconstat(void) für ser1 (AUX)
	void		*MacSys_seros;		// Bcostat(void) für ser1
	void		*MacSys_serin;		// Cconin(void) für ser1
	void		*MacSys_serout;		// Cconout( void *params ) für ser1
	void		*MacSys_SerOpen;	// Serielle Schnittstelle öffnen
	void		*MacSys_SerClose;	// Serielle Schnittstelle schließen
	void		*MacSys_SerRead;	// Lesen(buffer, len) => gelesene Zeichen
	void		*MacSys_SerWrite;	// Schreiben(buffer, len) => geschriebene Zeichen
	void		*MacSys_SerStat;		// Lese-/Schreibstatus
	void		*MacSys_SerIoctl;	// Ioctl-Aufrufe für serielle Schnittstelle
	CMagiC_CPPCCallback	MacSys_GetKeybOrMouse;	// Wird im Interrupt 6 aufgerufen
	void		*MacSys_dos_macfn;	// DosFn({int,void*} *) DOS-Funktionen 0x60..0xfe
	UINT32	MacSys_xfs_version;
	UINT32	MacSys_xfs_flags;
	CMacXFS_CPPCCallback	MacSys_xfs;	// Routine für das XFS
	CMacXFS_CPPCCallback	MacSys_xfs_dev;	//  Zugehöriger Dateitreiber
	CMacXFS_CPPCCallback	MacSys_drv2devcode;	// umrechnen Laufwerk->Devicenummer
	CMacXFS_CPPCCallback	MacSys_rawdrvr;	// LONG RawDrvr({int, long} *) Raw-Driver (Eject) für Mac
	CMagiC_CPPCCallback	MacSys_Daemon;	// Aufruf für den mmx-Daemon
	void		*MacSys_Yield;			// Rechenzeit abgeben
	OldMmSysHdr	MacSys_OldHdr;		// für Kompatibilität mit Behnes VDI
};

struct MagiC_SA
{
	UINT32	_ATARI_sa_handler;			// 0x00: Signalhandler
	UINT32	_ATARI_sa_sigextra;		// 0x04: OR-Maske bei Ausführung des Signals
	UINT16	_ATARI_sa_flags;
};

struct MagiC_FH
{
	UINT32	fh_fd;
	UINT16	fh_flag;
};

struct MagiC_ProcInfo
{
	UINT32	pr_magic;			/* magischer Wert, ähnlich wie bei MiNT */
	UINT16	pr_ruid;			/* "real user ID" */
	UINT16	pr_rgid;			/* "real group ID" */
	UINT16	pr_euid;			/* "effective user ID" */
	UINT16	pr_egid;			/* "effective group ID" */
	UINT16	pr_suid;			/* "saved user ID" */
	UINT16	pr_sgid;			/* "saved group ID" */
	UINT16	pr_auid;			/* "audit user ID" */
	UINT16	pr_pri;			/* "base process priority" (nur dummy) */
	UINT32	pr_sigpending;		/* wartende Signale */
	UINT32	pr_sigmask;		/* Signalmaske */
	MagiC_SA	pr_sigdata[32];
	UINT32	pr_usrva;			/* "User"-Wert (ab 9/96)	*/
	UINT32	pr_memlist;		/* Tabelle der "shared memory blocks" */
	char		pr_fname[128];	/* Pfad der zugehörigen PRG-Datei */
	char		pr_cmdlin[128];	/* Ursprüngliche Kommandozeile */
	UINT16	pr_flags;			/* Bit 0: kein Eintrag in u:\proc */
							/* Bit 1: durch Pfork() erzeugt */
	UINT8	pr_procname[10];	/* Prozeßname für u:\proc\ ohne Ext. */
	UINT16	pr_bconmap;		/* z.Zt. unbenutzt */
	MagiC_FH	pr_hndm6;		/* Handle -6: unbenutzt */
	MagiC_FH	pr_hndm5;		/* Handle -5: unbenutzt */
	MagiC_FH	pr_hndm4;		/* Handle -4: standardmäßig NUL: */
	MagiC_FH	pr_hndm3;		/* Handle -3: standardmäßig PRN: */
	MagiC_FH	pr_hndm2;		/* Handle -2: standardmäßig AUX: */
	MagiC_FH	pr_hndm1;		/* Handle -1: standardmäßig CON: */
	MagiC_FH	pr_handle[32];		/* Handles 0..31 */
};

struct MagiC_PD
{
	UINT32	p_lowtpa;		/* 0x00: Beginn TPA, des BP selbst */
	UINT32	p_hitpa;		/* 0x04: zeigt 1 Byte hinter TPA */
	UINT32	p_tbase;		/* 0x08: Beginn des TEXT - Segments */
	UINT32	p_tlen;		/* 0x0c: Länge  des TEXT - Segments */
	UINT32	p_dbase;		/* 0x10: Beginn des DATA - Segments */
	UINT32	p_dlen;		/* 0x14: Länge  des DATA - Segments */
	UINT32	p_bbase;		/* 0x18: Beginn des BSS  - Segments */
	UINT32	p_blen;		/* 0x1c: Länge  des BSS  - Segments */
	UINT32	p_dta;		/* 0x20: Aktueller DTA- Puffer */
	UINT32	p_parent;		/* 0x24: Zeiger auf BP des Parent */
	UINT16	p_procid;		/* 0x28: Prozeß- ID */
	UINT16	p_status;		/* 0x2a: ab MagiC 5.04 */
	UINT32	p_env;		/* 0x2c: Zeiger auf Environment */
	UINT8		p_devx[6];		/* 0x30: std-Handle <=> phs. Handle */
	UINT8		p_flags;		/* 0x36: Bit 0: Pdomain (MiNT:1/TOS:0) */
	UINT8		p_defdrv;		/* 0x37: Default- Laufwerk */
	UINT8		p_res3[8];		/* 0x38: Terminierungskontext für ACC */
	UINT8		p_drvx[32];		/* 0x40: Tabelle: Default-Path-Hdl. */
	UINT32	p_procdata;		/* 0x60: Zeiger auf PROCDATA */
	UINT16	p_umask;		/* 0x64: umask für Unix-Dateisysteme */
	UINT16	p_procgroup;	/* 0x66: Prozeûgruppe (ab 6.10.96) */
	UINT32	p_mem;		/* 0x68: soviel Speicher darf ich holen */
	UINT32	p_context;		/* 0x6c: unter MAGIX statt p_reg benutzt */
	UINT32	p_mflags;		/* 0x70: Bit 2: Malloc aus AltRAM erlaubt */
	UINT32	p_app;		/* 0x74: APPL, die den Prozeß gestartet	hat (main thread) */
	UINT32	p_ssp;		/* 0x78: ssp bei Start des Prozesses */
	UINT32	p_reg;		/* 0x7c: für Kompatibilität mit TOS */
	UINT8		p_cmdlin[128];	/* 0x80: Kommandozeile */
};

/* Werte für ap_status */

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
	UINT32  ap_next;			// Verkettungszeiger
	UINT16  ap_id;			// Application-ID
	UINT16  ap_parent;			// tatsächliche parent-ID
	UINT16  ap_parent2;		// ggf. die ap_id des VT52, dorthin ->CH_EXIT
	UINT16  ap_type;			// 0 = Main Thread/1 = Thread/2 = Signal Handler
	UINT32  ap_oldsigmask;		// Alte Signalmaske (für Signal-Handler)
	UINT32  ap_sigthr;			// Haupt-Thread: Zeiger auf aktiven Signalhandler
							// Signalhandler: Zeiger auf vorherigen oder NULL
	UINT16  ap_srchflg;			// für appl_search
	UINT32  ap_menutree;		// Menübaum
	UINT32  ap_attached;		// NULL oder Liste fÅr menu_attach()
	UINT32  ap_desktree;		// Desktop-Hintergrund
	UINT16  ap_1stob;			//  dazu erstes Objekt
	UINT8   ap_dummy1[2];		// zwei Leerzeichen vor ap_name
	UINT8   ap_name[8];		// Name (8 Zeichen mit trailing blanks)
	UINT8   ap_dummy2[2];		// Leerstelle und ggf. Ausblendzeichen
	UINT8   ap_dummy3;		// Nullbyte für EOS
	UINT8   ap_status;			// APSTAT_...
	UINT16  ap_hbits;			// eingetroffene Events
	UINT16  ap_rbits;			// erwartete Events
	UINT32  ap_evparm;			// Event-Daten, z.B. <pid> oder msg-Puffer
	UINT32  ap_nxttim;			// Nächste auf Timer wartende APP
	UINT32  ap_ms;			// Timer
	UINT32  ap_nxtalrm;		// Nächste auf Alarm wartende APP
	UINT32  ap_alrmms;			// Alarm
	UINT16  ap_isalarm;			// Flag
	UINT32  ap_nxtsem;			// Nächste auf Semaphore wartende APP
	UINT32  ap_semaph;			// auf diese Semaphore warten wir
	UINT16  ap_unselcnt;		// Länge der Tabelle ap_unselx
	UINT32  ap_unselx;			// Tabelle für evnt_(m)IO
	UINT32  ap_evbut;			// für evnt_button
	UINT32  ap_mgrect1;		// für evnt_mouse
	UINT32  ap_mgrect2;		// für evnt_mouse
	UINT16  ap_kbbuf[8];		// Puffer für 8 Tasten
	UINT16  ap_kbhead;			// Nächstes zu lesendes Zeichen
	UINT16  ap_kbtail;			// Nächstes zu schreibendes Zeichen
	UINT16  ap_kbcnt;			// Anzahl Zeichen im Puffer
	UINT16  ap_len;			// Message- Pufferlänge
	UINT8   ap_buf[0x300];		// Message- Puffer (768 Bytes = 48 Nachrichten)
	UINT16  ap_critic;			// Zähler für "kritische Phase"
	UINT8   ap_crit_act;		// Bit 0: killed
							// Bit 1: stopped
							// Bit 2: Signale testen
	UINT8   ap_stpsig;			// Flag "durch Signal gestoppt"
	UINT32  ap_sigfreeze;		// Signalhandler für SIGFREEZE
	UINT16  ap_recogn;			// Bit 0: verstehe AP_TERM
	UINT32  ap_flags;			// Bit 0: will keinen prop. AES-Zeichensatz
	UINT16  ap_doex;
	UINT16  ap_isgr;
	UINT16  ap_wasgr;
	UINT16  ap_isover;
	UINT32  ap_ldpd;			// PD des Loader-Prozesses
	UINT32  ap_env;			// Environment oder NULL
	UINT32  ap_xtail;			// Erw. Kommandozeile (> 128 Bytes) od. NULL
	UINT32  ap_thr_usp;			// usp für Threads
	UINT32  ap_memlimit;
	UINT32  ap_nice;			// z.Zt. unbenutzt
	UINT8   ap_cmd[128];		// Programmpfad
	UINT8   ap_tai[128];		// Programmparameter
	UINT16  ap_mhidecnt;		// lokaler Maus-Hide-Counter
	UINT16  ap_svd_mouse[37	];	// x/y/planes/bg/fg/msk[32]/moff_cnt
	UINT16  ap_prv_mouse[37];
	UINT16  ap_act_mouse[37];
	UINT32  ap_ssp;
	UINT32  ap_pd;
	UINT32  ap_etvterm;
	UINT32  ap_stkchk;			// magisches Wort für Stacküberprüfung
	UINT8   ap_stack[0];		// Stack
};

   	#pragma options align=reset
#endif