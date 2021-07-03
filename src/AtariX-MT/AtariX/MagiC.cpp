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
* Enthält alles, was mit "MagicMac OS" zu tun hat
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "MagiC.h"
#include "Atari.h"
//#include "Dialogue68kExc.h"
//#include "DialogueSysHalt.h"
#include "PascalStrings.h"
#include "missing.h"
#include "_fcntl.h"
#include "s_endian.h"
#include <time.h>
#include <sys/time.h>
#include "maptab.h"

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// Schalter

#ifdef _DEBUG
#define _DEBUG_WRITEPROTECT_ATARI_OS

//#define _DEBUG_NO_ATARI_KB_INTERRUPTS
//#define _DEBUG_NO_ATARI_MOUSE_INTERRUPTS
//#define _DEBUG_NO_ATARI_HZ200_INTERRUPTS
//#define _DEBUG_NO_ATARI_VBL_INTERRUPTS
#endif

// Kram für den 68k-Emulator
#if defined(USE_ASGARD_PPC_68K_EMU)
// Asgard 68k emulator (PPC Assembler)
#include "Asgard68000.h"
#define COUNT_CYCLES 0
#else
// Musashi 68k emulator ('C')
#include "m68k.h"
#include "m68kcpu.h"
#endif

typedef unsigned int m68k_data_type;
typedef unsigned int m68k_addr_type;

#ifdef MAGICMACX_DEBUG68K
uint32_t DebugCurrentPC;			// für Test der Bildschirmausgabe
static uint32_t WriteCounters[100];
extern void TellDebugCurrentPC(uint32_t pc);
void TellDebugCurrentPC(uint32_t pc)
{
	DebugCurrentPC = pc;
}
#endif

static CMagiC *pTheMagiC = 0;
unsigned char *OpcodeROM;		// Zeiger auf den 68k-Speicher
static uint32_t Adr68kVideo;			// Beginn Bildschirmspeicher 68k
static uint32_t Adr68kVideoEnd;			// Ende Bildschirmspeicher 68k
#ifdef _DEBUG
static uint32_t AdrOsRomStart;			// Beginn schreibgeschützter Bereich
static uint32_t AdrOsRomEnd;			// Ende schreibgeschützter Bereich
#endif
static unsigned char *HostVideoAddr;		// Beginn Bildschirmspeicher Host
//static unsigned char *HostVideo2Addr;		// Beginn Bildschirmspeicher Host (Hintergrundpuffer)
static atomic_char *p_bVideoBufChanged;
static bool bAtariVideoRamHostEndian = true;

static const char *AtariAddr2Description(uint32_t addr);

#if defined(MAGICMACX_DEBUG_SCREEN_ACCESS) || defined(PATCH_VDI_PPC)
static uint32_t p68k_OffscreenDriver = 0;
static uint32_t p68k_ScreenDriver = 0;
//#define ADDR_VDIDRVR_32BIT			0x1819c	// VDI-Treiber für "true colour" liegt hier
//#define p68k_OffscreenDriver		0x19cc0	// Offscreen-VDI-Treiber für "true colour" liegt hier
#endif

#ifdef PATCH_VDI_PPC
#include "VDI_PPC.c.h"
#endif

enum
{
	MacSys_enosys,       // index 0 reserved to catch errors

/*
 * ordinary C function callbacks that are provided by the emulator to the kernel
 */
    MacSys_gettime,    	 // LONG GetTime(void) Datum und Uhrzeit ermitteln
    MacSys_settime,      // void SetTime(LONG *time) Datum/Zeit setzen
    MacSys_Setpalette,   // void Setpalette( int ptr[16] )
    MacSys_Setcolor,     // int Setcolor( int nr, int val )
    MacSys_VsetRGB,      // void VsetRGB( WORD index, WORD count, LONG *array )
    MacSys_VgetRGB,      // void VgetRGB( WORD index, WORD count, LONG *array )
    MacSys_syshalt,      // SysHalt( char *str ) "System halted"
    MacSys_syserr,       // SysErr( void ) Bomben
	MacSys_coldboot,     // ColdBoot(void) Kaltstart ausführen
    MacSys_exit,         // Exit(void) beenden
    MacSys_debugout,     // MacPuts( char *str ) fürs Debugging
    MacSys_error,        // d0 = -1: kein Grafiktreiber
    MacSys_prtos,        // Bcostat(void) für PRT
    MacSys_prtin,        // Cconin(void) für PRT
    MacSys_prtout,       // Cconout( void *params ) für PRT
    MacSys_prtouts,      // LONG PrtOuts({char *buf, LONG count}) String auf Drucker
    MacSys_serconf,      // Rsconf( void *params ) für ser1
    MacSys_seris,        // Bconstat(void) für ser1 (AUX)
    MacSys_seros,        // Bcostat(void) für ser1
    MacSys_serin,        // Cconin(void) für ser1
    MacSys_serout,       // Cconout( void *params ) für ser1
    MacSys_SerOpen,      // Serielle Schnittstelle öffnen
    MacSys_SerClose,     // Serielle Schnittstelle schließen
    MacSys_SerRead,      // Lesen(buffer, len) => gelesene Zeichen
    MacSys_SerWrite,     // Schreiben(buffer, len) => geschriebene Zeichen
    MacSys_SerStat,      // Lese-/Schreibstatus
    MacSys_SerIoctl,     // Ioctl-Aufrufe für serielle Schnittstelle
    MacSys_dos_macfn,    // DosFn({int,void*} *) DOS-Funktionen 0x60..0xfe
    MacSys_Yield,		 // Rechenzeit abgeben

/*
 * member functions of CMagiC
 */
	MacSys_init,
	MacSys_biosinit,
	MacSys_VdiInit,
	MacSys_Exec68k,
	MacSys_GetKeybOrMouse,
	MacSys_Daemon,

/*
 * member functions of CMacXFS
 */
	MacSys_xfs,
	MacSys_xfs_dev,
	MacSys_drv2devcode,
	MacSys_rawdrvr,
	
/*
 * member functions of CXCmd
 */
	MacSys_Xcmd,

/* number of functions */	
	C_callback_NUM
};

typedef uint32_t (*C_callback_func)(uint32_t params, unsigned char *AdrOffset68k);
static C_callback_func g_callbacks[C_callback_NUM];

/**********************************************************************
 *
 * Avoid deprecation warnings
 *
 **********************************************************************/

static inline void OS_TerminateTask(MPTaskID task, OSStatus terminationStatus)
{
	(void) MPTerminateTask(task, terminationStatus);
}

static inline void OS_CreateCriticalRegion(MPCriticalRegionID * criticalRegion)
{
	(void) MPCreateCriticalRegion(criticalRegion);
}

static inline void OS_DeleteCriticalRegion(MPCriticalRegionID criticalRegion)
{
	MPDeleteCriticalRegion(criticalRegion);
}

static inline OSStatus OS_EnterCriticalRegion(MPCriticalRegionID criticalRegion, Duration timeout)
{
	return MPEnterCriticalRegion(criticalRegion, timeout);
}

static inline void OS_ExitCriticalRegion(MPCriticalRegionID criticalRegion)
{
	MPExitCriticalRegion(criticalRegion);
}

static inline void OS_CreateEvent(MPEventID * event)
{
	(void) MPCreateEvent(event);
}

static inline void OS_DeleteEvent(MPEventID event)
{
	(void) MPDeleteEvent(event);
}

static inline void OS_SetEvent(MPEventID event, MPEventFlags flags)
{
	(void) MPSetEvent(event, flags);
}

static inline OSStatus OS_WaitForEvent(MPEventID event, MPEventFlags *flags, Duration timeout)
{
	return MPWaitForEvent(event, flags, timeout);
}

static inline void OS_CreateQueue(MPQueueID * queue)
{
	(void) MPCreateQueue(queue);
}

static inline void OS_DeleteQueue(MPQueueID queue)
{
	(void) MPDeleteQueue(queue);
}

static inline OSStatus OS_WaitOnQueue(MPQueueID queue, void **param1, void **param2, void **param3, Duration timeout)
{
	return MPWaitOnQueue(queue, param1, param2, param3, timeout);
}

	

/**********************************************************************
*
* Berechnet aus der Adresse, welcher Chip angesprochen werden sollte.
* Rückgabe "?", wenn unbekannt.
*
* IN	addr
*
**********************************************************************/

static const char *AtariAddr2Description(uint32_t addr)
{
	// Rechne ST-Adresse in TT-Adresse um

	if	(addr >= 0xff0000 && addr < 0xffffff)
		addr |= 0xff000000;

	if	(addr == 0xffff8201)
		return("Videocontroller: Video base register high");

	if	(addr == 0xffff8203)
		return("Videocontroller: Video base register mid");

	if	(addr == 0xffff820d)
		return("Videocontroller: Video base register low (STE)");

	if	(addr == 0xffff8260)
		return("Videocontroller: ST shift mode register (0: 4 Planes, 1: 2 Planes, 2: 1 Plane)");

	// Interrupts A: Timer B/XMIT Err/XMIT Buffer Empty/RCV Err/RCV Buffer full/Timer A/Port I6/Port I7 (Bits 0..7)
	if	(addr == 0xfffffa0f)
		return("MFP: ISRA (Interrupt-in-service A)");

	// Interrupts B: Port I0/Port I1/Port I2/Port I3/Timer D/Timer C/Port I4/Port I5 (Bits 0..7)
	if	(addr == 0xfffffa11)
		return("MFP: ISRB (Interrupt-in-service B)");

	if	(addr >= 0xfffffa40 && addr < 0xfffffa54)
		return("SFP004");

	return("?");
}


/**********************************************************************
*
* 68k-Emu: Byte lesen
*
**********************************************************************/

m68k_addr_type m68k_read_memory_8(m68k_addr_type address)
{
#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
		return(*((uint8_t*) (OpcodeROM + address)));
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		return(*((uint8_t*) (HostVideoAddr + (address - Adr68kVideo))));
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::ReadByte(adr = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, AtariAddr2Description(address), Name);
/*
		if	(address == 0xfffffa11)
		{
			DebugWarning("CMagiC::ReadByte() --- Zugriff auf \"MFP ISRB\" wird ignoriert, damit ZBENCH.PRG läuft!");
		}
		else
*/
			pTheMagiC->SendBusError(address, "read byte");

		return(0xff);
	}
}


/**********************************************************************
*
* 68k-Emu: Wort lesen
*
**********************************************************************/

m68k_addr_type m68k_read_memory_16(m68k_addr_type address)
{
	uint16_t val;

#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
	{
		val = *((uint16_t *) (OpcodeROM + address));
		return(be16_to_cpu(val));
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		val = *((uint16_t *) (HostVideoAddr + (address - Adr68kVideo)));
		if (bAtariVideoRamHostEndian)
			return val;		// x86 has bgr instead of rgb
		else
			return(be16_to_cpu(val));
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::ReadWord(adr = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, AtariAddr2Description(address), Name);
		pTheMagiC->SendBusError(address, "read word");
		return(0xffff);		// eigentlich Busfehler
	}
}


/**********************************************************************
*
* 68k-Emu: Langwort lesen
*
**********************************************************************/

m68k_addr_type m68k_read_memory_32(m68k_addr_type address)
{
	uint32_t val;

#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
	{
		val = *((uint32_t*) (OpcodeROM + address));
		return(be32_to_cpu(val));
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		val = *((uint32_t *) (HostVideoAddr + (address - Adr68kVideo)));
		if (bAtariVideoRamHostEndian)
			return val;		// x86 has bgr instead of rgb
		else
			return(be32_to_cpu(val));
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";
		DebugError("CMagiC::ReadLong(adr = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, AtariAddr2Description(address), Name);
		return(0xffffffff);		// eigentlich Busfehler
	}
}


/**********************************************************************
*
* 68k-Emu: Byte schreiben
*
**********************************************************************/

void m68k_write_memory_8(m68k_addr_type address, m68k_data_type value)
{
#ifdef _DEBUG_WRITEPROTECT_ATARI_OS
	if	((address >= AdrOsRomStart) && (address < AdrOsRomEnd))
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("WriteByte() --- MagiC-ROM durch Prozeß %s überschrieben auf Adresse 0x%08x",
						Name, address);
		pTheMagiC->SendBusError(address, "write byte");
	}
#endif
#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
	{
		*((uint8_t *) (OpcodeROM + address)) = (uint8_t) value;
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		address -= Adr68kVideo;
		*((uint8_t *) (HostVideoAddr + address)) = (uint8_t) value;
		//*((uint8_t*) (HostVideo2Addr + address)) = (uint8_t) value;
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
		//usleep(100000);
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteByte(adr = 0x%08lx, dat = 0x%02hx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, (unsigned short)(unsigned char) value, AtariAddr2Description(address), Name);

		if	((address == 0xffff8201) || (address == 0xffff8203) || (address == 0xffff820d))
		{
			DebugWarning("CMagiC::WriteByte() --- Zugriff auf \"Video Base Register\" wird ignoriert, damit PD.PRG läuft!");
		}
		else
		if	(address == 0xfffffa11)
		{
			DebugWarning("CMagiC::WriteByte() --- Zugriff auf \"MFP ISRB\" wird ignoriert, damit ZBENCH.APP läuft!");
		}
		else
			pTheMagiC->SendBusError(address, "write byte");
	}
}


/**********************************************************************
*
* 68k-Emu: Wort schreiben
*
**********************************************************************/

void m68k_write_memory_16(m68k_addr_type address, m68k_data_type value)
{
#ifdef _DEBUG_WRITEPROTECT_ATARI_OS
	if	((address >= AdrOsRomStart-1) && (address < AdrOsRomEnd))
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("WriteWord() --- MagiC-ROM durch Prozeß %s überschrieben auf Adresse 0x%08x",
						Name, address);
		pTheMagiC->SendBusError(address, "write word");
	}
#endif
#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
		*((uint16_t *) (OpcodeROM + address)) = (uint16_t) cpu_to_be16(value);
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		address -= Adr68kVideo;
		if (bAtariVideoRamHostEndian)
			*((uint16_t *) (HostVideoAddr + address)) = (uint16_t) value;		// x86 has bgr instead of rgb
		else
			*((uint16_t *) (HostVideoAddr + address)) = (uint16_t) cpu_to_be16(value);

// //		*((uint16_t*) (HostVideo2Addr + address)) = (uint16_t) cpu_to_be16(value);
//		*((uint16_t *) (HostVideo2Addr + address)) = (uint16_t) value;	// x86 has bgr instead of rgb
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteWord(adr = 0x%08lx, dat = 0x%04hx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, (uint16_t) value, AtariAddr2Description(address), Name);
		pTheMagiC->SendBusError(address, "write word");
	}
}


/**********************************************************************
*
* 68k-Emu: Langwort schreiben
*
**********************************************************************/

void m68k_write_memory_32(m68k_addr_type address, m68k_data_type value)
{
#ifdef _DEBUG_WRITEPROTECT_ATARI_OS
	if	((address >= AdrOsRomStart - 3) && (address < AdrOsRomEnd))
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("WriteLong() --- MagiC-ROM durch Prozeß %s überschrieben auf Adresse 0x%08x",
						Name, address);
		pTheMagiC->SendBusError(address, "write long");
	}
#endif
#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
		*((uint32_t *) (OpcodeROM + address)) = (uint32_t) cpu_to_be32(value);
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
#ifdef MAGICMACX_DEBUG_SCREEN_ACCESS
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0x97a) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0xa96))
		{
			(WriteCounters[0])++;
			// Bildschirm-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_ScreenDriver + 0xbd8)
		{
			(WriteCounters[1])++;
			// Bildschirm-Treiber
			// 8*16 Text im Textmodus (echter VT52)
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_ScreenDriver + 0xbec)
		{
			(WriteCounters[2])++;
			// 8*16 Texthintergrund im Textmodus (echter VT52)
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0xfe4) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0xff2))
		{
			(WriteCounters[3])++;
			//value = 0x00ff0000;
		}
		else
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0x1018) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0x1026))
		{
			(WriteCounters[4])++;
			// gelöschter Bildschirm im Textmodus (echter VT52)
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0x107c) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0x1084))
		{
			(WriteCounters[5])++;
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0x10ac) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0x10ca))
		{
			(WriteCounters[6])++;
			// Maushintergrund
			//value = 0x00ff0000;
		}
		else
		if	((DebugCurrentPC >= p68k_ScreenDriver + 0x1188) &&
			 (DebugCurrentPC <= p68k_ScreenDriver + 0x11a6))
		{
			(WriteCounters[7])++;
			// Bildschirm-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_ScreenDriver + 0x11ea)
		{
			(WriteCounters[8])++;
			// Bildschirm-Treiber
			// Mauszeiger-Umriß
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_ScreenDriver + 0x11f8)
		{
			(WriteCounters[9])++;
			// Bildschirm-Treiber
			// Mauszeiger-Inneres
			//value = 0x00ffff00;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0xf7a)
		{
			(WriteCounters[10])++;
			// Offscreen-Treiber
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0xfac)
		{
			(WriteCounters[11])++;
			// Offscreen-Treiber
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0xfca)
		{
			(WriteCounters[12])++;
			// Offscreen-Treiber
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0xff6)
		{
			(WriteCounters[13])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x100a)
		{
			(WriteCounters[14])++;
			// Offscreen-Treiber
			// Linien
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x108a) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x10c6))
		{
			(WriteCounters[15])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x10e8)
		{
			(WriteCounters[16])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1126) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1162))
		{
			(WriteCounters[17])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x11ae) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x11ee))
		{
			(WriteCounters[18])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1212) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x124e))
		{
			(WriteCounters[19])++;
			// senkrechte Linien
			// Offscreen-Treiber
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1306)
		{
			(WriteCounters[20])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1312)
		{
			(WriteCounters[21])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1328)
		{
			(WriteCounters[22])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1344)
		{
			(WriteCounters[23])++;
			// Offscreen-Treiber
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x136c)
		{
			(WriteCounters[24])++;
			// Offscreen-Treiber
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x13c0)
		{
			(WriteCounters[25])++;
			// Offscreen-Treiber
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x13de)
		{
			(WriteCounters[26])++;
			// Offscreen-Treiber
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1444) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1452))
		{
			(WriteCounters[27])++;
			// Offscreen-Treiber
			// gefüllte Flächen (schon PPC)
			//value = 0x00ff0000;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1668) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1686))
		{
			(WriteCounters[28])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1690) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x16ae))
		{
			(WriteCounters[29])++;
			// Offscreen-Treiber
			// Teile gefüllter Flächen
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x16c4) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x16e2))
		{
			(WriteCounters[30])++;
			// Offscreen-Treiber
			//value = 0x00ff0000;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1742) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1760))
		{
			(WriteCounters[31])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x18f8) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1916))
		{
			(WriteCounters[32])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x19ec) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1a28))
		{
			(WriteCounters[33])++;
			// Offscreen-Treiber
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1b34)
		{
			(WriteCounters[34])++;
			// Offscreen-Treiber
			// Textvordergrund
			//value = 0x00ff0000;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1b3c)
		{
			(WriteCounters[35])++;
			// Offscreen-Treiber
			// Texthintergrund
			//value = 0x000000ff;
		}
		else
		if	(DebugCurrentPC == p68k_OffscreenDriver + 0x1b6a)
		{
			(WriteCounters[36])++;
			// Offscreen-Treiber
			// value = 0x000000ff;
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x1d6e) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x1d74))
		{
			(WriteCounters[37])++;
			// Offscreen-Treiber
			// Fenster verschieben!
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x207a) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x2086))
		{
			(WriteCounters[38])++;
			// Rasterkopie im Modus "Quelle AND Ziel"
			// Offscreen-Treiber
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x20cc) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x20d2))
		{
			(WriteCounters[39])++;
			// Offscreen-Treiber
		}
		else
		if	((DebugCurrentPC >= p68k_OffscreenDriver + 0x211a) &&
			 (DebugCurrentPC <= p68k_OffscreenDriver + 0x2126))
		{
			(WriteCounters[40])++;
			// Offscreen-Treiber
		}
		else
		{
			(WriteCounters[41])++;
			DebugError("#### falscher Bildspeicherzugriff bei PC = 0x%08x", DebugCurrentPC);
		}

