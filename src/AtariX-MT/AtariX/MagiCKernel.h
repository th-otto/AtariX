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
* Enthält alle Konstanten und Strukturen für den MagiC-Kernel
*
*/

#ifndef MAGICKERNEL_H_INCLUDED
#define MAGICKERNEL_H_INCLUDED

   	#pragma options align=packed

#define MagiCKernel_MAX_OPEN	32	// Größte Handlenummer-1 (0..31)

struct MagiCKernel_PD
{
	uint32_t p_lowtpa;
	uint32_t p_hitpa;
	uint32_t p_tbase;
	uint32_t p_tlen;
	uint32_t p_dbase;
	uint32_t p_dlen;
	uint32_t p_bbase;
	uint32_t p_blen;
	uint32_t p_dta;
	uint32_t p_parent;
	uint16_t p_procid;		/* 0x28: Prozeß- ID */
	uint16_t p_status;		/* 0x2a: ab MagiC 5.04 */
	uint32_t p_env;
	char p_devx[6];
	char p_flags;			/* 0x36: Bit 0: Pdomain (MiNT:1/TOS:0) */
	char p_defdrv;
	char p_res3[8];			/* 0x38: Terminierungskontext für ACC */
	char p_drvx[32];		/* 0x40: Tabelle: Default-Path-Hdl. */
	uint32_t p_procdata;	/* 0x60: Zeiger auf PROCDATA */
	uint16_t p_umask;		/* 0x64: umask für Unix-Dateisysteme */
	uint16_t p_procgroup;	/* 0x66: Prozeßgruppe (ab 6.10.96) */
	uint32_t p_mem;			/* 0x68: soviel Speicher darf ich holen */
	uint32_t p_context;		/* 0x6c: unter MAGIX statt p_reg benutzt */
	uint32_t p_mflags;		/* 0x70: Bit 2: Malloc aus AltRAM erlaubt */
	uint32_t p_app;			/* 0x74: APPL, die den Prozeß gestartet hat (main thread) */
	uint32_t p_ssp;			/* 0x78: ssp bei Start des Prozesses */
	uint32_t p_reg;			/* 0x7c: für Kompatibilität mit TOS */
	char p_cmdline[128];
};

struct MagiCKernel_SIGHNDL
{
	uint32_t	sa_handler;		/* 0x00: Signalhandler */
	uint32_t	sa_sigextra;	/* 0x04: OR-Maske bei Ausführung des Signals */
	uint16_t	sa_flags;
};

struct MagiCKernel_FH
{
	uint32_t	fh_fd;
	uint16_t	fh_flag;
};

