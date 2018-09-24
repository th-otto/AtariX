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
* Enth�lt alle Konstanten und Strukturen f�r den MagiC-Kernel
*
*/

#ifndef MAGICKERNEL_H_INCLUDED
#define MAGICKERNEL_H_INCLUDED

#include "osd_cpu.h"

   	#pragma options align=packed

#define MagiCKernel_MAX_OPEN	32	// Gr��te Handlenummer-1 (0..31)

struct MagiCKernel_PD
{
	UInt32 p_lowtpa;
	void *p_hitpa;
	void *p_tbase;
	unsigned long p_tlen;
	void *p_dbase;
	unsigned long p_dlen;
	void *p_bbase;
	unsigned long p_blen;
	void *p_dta;
	void *p_parent;
	UInt16 p_procid;		/* 0x28: Proze�- ID */
	UInt16 p_status;		/* 0x2a: ab MagiC 5.04 */
	void *p_env;
	char p_devx[6];
	char p_flags;		/* 0x36: Bit 0: Pdomain (MiNT:1/TOS:0) */
	char p_defdrv;
	char p_res3[8];		/* 0x38: Terminierungskontext f�r ACC */
	char p_drvx[32];		/* 0x40: Tabelle: Default-Path-Hdl. */
	UInt32 p_procdata;	/* 0x60: Zeiger auf PROCDATA */
	UInt16 p_umask;		/* 0x64: umask f�r Unix-Dateisysteme */
	UInt16 p_procgroup;	/* 0x66: Proze�gruppe (ab 6.10.96) */
	UInt32 p_mem;		/* 0x68: soviel Speicher darf ich holen */
	UInt32 p_context;		/* 0x6c: unter MAGIX statt p_reg benutzt */
	UInt32 p_mflags;		/* 0x70: Bit 2: Malloc aus AltRAM erlaubt */
	UInt32 p_app;		/* 0x74: APPL, die den Proze� gestartet hat (main thread) */
	UInt32 p_ssp;		/* 0x78: ssp bei Start des Prozesses */
	UInt32 p_reg;		/* 0x7c: f�r Kompatibilit�t mit TOS */
	char p_cmdline[128];
};

struct MagiCKernel_SIGHNDL
{
	UInt32	sa_handler;		/* 0x00: Signalhandler */
	UInt32	sa_sigextra;	/* 0x04: OR-Maske bei Ausf�hrung des Signals */
	UInt16	sa_flags;
};

struct MagiCKernel_FH
{
	UInt32	fh_fd;
	UInt16	fh_flag;
};