#endif
		address -= Adr68kVideo;
		if (bAtariVideoRamHostEndian)
			*((uint32_t *) (HostVideoAddr + address)) = value;		// x86 has brg instead of rgb
		else
			*((uint32_t *) (HostVideoAddr + address)) = cpu_to_be32(value);

// //		*((uint32_t*) (HostVideo2Addr + address)) = value;		// x86 has brg instead of rgb
//		*((uint32_t *) (HostVideo2Addr + address)) = cpu_to_be32(value);
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
	}
	else
	{
		const char *Name;
		uint32_t act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteLong(adr = 0x%08lx, dat = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", (unsigned long)address, (unsigned long)value, AtariAddr2Description(address), Name);
		pTheMagiC->SendBusError(address, "write long");
	}
}


// statische Variablen

uint32_t CMagiC::s_LastPrinterAccess = 0;

CMagiCSerial *pTheSerial;
CMagiCPrint *pThePrint;

/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMagiC::CMagiC()
{
	m_CurrModifierKeys = 0;
	m_RAM68k = NULL;
	m_RAM68ksize = 0;
	m_BasePage = NULL;
	m_EmuNotifQID = NULL;
	m_EmuTaskID = NULL;
	m_EventId = NULL;
	m_InterruptEventsId = NULL;
	m_KbCriticalRegionId = NULL;
	m_AECriticalRegionId = NULL;
	m_ScrCriticalRegionId = NULL;
	m_iNoOfAtariFiles = 0;
	m_pKbWrite = m_pKbRead = m_cKeyboardOrMouseData;
	m_bShutdown = false;
	m_bBusErrorPending = false;
	m_bInterrupt200HzPending = false;
	m_bInterruptVBLPending = false;
	m_bInterruptMouseKeyboardPending = false;
	m_bInterruptMouseButton[0] = m_bInterruptMouseButton[1] = false;
	m_InterruptMouseWhere.h = m_InterruptMouseWhere.v = 0;
	m_bInterruptPending = false;
	m_LineAVars = NULL;
	m_pMagiCScreen = NULL;
//	m_PrintFileRefNum = 0;
	pTheSerial = &m_MagiCSerial;
	pThePrint = &m_MagiCPrint;
	pTheMagiC = this;
	m_bAtariWasRun = false;
	m_bBIOSSerialUsed = false;
	m_bScreenBufferChanged = false;
	m_bEmulatorIsRunning = false;
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMagiC::~CMagiC()
{
	if	(m_EmuTaskID)
	{
		// Task abschießen
		OS_TerminateTask(m_EmuTaskID, noErr);
		// Auf Beendigung warten
		OS_WaitOnQueue(m_EmuNotifQID, NULL, NULL, NULL, kDurationForever);
		m_bEmulatorIsRunning = false;
	}
	if	(m_EmuNotifQID)
		OS_DeleteQueue(m_EmuNotifQID);
	if	(m_EventId)
		OS_DeleteEvent(m_EventId);
	if	(m_InterruptEventsId)
		OS_DeleteEvent(m_InterruptEventsId);
	if	(m_KbCriticalRegionId)
		OS_DeleteCriticalRegion(m_KbCriticalRegionId);
	if	(m_AECriticalRegionId)
		OS_DeleteCriticalRegion(m_AECriticalRegionId);
	if	(m_ScrCriticalRegionId)
		OS_DeleteCriticalRegion(m_ScrCriticalRegionId);
	if	(m_RAM68k)
		free(m_RAM68k);
}


/**********************************************************************
*
* Lädt eine TOS-Programmdatei und reloziert sie.
*
* stackSize		Benötigter Platz hinter dem BSS-Segment
* reladdr		hier hinladen, -1 = ans Ende
* aus:
* basePage		Zeiger auf die geladene BasePage
*
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

OSErr CMagiC::LoadReloc
(
	CFURLRef fileUrl,
	long stackSize,
	long reladdr,
	BasePage **basePage
)
{
	OSErr err = 0;
	int fd = -1;
	unsigned long len, codlen;
	ExeHeader exehead;
	BasePage *bp;
	unsigned char *relp;
	unsigned char relb;
	uint32_t *tp;
	unsigned char *tpaStart, *relBuf = NULL, *reloff, *tbase, *bbase;
	uint32_t loff;
	unsigned long tpaSize;
	unsigned long Fpos;
	unsigned long FileSize = 0;
	unsigned long RelocBufSize;
	char filename[MAXPATHLEN];
	long bytes_read;


	DebugInfo("CMagiC::LoadReloc()");

	if (!CFURLGetFileSystemRepresentation(fileUrl, true, (unsigned char *)filename, MAXPATHLEN))
	{
		err = openErr;
	}

	if (!err)
	{
		fd = open(filename, O_RDONLY|O_BINARY);
		if (fd < 0)
		{
			err = openErr;
		} else
		{
			FileSize = lseek(fd, 0l, SEEK_END);
			lseek(fd, 0l, SEEK_SET);
		}
	}

	if (err)
	{
		DebugError("CMagiC::LoadReloc() - Kann Datei nicht öffnen");
/*
MagicMacX kann die MagiC-Kernel-Datei MagicMacX.OS nicht finden.

Installieren Sie die Applikation neu.
[MagiCMacX beenden]
MagicMacX could not find the MagiC kernel file "MagicMacX.OS".

Reinstall the application.
[Quit program]
*/
		MyAlert(ALRT_NO_MAGICMAC_OS, kAlertStopAlert);
		goto exitReloc;
	}

	len = sizeof(ExeHeader);
	// PRG-Header einlesen
	bytes_read = read(fd, &exehead, len);
	if (bytes_read != len)
	{
	readerr:
		err = readErr;
		DebugError("CMagiC::LoadReloc() - Datei zu kurz");
		goto exitReloc;
	}

	DebugInfo("CMagiC::LoadReloc() - Length TEXT = %ld", (long)be32_to_cpu(exehead.tlen));
	DebugInfo("CMagiC::LoadReloc() - Length DATA = %ld", (long)be32_to_cpu(exehead.dlen));
	DebugInfo("CMagiC::LoadReloc() - Length BSS = %ld", (long)be32_to_cpu(exehead.blen));

	codlen = be32_to_cpu(exehead.tlen) + be32_to_cpu(exehead.dlen);
	if	(be32_to_cpu(exehead.blen) & 1)
	{
		// BSS-Segment auf gerade Länge
		exehead.blen = cpu_to_be32(be32_to_cpu(exehead.blen) + 1);
	}
	tpaSize = sizeof(BasePage) + codlen + be32_to_cpu(exehead.blen) + stackSize;

	DebugInfo("CMagiC::LoadReloc() - total length incl. basepage and stack = 0x%08lx (%ld)", tpaSize, tpaSize);

	if	(tpaSize > m_RAM68ksize)
	{
		DebugError("CMagiC::LoadReloc() - program size too large");
		err = memFullErr;
		goto exitReloc;
	}


// hier basepage auf durch 4 teilbare Adresse!

	if	(reladdr < 0)
		reladdr = (long) (m_RAM68ksize - ((tpaSize + 2) & ~3));
	if	(reladdr + tpaSize > m_RAM68ksize)
	{
		DebugError("CMagiC::LoadReloc() - illegal load address");
		err = 2;
		goto exitReloc;
	}

	tpaStart = m_RAM68k + reladdr;
	tbase = tpaStart + sizeof(BasePage);
	reloff = tbase;
	bbase = tbase + codlen;

	// Alle 68k-Adressen sind relativ zu <m_RAM68k>
	bp = (BasePage *) tpaStart;
	memset(bp, 0, sizeof(BasePage));
	bp->p_lowtpa = cpu_to_be32(tpaStart - m_RAM68k);
	bp->p_hitpa = cpu_to_be32(tpaStart - m_RAM68k + tpaSize);
	bp->p_tbase = cpu_to_be32(tbase - m_RAM68k);
	bp->p_tlen  = exehead.tlen;
	bp->p_dbase = cpu_to_be32(tbase - m_RAM68k + be32_to_cpu(exehead.tlen));
	bp->p_dlen  = exehead.dlen;
	bp->p_bbase = cpu_to_be32(bbase - m_RAM68k);
	bp->p_blen  = exehead.blen;
	bp->p_dta   = cpu_to_be32(bp->p_cmdline - m_RAM68k);
	bp->p_parent= 0;

	DebugInfo("CMagiC::LoadReloc() - Startadresse Atari = %p (host)", m_RAM68k);
	DebugInfo("CMagiC::LoadReloc() - Speichergröße Atari = 0x%08lx (= %lu kBytes)", (unsigned long)m_RAM68ksize, (unsigned long)m_RAM68ksize >> 10);
	DebugInfo("CMagiC::LoadReloc() - Ladeadresse des Systems (TEXT) = 0x%08lx (68k)", (unsigned long)be32_to_cpu(bp->p_tbase));

	#if defined(_DEBUG_BASEPAGE)
	{
		int i;
		const unsigned char *p = (const unsigned char *) bp;

		for (i = 0; i < sizeof(BasePage); i++)
		{
			DebugInfo("CMagiC::LoadReloc() - BasePage[%d] = 0x%02x", i, p[i]);
		}
	}
	#endif

	// TEXT+DATA einlesen
	bytes_read = read(fd, tbase, codlen);
	if	(codlen != bytes_read)
		goto readerr;

	if	(!be16_to_cpu(exehead.relmod))	// müssen relozieren
	{
		// Seek zur Reloc-Tabelle
		Fpos = (long) (be32_to_cpu(exehead.slen) + codlen + sizeof(exehead));
		if (lseek(fd, Fpos, SEEK_SET) != Fpos)
			goto readerr;
		len = 4;
		bytes_read = read(fd, &loff, len);
		if	(len != bytes_read)
			goto readerr;

		loff = be32_to_cpu(loff);

		if	(loff)	// müssen relozieren
		{
			Fpos += 4;
			RelocBufSize = (unsigned long) (FileSize - Fpos);
			// Puffer für Reloc-Daten allozieren
			relBuf = (unsigned char *) malloc(RelocBufSize + 2);
			if	(!relBuf)
			{
				err = memFullErr;
				goto exitReloc;
			}

			// 1. Longword relozieren
			tp = (uint32_t *) (reloff + loff);

			//*tp += (long) (reloff - m_RAM68k);
			*tp = cpu_to_be32((long) (reloff - m_RAM68k) + be32_to_cpu(*tp));

			// Reloc-Tabelle in einem Rutsch einlesen
			bytes_read = read(fd, relBuf, RelocBufSize);
			if	(RelocBufSize != bytes_read)
				goto readerr;
			relBuf[RelocBufSize] = '\0';	// Sicherheitshalber Ende-Zeichen

			relp = relBuf;
			while(*relp)
			{
				relb = *relp++;
				if	(relb == 1)
					tp = (uint32_t *) ((char *) tp + 254);
				else
				{
					tp = (uint32_t *) ((char *) tp + relb);

					*tp = cpu_to_be32((long) (reloff - m_RAM68k) + be32_to_cpu(*tp));
				}
			}
		}
		else
		{
			DebugWarning("CMagiC::LoadReloc() - Keine Relokation");
		}
	}

	memset (bbase, 0, be32_to_cpu(exehead.blen));	// BSS löschen

exitReloc:
	if	(err)
		*basePage = NULL;
	else
		*basePage = bp;

	if	(relBuf)
		free(relBuf);

	if	(fd >= 0)
		close(fd);

	return(err);
}


/**********************************************************************
*
* (INTERN) Initialisierung von Atari68kData.m_VDISetupData
*
**********************************************************************/