struct MagiCKernel_PROCDATA
{
	uint32_t	pr_magic;		/* magischer Wert, ähnlich wie bei MiNT */
	uint16_t	pr_ruid;		/* "real user ID" */
	uint16_t	pr_rgid;		/* "real group ID" */
	uint16_t	pr_euid;		/* "effective user ID" */
	uint16_t	pr_egid;		/* "effective group ID" */
	uint16_t	pr_suid;		/* "saved user ID" */
	uint16_t	pr_sgid;		/* "saved group ID" */
	uint16_t	pr_auid;		/* "audit user ID" */
	uint16_t	pr_pri;		/* "base process priority" (nur dummy) */
	uint32_t	pr_sigpending;	/* wartende Signale */
	uint32_t	pr_sigmask;	/* Signalmaske */
	struct MagiCKernel_SIGHNDL pr_sigdata[32];
	uint32_t	pr_usrval;		/* "User"-Wert (ab 9/96) */
	uint32_t	pr_memlist;		/* Tabelle der "shared memory blocks" */
	char		pr_fname[128];	/* Pfad der zugehörigen PRG-Datei */
	char		pr_cmdlin[128];	/* Ursprüngliche Kommandozeile */
	uint16_t	pr_flags;		/* Bit 0: kein Eintrag in u:\proc */
						/* Bit 1: durch Pfork() erzeugt */
	char		pr_procname[10];	/* Prozeßname für u:\proc\ ohne Ext.		*/
	uint16_t	pr_bconmap;	/* z.Zt. unbenutzt */
	struct MagiCKernel_FH pr_hndm6;	/* Handle -6: unbenutzt */
	struct MagiCKernel_FH pr_hndm5;	/* Handle -5: unbenutzt */
	struct MagiCKernel_FH pr_hndm4;	/* Handle -4: standardmäßig NUL: */
	struct MagiCKernel_FH pr_hndm3;	/* Handle -3: standardmäßig PRN: */
	struct MagiCKernel_FH pr_hndm2;	/* Handle -2: standardmäßig AUX: */
	struct MagiCKernel_FH pr_hndm1;	/* Handle -1: standardmäßig CON: */
	struct MagiCKernel_FH pr_handle[MagiCKernel_MAX_OPEN];	/* Handles 0..31 */
};

 struct MagiCKernel_APP
{
	uint32_t	ap_next;			// Verkettungszeiger
	uint16_t	ap_id;				// Application-ID
	uint16_t	ap_parent;			// tatsächliche parent-ID
	uint16_t	ap_parent2;			// ggf. die ap_id des VT52, dorthin ->CH_EXIT
	uint16_t	ap_type;			// 0 = Main Thread/1 = Thread/2 = Signal Handler
	uint32_t	ap_oldsigmask;		// Alte Signalmaske (für Signal-Handler)
	uint32_t	ap_sigthr;			// Haupt-Thread: Zeiger auf aktiven Signalhandler
									// Signalhandler: Zeiger auf vorherigen oder NULL
	uint16_t	ap_srchflg;			// für appl_search
	uint32_t	ap_menutree;		// Menübaum
	uint32_t	ap_attached;		// NULL oder Liste für menu_attach()
	uint32_t	ap_desktree;		// Desktop-Hintergrund
	uint16_t	ap_1stob;			//  dazu erstes Objekt
	uint8_t		ap_dummy1[2];		// zwei Leerzeichen vor ap_name
	uint8_t		ap_name[8];			// Name (8 Zeichen mit trailing blanks)
	uint8_t		ap_dummy2[2];		// Leerstelle und ggf. Ausblendzeichen
	uint8_t		ap_dummy3;			// Nullbyte für EOS
	uint8_t		ap_status;			// APSTAT_...
	uint16_t	ap_hbits;			// eingetroffene Events
	uint16_t	ap_rbits;			// erwartete Events
	uint32_t	ap_evparm;			// Event-Daten, z.B. <pid> oder msg-Puffer
	uint32_t	ap_nxttim;			// Nächste auf Timer wartende APP
	uint32_t	ap_ms;				// Timer
	uint32_t	ap_nxtalrm;			// Nächste auf Alarm wartende APP
	uint32_t	ap_alrmms;			// Alarm
	uint16_t	ap_isalarm;			// Flag
	uint32_t	ap_nxtsem;			// Nächste auf Semaphore wartende APP
	uint32_t	ap_semaph;			// auf diese Semaphore warten wir
	uint16_t	ap_unselcnt;		// Länge der Tabelle ap_unselx
	uint32_t	ap_unselx;			// Tabelle für evnt_(m)IO
	uint32_t	ap_evbut;			// für evnt_button
	uint32_t	ap_mgrect1;			// für evnt_mouse
	uint32_t	ap_mgrect2;			// für evnt_mouse
	uint16_t	ap_kbbuf[8];		// Puffer für 8 Tasten
	uint16_t	ap_kbhead;			// Nächstes zu lesendes Zeichen
	uint16_t	ap_kbtail;			// Nächstes zu schreibendes Zeichen
	uint16_t	ap_kbcnt;			// Anzahl Zeichen im Puffer
	uint16_t	ap_len;				// Message- Pufferlänge
	uint8_t		ap_buf[0x300];		// Message- Puffer (768 Bytes = 48 Nachrichten)
	uint16_t	ap_critic;			// Zähler für "kritische Phase"
	uint8_t		ap_crit_act;		// Bit 0: killed
									// Bit 1: stopped
									// Bit 2: Signale testen
	uint8_t		ap_stpsig;			// Flag "durch Signal gestoppt"
	uint32_t	ap_sigfreeze;		// Signalhandler für SIGFREEZE
	uint16_t	ap_recogn;			// Bit 0: verstehe AP_TERM
	uint32_t	ap_flags;			// Bit 0: will keinen prop. AES-Zeichensatz
	uint16_t	ap_doex;
	uint16_t	ap_isgr;
	uint16_t	ap_wasgr;
	uint16_t	ap_isover;
	uint32_t	ap_ldpd;			// PD des Loader-Prozesses
	uint32_t	ap_env;				// Environment oder NULL
	uint32_t	ap_xtail;			// Erw. Kommandozeile (> 128 Bytes) od. NULL
	uint32_t	ap_thr_usp;			// usp für Threads
	uint32_t	ap_memlimit;
	uint32_t	ap_nice;			// z.Zt. unbenutzt
	uint8_t		ap_cmd[128];		// Programmpfad
	uint8_t		ap_tai[128];		// Programmparameter
	uint16_t	ap_mhidecnt;		// lokaler Maus-Hide-Counter
	uint16_t	ap_svd_mouse[37	];	// x/y/planes/bg/fg/msk[32]/moff_cnt
	uint16_t	ap_prv_mouse[37];
	uint16_t	ap_act_mouse[37];
	uint32_t	ap_ssp;
	uint32_t	ap_pd;
	uint32_t	ap_etvterm;
	uint32_t	ap_stkchk;			// magisches Wort für Stacküberprüfung
	uint8_t		ap_stack[0];		// Stack
};

  	#pragma options align=reset
#endif