struct MagiCKernel_PROCDATA
{
	UInt32	pr_magic;		/* magischer Wert, �hnlich wie bei MiNT */
	UInt16	pr_ruid;		/* "real user ID" */
	UInt16	pr_rgid;		/* "real group ID" */
	UInt16	pr_euid;		/* "effective user ID" */
	UInt16	pr_egid;		/* "effective group ID" */
	UInt16	pr_suid;		/* "saved user ID" */
	UInt16	pr_sgid;		/* "saved group ID" */
	UInt16	pr_auid;		/* "audit user ID" */
	UInt16	pr_pri;		/* "base process priority" (nur dummy) */
	UInt32	pr_sigpending;	/* wartende Signale */
	UInt32	pr_sigmask;	/* Signalmaske */
	struct MagiCKernel_SIGHNDL pr_sigdata[32];
	UInt32	pr_usrval;		/* "User"-Wert (ab 9/96) */
	UInt32	pr_memlist;		/* Tabelle der "shared memory blocks" */
	char		pr_fname[128];	/* Pfad der zugeh�rigen PRG-Datei */
	char		pr_cmdlin[128];	/* Urspr�ngliche Kommandozeile */
	UInt16	pr_flags;		/* Bit 0: kein Eintrag in u:\proc */
						/* Bit 1: durch Pfork() erzeugt */
	char		pr_procname[10];	/* Proze�name f�r u:\proc\ ohne Ext.		*/
	UInt16	pr_bconmap;	/* z.Zt. unbenutzt */
	struct MagiCKernel_FH pr_hndm6;	/* Handle -6: unbenutzt */
	struct MagiCKernel_FH pr_hndm5;	/* Handle -5: unbenutzt */
	struct MagiCKernel_FH pr_hndm4;	/* Handle -4: standardm��ig NUL: */
	struct MagiCKernel_FH pr_hndm3;	/* Handle -3: standardm��ig PRN: */
	struct MagiCKernel_FH pr_hndm2;	/* Handle -2: standardm��ig AUX: */
	struct MagiCKernel_FH pr_hndm1;	/* Handle -1: standardm��ig CON: */
	struct MagiCKernel_FH pr_handle[MagiCKernel_MAX_OPEN];	/* Handles 0..31 */
};

 struct MagiCKernel_APP
{
	UINT32	ap_next;			// Verkettungszeiger
	UINT16	ap_id;			// Application-ID
	UINT16	ap_parent;			// tats�chliche parent-ID
	UINT16	ap_parent2;		// ggf. die ap_id des VT52, dorthin ->CH_EXIT
	UINT16	ap_type;			// 0 = Main Thread/1 = Thread/2 = Signal Handler
	UINT32	ap_oldsigmask;		// Alte Signalmaske (f�r Signal-Handler)
	UINT32	ap_sigthr;			// Haupt-Thread: Zeiger auf aktiven Signalhandler
							// Signalhandler: Zeiger auf vorherigen oder NULL
	UINT16	ap_srchflg;			// f�r appl_search
	UINT32	ap_menutree;		// Men�baum
	UINT32	ap_attached;		// NULL oder Liste f�r menu_attach()
	UINT32	ap_desktree;		// Desktop-Hintergrund
	UINT16	ap_1stob;			//  dazu erstes Objekt
	UINT8		ap_dummy1[2];		// zwei Leerzeichen vor ap_name
	UINT8		ap_name[8];		// Name (8 Zeichen mit trailing blanks)
	UINT8		ap_dummy2[2];		// Leerstelle und ggf. Ausblendzeichen
	UINT8		ap_dummy3;		// Nullbyte f�r EOS
	UINT8		ap_status;			// APSTAT_...
	UINT16	ap_hbits;			// eingetroffene Events
	UINT16	ap_rbits;			// erwartete Events
	UINT32	ap_evparm;			// Event-Daten, z.B. <pid> oder msg-Puffer
	UINT32	ap_nxttim;			// N�chste auf Timer wartende APP
	UINT32	ap_ms;			// Timer
	UINT32	ap_nxtalrm;		// N�chste auf Alarm wartende APP
	UINT32	ap_alrmms;			// Alarm
	UINT16	ap_isalarm;			// Flag
	UINT32	ap_nxtsem;			// N�chste auf Semaphore wartende APP
	UINT32	ap_semaph;			// auf diese Semaphore warten wir
	UINT16	ap_unselcnt;		// L�nge der Tabelle ap_unselx
	UINT32	ap_unselx;			// Tabelle f�r evnt_(m)IO
	UINT32	ap_evbut;			// f�r evnt_button
	UINT32	ap_mgrect1;		// f�r evnt_mouse
	UINT32	ap_mgrect2;		// f�r evnt_mouse
	UINT16	ap_kbbuf[8];		// Puffer f�r 8 Tasten
	UINT16	ap_kbhead;			// N�chstes zu lesendes Zeichen
	UINT16	ap_kbtail;			// N�chstes zu schreibendes Zeichen
	UINT16	ap_kbcnt;			// Anzahl Zeichen im Puffer
	UINT16	ap_len;			// Message- Pufferl�nge
	UINT8		ap_buf[0x300];		// Message- Puffer (768 Bytes = 48 Nachrichten)
	UINT16	ap_critic;			// Z�hler f�r "kritische Phase"
	UINT8		ap_crit_act;		// Bit 0: killed
							// Bit 1: stopped
							// Bit 2: Signale testen
	UINT8		ap_stpsig;			// Flag "durch Signal gestoppt"
	UINT32	ap_sigfreeze;		// Signalhandler f�r SIGFREEZE
	UINT16	ap_recogn;			// Bit 0: verstehe AP_TERM
	UINT32	ap_flags;			// Bit 0: will keinen prop. AES-Zeichensatz
	UINT16	ap_doex;
	UINT16	ap_isgr;
	UINT16	ap_wasgr;
	UINT16	ap_isover;
	UINT32	ap_ldpd;			// PD des Loader-Prozesses
	UINT32	ap_env;			// Environment oder NULL
	UINT32	ap_xtail;			// Erw. Kommandozeile (> 128 Bytes) od. NULL
	UINT32	ap_thr_usp;			// usp f�r Threads
	UINT32	ap_memlimit;
	UINT32	ap_nice;			// z.Zt. unbenutzt
	UINT8		ap_cmd[128];		// Programmpfad
	UINT8		ap_tai[128];		// Programmparameter
	UINT16	ap_mhidecnt;		// lokaler Maus-Hide-Counter
	UINT16	ap_svd_mouse[37	];	// x/y/planes/bg/fg/msk[32]/moff_cnt
	UINT16	ap_prv_mouse[37];
	UINT16	ap_act_mouse[37];
	UINT32	ap_ssp;
	UINT32	ap_pd;
	UINT32	ap_etvterm;
	UINT32	ap_stkchk;			// magisches Wort f�r Stack�berpr�fung
	UINT8		ap_stack[0];		// Stack
};

  	#pragma options align=reset
#endif