void CMagiC::Init_CookieData(MgMxCookieData *pCookieData)
{
	pCookieData->mgmx_magic     = cpu_to_be32(0x4d674d78L); /* 'MgMx' */
	pCookieData->mgmx_version   = cpu_to_be32(CGlobals::s_ProgramVersion.majorRev);
	pCookieData->mgmx_len       = cpu_to_be32(sizeof(MgMxCookieData));
	pCookieData->mgmx_xcmd      = cpu_to_be32(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_xcmd_exec = cpu_to_be32(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_internal  = cpu_to_be32(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_daemon    = cpu_to_be32(0);		// wird vom Kernel gesetzt
}


/**********************************************************************
*
* Pixmap ggf. nach big endian wandeln
*
**********************************************************************/

#if !defined(__BIG_ENDIAN__)
static void PixmapToBigEndian(MXVDI_PIXMAP *thePixMap)
{
	DebugInfo("Host-CPU ist little-endian. Rufe PixmapToBigEndian() auf.");

	if (thePixMap->pixelSize != 32)
	{
		DebugInfo("Experimentell: Bildspeicherzugriffe beim Atari umdrehen, wenn nicht 32 bit.");
		bAtariVideoRamHostEndian = false;
	}

	thePixMap->baseAddr32    = cpu_to_be32(thePixMap->baseAddr32);
	thePixMap->rowBytes      = cpu_to_be16(thePixMap->rowBytes);
	thePixMap->bounds_top    = cpu_to_be16(thePixMap->bounds_top);
	thePixMap->bounds_left   = cpu_to_be16(thePixMap->bounds_left);
	thePixMap->bounds_bottom = cpu_to_be16(thePixMap->bounds_bottom);
	thePixMap->bounds_right  = cpu_to_be16(thePixMap->bounds_right);
	thePixMap->pmVersion     = cpu_to_be16(thePixMap->pmVersion);
	thePixMap->packType      = cpu_to_be16(thePixMap->packType);
	thePixMap->packSize      = cpu_to_be32(thePixMap->packSize);
	thePixMap->hRes          = cpu_to_be32(thePixMap->hRes);
	thePixMap->vRes          = cpu_to_be32(thePixMap->vRes);
	thePixMap->pixelType     = cpu_to_be16(thePixMap->pixelType);
	thePixMap->pixelSize     = cpu_to_be16(thePixMap->pixelSize);
	thePixMap->cmpCount      = cpu_to_be16(thePixMap->cmpCount);
	thePixMap->cmpSize       = cpu_to_be16(thePixMap->cmpSize);
	thePixMap->planeBytes    = cpu_to_be32(thePixMap->planeBytes);
	thePixMap->pmTable32     = cpu_to_be32(thePixMap->pmTable32);
#if 0
	if (thePixMap->pixelFormat == k32BGRAPixelFormat)
	{
		DebugInfo("PixmapToBigEndian() -- k32BGRAPixelFormat => k32ARGBPixelFormat");
		thePixMap->pixelFormat = cpu_to_be32(k32ARGBPixelFormat);
	}
#endif
}
#endif


/**********************************************************************
*
* Debug-Hilfe
*
**********************************************************************/

#if DEBUG_68K_EMU
void _DumpAtariMem(const char *filename)
{
	if (pTheMagiC)
		pTheMagiC->DumpAtariMem(filename);
}

void CMagiC::DumpAtariMem(const char *filename)
{
	FILE *f;

	if (!m_RAM68k)
	{
		DebugError("DumpAtariMem() -- kein Atari-Speicher?!?");
		return;
	}

	f = fopen(filename, "wb");
	if (!f)
	{
		DebugError("DumpAtariMem() -- kann Datei nicht erstellen.");
		return;
	}
	fwrite(m_RAM68k, 1, m_RAM68ksize, f);
	fclose(f);
}
#endif



/**********************************************************************
 * Member function wrappers
 **********************************************************************/

uint32_t CMagiC::thunk_AtariInit(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->AtariInit(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_AtariBIOSInit(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->AtariBIOSInit(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_AtariVdiInit(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->AtariVdiInit(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_AtariExec68k(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->AtariExec68k(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_AtariGetKeyboardOrMouseData(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->AtariGetKeyboardOrMouseData(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_MmxDaemon(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->MmxDaemon(params, AdrOffset68k);
}

uint32_t CMagiC::thunk_XFSFunctions(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->m_MacXFS.XFSFunctions(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_XFSDevFunctions(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->m_MacXFS.XFSDevFunctions(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_Drv2DevCode(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->m_MacXFS.Drv2DevCode(params, AdrOffset68k);
}
uint32_t CMagiC::thunk_RawDrvr(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->m_MacXFS.RawDrvr(params, AdrOffset68k);
}

uint32_t CMagiC::thunk_XCmdCommand(uint32_t params, unsigned char *AdrOffset68k)
{
	return pTheMagiC->m_pXCmd->Command(params, AdrOffset68k);
}


/**********************************************************************
*
* Initialisierung
* => 0 = OK, sonst = Fehler
*
* Virtueller 68k-Adreßraum:
*
*	Systemvariablen/(X)BIOS-Variablen
*	frei
*	Atari68kData						<- _memtop
*	Kernel-Basepage (256 Bytes)
*	MacXSysHdr
*	Rest des Kernels
*	Ende des Atari-RAM
*	Beginn des virtuellen VRAM			<- _v_bas_ad
*	Ende des virtuellen VRAM
*										<- _phystop
*
* Die Größe des virtuellen VRAM berechnet sich aus der Anzahl der
* Bildschirmzeilen des Atari und der physikalischen (!) Bildschirm-
* Zeilenlänge des Mac, d.h. ist abhängig von der physikalischen
* Auflösung des Mac-Bildschirms.
*
**********************************************************************/

int CMagiC::Init(CMagiCScreen *pMagiCScreen, CXCmd *pXCmd)
{
	OSErr err;
	Atari68kData *pAtari68kData;
	struct MacXSysHdr *pMacXSysHdr;
	struct SYSHDR *pSysHdr;
	uint32_t AtariMemtop;		// Ende Atari-Benutzerspeicher
	uint32_t chksum;
	unsigned numVideoLines;


	// Konfiguration

	DebugInfo("MultiThread-Version für MacOS X mit Carbon-Events");
#ifdef EMULATE_68K_TRACE
	DebugInfo("68k-Trace wird emuliert (Emulator läuft etwas langsamer)");
#else
	DebugInfo("68k-Trace wird nicht emuliert (Emulator läuft etwas schneller)");
#endif
#ifdef _DEBUG_WRITEPROTECT_ATARI_OS
	DebugInfo("68k-ROM wird schreibgeschützt (Emulator läuft etwas langsamer)");
#else
	DebugInfo("68k-ROM wird nicht schreibgeschützt (Emulator läuft etwas schneller)");
#endif
#ifdef WATCH_68K_PC
	DebugInfo("68k-PC wird auf Gültigkeit überprüft (Emulator läuft etwas langsamer)");
#else
	DebugInfo("68k-PC wird nicht auf Gültigkeit überprüft (Emulator läuft etwas schneller)");
#endif
	DebugInfo("Mac-Menü %s", (CGlobals::s_bShowMacMenu) ? "ein" : "aus");
	DebugInfo("Autostart %s", (CGlobals::s_Preferences.m_bAutoStartMagiC) ? "ein" : "aus");

	p_bVideoBufChanged = &bVideoBufChanged;

	// Bildschirmdaten

	m_pMagiCScreen = pMagiCScreen;

	// Tastaturtabelle lesen

	(void) CMagiCKeyboard::Init();

	m_RAM68ksize = Globals.s_Preferences.m_AtariMemSize;
	numVideoLines = m_pMagiCScreen->m_PixMap.bounds_bottom - m_pMagiCScreen->m_PixMap.bounds_top + 1;
	m_FgBufferLineLenInBytes = (m_pMagiCScreen->m_PixMap.rowBytes & 0x3fff);
	m_Video68ksize = m_FgBufferLineLenInBytes * numVideoLines;
	// Atari-Speicher holen
	m_RAM68k = (unsigned char *) malloc(m_RAM68ksize);
	if	(!m_RAM68k)
	{
/*
Der Applikation steht nicht genügend Speicher zur Verfügung.

Weisen Sie der Applikation mit Hilfe des Finder-Dialogs "Information" mehr Speicher zu!
[Abbruch]
The application ran out of memory.

Assign more memory to the application using the Finder dialogue "Information"!
[Cancel]
*/
		MyAlert(ALRT_NOT_ENOUGH_MEM, kAlertStopAlert);
		return(1);
	}

	// Atari-Kernel lesen
	err = LoadReloc(CGlobals::s_MagiCKernelUrl, 0, -1, &m_BasePage);
	if	(err)
		return(err);

	// 68k Speicherbegrenzungen ausrechnen
	Adr68kVideo = m_RAM68ksize;
	DebugInfo("68k-Videospeicher beginnt bei 68k-Adresse 0x%08x und ist %zu Bytes groß.", Adr68kVideo, m_Video68ksize);
	Adr68kVideoEnd = Adr68kVideo + m_Video68ksize;
	m_pFgBuffer = (unsigned char *) m_pMagiCScreen->hostScreen;

	UpdateAtariDoubleBuffer();

	// Atari-Systemvariablen setzen

	*((uint32_t *)(m_RAM68k+phystop)) = cpu_to_be32(Adr68kVideoEnd);
	*((uint32_t *)(m_RAM68k+_v_bas_ad)) = cpu_to_be32(m_RAM68ksize);
	AtariMemtop = ((uint32_t) ((unsigned char *) m_BasePage - m_RAM68k)) - sizeof(Atari68kData);
	*((uint32_t *)(m_RAM68k+_memtop)) = cpu_to_be32(AtariMemtop);
	*((uint16_t *)(m_RAM68k+sshiftmd)) = cpu_to_be16(2);		// ST high (640*400*2)
	*((uint16_t *)(m_RAM68k+_cmdload)) = cpu_to_be16(0);		// AES booten
	*((uint16_t *)(m_RAM68k+_nflops)) = cpu_to_be16(0);		// keine Floppies

	// Atari-68k-Daten setzen

	pAtari68kData = (Atari68kData *) (m_RAM68k + AtariMemtop);
	pAtari68kData->m_PixMap = m_pMagiCScreen->m_PixMap;
	// left und top scheinen nicht abgefragt zu werden, nur right und bottom
	pAtari68kData->m_PixMap.baseAddr32 = Adr68kVideo;		// virtuelle 68k-Adresse

	#if !defined(__BIG_ENDIAN__)
	PixmapToBigEndian(&pAtari68kData->m_PixMap);
	#endif

	DebugInfo("CMagiC::Init() - Adresse Basepage des Systems = 0x%08lx (68k)", AtariMemtop + sizeof(Atari68kData));
	DebugInfo("CMagiC::Init() - Adresse Atari68kData = 0x%08lx (68k)", (unsigned long)AtariMemtop);

	// neue Daten

	Init_CookieData(&pAtari68kData->m_CookieData);

	// Alle Einträge der Übergabestruktur füllen

	pMacXSysHdr = (MacXSysHdr *) (m_BasePage + 1);		// Zeiger hinter Basepage

	if	(be32_to_cpu(pMacXSysHdr->MacSys_magic) != 'MagC')
	{
		DebugError("CMagiC::Init() - Falsches magic");
		goto err_inv_os;
	}

	assert(sizeof(C_callback_CPP) == 16);

	if	(be32_to_cpu(pMacXSysHdr->MacSys_len) != sizeof(*pMacXSysHdr))
	{
		DebugError("CMagiC::Init() - Strukturlänge stimmt nicht (Header: %lu Bytes, Soll: %lu Bytes)", (unsigned long)pMacXSysHdr->MacSys_len, (unsigned long)sizeof(*pMacXSysHdr));
		err_inv_os:
/*
Die Datei "MagicMacX.OS" ist defekt oder gehört zu einer anderen Programmversion als der Emulator.

Installieren Sie die Applikation neu.
[MagiCMacX beenden]
The file "MagicMacX.OS" seems to be corrupted or belongs to a different (newer or older) version as the emulator program.

Reinstall the application.
[Quit program]
*/
		MyAlert(ALRT_INVALID_MAGICMAC_OS, kAlertStopAlert);
		return(1);
	}

	/*
	 * set callbacks for kernel
	 */
	m_pXCmd = pXCmd;
#define SetThunk(a,b) { g_callbacks[a] = b; pMacXSysHdr->a = a; }
#define SetThunkCPP(a,b) { g_callbacks[a] = b; pMacXSysHdr->a.func = a; }
	g_callbacks[MacSys_enosys] = AtariEnosys;

	pMacXSysHdr->MacSys_verMac = cpu_to_be32(10);
	pMacXSysHdr->MacSys_cpu = cpu_to_be16(20);		// 68020
	pMacXSysHdr->MacSys_fpu = cpu_to_be16(0);		// keine FPU
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L491 */
	SetThunkCPP(MacSys_init, thunk_AtariInit);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L706 */
	SetThunkCPP(MacSys_biosinit, thunk_AtariBIOSInit);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L763 */
	SetThunkCPP(MacSys_VdiInit, thunk_AtariVdiInit);
	SetThunkCPP(MacSys_Exec68k, thunk_AtariExec68k);
	pMacXSysHdr->MacSys_pixmap = cpu_to_be32((uint32_t)((char *)&pAtari68kData->m_PixMap - (char *)m_RAM68k));
	pMacXSysHdr->MacSys_pMMXCookie = cpu_to_be32((uint32_t)((char *)&pAtari68kData->m_CookieData - (char *)m_RAM68k));
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L866 */
	SetThunkCPP(MacSys_Xcmd, thunk_XCmdCommand);
	pMacXSysHdr->MacSys_PPCAddr = cpu_to_be32((uint32_t) (uintptr_t) m_RAM68k);
	pMacXSysHdr->MacSys_VideoAddr = cpu_to_be32(m_pMagiCScreen->m_PixMap.baseAddr32);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2524 */
	SetThunk(MacSys_gettime, AtariGettime);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2538 */
	SetThunk(MacSys_settime, AtariSettime);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2634 */
	SetThunk(MacSys_Setpalette, AtariSetpalette);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2647 */
	SetThunk(MacSys_Setcolor, AtariSetcolor);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2669 */
	SetThunk(MacSys_VsetRGB, AtariVsetRGB);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2681 */
	SetThunk(MacSys_VgetRGB, AtariVgetRGB);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1667 */
	SetThunk(MacSys_syshalt, AtariSysHalt);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1724 */
	SetThunk(MacSys_syserr, AtariSysErr);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3069 */
	SetThunk(MacSys_coldboot, AtariColdBoot);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/inc/puntaes.s#L95 */
	SetThunk(MacSys_exit, AtariExit);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1644 */
	SetThunk(MacSys_debugout, AtariDebugOut);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L830 */
	SetThunk(MacSys_error, AtariError);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3276 */
	SetThunk(MacSys_prtos, AtariPrtOs);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3271 */
	SetThunk(MacSys_prtin, AtariPrtIn);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3264 */
	SetThunk(MacSys_prtout, AtariPrtOut);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3291 */
	SetThunk(MacSys_prtouts, AtariPrtOutS);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3225 */
	SetThunk(MacSys_serconf, AtariSerConf);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3254 */
	SetThunk(MacSys_seris, AtariSerIs);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3259 */
	SetThunk(MacSys_seros, AtariSerOs);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3235 */
	SetThunk(MacSys_serin, AtariSerIn);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3247 */
	SetThunk(MacSys_serout, AtariSerOut);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L145 */
	SetThunk(MacSys_SerOpen, AtariSerOpen);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L159 */
	SetThunk(MacSys_SerClose, AtariSerClose);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L173 */
	SetThunk(MacSys_SerRead, AtariSerRead);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L191 */
	SetThunk(MacSys_SerWrite, AtariSerWrite);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L209 */
	SetThunk(MacSys_SerStat, AtariSerStat);
	/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L234 */
	SetThunk(MacSys_SerIoctl, AtariSerIoctl);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2830 */
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2870 */
	SetThunkCPP(MacSys_GetKeybOrMouse, thunk_AtariGetKeyboardOrMouseData);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L848 */
	SetThunk(MacSys_dos_macfn, AtariDOSFn);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s */
	SetThunkCPP(MacSys_xfs, thunk_XFSFunctions);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxfs.s#L1183 */
	SetThunkCPP(MacSys_xfs_dev, thunk_XFSDevFunctions);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1772 */
	SetThunkCPP(MacSys_drv2devcode, thunk_Drv2DevCode);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1753 */
	SetThunkCPP(MacSys_rawdrvr, thunk_RawDrvr);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L814 */
	SetThunkCPP(MacSys_Daemon, thunk_MmxDaemon);
	/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3391 */
	SetThunk(MacSys_Yield, AtariYield);

#undef SetThunk
#undef SetThunkCPP

	// ssp nach Reset
	*((uint32_t *)(m_RAM68k + 0)) = cpu_to_be32(512*1024);		// Stack auf 512k
	// pc nach Reset
	*((uint32_t *)(m_RAM68k + 4)) = pMacXSysHdr->MacSys_syshdr;

	// TOS-SYSHDR bestimmen

	pSysHdr = (SYSHDR *) (m_RAM68k + be32_to_cpu(pMacXSysHdr->MacSys_syshdr));

	// Adresse für kbshift, kbrepeat und act_pd berechnen

	m_AtariKbData = m_RAM68k + be32_to_cpu(pSysHdr->kbshift);
	m_pAtariActPd = (uint32_t *) (m_RAM68k + be32_to_cpu(pSysHdr->_run));

	// Andere Atari-Strukturen

	m_pAtariActAppl = (uint32_t *) (m_RAM68k + be32_to_cpu(pMacXSysHdr->MacSys_act_appl));

	// Prüfsumme für das System berechnen

	chksum = 0;
	uint32_t *fromptr = (uint32_t *) (m_RAM68k + be32_to_cpu(pMacXSysHdr->MacSys_syshdr));
	uint32_t *toptr = (uint32_t *) (m_RAM68k + be32_to_cpu(m_BasePage->p_tbase) + be32_to_cpu(m_BasePage->p_tlen) + be32_to_cpu(m_BasePage->p_dlen));
#ifdef _DEBUG
//	AdrOsRomStart = be32_to_cpu(pMacXSysHdr->MacSys_syshdr);			// Beginn schreibgeschützter Bereich
	AdrOsRomStart = be32_to_cpu(m_BasePage->p_tbase);		// Beginn schreibgeschützter Bereich
	AdrOsRomEnd = be32_to_cpu(m_BasePage->p_tbase) + be32_to_cpu(m_BasePage->p_tlen) + be32_to_cpu(m_BasePage->p_dlen);	// Ende schreibgeschützter Bereich
#endif
	do
	{
		chksum += cpu_to_be32(*fromptr++);
	}
	while(fromptr < toptr);

	*((uint32_t *)(m_RAM68k + os_chksum)) = cpu_to_be32(chksum);

	// dump Atari

//	DumpAtariMem("AtariMemAfterInit.data");

	// Adreßüberprüfung fürs XFS

	m_MacXFS.Set68kAdressRange(m_RAM68ksize);

	// Laufwerk C: machen

	*((uint32_t *)(m_RAM68k + _drvbits)) = cpu_to_be32(0);		// noch keine Laufwerke
	m_MacXFS.SetXFSDrive(
					'C'-'A',							// drvnum
					CMacXFS::MacDir,					// drvType
					CGlobals::s_rootfsUrl,				// path
					(Globals.s_Preferences.m_drvFlags['C'-'A'] & 2) ? false : true,	// lange Dateinamen
					(Globals.s_Preferences.m_drvFlags['C'-'A'] & 1) ? true : false,	// umgekehrte Verzeichnis-Reihenfolge (Problem bei OS X 10.2!)
					m_RAM68k);
	*((uint16_t *)(m_RAM68k + _bootdev)) = cpu_to_be16('C'-'A');	// Boot-Laufwerk C:

	// Andere Laufwerke außer C: machen

	for	(short i = 0; i < NDRIVES; i++)
	{
		ChangeXFSDrive(i);
	}

	//
	// 68k-Emulator (Mushashi) initialisieren
	//

#if defined(USE_ASGARD_PPC_68K_EMU)

	OpcodeROM = m_RAM68k;	// ROM == RAM
	Asgard68000SetIRQCallback(IRQCallback, this);
	Asgard68000SetHiMem(m_RAM68ksize);
	m_bSpecialExec = false;
//	Asgard68000Reset();
//	CPU mit vbr, sr und cacr
	Asgard68000Reset();

#else
	// Der 68020 ist die leistungsfähigste CPU, die Musashi unterstützt
	m68k_set_cpu_type(M68K_CPU_TYPE_68020);
	m68k_init();

	// Emulator starten

	OpcodeROM = m_RAM68k;	// ROM == RAM
	m68k_set_int_ack_callback(IRQCallback);
	m68k_SetBaseAddr(m_RAM68k);
	m68k_SetHiMem(m_RAM68ksize);
	m_bSpecialExec = false;

	// Reset Musashi 68k emulator
	m68k_pulse_reset();

#endif

	return(0);
}


/**********************************************************************
*
* Initialisiere zweiten Atari-Bildschirmspeicher als Hintergrundpuffer.
*
**********************************************************************/

void CMagiC::UpdateAtariDoubleBuffer(void)
{
	DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideoAddr =%p", (void *)m_pFgBuffer);
	HostVideoAddr = m_pFgBuffer;

#if 0
	if	(m_pBgBuffer)
	{
		DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideo2Addr =%p", (void *)m_pBgBuffer);
		HostVideo2Addr = m_pBgBuffer;
	}
	else
	{
		DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideo2Addr =%p", (void *)m_pFgBuffer);
		HostVideo2Addr = m_pFgBuffer;
	}
#endif
}


/**********************************************************************
*
* (STATISCH) gibt drvbits zurück
*
**********************************************************************/
/*
uint32_t CMagiC::GetAtariDrvBits(void)
{
	*((uint32_t *)(pTheMagiC->m_RAM68k + _drvbits)) = cpu_to_be32(0);		// noch keine Laufwerke

	newbits |= (1L << ('m'-'a'));	// virtuelles Laufwerk M: immer präsent
	*(long*)(&AdrOffset68k[_drvbits]) &= -1L-xfs_drvbits;		// alte löschen
	*(long*)(&AdrOffset68k[_drvbits]) |= newbits;			// neue setzen
	xfs_drvbits = newbits;
}
*/

/**********************************************************************
*
* (STATISCH) gibt Namen und PD des aktuellen Atari-Programms zurück
*
**********************************************************************/

void CMagiC::GetActAtariPrg(const char **pName, uint32_t *pact_pd)
{
	uint32_t pact_appl;
	uint32_t pprocdata;
	MagiC_PD *pMagiCPd;
	MagiC_ProcInfo *pMagiCProcInfo;
	MagiC_APP *pMagiCApp;


	*pact_pd = be32_to_cpu(*pTheMagiC->m_pAtariActPd);
	if	((*pact_pd != 0) && (*pact_pd < pTheMagiC->m_RAM68ksize))
	{
		pMagiCPd = (MagiC_PD *) (OpcodeROM + *pact_pd);
		pprocdata = be32_to_cpu(pMagiCPd->p_procdata);
		if	((pprocdata != 0) && (pprocdata < pTheMagiC->m_RAM68ksize))
		{
			pMagiCProcInfo = (MagiC_ProcInfo *) (OpcodeROM + pprocdata);
			*pName = pMagiCProcInfo->pr_fname;	// array, not pointer!
		}
		else
			*pName = NULL;
	}
	else
		*pName = NULL;

	pact_appl = be32_to_cpu(*pTheMagiC->m_pAtariActAppl);
	if	((pact_appl != 0) && (pact_appl < pTheMagiC->m_RAM68ksize))
	{
		pMagiCApp = (MagiC_APP *) (OpcodeROM + pact_appl);
	}
}


/**********************************************************************
*
* Atari-Laufwerk hat sich geändert
*
**********************************************************************/

void CMagiC::ChangeXFSDrive(short drvNr)
{
	CMacXFS::MacXFSDrvType NewType;

	if	(drvNr == 'U' - 'A')
		return;					// C:, M:, U:  sind nicht änderbar

	if	((drvNr == 'C' - 'A') || (drvNr == 'M' - 'A'))
	{
		m_MacXFS.ChangeXFSDriveFlags(
					drvNr,				// Laufwerknummer
					(Globals.s_Preferences.m_drvFlags[drvNr] & 2) ? false : true,	// lange Dateinamen
					(Globals.s_Preferences.m_drvFlags[drvNr] & 1) ? true : false	// umgekehrte Verzeichnis-Reihenfolge (Problem bei OS X 10.2!)
					);
	}
	else
	{
		NewType = (Globals.s_Preferences.m_drvPath[drvNr] == NULL) ? CMacXFS::NoMacXFS : CMacXFS::MacDir;

		m_MacXFS.SetXFSDrive(
					drvNr,				// Laufwerknummer
					NewType,			// Laufwerktyp: Mac-Verzeichnis oder nichts
					Globals.s_Preferences.m_drvPath[drvNr],
					(Globals.s_Preferences.m_drvFlags[drvNr] & 2) ? false : true,	// lange Dateinamen
					(Globals.s_Preferences.m_drvFlags[drvNr] & 1) ? true : false,	// umgekehrte Verzeichnis-Reihenfolge (Problem bei OS X 10.2!)
					m_RAM68k);
		}
}


/**********************************************************************
*
* Ausführungs-Thread starten
* => 0 = OK, sonst = Fehler
*
* Erstellt und startet den MP-Thread, der den Atari ausführt.
*
**********************************************************************/

int CMagiC::CreateThread( void )
{
	OSStatus errl;	// 32 Bit

	if	(!m_BasePage)
		return(-1);

	if	(!MPLibraryIsLoaded())
	{
		DebugError("CMagiC::CreateThread() - MP-Bibliothek nicht geladen");
		return(-2);
	}

	// Message-Queue für den MP-Thread erstellen
	OS_CreateQueue(&m_EmuNotifQID);

	m_bCanRun = false;		// nicht gestartet

	// Event für Start/Stop erstellen

	OS_CreateEvent(&m_EventId);

	// Event für Interrupts erstellen (idle loop aufwecken)

	OS_CreateEvent(&m_InterruptEventsId);

	// CriticalRegion für Tastaturpuffer und Bildschirmpufferadressen erstellen

	OS_CreateCriticalRegion(&m_KbCriticalRegionId);
	OS_CreateCriticalRegion(&m_AECriticalRegionId);
	OS_CreateCriticalRegion(&m_ScrCriticalRegionId);

	// MP-Thread ("Task") erstellen
	errl = MPCreateTask(
		_EmuThread,		// Einsprungspunkt
		this,				// Parameter
		65536,//32768,			// Stacksize
		m_EmuNotifQID,		// Message-Queue
		(void *) 'quit',		// 1. Teil der Terminate-Nachricht
		(void *) NULL,		// 2. Teil der Terminate-Nachricht
		(MPTaskOptions) 0,	// keine Schweinereien
		&m_EmuTaskID		// Rückgabe: Task-ID
		);
	if	(errl)
	{
		DebugError("CMagiC::CreateThread() - Fehler beim Erstellen des Threads");
		OS_DeleteQueue(m_EmuNotifQID);
		m_EmuNotifQID = NULL;
		return(errl);
	}

	DebugInfo("CMagiC::CreateThread() - erfolgreich");

	errl = MPSetTaskWeight(m_EmuTaskID, 300);	// 100 ist default, 200 ist die blue task

	// Workaround für Fehler in OS X 10.0.0
	if	(errl > 0)
	{
		DebugWarning("CMagiC::CreateThread() - Betriebssystem-Fehler beim Priorisieren des Threads");
		errl = 0;
	}

	if	(errl)
	{
		DebugError("CMagiC::CreateThread() - Fehler beim Priorisieren des Threads");
		OS_DeleteQueue(m_EmuNotifQID);
		m_EmuNotifQID = NULL;
		return(errl);
	}

	return(0);
}


/**********************************************************************
*
* Läßt den Ausführungs-Thread loslaufen
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

void CMagiC::StartExec( void )
{
	m_bAtariWasRun = true;

	m_bCanRun = true;		// darf laufen
	m_AtariKbData[0] = 0;		// kbshift löschen
	m_AtariKbData[1] = 0;		// kbrepeat löschen
	OS_SetEvent(			// aufwecken
			m_EventId,
			EMU_EVNT_RUN);
}


/**********************************************************************
*
* Hält den Ausführungs-Thread an
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

void CMagiC::StopExec( void )
{
#if defined(USE_ASGARD_PPC_68K_EMU)
	Asgard68000SetExitImmediately();
#else
	m68k_StopExecution();
#endif
	m_bCanRun = false;		// darf nicht laufen
#ifdef MAGICMACX_DEBUG68K
	for	(int i = 0; i < 100; i++)
		DebugInfo("### VideoRamWriteCounter(%2d) = %d", i, WriteCounters[i]);
#endif
}


/**********************************************************************
*
* Terminiert den Ausführungs-Thread bei Programm-Ende
*
**********************************************************************/

void CMagiC::TerminateThread(void)
{
	DebugInfo("CMagiC::TerminateThread()");
	OS_SetEvent(
			m_EventId,
			EMU_EVNT_TERM);
	StopExec();
}


/**********************************************************************
*
* Der Arbeits-Thread
*
**********************************************************************/

OSStatus CMagiC::_EmuThread(void *param)
{
	return(((CMagiC *) param)->EmuThread());
}

OSStatus CMagiC::EmuThread( void )
{
	OSStatus err;
	MPEventFlags EventFlags;
	bool bNewBstate[2];
	bool bNewMpos;
	bool bNewKey;


	m_bEmulatorIsRunning = true;

	for	(;;)
	{

		while (!m_bCanRun)
		{
			// wir warten darauf, daß wir laufen dürfen

			DebugInfo("CMagiC::EmuThread() -- MPWaitForEvent");
			err = OS_WaitForEvent(
						m_EventId,
						&EventFlags,
						kDurationForever);
			if	(err)
			{
				DebugError("CMagiC::EmuThread() -- Fehler bei MPWaitForEvent");
				break;
			}

			DebugInfo("CMagiC::EmuThread() -- MPWaitForEvent beendet");

			// wir prüfen, ob wir zum Beenden aufgefordert wurden

			if	(EventFlags & EMU_EVNT_TERM)
			{
				DebugInfo("CMagiC::EmuThread() -- normaler Abbruch");
				goto end_of_thread;	// normaler Abbruch, Thread-Ende
			}
		}

		// längere Ausführungsphase

//		DebugInfo("CMagiC::EmuThread() -- Starte 68k-Emulator");
		m_bWaitEmulatorForIRQCallback = false;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000Execute();
#else
		m68k_execute();
#endif
//		DebugInfo("CMagiC::EmuThread() --- %d 68k-Zyklen", CyclesExecuted);

		// Bildschirmadressen geändert
		if	(m_bScreenBufferChanged)
		{
			OS_EnterCriticalRegion(m_ScrCriticalRegionId, kDurationForever);
			UpdateAtariDoubleBuffer();
			m_bScreenBufferChanged = false;
			OS_ExitCriticalRegion(m_ScrCriticalRegionId);
		}

		// ausstehende Busfehler bearbeiten
		if	(m_bBusErrorPending)
		{
#if defined(USE_ASGARD_PPC_68K_EMU)
			Asgard68000SetBusError();
#else
			m68k_exception_bus_error();
#endif
			m_bBusErrorPending = false;
		}

		// ausstehende Interrupts bearbeiten
		if	(m_bInterruptPending)
		{
			m_bWaitEmulatorForIRQCallback = true;
			do
			{
#if defined(USE_ASGARD_PPC_68K_EMU)
				Asgard68000Execute();		// warte bis IRQ-Callback
#else
				m68k_execute();		// warte bis IRQ-Callback
#endif
//				DebugInfo("CMagiC::Exec() --- Interrupt Pending => %d 68k-Zyklen", CyclesExecuted);
			}
			while(m_bInterruptPending);
		}

		// aufgelaufene Maus-Interrupts bearbeiten

		if	(m_bInterruptMouseKeyboardPending)
		{
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::EmuThread() --- Enter critical region m_KbCriticalRegionId");
#endif
			OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
			if	(GetKbBufferFree() < 3)
			{
				DebugError("CMagiC::EmuThread() --- Tastenpuffer ist voll");
			}
			else
			{
				bNewBstate[0] = m_MagiCMouse.SetNewButtonState(0, m_bInterruptMouseButton[0]);
				bNewBstate[1] = m_MagiCMouse.SetNewButtonState(1, m_bInterruptMouseButton[1]);
				bNewMpos =  m_MagiCMouse.SetNewPosition(m_InterruptMouseWhere);
				bNewKey = (m_pKbRead != m_pKbWrite);
				if	(bNewBstate[0] || bNewBstate[1] || bNewMpos || bNewKey)
				{
					// Interrupt-Vektor 70 für Tastatur/MIDI mitliefern
					m_bInterruptPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
					Asgard68000SetIRQLineAndExcVector(k68000IRQLineIRQ6, k68000IRQStateAsserted, 70);
#else
					m68k_set_irq(M68K_IRQ_6);	// autovector interrupt 70
#endif
				}
			}
			m_bInterruptMouseKeyboardPending = false;

#if 0
			errl = MPResetEvent(			// kein "pending kb interrupt"
					m_InterruptEventsId,
					EMU_INTPENDING_KBMOUSE);
#endif
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::EmuThread() --- Exited critical region m_KbCriticalRegionId");
#endif
			m_bWaitEmulatorForIRQCallback = true;
			while(m_bInterruptPending)
#if defined(USE_ASGARD_PPC_68K_EMU)
				Asgard68000Execute();		// warte bis IRQ-Callback
#else
				m68k_execute();		// warte bis IRQ-Callback
#endif
		}

		// aufgelaufene 200Hz-Interrupts bearbeiten

		if	(m_bInterrupt200HzPending)
		{
			m_bInterrupt200HzPending = false;
/*
			errl = MPResetEvent(			// kein "pending kb interrupt"
					m_InterruptEventsId,
					EMU_INTPENDING_200HZ);
*/
			m_bInterruptPending = true;
			m_bWaitEmulatorForIRQCallback = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
			Asgard68000SetIRQLineAndExcVector(k68000IRQLineIRQ5, k68000IRQStateAsserted, 69);
#else
			m68k_set_irq(M68K_IRQ_5);		// autovector interrupt 69
#endif
			while(m_bInterruptPending)
			{
#if defined(USE_ASGARD_PPC_68K_EMU)
				Asgard68000Execute();		// warte bis IRQ-Callback
#else
				m68k_execute();		// warte bis IRQ-Callback
#endif
//				DebugInfo("CMagiC::EmuThread() --- m_bInterrupt200HzPending => %d 68k-Zyklen", CyclesExecuted);
			}
		}

		// aufgelaufene VBL-Interrupts bearbeiten

		if	(m_bInterruptVBLPending)
		{
			m_bInterruptVBLPending = false;
/*
			errl = MPResetEvent(			// kein "pending kb interrupt"
					m_InterruptEventsId,
					EMU_INTPENDING_VBL);
*/
			m_bInterruptPending = true;
			m_bWaitEmulatorForIRQCallback = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
			Asgard68000SetIRQLine(k68000IRQLineIRQ4, k68000IRQStateAsserted);
#else
			m68k_set_irq(M68K_IRQ_4);
#endif
			while(m_bInterruptPending)
			{
#if defined(USE_ASGARD_PPC_68K_EMU)
				Asgard68000Execute();		// warte bis IRQ-Callback
#else
				m68k_execute();		// warte bis IRQ-Callback
#endif
//				DebugInfo("CMagiC::Exec() --- m_bInterruptVBLPending => %d 68k-Zyklen", CyclesExecuted);
			}
		}

		// ggf. Druckdatei abschließen

		if	(*((uint32_t *)(m_RAM68k+_hz_200)) - s_LastPrinterAccess > 200 * 10)
		{
			m_MagiCPrint.ClosePrinterFile();
		}
	}	// for


  end_of_thread:

	// Main Task mitteilen, daß der Emulator-Thread beendet wurde
	pTheMagiC->m_bAtariWasRun = false;
//	SendMessageToMainThread(true, kHICommandQuit);		// veraltet?

	m_bEmulatorIsRunning = false;
	return(0);
}


/**********************************************************************
*
* (statisch) IRQ-Callback.
*
**********************************************************************/

#if defined(USE_ASGARD_PPC_68K_EMU)
int CMagiC::IRQCallback(int IRQLine, void *thisPtr)
{
	CMagiC *cm = (CMagiC *) thisPtr;
	// Interrupt-Leitungen zurücksetzen
	Asgard68000SetIRQLine(IRQLine, k68000IRQStateClear);
	// Verarbeitung bestätigen
	cm->m_bInterruptPending = false;
	if	(cm->m_bWaitEmulatorForIRQCallback)
		Asgard68000SetExitImmediately();
	return(0);
}
#else
int CMagiC::IRQCallback(int IRQLine)
{
	CMagiC *cm = (CMagiC *) pTheMagiC;
	// Interrupt-Leitungen zurücksetzen
	m68k_clear_irq(IRQLine);
	//Asgard68000SetIRQLine(IRQLine, k68000IRQStateClear);
	// Verarbeitung bestätigen
	cm->m_bInterruptPending = false;
	if	(cm->m_bWaitEmulatorForIRQCallback)
		m68k_StopExecution();

	if	(IRQLine == M68K_IRQ_5)
		return(69);		// autovector
	else
	if	(IRQLine == M68K_IRQ_6)
		return(70);		// autovector
	else

	// dieser Rückgabewert sollte die Interrupt-Anforderung löschen
	return(M68K_INT_ACK_AUTOVECTOR);
}
#endif


/**********************************************************************
*
* privat: Freien Platz in Tastaturpuffer ermitteln
*
**********************************************************************/

int CMagiC::GetKbBufferFree( void )
{
	int nCharsInBuffer;

	nCharsInBuffer = (m_pKbRead <= m_pKbWrite) ?
							(m_pKbWrite - m_pKbRead) :
							(KEYBOARDBUFLEN - (m_pKbRead - m_pKbWrite));
	return(KEYBOARDBUFLEN - nCharsInBuffer - 1);
}

/**********************************************************************
*
* privat: Zeichen in Tastaturpuffer einfügen.
*
* Es muß VORHER sichergestellt werden, daß genügend Platz ist.
*
**********************************************************************/

void CMagiC::PutKeyToBuffer(unsigned char key)
{
#ifdef _DEBUG_KB_CRITICAL_REGION
	DebugInfo("CMagiC::PutKeyToBuffer() --- Enter critical region m_KbCriticalRegionId");
#endif
	OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
	*m_pKbWrite++ = key;
	if	(m_pKbWrite >= m_cKeyboardOrMouseData + KEYBOARDBUFLEN)
		m_pKbWrite = m_cKeyboardOrMouseData;
	OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
	DebugInfo("CMagiC::PutKeyToBuffer() --- Exited critical region m_KbCriticalRegionId");
#endif
}


/**********************************************************************
*
* Busfehler melden (wird im Emulator-Thread aufgerufen).
*
* <addr> ist im Host-Format, d.h. "little endian" auf x86
*
**********************************************************************/

void CMagiC::SendBusError(uint32_t addr, const char *AccessMode)
{
#if defined(USE_ASGARD_PPC_68K_EMU)
	Asgard68000SetExitImmediately();
#else
	m68k_StopExecution();
#endif
	m_bBusErrorPending = true;
	m_BusErrorAddress = addr;
	strcpy(m_BusErrorAccessMode, AccessMode);
}


/**********************************************************************
*
* Eine Atari-Datei ist doppelgeklickt oder auf das Icon von
* MagicMacX geschoben worden (Apple-Event 'odoc').
*
* Wird im "main thread" aufgerufen
*
**********************************************************************/

void CMagiC::SendAtariFile(const char *pBuf)
{
	int iWritePos;


#ifdef _DEBUG
	DebugInfo("CMagiC::SendAtariFile(%s)", pBuf);
#endif
	OS_EnterCriticalRegion(m_AECriticalRegionId, kDurationForever);
	// kopiere zu startenden Pfad in die Tabelle, bis sie voll ist
	if	(m_iNoOfAtariFiles < N_ATARI_FILES)
	{
		if	(!m_iNoOfAtariFiles)
		{
			// dies ist die einzige Datei. Packe sie nach vorn.
			m_iOldestAtariFile = 0;
		}
		iWritePos = (m_iOldestAtariFile + m_iNoOfAtariFiles) % N_ATARI_FILES;
		strcpy(m_szStartAtariFiles[iWritePos], pBuf);
		m_iNoOfAtariFiles++;
	}
	OS_ExitCriticalRegion(m_AECriticalRegionId);
}


/**********************************************************************
*
* Das Programm soll beendet werden, dazu soll sich der Emulator
* sauber, d.h. über Shutdown beenden.
*
* Wird im "main thread" aufgerufen
*
**********************************************************************/

void CMagiC::SendShutdown(void)
{
	m_bShutdown = true;
}


/**********************************************************************
*
* Tastaturdaten schicken.
*
* Wird von der "main event loop" aufgerufen.
* Löst einen Interrupt 6 aus.
*
* Rückgabe != 0, wenn die letzte Nachricht noch aussteht.
*
**********************************************************************/

#if 0
int CMagiC::SendKeyboard(uint32_t message, bool KeyUp)
{
	unsigned char val;


#ifdef _DEBUG_NO_ATARI_KB_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
	//	DebugInfo("CMagiC::SendKeyboard() --- message == %08x, KeyUp == %d", message, (int) KeyUp);
		if	(Globals.s_Preferences.m_KeyCodeForRightMouseButton)
		{
			if	((message >> 8) == Globals.s_Preferences.m_KeyCodeForRightMouseButton)
			{
				// Emulation der rechten Maustaste
				return(SendMouseButton( 1, !KeyUp ));
			}
		}

#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboard() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		if	(GetKbBufferFree() < 1)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
			DebugError("CMagiC::SendKeyboard() --- Tastenpuffer ist voll");
			return(1);
		}

		// Umrechnen in Atari-Scancode

		val = m_MagiCKeyboard.GetScanCode(message);
		if	(!val)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
			return(0);		// unbekannte Taste
		}

		if	(KeyUp)
			val |= 0x80;

		PutKeyToBuffer(val);

		// Interrupt-Vektor 70 für Tastatur/MIDI mitliefern

		m_bInterruptMouseKeyboardPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif

		OS_SetEvent(			// aufwecken, wenn in "idle task"
				m_InterruptEventsId,
				EMU_INTPENDING_KBMOUSE);

		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
	}

	return(0);	// OK
}
#endif


/**********************************************************************
 *
 * Tastaturdaten schicken.
 *
 * Wird von der "main event loop" aufgerufen.
 * Löst einen Interrupt 6 aus.
 *
 * Rückgabe != 0, wenn die letzte Nachricht noch aussteht.
 *
 **********************************************************************/

int CMagiC::SendSdlKeyboard(int sdlScanCode, bool KeyUp)
{
	unsigned char val;
	
	
#ifdef _DEBUG_NO_ATARI_KB_INTERRUPTS
	return(0);
#endif
	
	if (m_bEmulatorIsRunning)
	{
		//	DebugInfo("CMagiC::SendKeyboard() --- message == %08x, KeyUp == %d", message, (int) KeyUp);
		
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboard() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		if	(GetKbBufferFree() < 1)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
			DebugError("CMagiC::SendKeyboard() --- Tastenpuffer ist voll");
			return(1);
		}
		
		// Umrechnen in Atari-Scancode
		
		val = m_MagiCKeyboard.SdlScanCode2AtariScanCode(sdlScanCode);
		if	(!val)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
			return(0);		// unbekannte Taste
		}
		
		if	(KeyUp)
			val |= 0x80;
		
		PutKeyToBuffer(val);
		
		// Interrupt-Vektor 70 für Tastatur/MIDI mitliefern
		
		m_bInterruptMouseKeyboardPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif
		
		OS_SetEvent(			// aufwecken, wenn in "idle task"
					m_InterruptEventsId,
					EMU_INTPENDING_KBMOUSE);
		
		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
#endif
	}
	
	return(0);	// OK
}


/**********************************************************************
*
* Status von Shift/Alt/Ctrl schicken.
*
* Wird von der "main event loop" aufgerufen.
* Löst einen Interrupt 6 aus.
*
* Rückgabe != 0, wenn die letzte Nachricht noch aussteht.
*
**********************************************************************/

int CMagiC::SendKeyboardShift( uint32_t modifiers )
{
	unsigned char val;
	bool bAutoBreak;
	bool done = false;
	int nKeys;


#ifdef _DEBUG_NO_ATARI_KB_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
		// Emulation der rechten Maustaste mit der linken und gedrückter Cmd-Taste
#if 0
		if	((!Globals.s_Preferences.m_KeyCodeForRightMouseButton) &&
			 (m_CurrModifierKeys & cmdKey) != (modifiers & cmdKey))
		{
			// linken Mausknopf immer loslassen
			SendMouseButton(1, false);
		}
#endif
		m_CurrModifierKeys = modifiers;
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboardShift() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		for	(;;)
		{
			// Umrechnen in Atari-Scancode und abschicken

			val = m_MagiCKeyboard.GetModifierScanCode(modifiers, &bAutoBreak);
			if	(!val)
				break;		// unbekannte Taste oder schon gedrückt

			nKeys = (bAutoBreak) ? 2 : 1;
			if	(GetKbBufferFree() < nKeys)
			{
				OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
				DebugInfo("CMagiC::SendKeyboardShift() --- Exited critical region m_KbCriticalRegionId");
#endif
				DebugError("CMagiC::SendKeyboardShift() --- Tastenpuffer ist voll");
				return(1);
			}

	//		DebugInfo("CMagiC::SendKeyboardShift() --- val == 0x%04x", (int) val);
			PutKeyToBuffer(val);
			// Bei CapsLock wird der break code automatisch mitgeschickt
			if	(bAutoBreak)
				PutKeyToBuffer((unsigned char) (val | 0x80));

			done = true;
		}

		if	(done)
		{
			m_bInterruptMouseKeyboardPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
			Asgard68000SetExitImmediately();
#else
			m68k_StopExecution();
#endif
		}

		OS_SetEvent(			// aufwecken, wenn in "idle task"
				m_InterruptEventsId,
				EMU_INTPENDING_KBMOUSE);

		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendKeyboardShift() --- Exited critical region m_KbCriticalRegionId");
#endif
	}

	return(0);	// OK
}


/**********************************************************************
*
* Mausposition schicken (Atari-Bildschirm-relativ).
*
* Je nach Compiler-Schalter:
* -	Wenn EVENT_MOUSE definiert ist, wird die Funktion von der
*	"main event loop" aufgerufen und löst beim Emulator
*	einen Interrupt 6 aus.
* -	Wenn EVENT_MOUSE nicht definiert ist, wird die Funktion im
*	200Hz-Interrupt (mit 50 Hz) aufgerufen.
*
* Rückgabe != 0, wenn die letzte Nachricht noch aussteht.
*
**********************************************************************/

int CMagiC::SendMousePosition(int x, int y)
{
#ifdef _DEBUG_NO_ATARI_MOUSE_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
		if	(x < 0)
			x = 0;
		if	(y < 0)
			y = 0;

#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendMousePosition() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		m_InterruptMouseWhere.h = (short) x;
		m_InterruptMouseWhere.v = (short) y;
		m_bInterruptMouseKeyboardPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif

		OS_SetEvent(			// aufwecken, wenn in "idle task"
				m_InterruptEventsId,
				EMU_INTPENDING_KBMOUSE);

		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendMousePosition() --- Exited critical region m_KbCriticalRegionId");
#endif
	}

	return(0);	// OK
}


/**********************************************************************
*
* Mausknopf schicken.
*
* Wird von der "main event loop" aufgerufen.
* Löst einen Interrupt 6 aus.
*
* Rückgabe != 0, wenn die letzte Nachricht noch aussteht.
*
**********************************************************************/

int CMagiC::SendMouseButton(unsigned int NumOfButton, bool bIsDown)
{
#ifdef _DEBUG_NO_ATARI_MOUSE_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
		if	(NumOfButton > 1)
		{
			DebugWarning("CMagiC::SendMouseButton() --- Mausbutton %d nicht unterstützt", NumOfButton + 1);
			return(1);
		}

#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendMouseButton() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
#if 0
		if	(!Globals.s_Preferences.m_KeyCodeForRightMouseButton)
		{

			// Emulation der rechten Maustaste mit der linken und gedrückter Cmd-Taste

			if	(bIsDown)
			{
				if	(m_CurrModifierKeys & cmdKey)
				{
					// linke Maustaste gedrückt mit gedrückter Cmd-Taste:
					// rechte, nicht linke
					m_bInterruptMouseButton[0] = false;
					m_bInterruptMouseButton[1] = true;
				}
				else
				{
					// linke Maustaste gedrückt ohne gedrückter Cmd-Taste:
					// linke, nicht rechte
					m_bInterruptMouseButton[0] = true;
					m_bInterruptMouseButton[1] = false;
				}
			}
			else
			{
				m_bInterruptMouseButton[0] = false;
				m_bInterruptMouseButton[1] = false;
			}
		}
		else
#endif
		{
			m_bInterruptMouseButton[NumOfButton] = bIsDown;
		}

		m_bInterruptMouseKeyboardPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif

		OS_SetEvent(			// aufwecken, wenn in "idle task"
				m_InterruptEventsId,
				EMU_INTPENDING_KBMOUSE);

		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::SendMouseButton() --- Exited critical region m_KbCriticalRegionId");
#endif
	}

	return(0);	// OK
}


/**********************************************************************
*
* 200Hz-Timer auslösen.
*
* Wird im Interrupt aufgerufen!
*
* Löst einen 68k-Interrupt 5 aus (abweichend vom Atari, da ist es Interrupt 6,
* der Interruptvektor 69 ist aber derselbe).
*
**********************************************************************/

int CMagiC::SendHz200( void )
{
#ifdef _DEBUG_NO_ATARI_HZ200_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
#if 0
		/* is of no use, because guest calls: "jsr v_clswk" to close VDI, no more redraws! */

		if (m_AtariShutDownDelay)
		{
			// delayed shutdown
			m_AtariShutDownDelay--;
			//(void) atomic_exchange(p_bVideoBufChanged, 1);
			if (!m_AtariShutDownDelay)
			{
				DebugInfo("CMagiC::SendHz200() -- execute delayed shutdown");

				// Emulator-Thread anhalten
				pTheMagiC->m_bCanRun = false;
				//	pTheMagiC->m_bAtariWasRun = false;	(entf. 4.11.07)
				
				// setze mir selbst einen Event zum Beenden (4.11.07)
				OS_SetEvent(
							pTheMagiC->m_EventId,
							EMU_EVNT_TERM);
				return(0);
			}
		}
#endif

		m_bInterrupt200HzPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif

		OS_SetEvent(			// aufwecken, wenn in "idle task"
			m_InterruptEventsId,
			EMU_INTPENDING_200HZ);
	}
	return(0);	// OK
}


/**********************************************************************
*
* VBL auslösen.
*
* Wird im Interrupt aufgerufen!
*
* Löst einen 68k-Interrupt 4 aus.
*
**********************************************************************/

int CMagiC::SendVBL( void )
{
#ifdef _DEBUG_NO_ATARI_VBL_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
		m_bInterruptVBLPending = true;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif

		OS_SetEvent(			// aufwecken, wenn in "idle task"
			m_InterruptEventsId,
			EMU_INTPENDING_VBL);
	}
	return(0);	// OK
}


/**********************************************************************
*
* Callback des Emulators: Erste Initialisierung beendet
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L491 */
uint32_t CMagiC::AtariInit(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Atari-BIOS initialisiert
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L706 */
uint32_t CMagiC::AtariBIOSInit(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: VDI initialisiert
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L763 */
uint32_t CMagiC::AtariVdiInit(uint32_t params, unsigned char *AdrOffset68k)
{
//#pragma unused(params)
//#pragma unused(AdrOffset68k)
	Point PtAtariMousePos;
	uint16_t *p_linea_BYTES_LIN;


	m_LineAVars = AdrOffset68k + params;
	// Aktuelle Mausposition: Bildschirmmitte
	PtAtariMousePos.h = (short) ((m_pMagiCScreen->m_PixMap.bounds_right - m_pMagiCScreen->m_PixMap.bounds_left) >> 1);
	PtAtariMousePos.v = (short) ((m_pMagiCScreen->m_PixMap.bounds_bottom - m_pMagiCScreen->m_PixMap.bounds_top) >> 1);
	m_MagiCMouse.Init(m_LineAVars, PtAtariMousePos);

	// Umgehe Fehler in Behnes MagiC-VDI. Bei Bildbreiten von 2034 Pixeln und true colour werden
	// fälschlicherweise 0 Bytes pro Bildschirmzeile berechnet. Bei größeren Bildbreiten werden
	// andere, ebenfalls fehlerhafte Werte berechnet.

	p_linea_BYTES_LIN = (uint16_t *) (m_LineAVars - 2);

#ifdef PATCH_VDI_PPC
	if	((CGlobals::s_Preferences.m_bPPC_VDI_Patch) &&
		 (CGlobals::s_PhysicalPixelSize == 32) &&
		 (CGlobals::s_pixelSize == 32) &&
		 (CGlobals::s_pixelSize2 == 32))
	{
		DebugInfo("CMagiC::AtariVdiInit() --- PPC");
//		DebugInfo("CMagiC::AtariVdiInit() --- (LINEA-2) = %u", *((uint16_t *) (m_LineAVars - 2)));
// Hier die Atari-Bildschirmbreite in Bytes eintragen, Behnes VDI kriegt hie ab 2034 Pixel Bildbreite
// immer Null raus, das führt zu Schrott.
//		*((uint16_t *) (m_LineAVars - 2)) = cpu_to_be16(8136);	// 2034 * 4
		patchppc(AdrOffset68k);
	}
#endif
	return(0);
}


/**********************************************************************
*
* 68k-Code im PPC-Code ausführen.
* params = {pc,sp,arg}		68k-Programm ausführen
* params = NULL			zurück in den PPC-Code
*
**********************************************************************/

uint32_t CMagiC::AtariExec68k(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(AdrOffset68k)
	char Old68kContext[128];
	uint32_t ret;
   	#pragma options align=packed
	struct New68Context
	{
		uint32_t regPC;
		uint32_t regSP;
		uint32_t arg;
	};
   	#pragma options align=reset
	New68Context *pNew68Context = (New68Context *) params;

	if	(!pNew68Context)
	{
		if	(!m_bSpecialExec)
		{
			DebugError("CMagiC::AtariExec68k() --- Kann speziellen Modus nicht beenden");
			return(0xffffffff);
		}
		// speziellen Modus beenden
		m_bSpecialExec = false;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000SetExitImmediately();
#else
		m68k_StopExecution();
#endif
		return(0);
	}

#if defined(USE_ASGARD_PPC_68K_EMU)
	if (Asgard68000GetContext(NULL) > 1024)
#else
	if (m68k_context_size() > 1024)
#endif
	{
		DebugError("CMagiC::AtariExec68k() --- Kontext zu groß");
		return(0xffffffff);
	}

	// alten 68k-Kontext retten
#if defined(USE_ASGARD_PPC_68K_EMU)
	(void) Asgard68000GetContext(Old68kContext);
#else
	(void) m68k_get_context(Old68kContext);
#endif
	// PC und sp setzen
#if defined(USE_ASGARD_PPC_68K_EMU)
	Asgard68000Reset();
	Asgard68000SetReg(k68000RegisterIndexPC, pNew68Context->regPC);
	Asgard68000SetReg(k68000RegisterIndexSP, pNew68Context->regSP);
	Asgard68000SetReg(k68000RegisterIndexA0, pNew68Context->arg);
	Asgard68000SetReg(k68000RegisterIndexSR, 0x2700);

	// 68k im PPC im 68k ausführen
	m_bSpecialExec = true;
	while(m_bSpecialExec)
		Asgard68000Execute();
	// alles zurück
	ret = Asgard68000GetReg(k68000RegisterIndexD0);
	(void) Asgard68000SetContext(Old68kContext);
#else
	m68k_pulse_reset();
	m68k_set_reg(M68K_REG_PC, be32_to_cpu(pNew68Context->regPC));
	m68k_set_reg(M68K_REG_SP, be32_to_cpu(pNew68Context->regSP));
	m68k_set_reg(M68K_REG_A0, be32_to_cpu(pNew68Context->arg));
	m68k_set_reg(M68K_REG_SR, 0x2700);

	// 68k im PPC im 68k ausführen
	m_bSpecialExec = true;
	while(m_bSpecialExec)
		m68k_execute();
	// alles zurück
	ret = m68k_get_reg(NULL, M68K_REG_D0);
	(void) m68k_set_context(Old68kContext);
#endif
	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: DOS-Funktionen 0x60-0xfe
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L848 */
uint32_t CMagiC::AtariDOSFn(uint32_t params, unsigned char *AdrOffset68k)
{
  	#pragma options align=packed
	struct AtariDOSFnParm
	{
		uint16_t dos_fnr;
		uint32_t parms;
	};
   	#pragma options align=reset

#ifdef _DEBUG
	AtariDOSFnParm *theAtariDOSFnParm = (AtariDOSFnParm *) (AdrOffset68k + params);
#endif
	DebugInfo("CMagiC::AtariDOSFn(fn = 0x%x)", be16_to_cpu(theAtariDOSFnParm->dos_fnr));
	return((uint32_t) EINVFN);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Gettime
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2524 */
uint32_t CMagiC::AtariGettime(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	struct timeval tv;
	time_t t;
	struct tm tm;
	
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	tm = *localtime(&t);
	return (((unsigned) tm.tm_sec) >> 1) +
		   (tm.tm_min << 5) +
		   ((unsigned short) tm.tm_hour << 11) +
		   (((uint32_t) tm.tm_mday) << 16) +
		   (((uint32_t) (tm.tm_mon + 1)) << 21) +
		   ((((uint32_t) tm.tm_year) + 1900 - 1980) << 25);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Settime
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2538 */
uint32_t CMagiC::AtariSettime(uint32_t params, unsigned char *AdrOffset68k)
{
	uint32_t time;
	struct timeval tv;
	struct tm tm;
	uint32_t *ptime;

	ptime = (uint32_t *)(AdrOffset68k + params);
	time = be32_to_cpu(*ptime);
	tm.tm_sec = (short) ((time&31)<<1);
	tm.tm_min = (short) ((time>>5)&63);
	tm.tm_hour = (short) ((time>>11)&31);
	tm.tm_mday = (short) ((time>>16)&31);
	tm.tm_mon = (short) ((time>>21)&15) - 1;
	tm.tm_year = (short) ((time>>25)+1980)-1900;
	tm.tm_isdst = -1;
	tv.tv_sec = mktime(&tm);
	tv.tv_usec = 0;
	settimeofday(&tv, NULL);

	return 0;
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Setpalette
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2634 */
uint32_t CMagiC::AtariSetpalette(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params,AdrOffset68k)
	DebugWarning("CMagiC::AtariSetpalette() -- nicht unterstützt");
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Setcolor
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2647 */
uint32_t CMagiC::AtariSetcolor(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params,AdrOffset68k)
	DebugWarning("CMagiC::AtariSetcolor() -- nicht unterstützt");
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS VsetRGB
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2669 */
uint32_t CMagiC::AtariVsetRGB(uint32_t params, unsigned char *AdrOffset68k)
{
	int i,j;
	uint32_t c;
	UInt32 *pColourTable;
  	#pragma options align=packed
	struct VsetRGBParm
	{
		uint16_t index;
		uint16_t cnt;
		uint32_t pValues;
	};
   	#pragma options align=reset

	VsetRGBParm *theVsetRGBParm = (VsetRGBParm *) (AdrOffset68k + params);
	const UInt8 *pValues = (const UInt8 *) (AdrOffset68k + be32_to_cpu(theVsetRGBParm->pValues));
	uint16_t index = be16_to_cpu(theVsetRGBParm->index);
	uint16_t cnt = be16_to_cpu(theVsetRGBParm->cnt);
	DebugInfo("CMagiC::AtariVsetRGB(index=%u, cnt=%u, 0x%02x%02x%02x%02x)",
			  (unsigned) index, (unsigned) cnt,
			  (unsigned) pValues[0], (unsigned) pValues[1], (unsigned) pValues[2], (unsigned) pValues[3]);

	// durchlaufe alle zu ändernden Farben
	pColourTable = pTheMagiC->m_pMagiCScreen->m_pColourTable;
	j = MIN(MAGIC_COLOR_TABLE_LEN, index + cnt);
	for	(i = index, pColourTable += index;
		i < j;
		i++, pValues += 4,pColourTable++)
	{
		// Atari: 00rrggbb
		// 0xff000000		black
		// 0xffff0000		red
		// 0xff00ff00		green
		// 0xff0000ff		blue
		c = (pValues[1] << 16) | (pValues[2] << 8) | (pValues[3] << 0) | (0xff000000);
		*pColourTable++ = c;
	}

	(void) atomic_exchange(p_bVideoBufChanged, 1);

	return(0);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS VgetRGB
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2681 */
uint32_t CMagiC::AtariVgetRGB(uint32_t params, unsigned char *AdrOffset68k)
{
	int i,j;
	UInt32 *pColourTable;
   	#pragma options align=packed
	struct VgetRGBParm
	{
		uint16_t index;
		uint16_t cnt;
		uint32_t pValues;
	};
    	#pragma options align=reset

 	VgetRGBParm *theVgetRGBParm = (VgetRGBParm *) (AdrOffset68k + params);
 	UInt8 *pValues = (UInt8 *) (AdrOffset68k + be32_to_cpu(theVgetRGBParm->pValues));
	uint16_t index = be16_to_cpu(theVgetRGBParm->index);
	uint16_t cnt = be16_to_cpu(theVgetRGBParm->cnt);
	DebugInfo("CMagiC::AtariVgetRGB(index=%d, cnt=%d)", index, cnt);

	// durchlaufe alle zu ändernden Farben
	pColourTable = pTheMagiC->m_pMagiCScreen->m_pColourTable;
	j = MIN(MAGIC_COLOR_TABLE_LEN, index + cnt);
	for	(i = index, pColourTable += index;
		i < j;
		i++, pValues++, pColourTable++)
	{
#if 0//SDL_BYTEORDER == SDL_BIG_ENDIAN
		pValues[0] = 0;
		pValues[1] = (*pColourTable) >> 24;
		pValues[2] = (*pColourTable) >> 16;
		pValues[3] = (*pColourTable) >> 8;
		//		rmask = 0xff000000;
		//		gmask = 0x00ff0000;
		//		bmask = 0x0000ff00;
		//		amask = 0x000000ff;
#else
		pValues[0] = 0;
		pValues[1] = (*pColourTable) >> 0;
		pValues[2] = (*pColourTable) >> 8;
		pValues[3] = (*pColourTable) >> 16;
		//		rmask = 0x000000ff;
		//		gmask = 0x0000ff00;
		//		bmask = 0x00ff0000;
		//		amask = 0xff000000;
#endif
	}

	return(0);
}


/**********************************************************************
*
* (STATIC) Nachricht (a)synchron an Haupt-Thread schicken
*
**********************************************************************/

void CMagiC::SendMessageToMainThread( bool bAsync, uint32_t command )
{
	EventRef ev;
	HICommand commandStruct;

	CreateEvent(
			NULL,
			kEventClassCommand,
			kEventProcessCommand,
			GetCurrentEventTime(),
			kEventAttributeNone,
			&ev);

	commandStruct.attributes = 0;
	commandStruct.menu.menuRef = 0;
	commandStruct.menu.menuItemIndex = 0;
	commandStruct.commandID = command;

	SetEventParameter(
			ev,
			kEventParamDirectObject,
			typeHICommand,			// gewünschter Typ
			sizeof(commandStruct),		// max. erlaubte Länge
			(void *) &commandStruct
			);

	if	(bAsync)
		PostEventToQueue(GetMainEventQueue(), ev, kEventPriorityStandard);
	else
		SendEventToApplication(ev);
}


/**********************************************************************
*
* Callback des Emulators: System aufgrund eines fatalen Fehlers anhalten
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1667 */
uint32_t CMagiC::AtariSysHalt(uint32_t params, unsigned char *AdrOffset68k)
{
	char *ErrMsg = (char *) (AdrOffset68k + params);

	DebugError("CMagiC::AtariSysHalt() -- %s", ErrMsg);

// Daten werden getrennt von der Nachricht geliefert

	SendSysHaltReason(ErrMsg);
	pTheMagiC->StopExec();
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Fehler ausgeben (68k-Exception)
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1724 */
uint32_t CMagiC::AtariSysErr(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
	uint32_t act_pd;
	uint32_t m68k_pc;
	const char *AtariPrgFname;

#if 0		// #ifdef PATCH_VDI_PPC
	// patche den Bildschirm(Offscreen-)Treiber
	static int patched = 0;
	if	(!patched)
	{
		patchppc(AdrOffset68k);
		patched = 1;
	}
#endif

	DebugError("CMagiC::AtariSysErr()");

	// FÜR DIE FEHLERSUCHE: GIB DIE LETZTEN TRACE-INFORMATIONEN AUS.
	#if DEBUG_68K_EMU
	m68k_trace_print("68k-trace-dump-syserr.txt");
	_DumpAtariMem("atarimem.bin");
	#endif

	GetActAtariPrg(&AtariPrgFname, &act_pd);
	m68k_pc = be32_to_cpu(*((uint32_t *) (AdrOffset68k + proc_stk + 2)));

	DebugInfo("CMagiC::AtariSysErr() -- act_pd = 0x%08lx", (unsigned long)act_pd);
	DebugInfo("CMagiC::AtariSysErr() -- Prozeßpfad = %s", (AtariPrgFname) ? AtariPrgFname : "<unknown>");
#if defined(_DEBUG)
	if (m68k_pc < pTheMagiC->m_RAM68ksize - 8)
	{
		uint16_t opcode1 = be16_to_cpu(*((uint16_t *) (AdrOffset68k + m68k_pc)));
		uint16_t opcode2 = be16_to_cpu(*((uint16_t *) (AdrOffset68k + m68k_pc + 2)));
		uint16_t opcode3 = be16_to_cpu(*((uint16_t *) (AdrOffset68k + m68k_pc + 4)));
		DebugInfo("CMagiC::AtariSysErr() -- opcode = 0x%04x 0x%04x 0x%04x", (unsigned) opcode1, (unsigned) opcode2, (unsigned) opcode3);
	}
#endif

	Send68kExceptionData(
				(uint16_t) (AdrOffset68k[proc_pc /*0x3c4*/]),		// Exception-Nummer
				pTheMagiC->m_BusErrorAddress,
				pTheMagiC->m_BusErrorAccessMode,
				m68k_pc,															// pc
				be16_to_cpu(*((uint16_t *) (AdrOffset68k + proc_stk))),		// sr
				be32_to_cpu(*((uint32_t *) (AdrOffset68k + proc_usp))),		// usp
				(uint32_t *) (AdrOffset68k + proc_regs /*0x384*/),					// Dx (big endian)
				(uint32_t *) (AdrOffset68k + proc_regs + 32),							// Ax (big endian)
				AtariPrgFname,
				act_pd);

	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Kaltstart
*
* Zur Zeit nur Dummy
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3069 */
uint32_t CMagiC::AtariColdBoot(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	DebugInfo("CMagiC::AtariColdBoot()");
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Emulator normal beenden
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/inc/puntaes.s#L95 */
uint32_t CMagiC::AtariExit(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)

	DebugInfo("CMagiC::AtariExit()");

#if 0
//	is of no use, because guest calls: "jsr v_clswk" to close VDI, no more redraws!
	// shutdown is done in the 200Hz timer call, delay shutdown for 0,5s to let guest complete the redraw
	pTheMagiC->m_AtariShutDownDelay = 1000;
	DebugInfo("CMagiC::AtariExit() -- delay for %u ticks", pTheMagiC->m_AtariShutDownDelay);
#else
	// FÜR DIE FEHLERSUCHE: GIB DIE LETZTEN TRACE-INFORMATIONEN AUS.
//	m68k_trace_print("68k-trace-dump-exit.txt");

	// Emulator-Thread anhalten
	pTheMagiC->m_bCanRun = false;
//	pTheMagiC->m_bAtariWasRun = false;	(entf. 4.11.07)

	// setze mir selbst einen Event zum Beenden (4.11.07)
	OS_SetEvent(
			pTheMagiC->m_EventId,
			EMU_EVNT_TERM);

	// Nachricht and Haupt-Thread zum Beenden (entf. 4.11.07)
//	SendMessageToMainThread(true, kHICommandQuit);
#ifdef MAGICMACX_DEBUG68K
	for	(int i = 0; i < 100; i++)
		DebugInfo("### VideoRamWriteCounter(%2d) = %d", i, WriteCounters[i]);
#endif
#endif
	return(0);
}

/**********************************************************************
*
* Perform Callbacks
*
**********************************************************************/
uint32_t cmagic_hostcall(uint32_t func, uint32_t params, unsigned char *AdrOffset68k)
{
	C_callback_func proc;

	/*
	 * FIXME: the exec_macfn hook in the MgMx cookie
	 * will pass a symbol address to be called as function number.
	 * That symbol address is returned by the XCmd interface.
	 */
	if (func >= C_callback_NUM)
		func = MacSys_enosys;
	proc = g_callbacks[func];
	return proc(params, AdrOffset68k);
}


/**********************************************************************
*
* Callback des Emulators: illegal function call
*
**********************************************************************/

uint32_t CMagiC::AtariEnosys(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	MyAlert(ALRT_ILLEGAL_FUNC, kAlertStopAlert);
	pTheMagiC->StopExec();	// fatal error for execution thread
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Debug-Ausgaben
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L1644 */
uint32_t CMagiC::AtariDebugOut(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
#ifdef _DEBUG
	unsigned char *atari_ptr = AdrOffset68k + params;
	char buffer[2048];
	int i;
	unsigned short ch;
	
	for (i = 0; *atari_ptr != 0 && i < (int)sizeof(buffer) - 4; atari_ptr++)
	{
		ch = atari_to_utf16[*atari_ptr];
		/* inplace variant of g_unichar_to_utf8, for speed */
		if (ch < 0x80)
		{
			buffer[i++] = ch;
		} else if (ch < 0x800)
		{
			buffer[i++] = ((ch >> 6) & 0x3f) | 0xc0;
			buffer[i++] = (ch & 0x3f) | 0x80;
		} else
		{
			buffer[i++] = ((ch >> 12) & 0x0f) | 0xe0;
			buffer[i++] = ((ch >> 6) & 0x3f) | 0x80;
			buffer[i++] = (ch & 0x3f) | 0x80;
		}
	}
	buffer[i] = '\0';
	DebugInfo("CMagiC::AtariDebugOut(%s)", buffer);
#endif
	return 0;
}


/**********************************************************************
*
* Callback des Emulators: Fehler-Alert
*
* zur Zeit nur Dummy
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L830 */
uint32_t CMagiC::AtariError(uint32_t params, unsigned char *AdrOffset68k)
{
#ifdef _DEBUG
	uint16_t errorCode = be16_to_cpu(*((uint16_t *) (AdrOffset68k + params)));
#endif

	DebugInfo("CMagiC::AtariError(%hd)", errorCode);
	/*
	 Das System kann keinen passenden Grafiktreiber finden.
	 
	 Installieren Sie einen Treiber, oder wechseln Sie die Bildschirmauflösung unter MacOS, und starten Sie MagiCMacX neu.
	 [MagiCMacX beenden]
	 
	 The system could not find an appropriate graphics driver.
	 
	 Install a driver, or change the monitor resolution resp. colour depth using the system's control panel. Finally, restart  MagiCMacX.
	 [Quit MagiCMacX]
	 */
	MyAlert(ALRT_NO_VIDEO_DRIVER, kAlertStopAlert);
	pTheMagiC->StopExec();	// fatal error for execution thread
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Ausgabestatus des Druckers abfragen
* Rückgabe: -1 = bereit 0 = nicht bereit
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3276 */
uint32_t CMagiC::AtariPrtOs(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	return(pThePrint->GetOutputStatus());
}


/**********************************************************************
*
* Callback des Emulators: Zeichen von Drucker lesen
* Rückgabe: Zeichen in Bit 0..7, andere Bits = 0
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3271 */
uint32_t CMagiC::AtariPrtIn(uint32_t params, unsigned char *AdrOffset68k)
{
	unsigned char c;
	uint32_t n;

#pragma unused(params)
#pragma unused(AdrOffset68k)
	n = pThePrint->Read(&c, 1);
	if	(!n)
		return(0);
	else
		return(c);
}


/**********************************************************************
*
* Callback des Emulators: Zeichen auf Drucker ausgeben
* params		Zeiger auf auszugebendes Zeichen (16 Bit)
* Rückgabe: 0 = Timeout -1 = OK
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3264 */
uint32_t CMagiC::AtariPrtOut(uint32_t params, unsigned char *AdrOffset68k)
{
	uint32_t ret;

	DebugInfo("CMagiC::AtariPrtOut()");
	ret = pThePrint->Write(AdrOffset68k + params + 1, 1);
	// Zeitpunkt (200Hz) des letzten Druckerzugriffs merken
	s_LastPrinterAccess = be32_to_cpu(*((uint32_t *) (AdrOffset68k + _hz_200)));
	if	(ret == 1)
		return(0xffffffff);		// OK
	else
		return(0);				// Fehler
}


/**********************************************************************
*
* Callback des Emulators: mehrere Zeichen auf Drucker ausgeben
* Rückgabe: Anzahl geschriebener Zeichen
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3291 */
uint32_t CMagiC::AtariPrtOutS(uint32_t params, unsigned char *AdrOffset68k)
{
	struct PrtOutParm
	{
		uint32_t buf;
		uint32_t cnt;
	};
 	PrtOutParm *thePrtOutParm = (PrtOutParm *) (AdrOffset68k + params);
 	uint32_t ret;


//	DebugInfo("CMagiC::AtariPrtOutS()");
	ret = pThePrint->Write(AdrOffset68k + be32_to_cpu(thePrtOutParm->buf), be32_to_cpu(thePrtOutParm->cnt));
	// Zeitpunkt (200Hz) des letzten Druckerzugriffs merken
	s_LastPrinterAccess = be32_to_cpu(*((uint32_t *) (AdrOffset68k + _hz_200)));
	return(ret);
}


/**********************************************************************
*
* Serielle Schnittstelle für BIOS-Zugriff öffnen, wenn nötig.
* Der BIOS-Aufruf erfolgt auch indirekt über das Atari-GEMDOS,
* wenn die Standard-Schnittstelle AUX: angesprochen wird.
* Die moderne Geräteschnittstelle u:\dev\SERIAL wird durch einen
* nachladbaren Gerätetreiber respräsentiert und verwendet
* andere - effizientere - Routinen.
*
* Wird aufgerufen von:
*	AtariSerOut()
*	AtariSerIn()
*	AtariSerOs()
*	AtariSerIs()
*	AtariSerConf()
*
**********************************************************************/

uint32_t CMagiC::OpenSerialBIOS(void)
{
	// schon geöffnet => OK
	if	(m_bBIOSSerialUsed)
	{
		return(0);
	}

	// schon vom DOS geöffnet => Fehler
	if	(m_MagiCSerial.IsOpen())
	{
		DebugError("CMagiC::OpenSerialBIOS() -- schon vom DOS geöffnet => Fehler");
		return((uint32_t) ERROR);
	}

	if	(-1 == (int) m_MagiCSerial.Open(CGlobals::s_Preferences.m_szAuxPath))
	{
		DebugInfo("CMagiC::OpenSerialBIOS() -- kann \"%s\" nicht öffnen.", CGlobals::s_Preferences.m_szAuxPath);
		return((uint32_t) ERROR);
	}

	m_bBIOSSerialUsed = true;
	DebugWarning("CMagiC::OpenSerialBIOS() -- Serielle Schnittstelle vom BIOS geöffnet.");
	DebugWarning("   Jetzt kann sie nicht mehr geschlossen werden!");
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Serconf
*
* Wird aufgerufen von der Atari-BIOS-Funktion Rsconf() für
* die serielle Schnittstelle. Rsconf() wird auch vom
* Atari-GEMDOS aufgerufen für die Geräte AUX und AUXNB,
* und zwar hier mit Spezialwerten:
*	Rsconf(-2,-2,-1,-1,-1,-1, 'iocl', dev, cmd, parm) 
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3225 */
uint32_t CMagiC::AtariSerConf(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerConfParm
	{
		uint16_t baud;
		uint16_t ctrl;
		uint16_t ucr;
		uint16_t rsr;
		uint16_t tsr;
		uint16_t scr;
		uint32_t xtend_magic;	// ist ggf. 'iocl'
		uint16_t biosdev;		// Ioctl-Bios-Gerät
		uint16_t cmd;			// Ioctl-Kommando
		uint32_t parm;		// Ioctl-Parameter
		uint32_t ptr2zero;		// deref. und auf ungleich 0 setzen!
	};
    	#pragma options align=reset
	unsigned int nBitsTable[] =
	{
		8,7,6,5
	};
	unsigned int baudtable[] =
	{
		19200,
		9600,
		4800,
		3600,
		2400,
		2000,
		1800,
		1200,
		600,
		300,
		200,
		150,
		134,
		110,
		75,
		50
	};
	unsigned int nBits, nStopBits;


	DebugInfo("CMagiC::AtariSerConf()");

	// serielle Schnittstelle öffnen, wenn nötig
	if	(pTheMagiC->OpenSerialBIOS())
	{
		DebugInfo("CMagiC::AtariSerConf() -- kann serielle Schnittstelle nicht öffnen => Fehler.");
		return((uint32_t) ERROR);
	}

	SerConfParm *theSerConfParm = (SerConfParm *) (AdrOffset68k + params);

	// Rsconf(-2,-2,-1,-1,-1,-1, 'iocl', dev, cmd, parm) macht Fcntl

	if	((be16_to_cpu(theSerConfParm->baud) == 0xfffe) &&
		 (be16_to_cpu(theSerConfParm->ctrl) == 0xfffe) &&
		 (be16_to_cpu(theSerConfParm->ucr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->rsr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->tsr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->scr) == 0xffff) &&
		 (be32_to_cpu(theSerConfParm->xtend_magic) == 'iocl'))
	{
		uint32_t grp;
		uint32_t mode;
		uint32_t ret;
		bool bSet;
		unsigned long NewBaudrate, OldBaudrate;
		uint16_t flags;

		bool bXonXoff;
		bool bRtsCts;
		bool bParityEnable;
		bool bParityEven;
		unsigned int nBits;
		unsigned int nStopBits;


		DebugInfo("CMagiC::AtariSerConf() -- Fcntl(dev=%d, cmd=0x%04x, parm=0x%08x)", be16_to_cpu(theSerConfParm->biosdev), be16_to_cpu(theSerConfParm->cmd), be32_to_cpu(theSerConfParm->parm));
		*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->ptr2zero))) = cpu_to_be32(0xffffffff);	// wir kennen Fcntl
		switch(be16_to_cpu(theSerConfParm->cmd))
		{
			case TIOCBUFFER:
				// Inquire/Set buffer settings
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCBUFFER) -- nicht unterstützt");
				ret = (uint32_t) EINVFN;
				break;

			case TIOCCTLMAP:
				// Inquire I/O-lines and signaling capabilities
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLMAP) -- nicht unterstützt");
				ret = (uint32_t) EINVFN;
				break;

			case TIOCCTLGET:
				// Inquire I/O-lines and signals
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLGET) -- nicht unterstützt");
				ret = (uint32_t) EINVFN;
				break;

			case TIOCCTLSET:
				// Set I/O-lines and signals
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLSET) -- nicht unterstützt");
				ret = (uint32_t) EINVFN;
				break;

			case TIOCGPGRP:
				//get terminal process group
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCGPGRP) -- nicht unterstützt");
				ret = (uint32_t) EINVFN;
				break;

			case TIOCSPGRP:
				//set terminal process group
				grp = be32_to_cpu(*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->parm))));
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCSPGRP, %d)", (uint32_t) grp);
				ret = (uint32_t) EINVFN;
				break;

			case TIOCFLUSH:
				// Leeren der seriellen Puffer
				mode = be32_to_cpu(theSerConfParm->parm);
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCFLUSH, %d)", mode);
				switch(mode)
				{
					// Der Sendepuffer soll komplett gesendet werden. Die Funktion kehrt 
					// erst zurück, wenn der Puffer leer ist (return E_OK, =0) oder ein 
					// systeminterner Timeout abgelaufen ist (return EDRVNR, =-2). Der 
					// Timeout wird vom System sinnvoll bestimmt.
					case 0:
						ret = pTheSerial->Drain();
						break;

					// Der Empfangspuffer wird gelöscht.
					case 1:
						ret = pTheSerial->Flush(true, false);
						break;

					// Der Sendepuffer wird gelöscht.
					case 2:
						ret = pTheSerial->Flush(false, true);
						break;
         
					// Empfangspuffer und Sendepuffer werden gelîscht.
					case 3:
						ret = pTheSerial->Flush(true, true);
						break;

					default:
						ret = (uint32_t) EINVFN;
						break;
				}
				break;

			case TIOCIBAUD:
			case TIOCOBAUD:
				// Eingabegeschwindigkeit festlegen
				NewBaudrate = be32_to_cpu(*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->parm))));
				bSet = ((int) NewBaudrate != -1) && (NewBaudrate != 0);
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(%s, %ld)", (be16_to_cpu(theSerConfParm->cmd) == TIOCIBAUD) ? "TIOCIBAUD" : "TIOCOBAUD", NewBaudrate);

				if	(be16_to_cpu(theSerConfParm->cmd) == TIOCIBAUD)
					ret = pTheSerial->Config(
						bSet,						// Input-Rate ggf. ändern
						NewBaudrate,				// neue Input-Rate
						&OldBaudrate,				// alte Baud-Rate
						false,						// Output-Rate nicht ändern
						0,						// neue Output-Rate
						NULL,						// alte Output-Rate egal
						false,						// Xon/Xoff ändern
						false,
						NULL,
						false,						// Rts/Cts ändern
						false,
						NULL,
						false,						// parity enable ändern
						false,
						NULL,
						false,						// parity even ändern
						false,
						NULL,
						false,						// n Bits ändern
						0,
						NULL,
						false,						// Stopbits ändern
						0,
						NULL);
				else
					ret = pTheSerial->Config(
						false,						// Input-Rate nicht ändern
						0,						// neue Input-Rate
						NULL,						// alte Input-Rate egal
						bSet,						// Output-Rate ggf. ändern
						NewBaudrate,				// neue Output-Rate
						&OldBaudrate,				// alte Output-Rate
						false,						// Xon/Xoff ändern
						false,
						NULL,
						false,						// Rts/Cts ändern
						false,
						NULL,
						false,						// parity enable ändern
						false,
						NULL,
						false,						// parity even ändern
						false,
						NULL,
						false,						// n Bits ändern
						0,
						NULL,
						false,						// Stopbits ändern
						0,
						NULL);

				*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->parm))) = cpu_to_be32(OldBaudrate);
				if	((int) ret == -1)
					ret = (uint32_t) ATARIERR_ERANGE;
				break;

			case TIOCGFLAGS:
				// Übertragungsprotokolleinstellungen erfragen

				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCGFLAGS, %d)", be32_to_cpu(theSerConfParm->parm));
				(void) pTheSerial->Config(
							false,						// Input-Rate nicht ändern
							0,						// neue Input-Rate
							NULL,						// alte Input-Rate egal
							false,						// Output-Rate nicht ändern
							0,						// neue Output-Rate
							NULL,						// alte Output-Rate egal
							false,						// Xon/Xoff nicht ändern
							false,						// neuer Wert
							&bXonXoff,					// alter Wert
							false,						// Rts/Cts nicht ändern
							false,
							&bRtsCts,
							false,						// parity enable nicht ändern
							false,
							&bParityEnable,
							false,						// parity even nicht ändern
							false,
							&bParityEven,
							false,						// n Bits nicht ändern
							0,
							&nBits,
							false,						// Stopbits nicht ändern
							0,
							&nStopBits);
				// Rückgabewert zusammenbauen
				flags = 0;
				if	(bXonXoff)
					flags |= 0x1000;
				if	(bRtsCts)
					flags |= 0x2000;
				if	(bParityEnable)
					flags |= (bParityEven) ? 0x4000 : 0x8000;
				if	(nStopBits == 1)
					flags |= 1;
				else
				if	(nStopBits == 2)
					flags |= 3;
				if	(nBits == 5)
					flags |= 0xc;
				else
				if	(nBits == 6)
					flags |= 0x8;
				else
				if	(nBits == 7)
					flags |= 0x4;
				*((uint16_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->parm))) = cpu_to_be16(flags);
				ret = (uint32_t) E_OK;
				break;

			case TIOCSFLAGS:
				// Übertragungsprotokolleinstellungen setzen
				flags = be16_to_cpu(*((uint16_t *) (AdrOffset68k + be32_to_cpu(theSerConfParm->parm))));
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCSFLAGS, 0x%04x)", (uint32_t) flags);
				bXonXoff = (flags & 0x1000) != 0;
				DebugInfo("CMagiC::AtariSerConf() -- XON/XOFF %s", (bXonXoff) ? "ein" : "aus");
				bRtsCts = (flags & 0x2000) != 0;
				DebugInfo("CMagiC::AtariSerConf() -- RTS/CTS %s", (bRtsCts) ? "ein" : "aus");
				bParityEnable = (flags & (0x4000+0x8000)) != 0;
				DebugInfo("CMagiC::AtariSerConf() -- Parität %s", (bParityEnable) ? "ein" : "aus");
				bParityEven= (flags & 0x4000) != 0;
				DebugInfo("CMagiC::AtariSerConf() -- Parität %s", (bParityEven) ? "gerade (even)" : "ungerade (odd)");
				nBits = 8U - ((flags & 0xc) >> 2);
				DebugInfo("CMagiC::AtariSerConf() -- %d Bits", nBits);
				nStopBits = flags & 3U;
				DebugInfo("CMagiC::AtariSerConf() -- %d Stop-Bits%s", nStopBits, (nStopBits == 0) ? " (Synchron-Modus?)" : "");
				if	((nStopBits == 0) || (nStopBits == 2))
					return((uint32_t) ATARIERR_ERANGE);
				if	(nStopBits == 3)
					nStopBits = 2;
				ret = pTheSerial->Config(
							false,						// Input-Rate nicht ändern
							0,						// neue Input-Rate
							NULL,						// alte Input-Rate egal
							false,						// Output-Rate nicht ändern
							0,						// neue Output-Rate
							NULL,						// alte Output-Rate egal
							true,						// Xon/Xoff ändern
							bXonXoff,					// neuer Wert
							NULL,						// alter Wert egal
							true,						// Rts/Cts ändern
							bRtsCts,
							NULL,
							true,						// parity enable ändern
							bParityEnable,
							NULL,
							false,						// parity even nicht ändern
							bParityEven,
							NULL,
							true,						// n Bits ändern
							nBits,
							NULL,
							true,						// Stopbits ändern
							nStopBits,
							NULL);
				if	((int) ret == -1)
					ret = (uint32_t) ATARIERR_ERANGE;
				break;

			default:
				DebugError("CMagiC::AtariSerConf() -- Fcntl(0x%04x -- unbekannt", be16_to_cpu(theSerConfParm->cmd) & 0xffff);
				ret = (uint32_t) EINVFN;
				break;
		}
		return(ret);
	}

	// Rsconf(-2,-1,-1,-1,-1,-1) gibt aktuelle Baudrate zurück

	if	((be16_to_cpu(theSerConfParm->baud) == 0xfffe) &&
		 (be16_to_cpu(theSerConfParm->ctrl) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->ucr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->rsr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->tsr) == 0xffff) &&
		 (be16_to_cpu(theSerConfParm->scr) == 0xffff))
	{
//		unsigned long OldInputBaudrate;
//		return((uint32_t) pTheSerial->GetBaudRate());
	}

	if	(be16_to_cpu(theSerConfParm->baud) >= sizeof(baudtable)/sizeof(baudtable[0]))
	{
		DebugError("CMagiC::AtariSerConf() -- ungültige Baudrate von Rsconf()");
		return((uint32_t) ATARIERR_ERANGE);
	}

	nBits = nBitsTable[(be16_to_cpu(theSerConfParm->ucr) >> 5) & 3];
	nStopBits = (unsigned int) (((be16_to_cpu(theSerConfParm->ucr) >> 3) == 3) ? 2 : 1);

	return(pTheSerial->Config(
					true,						// Input-Rate ändern
					baudtable[be16_to_cpu(theSerConfParm->baud)],	// neue Input-Rate
					NULL,						// alte Input-Rate egal
					true,						// Output-Rate ändern
					baudtable[be16_to_cpu(theSerConfParm->baud)],	// neue Output-Rate
					NULL,						// alte Output-Rate egal
					true,						// Xon/Xoff ändern
					(be16_to_cpu(theSerConfParm->ctrl) & 1) != 0,	// neuer Wert
					NULL,						// alter Wert egal
					true,						// Rts/Cts ändern
					(be16_to_cpu(theSerConfParm->ctrl) & 2) != 0,
					NULL,
					true,						// parity enable ändern
					(be16_to_cpu(theSerConfParm->ucr) & 4) != 0,
					NULL,
					true,						// parity even ändern
					(be16_to_cpu(theSerConfParm->ucr) & 2) != 0,
					NULL,
					true,						// n Bits ändern
					nBits,
					NULL,
					true,						// Stopbits ändern
					nStopBits,
					NULL));
}


/**********************************************************************
*
* Callback des Emulators: Lesestatus der seriellen Schnittstelle
* Rückgabe: -1 = bereit 0 = nicht bereit
*
* Wird aufgerufen von der Atari-BIOS-Funktion Bconstat() für
* die serielle Schnittstelle.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3254 */
uint32_t CMagiC::AtariSerIs(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
//	DebugInfo("CMagiC::AtariSerIs()");

	// serielle Schnittstelle öffnen, wenn nötig
	if	(!pTheMagiC->OpenSerialBIOS())
	{
		return(0);
	}

	return(pTheSerial->ReadStatus() ? 0xffffffff : 0);
}


/**********************************************************************
*
* Callback des Emulators: Ausgabestatus der seriellen Schnittstelle
* Rückgabe: -1 = bereit 0 = nicht bereit
*
* Wird aufgerufen von der Atari-BIOS-Funktion Bcostat() für
* die serielle Schnittstelle.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3259 */
uint32_t CMagiC::AtariSerOs(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
//	DebugInfo("CMagiC::AtariSerOs()");

	// serielle Schnittstelle öffnen, wenn nötig
	if	(!pTheMagiC->OpenSerialBIOS())
	{
		return(0);
	}

	return(pTheSerial->WriteStatus() ? 0xffffffff : 0);
}


/**********************************************************************
*
* Callback des Emulators: Zeichen von serieller Schnittstelle lesen
* Rückgabe: Zeichen in Bit 0..7, andere Bits = 0, 0xffffffff bei Fehler
*
* Wird aufgerufen von der Atari-BIOS-Funktion Bconin() für
* die serielle Schnittstelle.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3235 */
uint32_t CMagiC::AtariSerIn(uint32_t params, unsigned char *AdrOffset68k)
{
	char c;
	uint32_t ret;
#pragma unused(params)
#pragma unused(AdrOffset68k)

//	DebugInfo("CMagiC::AtariSerIn()");

	// serielle Schnittstelle öffnen, wenn nötig
	if	(!pTheMagiC->OpenSerialBIOS())
	{
		return(0);
	}

	ret = pTheSerial->Read(1, &c);
	if	(ret > 0)
	{
		return((uint32_t) c & 0x000000ff);
	}
	else
		return(0xffffffff);
}


/**********************************************************************
*
* Callback des Emulators: Zeichen auf serielle Schnittstelle ausgeben
* params		Zeiger auf auszugebendes Zeichen (16 Bit)
* Rückgabe: 1 = OK, 0 = nichts geschrieben, sonst Fehlercode
*
* Wird aufgerufen von der Atari-BIOS-Funktion Bconout() für
* die serielle Schnittstelle.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3247 */
uint32_t CMagiC::AtariSerOut(uint32_t params, unsigned char *AdrOffset68k)
{
//	DebugInfo("CMagiC::AtariSerOut()");

	// serielle Schnittstelle öffnen, wenn nötig
	if	(!pTheMagiC->OpenSerialBIOS())
	{
		return(0);
	}

	return(pTheSerial->Write(1, (char *) AdrOffset68k + params + 1));
}


/**********************************************************************
*
* Callback des Emulators: Serielle Schnittstelle öffnen
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L145 */
uint32_t CMagiC::AtariSerOpen(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)

	DebugInfo("CMagiC::AtariSerOpen()");

	// schon durch BIOS geöffnet => OK
	if	(pTheMagiC->m_bBIOSSerialUsed)
	{
		DebugInfo("CMagiC::AtariSerOpen() -- schon durch BIOS geöffnet => OK");
		return(0);
	}

	// schon vom DOS geöffnet => Fehler
	if	(pTheMagiC->m_MagiCSerial.IsOpen())
	{
		DebugInfo("CMagiC::AtariSerOpen() -- schon vom DOS geöffnet => Fehler");
		return((uint32_t) EACCDN);
	}

	if	(-1 == (int) pTheMagiC->m_MagiCSerial.Open(CGlobals::s_Preferences.m_szAuxPath))
	{
		DebugInfo("CMagiC::AtariSerOpen() -- kann \"%s\" nicht öffnen.", CGlobals::s_Preferences.m_szAuxPath);
		return((uint32_t) ERROR);
	}

	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Serielle Schnittstelle schließen
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L159 */
uint32_t CMagiC::AtariSerClose(uint32_t params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)

	DebugInfo("CMagiC::AtariSerClose()");

	// schon durch BIOS geöffnet => OK
	if	(pTheMagiC->m_bBIOSSerialUsed)
		return(0);

	// nicht vom DOS geöffnet => Fehler
	if	(!pTheMagiC->m_MagiCSerial.IsOpen())
	{
		return((uint32_t) EACCDN);
	}

	if	(pTheMagiC->m_MagiCSerial.Close())
	{
		return((uint32_t) ERROR);
	}

	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Mehrere Zeichen von serieller Schnittstelle lesen
* Rückgabe: Anzahl Zeichen
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L173 */
uint32_t CMagiC::AtariSerRead(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerReadParm
	{
		uint32_t buf;
		uint32_t len;
	};
    	#pragma options align=reset
	uint32_t ret;

	SerReadParm *theSerReadParm = (SerReadParm *) (AdrOffset68k + params);
//	DebugInfo("CMagiC::AtariSerRead(buflen = %d)", theSerReadParm->len);

	ret = pTheSerial->Read(be32_to_cpu(theSerReadParm->len),
						(char *) (AdrOffset68k +  be32_to_cpu(theSerReadParm->buf)));
	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Mehrere Zeichen auf serielle Schnittstelle ausgeben
* params		Zeiger auf auszugebendes Zeichen (16 Bit)
* Rückgabe: Anzahl Zeichen
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L191 */
uint32_t CMagiC::AtariSerWrite(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerWriteParm
	{
		uint32_t buf;
		uint32_t len;
	};
    	#pragma options align=reset
	uint32_t ret;

	SerWriteParm *theSerWriteParm = (SerWriteParm *) (AdrOffset68k + params);
//	DebugInfo("CMagiC::AtariSerWrite(buflen = %d)", theSerWriteParm->len);

	ret = pTheSerial->Write(be32_to_cpu(theSerWriteParm->len),
							(char *) (AdrOffset68k +  be32_to_cpu(theSerWriteParm->buf)));
	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Status für serielle Schnittstelle
* params		Zeiger auf Struktur
* Rückgabe: 0xffffffff (kann schreiben) oder 0 (kann nicht schreiben)
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L209 */
uint32_t CMagiC::AtariSerStat(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerStatParm
	{
		uint16_t rwflag;
	};
    	#pragma options align=reset

//	DebugInfo("CMagiC::AtariSerWrite()");
	SerStatParm *theSerStatParm = (SerStatParm *) (AdrOffset68k + params);

	return((be16_to_cpu(theSerStatParm->rwflag)) ?
				(pTheSerial->WriteStatus() ? 0xffffffff : 0) :
				(pTheSerial->ReadStatus() ? 0xffffffff : 0));
}


/**********************************************************************
*
* Callback des Emulators: Ioctl für serielle Schnittstelle
* params		Zeiger auf Struktur
* Rückgabe: Fehlercode
*
* Wird vom SERIAL-Treiber für MagicMacX verwendet
* (DEV_SER.DEV), jedoch nicht im MagiC-Kernel selbst.
*.
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/tools/dev_ser/dev_sers.s#L234 */
uint32_t CMagiC::AtariSerIoctl(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerIoctlParm
	{
		uint16_t cmd;
		uint32_t parm;
	};
    	#pragma options align=reset

//	DebugInfo("CMagiC::AtariSerWrite()");
	SerIoctlParm *theSerIoctlParm = (SerIoctlParm *) (AdrOffset68k + params);

	uint32_t grp;
	uint32_t mode;
	uint32_t ret;
	bool bSet;
	unsigned long NewBaudrate, OldBaudrate;
	uint16_t flags;

	bool bXonXoff;
	bool bRtsCts;
	bool bParityEnable;
	bool bParityEven;
	unsigned int nBits;
	unsigned int nStopBits;


	DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(cmd=0x%04x, parm=0x%08x)", be16_to_cpu(theSerIoctlParm->cmd), be32_to_cpu(theSerIoctlParm->parm));
	switch(be16_to_cpu(theSerIoctlParm->cmd))
	{
		case TIOCBUFFER:
			// Inquire/Set buffer settings
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCBUFFER) -- nicht unterstützt");
			ret = (uint32_t) EINVFN;
			break;

		case TIOCCTLMAP:
			// Inquire I/O-lines and signaling capabilities
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLMAP) -- nicht unterstützt");
			ret = (uint32_t) EINVFN;
			break;

		case TIOCCTLGET:
			// Inquire I/O-lines and signals
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLGET) -- nicht unterstützt");
			ret = (uint32_t) EINVFN;
			break;

		case TIOCCTLSET:
			// Set I/O-lines and signals
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLSET) -- nicht unterstützt");
			ret = (uint32_t) EINVFN;
			break;

		case TIOCGPGRP:
			//get terminal process group
			DebugWarning("CMagiC::AtariSerIoctl() -- Fcntl(TIOCGPGRP) -- nicht unterstützt");
			ret = (uint32_t) EINVFN;
			break;

		case TIOCSPGRP:
			//set terminal process group
			grp = be32_to_cpu(*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerIoctlParm->parm))));
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCSPGRP, %d)", (uint32_t) grp);
			ret = (uint32_t) EINVFN;
			break;

		case TIOCFLUSH:
			// Leeren der seriellen Puffer
			mode = be32_to_cpu(theSerIoctlParm->parm);
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCFLUSH, %d)", mode);
			switch(mode)
			{
				// Der Sendepuffer soll komplett gesendet werden. Die Funktion kehrt 
				// erst zurück, wenn der Puffer leer ist (return E_OK, =0) oder ein 
				// systeminterner Timeout abgelaufen ist (return EDRVNR, =-2). Der 
				// Timeout wird vom System sinnvoll bestimmt.
				case 0:
					ret = pTheSerial->Drain();
					break;

				// Der Empfangspuffer wird gelöscht.
				case 1:
					ret = pTheSerial->Flush(true, false);
					break;

				// Der Sendepuffer wird gelöscht.
				case 2:
					ret = pTheSerial->Flush(false, true);
					break;
    
				// Empfangspuffer und Sendepuffer werden gelîscht.
				case 3:
					ret = pTheSerial->Flush(true, true);
					break;

				default:
					ret = (uint32_t) EINVFN;
					break;
			}
			break;

		case TIOCIBAUD:
		case TIOCOBAUD:
			// Eingabegeschwindigkeit festlegen
			NewBaudrate = be32_to_cpu(*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerIoctlParm->parm))));
			bSet = ((int) NewBaudrate != -1) && (NewBaudrate != 0);
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(%s, %ld)", (be16_to_cpu(theSerIoctlParm->cmd) == TIOCIBAUD) ? "TIOCIBAUD" : "TIOCOBAUD", NewBaudrate);

			if	(be16_to_cpu(theSerIoctlParm->cmd) == TIOCIBAUD)
				ret = pTheSerial->Config(
					bSet,						// Input-Rate ggf. ändern
					NewBaudrate,				// neue Input-Rate
					&OldBaudrate,				// alte Baud-Rate
					false,						// Output-Rate nicht ändern
					0,						// neue Output-Rate
					NULL,						// alte Output-Rate egal
					false,						// Xon/Xoff ändern
					false,
					NULL,
					false,						// Rts/Cts ändern
					false,
					NULL,
					false,						// parity enable ändern
					false,
					NULL,
					false,						// parity even ändern
					false,
					NULL,
					false,						// n Bits ändern
					0,
					NULL,
					false,						// Stopbits ändern
					0,
					NULL);
			else
				ret = pTheSerial->Config(
					false,						// Input-Rate nicht ändern
					0,						// neue Input-Rate
					NULL,						// alte Input-Rate egal
					bSet,						// Output-Rate ggf. ändern
					NewBaudrate,				// neue Output-Rate
					&OldBaudrate,				// alte Output-Rate
					false,						// Xon/Xoff ändern
					false,
					NULL,
					false,						// Rts/Cts ändern
					false,
					NULL,
					false,						// parity enable ändern
					false,
					NULL,
					false,						// parity even ändern
					false,
					NULL,
					false,						// n Bits ändern
					0,
					NULL,
					false,						// Stopbits ändern
					0,
					NULL);

			*((uint32_t *) (AdrOffset68k + be32_to_cpu(theSerIoctlParm->parm))) = cpu_to_be32(OldBaudrate);
			if	((int) ret == -1)
				ret = (uint32_t) ATARIERR_ERANGE;
			break;

		case TIOCGFLAGS:
			// Übertragungsprotokolleinstellungen erfragen

			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCGFLAGS, %d)", be32_to_cpu(theSerIoctlParm->parm));
			(void) pTheSerial->Config(
						false,						// Input-Rate nicht ändern
						0,						// neue Input-Rate
						NULL,						// alte Input-Rate egal
						false,						// Output-Rate nicht ändern
						0,						// neue Output-Rate
						NULL,						// alte Output-Rate egal
						false,						// Xon/Xoff nicht ändern
						false,						// neuer Wert
						&bXonXoff,					// alter Wert
						false,						// Rts/Cts nicht ändern
						false,
						&bRtsCts,
						false,						// parity enable nicht ändern
						false,
						&bParityEnable,
						false,						// parity even nicht ändern
						false,
						&bParityEven,
						false,						// n Bits nicht ändern
						0,
						&nBits,
						false,						// Stopbits nicht ändern
						0,
						&nStopBits);
			// Rückgabewert zusammenbauen
			flags = 0;
			if	(bXonXoff)
				flags |= 0x1000;
			if	(bRtsCts)
				flags |= 0x2000;
			if	(bParityEnable)
				flags |= (bParityEven) ? 0x4000 : 0x8000;
			if	(nStopBits == 1)
				flags |= 1;
			else
			if	(nStopBits == 2)
				flags |= 3;
			if	(nBits == 5)
				flags |= 0xc;
			else
			if	(nBits == 6)
				flags |= 0x8;
			else
			if	(nBits == 7)
				flags |= 0x4;
			*((uint16_t *) (AdrOffset68k + be32_to_cpu(theSerIoctlParm->parm))) = cpu_to_be16(flags);
			ret = (uint32_t) E_OK;
			break;

		case TIOCSFLAGS:
			// Übertragungsprotokolleinstellungen setzen
			flags = be16_to_cpu(*((uint16_t *) (AdrOffset68k + be32_to_cpu(theSerIoctlParm->parm))));
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCSFLAGS, 0x%04x)", (uint32_t) flags);
			bXonXoff = (flags & 0x1000) != 0;
			DebugInfo("CMagiC::AtariSerIoctl() -- XON/XOFF %s", (bXonXoff) ? "ein" : "aus");
			bRtsCts = (flags & 0x2000) != 0;
			DebugInfo("CMagiC::AtariSerIoctl() -- RTS/CTS %s", (bRtsCts) ? "ein" : "aus");
			bParityEnable = (flags & (0x4000+0x8000)) != 0;
			DebugInfo("CMagiC::AtariSerIoctl() -- Parität %s", (bParityEnable) ? "ein" : "aus");
			bParityEven= (flags & 0x4000) != 0;
			DebugInfo("CMagiC::AtariSerIoctl() -- Parität %s", (bParityEven) ? "gerade (even)" : "ungerade (odd)");
			nBits = 8U - ((flags & 0xc) >> 2);
			DebugInfo("CMagiC::AtariSerIoctl() -- %d Bits", nBits);
			nStopBits = flags & 3U;
			DebugInfo("CMagiC::AtariSerIoctl() -- %d Stop-Bits%s", nStopBits, (nStopBits == 0) ? " (Synchron-Modus?)" : "");
			if	((nStopBits == 0) || (nStopBits == 2))
				return((uint32_t) ATARIERR_ERANGE);
			if	(nStopBits == 3)
				nStopBits = 2;
			ret = pTheSerial->Config(
						false,						// Input-Rate nicht ändern
						0,						// neue Input-Rate
						NULL,						// alte Input-Rate egal
						false,						// Output-Rate nicht ändern
						0,						// neue Output-Rate
						NULL,						// alte Output-Rate egal
						true,						// Xon/Xoff ändern
						bXonXoff,					// neuer Wert
						NULL,						// alter Wert egal
						true,						// Rts/Cts ändern
						bRtsCts,
						NULL,
						true,						// parity enable ändern
						bParityEnable,
						NULL,
						false,						// parity even nicht ändern
						bParityEven,
						NULL,
						true,						// n Bits ändern
						nBits,
						NULL,
						true,						// Stopbits ändern
						nStopBits,
						NULL);
			if	((int) ret == -1)
				ret = (uint32_t) ATARIERR_ERANGE;
			break;

		default:
			DebugError("CMagiC::AtariSerIoctl() -- Fcntl(0x%04x -- unbekannt", be16_to_cpu(theSerIoctlParm->cmd) & 0xffff);
			ret = (uint32_t) EINVFN;
			break;
	}

	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Idle Task
* params		Zeiger auf uint32_t
* Rückgabe:
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L3391 */
uint32_t CMagiC::AtariYield(uint32_t params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct YieldParm
	{
		uint32_t num;
	};
    	#pragma options align=reset
	OSStatus err;
	MPEventFlags EventFlags;


	// zuerst testen, ob während des letzten Assembler-Befehls gerade
	// Ereignisse eingetroffen sind, die im Interrupt bearbeitet worden
	// sind. Wenn ja, hier nicht warten, sondern gleich weitermachen.

	YieldParm *theYieldParm = (YieldParm *) (AdrOffset68k + params);
	if	(be32_to_cpu(theYieldParm->num))
		return(0);

//	MPYield();

	err = OS_WaitForEvent(
				pTheMagiC->m_InterruptEventsId,
				&EventFlags,
				kDurationForever);
	if	(err)
	{
		DebugError("CMagiC::EmuThread() -- Fehler bei MPWaitForEvent");
		return(0);
	}

/*
	if	(EventFlags & EMU_EVNT_TERM)
	{
		DebugInfo("CMagiC::EmuThread() -- normaler Abbruch");
		break;	// normaler Abbruch, Thread-Ende
	}
*/
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Tastatur- und Mausdaten abholen
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2830 */
/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L2870 */
uint32_t CMagiC::AtariGetKeyboardOrMouseData(uint32_t params, unsigned char *AdrOffset68k)
{
	uint32_t ret;
	char buf[3];
#pragma unused(AdrOffset68k)

#ifdef _DEBUG_KB_CRITICAL_REGION
	DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Enter critical region m_KbCriticalRegionId");
#endif
	OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
	ret = m_pKbRead != m_pKbWrite;		// Daten im Puffer?

	// Wenn keine Taste mehr im Puffer => Maus abfragen
	if	(!ret)
	{
		// Die Maus wird erst erkannt, wenn VDI initialisiert ist
		if	(m_LineAVars)
			ret = m_MagiCMouse.GetNewPositionAndButtonState(buf);
		if	(ret)
		{
			PutKeyToBuffer((unsigned char) buf[0]);
			PutKeyToBuffer((unsigned char) buf[1]);
			PutKeyToBuffer((unsigned char) buf[2]);
		}
	}

	if	(params)
	{
		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
#endif
		// Taste wurde verarbeitet. Entspricht beim Atari dem Löschen des
		// "interrupt service bit"
//		if	(!ret)
//			Asgard68000SetIRQLine(k68000IRQLineIRQ6, k68000IRQStateClear);
		return(ret);		// ggf. weitere Interrupts
	}

	if	(!ret)
	{
		OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
		DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
#endif
		DebugError("AtariGetKeyboardOrMouseData() --- Keine Daten");
		return(0);					// kein Zeichen?
	}

	ret = *m_pKbRead++;
	if	(m_pKbRead >= m_cKeyboardOrMouseData + KEYBOARDBUFLEN)
		m_pKbRead = m_cKeyboardOrMouseData;
#if defined(_DEBUG_KBD_AND_MOUSE)
	DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() - Sende 0x%02x", ret);
#endif
	OS_ExitCriticalRegion(m_KbCriticalRegionId);
#if defined(_DEBUG_KB_CRITICAL_REGION)
	DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
#endif
	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Programmstart aus Apple-Events abholen
*
**********************************************************************/

/* called from https://github.com/th-otto/MagicMac/blob/master/kernel/bios/magcmacx/macxbios.s#L814 */
uint32_t CMagiC::MmxDaemon(uint32_t params, unsigned char *AdrOffset68k)
{
	uint32_t ret;
   	#pragma options align=packed
	struct MmxDaemonParm
	{
		uint16_t cmd;
		uint32_t parm;
	};
	unsigned char *pBuf;
    	#pragma options align=reset


//	DebugInfo("CMagiC::MmxDaemon()");
	MmxDaemonParm *theMmxDaemonParm = (MmxDaemonParm *) (AdrOffset68k + params);

	switch(be16_to_cpu(theMmxDaemonParm->cmd))
	{
		// ermittle zu startende Programme/Dateien aus AppleEvent 'odoc'
		case 1:
			OS_EnterCriticalRegion(m_AECriticalRegionId, kDurationForever);
			if	(m_iNoOfAtariFiles)
			{
				// Es liegen Anforderungen vor.
				// Zieladresse:
				pBuf = AdrOffset68k + be32_to_cpu(theMmxDaemonParm->parm);
				// von Quelladresse kopieren
				strcpy((char *) pBuf, m_szStartAtariFiles[m_iOldestAtariFile]);
				ret = E_OK;
				m_iNoOfAtariFiles--;
			}
			else
				ret = (uint32_t) EFILNF;
			OS_ExitCriticalRegion(m_AECriticalRegionId);
			break;

		// ermittle shutdown-Status
		case 2:
			ret = m_bShutdown;
			break;

		default:
			ret = EUNCMD;
			break;
	}

	return(ret);
}
