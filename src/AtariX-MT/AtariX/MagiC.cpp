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
UInt32 DebugCurrentPC;			// für Test der Bildschirmausgabe
static UInt32 WriteCounters[100];
extern void TellDebugCurrentPC(UInt32 pc);
void TellDebugCurrentPC(UInt32 pc)
{
	DebugCurrentPC = pc;
}
#endif

static CMagiC *pTheMagiC = 0;
unsigned char *OpcodeROM;		// Zeiger auf den 68k-Speicher
static UINT32 Adr68kVideo;			// Beginn Bildschirmspeicher 68k
static UINT32 Adr68kVideoEnd;			// Ende Bildschirmspeicher 68k
#ifdef _DEBUG
static UINT32 AdrOsRomStart;			// Beginn schreibgeschützter Bereich
static UINT32 AdrOsRomEnd;			// Ende schreibgeschützter Bereich
#endif
static unsigned char *HostVideoAddr;		// Beginn Bildschirmspeicher Host
//static unsigned char *HostVideo2Addr;		// Beginn Bildschirmspeicher Host (Hintergrundpuffer)
static atomic_char *p_bVideoBufChanged;
static bool bAtariVideoRamHostEndian = true;

static const char *AtariAddr2Description(UInt32 addr);

#if defined(MAGICMACX_DEBUG_SCREEN_ACCESS) || defined(PATCH_VDI_PPC)
static UInt32 p68k_OffscreenDriver = 0;
static UInt32 p68k_ScreenDriver = 0;
//#define ADDR_VDIDRVR_32BIT			0x1819c	// VDI-Treiber für "true colour" liegt hier
//#define p68k_OffscreenDriver		0x19cc0	// Offscreen-VDI-Treiber für "true colour" liegt hier
#endif

#ifdef PATCH_VDI_PPC
#include "VDI_PPC.c.h"
#endif


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

static const char *AtariAddr2Description(UInt32 addr)
{
	// Rechne ST-Adresse in TT-Adresse um

	if	((addr >= 0xff0000) && (addr < 0xffffff))
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

	if	((addr >= 0xfffffa40) && (addr < 0xfffffa54))
		return("MC68881");

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
		return(*((UINT8*) (OpcodeROM + address)));
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		return(*((UINT8*) (HostVideoAddr + (address - Adr68kVideo))));
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::ReadByte(adr = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", address, AtariAddr2Description(address), Name);
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
	UInt16 val;

#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
	{
		val = *((UINT16 *) (OpcodeROM + address));
		return(CFSwapInt16BigToHost(val));
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		val = *((UINT16 *) (HostVideoAddr + (address - Adr68kVideo)));
		if (bAtariVideoRamHostEndian)
			return val;		// x86 has bgr instead of rgb
		else
			return(CFSwapInt16BigToHost(val));
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::ReadWord(adr = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", address, AtariAddr2Description(address), Name);
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
	UInt32 val;

#if !COUNT_CYCLES
	if	(address < Adr68kVideo)
	{
		val = *((UINT32*) (OpcodeROM + address));
		return(CFSwapInt32BigToHost(val));
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		val = *((UINT32 *) (HostVideoAddr + (address - Adr68kVideo)));
		if (bAtariVideoRamHostEndian)
			return val;		// x86 has bgr instead of rgb
		else
			return(CFSwapInt32BigToHost(val));
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";
		DebugError("CMagiC::ReadLong(adr = 0x%08lx) --- Busfehler bei Prozeß %s", address, Name);
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
		UINT32 act_pd;
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
		*((UINT8 *) (OpcodeROM + address)) = (UINT8) value;
	}
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		address -= Adr68kVideo;
		*((UINT8 *) (HostVideoAddr + address)) = (UINT8) value;
		//*((UINT8*) (HostVideo2Addr + address)) = (UINT8) value;
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
		//usleep(100000);
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteByte(adr = 0x%08lx, dat = 0x%02hx) --- Busfehler (%s) durch Prozeß %s", address, (UInt8) value, AtariAddr2Description(address), Name);

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
		UINT32 act_pd;
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
		*((UINT16 *) (OpcodeROM + address)) = (UINT16) CFSwapInt16HostToBig(value);
	else
#endif
	if	(address < Adr68kVideoEnd)
	{
		address -= Adr68kVideo;
		if (bAtariVideoRamHostEndian)
			*((UINT16 *) (HostVideoAddr + address)) = (UINT16) value;		// x86 has bgr instead of rgb
		else
			*((UINT16 *) (HostVideoAddr + address)) = (UINT16) CFSwapInt16HostToBig(value);

// //		*((UINT16*) (HostVideo2Addr + address)) = (UINT16) CFSwapInt16HostToBig(value);
//		*((UINT16 *) (HostVideo2Addr + address)) = (UINT16) value;	// x86 has bgr instead of rgb
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteWord(adr = 0x%08lx, dat = 0x%04hx) --- Busfehler(%s) durch Prozeß %s", address, (UInt16) value, AtariAddr2Description(address), Name);
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
		UINT32 act_pd;
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
		*((UINT32 *) (OpcodeROM + address)) = (UINT32) CFSwapInt32HostToBig(value);
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
			CDebug::DebugError("#### falscher Bildspeicherzugriff bei PC = 0x%08x", DebugCurrentPC);
		}

#endif
		address -= Adr68kVideo;
		if (bAtariVideoRamHostEndian)
			*((UINT32 *) (HostVideoAddr + address)) = value;		// x86 has brg instead of rgb
		else
			*((UINT32 *) (HostVideoAddr + address)) = CFSwapInt32HostToBig(value);

// //		*((UINT32*) (HostVideo2Addr + address)) = value;		// x86 has brg instead of rgb
//		*((UINT32 *) (HostVideo2Addr + address)) = CFSwapInt32HostToBig(value);
		(void) atomic_exchange(p_bVideoBufChanged, 1);
		//DebugInfo("vchg");
	}
	else
	{
		const char *Name;
		UINT32 act_pd;
		CMagiC::GetActAtariPrg(&Name, &act_pd);
		if	(!Name)
			Name = "<unknown>";

		DebugError("CMagiC::WriteLong(adr = 0x%08lx, dat = 0x%08lx) --- Busfehler (%s) durch Prozeß %s", address, value, AtariAddr2Description(address), Name);
		pTheMagiC->SendBusError(address, "write long");
	}
}


// statische Variablen

UInt32 CMagiC::s_LastPrinterAccess = 0;

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
	FSIORefNum f;
	bool bf = false;
	unsigned long len, codlen;
	ExeHeader exehead;
	BasePage *bp;
	unsigned char *relp;
	unsigned char relb;
	unsigned long *tp;
	unsigned char *tpaStart, *relBuf = NULL, *reloff, *tbase, *bbase;
	unsigned long loff, tpaSize;
	long Fpos;
	SInt64 FileSize;
	unsigned long RelocBufSize;
	FSRef fs;
	HFSUniStr255 dataForkName;
	ByteCount bytes_read;


	DebugInfo("CMagiC::LoadReloc()");

	if (!CFURLGetFSRef(fileUrl, &fs))
	{
		err = openErr;
	}

	if (!err)
	{
		err = FSGetDataForkName(&dataForkName);
	}

	if (!err)
	{
		err = FSOpenFork(&fs, dataForkName.length, dataForkName.unicode, fsRdPerm, &f);
	}

	if	(err)
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

	bf = true;		// Datei geöffnet
	err = FSGetForkSize(f, &FileSize);
	assert(!err);

	len = sizeof(ExeHeader);
	// PRG-Header einlesen
	err = FSReadFork(f, fsAtMark, 0, len, &exehead, &bytes_read);
	if (err)
	{
		readerr:
		DebugError("CMagiC::LoadReloc() - Lesefehler");
		goto exitReloc;
	}
	if (bytes_read != len)
	{
		DebugError("CMagiC::LoadReloc() - Datei zu kurz");
		goto exitReloc;
	}

	DebugInfo("CMagiC::LoadReloc() - Länge TEXT = %ld", CFSwapInt32BigToHost(exehead.tlen));
	DebugInfo("CMagiC::LoadReloc() - Länge DATA = %ld", CFSwapInt32BigToHost(exehead.dlen));
	DebugInfo("CMagiC::LoadReloc() - Länge BSS = %ld", CFSwapInt32BigToHost(exehead.blen));

	codlen = CFSwapInt32BigToHost(exehead.tlen) + CFSwapInt32BigToHost(exehead.dlen);
	if	(CFSwapInt32BigToHost(exehead.blen) & 1)
	{
	//	exehead.blen++;		// BSS-Segment auf gerade Länge
		exehead.blen = CFSwapInt32HostToBig(CFSwapInt32BigToHost(exehead.blen) + 1);
	}
	tpaSize = sizeof(BasePage) + codlen + CFSwapInt32BigToHost(exehead.blen) + stackSize;

	DebugInfo("CMagiC::LoadReloc() - result. Gesamtlänge inkl. Basepage und Stack = 0x%08x (%ld)", tpaSize, tpaSize);

	if	(tpaSize > m_RAM68ksize)
	{
		DebugError("CMagiC::LoadReloc() - Zuwenig Speicher");
		err = memFullErr;
		goto exitReloc;
	}


// hier basepage auf durch 4 teilbare Adresse!

	if	(reladdr < 0)
		reladdr = (long) (m_RAM68ksize - ((tpaSize + 2) & ~3));
	if	(reladdr + tpaSize > m_RAM68ksize)
	{
		DebugError("CMagiC::LoadReloc() - Ungültige Ladeadresse");
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
	bp->p_lowtpa = (void *) CFSwapInt32HostToBig(tpaStart - m_RAM68k);
	bp->p_hitpa = (void *) CFSwapInt32HostToBig(tpaStart - m_RAM68k + tpaSize);
	bp->p_tbase = (void *) CFSwapInt32HostToBig(tbase - m_RAM68k);
	bp->p_tlen  = exehead.tlen;
	bp->p_dbase = (void *) CFSwapInt32HostToBig(tbase - m_RAM68k + CFSwapInt32BigToHost(exehead.tlen));
	bp->p_dlen  = exehead.dlen;
	bp->p_bbase = (void *) CFSwapInt32HostToBig(bbase - m_RAM68k);
	bp->p_blen  = exehead.blen;
	bp->p_dta   = (void *) CFSwapInt32HostToBig(bp->p_cmdline - m_RAM68k);
	bp->p_parent= NULL;

	DebugInfo("CMagiC::LoadReloc() - Startadresse Atari = 0x%08lx (host)", m_RAM68k);
	DebugInfo("CMagiC::LoadReloc() - Speichergröße Atari = 0x%08lx (= %lu kBytes)", m_RAM68ksize, m_RAM68ksize >> 10);
	DebugInfo("CMagiC::LoadReloc() - Ladeadresse des Systems (TEXT) = 0x%08lx (68k)", CFSwapInt32BigToHost((uint32_t) (bp->p_tbase)));

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
	err = FSReadFork(f, fsAtMark, 0, codlen, tbase, &bytes_read);
	if	(err)
		goto readerr;
	if	(codlen != bytes_read)
		goto readerr;

	if	(!CFSwapInt16BigToHost(exehead.relmod))	// müssen relozieren
	{
		// Seek zur Reloc-Tabelle
		Fpos = (long) (CFSwapInt32BigToHost(exehead.slen) + codlen + sizeof(exehead));
		err = FSSetForkPosition(f, fsFromStart, Fpos);
		if	(err)
			goto readerr;
		len = 4;
		err = FSReadFork(f, fsAtMark, 0, len, &loff, &bytes_read);
		if	(err)
			goto readerr;
		if	(len != bytes_read)
			goto readerr;

		loff = CFSwapInt32BigToHost(loff);

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
			tp = (unsigned long *) (reloff + loff);

			//*tp += (long) (reloff - m_RAM68k);
			*tp = CFSwapInt32HostToBig((long) (reloff - m_RAM68k) + CFSwapInt32BigToHost(*tp));

			// Reloc-Tabelle in einem Rutsch einlesen
			err = FSReadFork(f, fsAtMark, 0, RelocBufSize, relBuf, &bytes_read);
			if	(err)
				goto readerr;
			if	(RelocBufSize != bytes_read)
				goto readerr;
			relBuf[RelocBufSize] = '\0';	// Sicherheitshalber Ende-Zeichen

			relp = relBuf;
			while(*relp)
			{
				relb = *relp++;
				if	(relb == 1)
					tp = (unsigned long *) ((char *) tp + 254);
				else
				{
					tp = (unsigned long*) ((char *) tp + (unsigned char) relb);

					//*tp += (long) (reloff - m_RAM68k);
					*tp = CFSwapInt32HostToBig((long) (reloff - m_RAM68k) + CFSwapInt32BigToHost(*tp));
				}
			}
		}
		else
		{
			DebugWarning("CMagiC::LoadReloc() - Keine Relokation");
		}
	}

	memset (bbase, 0, CFSwapInt32BigToHost(exehead.blen));	// BSS löschen

exitReloc:
	if	(err)
		*basePage = NULL;
	else
		*basePage = bp;

	if	(relBuf)
		free(relBuf);

	if	(bf)
		FSCloseFork(f);

	return(err);
}


/**********************************************************************
*
* (INTERN) Initialisierung von Atari68kData.m_VDISetupData
*
**********************************************************************/

void CMagiC::Init_CookieData(MgMxCookieData *pCookieData)
{
	pCookieData->mgmx_magic     = CFSwapInt32HostToBig('MgMx');
	pCookieData->mgmx_version   = CFSwapInt32HostToBig(CGlobals::s_ProgramVersion.majorRev);
	pCookieData->mgmx_len       = CFSwapInt32HostToBig(sizeof(MgMxCookieData));
	pCookieData->mgmx_xcmd      = CFSwapInt32HostToBig(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_xcmd_exec = CFSwapInt32HostToBig(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_internal  = CFSwapInt32HostToBig(0);		// wird vom Kernel gesetzt
	pCookieData->mgmx_daemon    = CFSwapInt32HostToBig(0);		// wird vom Kernel gesetzt
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

	thePixMap->baseAddr      = (UINT8 *) CFSwapInt32HostToBig((UInt32) thePixMap->baseAddr);
	thePixMap->rowBytes      = CFSwapInt16HostToBig(thePixMap->rowBytes);
	thePixMap->bounds_top    = CFSwapInt16HostToBig(thePixMap->bounds_top);
	thePixMap->bounds_left   = CFSwapInt16HostToBig(thePixMap->bounds_left);
	thePixMap->bounds_bottom = CFSwapInt16HostToBig(thePixMap->bounds_bottom);
	thePixMap->bounds_right  = CFSwapInt16HostToBig(thePixMap->bounds_right);
	thePixMap->pmVersion     = CFSwapInt16HostToBig(thePixMap->pmVersion);
	thePixMap->packType      = CFSwapInt16HostToBig(thePixMap->packType);
	thePixMap->packSize      = CFSwapInt32HostToBig(thePixMap->packSize);
	thePixMap->hRes          = CFSwapInt32HostToBig(thePixMap->hRes);
	thePixMap->vRes          = CFSwapInt32HostToBig(thePixMap->vRes);
	thePixMap->pixelType     = CFSwapInt16HostToBig(thePixMap->pixelType);
	thePixMap->pixelSize     = CFSwapInt16HostToBig(thePixMap->pixelSize);
	thePixMap->cmpCount      = CFSwapInt16HostToBig(thePixMap->cmpCount);
	thePixMap->cmpSize       = CFSwapInt16HostToBig(thePixMap->cmpSize);
	thePixMap->planeBytes    = CFSwapInt32HostToBig(thePixMap->planeBytes);
/*
	if (thePixMap->pixelFormat == k32BGRAPixelFormat)
	{
		DebugInfo("PixmapToBigEndian() -- k32BGRAPixelFormat => k32ARGBPixelFormat");
		thePixMap->pixelFormat = CFSwapInt32HostToBig(k32ARGBPixelFormat);
	}
*/
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
	UINT32 AtariMemtop;		// Ende Atari-Benutzerspeicher
	UINT32 chksum;
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
	DebugInfo("68k-Videospeicher beginnt bei 68k-Adresse 0x%08x und ist %u Bytes groß.", Adr68kVideo, m_Video68ksize);
	Adr68kVideoEnd = Adr68kVideo + m_Video68ksize;
	m_pFgBuffer = (unsigned char *) m_pMagiCScreen->m_PixMap.baseAddr;

	UpdateAtariDoubleBuffer();

	// Atari-Systemvariablen setzen

	*((UINT32 *)(m_RAM68k+phystop)) = CFSwapInt32HostToBig(Adr68kVideoEnd);
	*((UINT32 *)(m_RAM68k+_v_bas_ad)) = CFSwapInt32HostToBig(m_RAM68ksize);
	AtariMemtop = ((UINT32) ((unsigned char *) m_BasePage - m_RAM68k)) - sizeof(Atari68kData);
	*((UINT32 *)(m_RAM68k+_memtop)) = CFSwapInt32HostToBig(AtariMemtop);
	*((UINT16 *)(m_RAM68k+sshiftmd)) = CFSwapInt16HostToBig(2);		// ST high (640*400*2)
	*((UINT16 *)(m_RAM68k+_cmdload)) = CFSwapInt16HostToBig(0);		// AES booten
	*((UINT16 *)(m_RAM68k+_nflops)) = CFSwapInt16HostToBig(0);		// keine Floppies

	// Atari-68k-Daten setzen

	pAtari68kData = (Atari68kData *) (m_RAM68k + AtariMemtop);
	pAtari68kData->m_PixMap = m_pMagiCScreen->m_PixMap;
	// left und top scheinen nicht abgefragt zu werden, nur right und bottom
	pAtari68kData->m_PixMap.baseAddr = (UINT8 *) Adr68kVideo;		// virtuelle 68k-Adresse

	#if !defined(__BIG_ENDIAN__)
	PixmapToBigEndian(&pAtari68kData->m_PixMap);
	#endif

	DebugInfo("CMagiC::Init() - Adresse Basepage des Systems = 0x%08lx (68k)", AtariMemtop + sizeof(Atari68kData));
	DebugInfo("CMagiC::Init() - Adresse Atari68kData = 0x%08lx (68k)", AtariMemtop);

	// neue Daten

	Init_CookieData(&pAtari68kData->m_CookieData);

	// Alle Einträge der Übergabestruktur füllen

	pMacXSysHdr = (MacXSysHdr *) (m_BasePage + 1);		// Zeiger hinter Basepage

	if	(CFSwapInt32BigToHost(pMacXSysHdr->MacSys_magic) != 'MagC')
	{
		DebugError("CMagiC::Init() - Falsches magic");
		goto err_inv_os;
	}

	assert(sizeof(CMagiC_CPPCCallback) == 16);

	if	(CFSwapInt32BigToHost(pMacXSysHdr->MacSys_len) != sizeof(*pMacXSysHdr))
	{
		DebugError("CMagiC::Init() - Strukturlänge stimmt nicht (Header: %u Bytes, Soll: %u Bytes)", pMacXSysHdr->MacSys_len, sizeof(*pMacXSysHdr));
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

	pMacXSysHdr->MacSys_verMac = CFSwapInt32HostToBig(10);
	pMacXSysHdr->MacSys_cpu = CFSwapInt16HostToBig(20);		// 68020
	pMacXSysHdr->MacSys_fpu = CFSwapInt16HostToBig(0);		// keine FPU
	pMacXSysHdr->MacSys_init.m_Callback = &CMagiC::AtariInit;
	pMacXSysHdr->MacSys_init.m_thisptr = this;
	pMacXSysHdr->MacSys_biosinit.m_Callback = &CMagiC::AtariBIOSInit;
	pMacXSysHdr->MacSys_biosinit.m_thisptr = this;
	pMacXSysHdr->MacSys_VdiInit.m_Callback = &CMagiC::AtariVdiInit;
	pMacXSysHdr->MacSys_VdiInit.m_thisptr = this;
	pMacXSysHdr->MacSys_Exec68k.m_Callback = &CMagiC::AtariExec68k;
	pMacXSysHdr->MacSys_Exec68k.m_thisptr = this;
	pMacXSysHdr->MacSys_pixmap = CFSwapInt32HostToBig(((UINT32) &pAtari68kData->m_PixMap) - (UINT32) m_RAM68k);
	pMacXSysHdr->MacSys_pMMXCookie = CFSwapInt32HostToBig(((UINT32) &pAtari68kData->m_CookieData) - (UINT32) m_RAM68k);
	pMacXSysHdr->MacSys_Xcmd.m_Callback = &CXCmd::Command;
	pMacXSysHdr->MacSys_Xcmd.m_thisptr = pXCmd;
	pMacXSysHdr->MacSys_PPCAddr = (void *) CFSwapInt32HostToBig((UInt32) m_RAM68k);
	pMacXSysHdr->MacSys_VideoAddr = (void *) CFSwapInt32HostToBig((UInt32) m_pMagiCScreen->m_PixMap.baseAddr);
	pMacXSysHdr->MacSys_gettime = (void *) AtariGettime;
	pMacXSysHdr->MacSys_settime = (void *) AtariSettime;
	pMacXSysHdr->MacSys_Setpalette = (void *) AtariSetpalette;
	pMacXSysHdr->MacSys_Setcolor = (void *) AtariSetcolor;
	pMacXSysHdr->MacSys_VsetRGB = (void *) AtariVsetRGB;
	pMacXSysHdr->MacSys_VgetRGB = (void *) AtariVgetRGB;
	pMacXSysHdr->MacSys_syshalt = (void *) AtariSysHalt;
	pMacXSysHdr->MacSys_syserr = (void *) AtariSysErr;
	pMacXSysHdr->MacSys_coldboot = (void *) AtariColdBoot;
	pMacXSysHdr->MacSys_exit = (void *) AtariExit;
	pMacXSysHdr->MacSys_debugout = (void *) AtariDebugOut;
	pMacXSysHdr->MacSys_error = (void *) AtariError;
	pMacXSysHdr->MacSys_prtos = (void *) AtariPrtOs;
	pMacXSysHdr->MacSys_prtin = (void *) AtariPrtIn;
	pMacXSysHdr->MacSys_prtout = (void *) AtariPrtOut;
	pMacXSysHdr->MacSys_prtouts = (void *) AtariPrtOutS;
	pMacXSysHdr->MacSys_serconf = (void *) AtariSerConf;
	pMacXSysHdr->MacSys_seris = (void *) AtariSerIs;
	pMacXSysHdr->MacSys_seros = (void *) AtariSerOs;
	pMacXSysHdr->MacSys_serin = (void *) AtariSerIn;
	pMacXSysHdr->MacSys_serout = (void *) AtariSerOut;
	pMacXSysHdr->MacSys_SerOpen = (void *) AtariSerOpen;
	pMacXSysHdr->MacSys_SerClose = (void *) AtariSerClose;
	pMacXSysHdr->MacSys_SerRead = (void *) AtariSerRead;
	pMacXSysHdr->MacSys_SerWrite = (void *) AtariSerWrite;
	pMacXSysHdr->MacSys_SerStat = (void *) AtariSerStat;
	pMacXSysHdr->MacSys_SerIoctl = (void *) AtariSerIoctl;
	pMacXSysHdr->MacSys_GetKeybOrMouse.m_Callback = &CMagiC::AtariGetKeyboardOrMouseData;
	pMacXSysHdr->MacSys_GetKeybOrMouse.m_thisptr = this;
	pMacXSysHdr->MacSys_dos_macfn = (void *) AtariDOSFn;
	pMacXSysHdr->MacSys_xfs.m_Callback = &CMacXFS::XFSFunctions;
	pMacXSysHdr->MacSys_xfs.m_thisptr = &m_MacXFS;
	pMacXSysHdr->MacSys_xfs_dev.m_Callback = &CMacXFS::XFSDevFunctions;
	pMacXSysHdr->MacSys_xfs_dev.m_thisptr = &m_MacXFS;
	pMacXSysHdr->MacSys_drv2devcode.m_Callback = &CMacXFS::Drv2DevCode;
	pMacXSysHdr->MacSys_drv2devcode.m_thisptr = &m_MacXFS;
	pMacXSysHdr->MacSys_rawdrvr.m_Callback = &CMacXFS::RawDrvr;
	pMacXSysHdr->MacSys_rawdrvr.m_thisptr = &m_MacXFS;
	pMacXSysHdr->MacSys_Daemon.m_Callback = &CMagiC::MmxDaemon;
	pMacXSysHdr->MacSys_Daemon.m_thisptr = this;
	pMacXSysHdr->MacSys_Yield = (void *) AtariYield;

	// ssp nach Reset
	*((UINT32 *)(m_RAM68k + 0)) = CFSwapInt32HostToBig(512*1024);		// Stack auf 512k
	// pc nach Reset
	*((UINT32 *)(m_RAM68k + 4)) = pMacXSysHdr->MacSys_syshdr;

	// TOS-SYSHDR bestimmen

	pSysHdr = (SYSHDR *) (m_RAM68k + CFSwapInt32BigToHost(pMacXSysHdr->MacSys_syshdr));

	// Adresse für kbshift, kbrepeat und act_pd berechnen

	m_AtariKbData = m_RAM68k + CFSwapInt32BigToHost(pSysHdr->kbshift);
	m_pAtariActPd = (UINT32 *) (m_RAM68k + CFSwapInt32BigToHost(pSysHdr->_run));

	// Andere Atari-Strukturen

	m_pAtariActAppl = (UINT32 *) (m_RAM68k + CFSwapInt32BigToHost(pMacXSysHdr->MacSys_act_appl));

	// Prüfsumme für das System berechnen

	chksum = 0;
	UINT32 *fromptr = (UINT32 *) (m_RAM68k + CFSwapInt32BigToHost(pMacXSysHdr->MacSys_syshdr));
	UINT32 *toptr = (UINT32 *) (m_RAM68k + CFSwapInt32BigToHost((UINT32) m_BasePage->p_tbase) + CFSwapInt32BigToHost(m_BasePage->p_tlen) + CFSwapInt32BigToHost(m_BasePage->p_dlen));
#ifdef _DEBUG
//	AdrOsRomStart = CFSwapInt32BigToHost(pMacXSysHdr->MacSys_syshdr);			// Beginn schreibgeschützter Bereich
	AdrOsRomStart = CFSwapInt32BigToHost((UINT32) m_BasePage->p_tbase);		// Beginn schreibgeschützter Bereich
	AdrOsRomEnd = CFSwapInt32BigToHost((UINT32) m_BasePage->p_tbase) + CFSwapInt32BigToHost(m_BasePage->p_tlen) + CFSwapInt32BigToHost(m_BasePage->p_dlen);	// Ende schreibgeschützter Bereich
#endif
	do
	{
		chksum += CFSwapInt32HostToBig(*fromptr++);
	}
	while(fromptr < toptr);

	*((UINT32 *)(m_RAM68k + os_chksum)) = CFSwapInt32HostToBig(chksum);

	// dump Atari

//	DumpAtariMem("AtariMemAfterInit.data");

	// Adreßüberprüfung fürs XFS

	m_MacXFS.Set68kAdressRange(m_RAM68ksize);

	// Laufwerk C: machen

	*((UINT32 *)(m_RAM68k + _drvbits)) = CFSwapInt32HostToBig(0);		// noch keine Laufwerke
	m_MacXFS.SetXFSDrive(
					'C'-'A',							// drvnum
					CMacXFS::MacDir,					// drvType
					CGlobals::s_rootfsUrl,				// path
					(Globals.s_Preferences.m_drvFlags['C'-'A'] & 2) ? false : true,	// lange Dateinamen
					(Globals.s_Preferences.m_drvFlags['C'-'A'] & 1) ? true : false,	// umgekehrte Verzeichnis-Reihenfolge (Problem bei OS X 10.2!)
					m_RAM68k);
	*((UINT16 *)(m_RAM68k + _bootdev)) = CFSwapInt16HostToBig('C'-'A');	// Boot-Laufwerk C:

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
	DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideoAddr =0x%08x", m_pFgBuffer);
	HostVideoAddr = m_pFgBuffer;

/*
	if	(m_pBgBuffer)
	{
		DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideo2Addr =0x%08x", m_pBgBuffer);
		HostVideo2Addr = m_pBgBuffer;
	}
	else
	{
		DebugInfo("CMagiC::UpdateAtariDoubleBuffer() --- HostVideo2Addr =0x%08x", m_pFgBuffer);
		HostVideo2Addr = m_pFgBuffer;
	}
*/
}


/**********************************************************************
*
* (STATISCH) gibt drvbits zurück
*
**********************************************************************/
/*
UInt32 CMagiC::GetAtariDrvBits(void)
{
	*((UINT32 *)(pTheMagiC->m_RAM68k + _drvbits)) = CFSwapInt32HostToBig(0);		// noch keine Laufwerke

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

void CMagiC::GetActAtariPrg(const char **pName, UINT32 *pact_pd)
{
	UINT32 pact_appl;
	UINT32 pprocdata;
	MagiC_PD *pMagiCPd;
	MagiC_ProcInfo *pMagiCProcInfo;
	MagiC_APP *pMagiCApp;


	*pact_pd = CFSwapInt32BigToHost(*pTheMagiC->m_pAtariActPd);
	if	((*pact_pd != 0) && (*pact_pd < pTheMagiC->m_RAM68ksize))
	{
		pMagiCPd = (MagiC_PD *) (OpcodeROM + *pact_pd);
		pprocdata = CFSwapInt32BigToHost(pMagiCPd->p_procdata);
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

	pact_appl = CFSwapInt32BigToHost(*pTheMagiC->m_pAtariActAppl);
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
		CDebug::DebugInfo("### VideoRamWriteCounter(%2d) = %d", i, WriteCounters[i]);
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

//		CDebug::DebugInfo("CMagiC::EmuThread() -- Starte 68k-Emulator");
		m_bWaitEmulatorForIRQCallback = false;
#if defined(USE_ASGARD_PPC_68K_EMU)
		Asgard68000Execute();
#else
		m68k_execute();
#endif
//		CDebug::DebugInfo("CMagiC::EmuThread() --- %d 68k-Zyklen", CyclesExecuted);

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
//				CDebug::DebugInfo("CMagiC::Exec() --- Interrupt Pending => %d 68k-Zyklen", CyclesExecuted);
			}
			while(m_bInterruptPending);
		}

		// aufgelaufene Maus-Interrupts bearbeiten

		if	(m_bInterruptMouseKeyboardPending)
		{
#ifdef _DEBUG_KB_CRITICAL_REGION
			CDebug::DebugInfo("CMagiC::EmuThread() --- Enter critical region m_KbCriticalRegionId");
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

/*
			errl = MPResetEvent(			// kein "pending kb interrupt"
					m_InterruptEventsId,
					EMU_INTPENDING_KBMOUSE);
*/
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			CDebug::DebugInfo("CMagiC::EmuThread() --- Exited critical region m_KbCriticalRegionId");
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
//				CDebug::DebugInfo("CMagiC::EmuThread() --- m_bInterrupt200HzPending => %d 68k-Zyklen", CyclesExecuted);
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
//				CDebug::DebugInfo("CMagiC::Exec() --- m_bInterruptVBLPending => %d 68k-Zyklen", CyclesExecuted);
			}
		}

		// ggf. Druckdatei abschließen

		if	(*((UInt32 *)(m_RAM68k+_hz_200)) - s_LastPrinterAccess > 200 * 10)
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
	CDebug::DebugInfo("CMagiC::PutKeyToBuffer() --- Enter critical region m_KbCriticalRegionId");
#endif
	OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
	*m_pKbWrite++ = key;
	if	(m_pKbWrite >= m_cKeyboardOrMouseData + KEYBOARDBUFLEN)
		m_pKbWrite = m_cKeyboardOrMouseData;
	OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
	CDebug::DebugInfo("CMagiC::PutKeyToBuffer() --- Exited critical region m_KbCriticalRegionId");
#endif
}


/**********************************************************************
*
* Busfehler melden (wird im Emulator-Thread aufgerufen).
*
* <addr> ist im Host-Format, d.h. "little endian" auf x86
*
**********************************************************************/

void CMagiC::SendBusError(UInt32 addr, const char *AccessMode)
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

/*
int CMagiC::SendKeyboard(UInt32 message, bool KeyUp)
{
	unsigned char val;


#ifdef _DEBUG_NO_ATARI_KB_INTERRUPTS
	return(0);
#endif

	if (m_bEmulatorIsRunning)
	{
	//	CDebug::DebugInfo("CMagiC::SendKeyboard() --- message == %08x, KeyUp == %d", message, (int) KeyUp);
		if	(Globals.s_Preferences.m_KeyCodeForRightMouseButton)
		{
			if	((message >> 8) == Globals.s_Preferences.m_KeyCodeForRightMouseButton)
			{
				// Emulation der rechten Maustaste
				return(SendMouseButton( 1, !KeyUp ));
			}
		}

	#ifdef _DEBUG_KB_CRITICAL_REGION
		CDebug::DebugInfo("CMagiC::SendKeyboard() --- Enter critical region m_KbCriticalRegionId");
	#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		if	(GetKbBufferFree() < 1)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
	#ifdef _DEBUG_KB_CRITICAL_REGION
			CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
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
			CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
	#endif
	}

	return(0);	// OK
}
*/


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
		//	CDebug::DebugInfo("CMagiC::SendKeyboard() --- message == %08x, KeyUp == %d", message, (int) KeyUp);
		
#ifdef _DEBUG_KB_CRITICAL_REGION
		CDebug::DebugInfo("CMagiC::SendKeyboard() --- Enter critical region m_KbCriticalRegionId");
#endif
		OS_EnterCriticalRegion(m_KbCriticalRegionId, kDurationForever);
		if	(GetKbBufferFree() < 1)
		{
			OS_ExitCriticalRegion(m_KbCriticalRegionId);
#ifdef _DEBUG_KB_CRITICAL_REGION
			CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
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
			CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendKeyboard() --- Exited critical region m_KbCriticalRegionId");
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

int CMagiC::SendKeyboardShift( UInt32 modifiers )
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
	/*
		if	((!Globals.s_Preferences.m_KeyCodeForRightMouseButton) &&
			 (m_CurrModifierKeys & cmdKey) != (modifiers & cmdKey))
		{
			// linken Mausknopf immer loslassen
			SendMouseButton(1, false);
		}
	*/
		m_CurrModifierKeys = modifiers;
	#ifdef _DEBUG_KB_CRITICAL_REGION
		CDebug::DebugInfo("CMagiC::SendKeyboardShift() --- Enter critical region m_KbCriticalRegionId");
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
				CDebug::DebugInfo("CMagiC::SendKeyboardShift() --- Exited critical region m_KbCriticalRegionId");
	#endif
				DebugError("CMagiC::SendKeyboardShift() --- Tastenpuffer ist voll");
				return(1);
			}

	//		CDebug::DebugInfo("CMagiC::SendKeyboardShift() --- val == 0x%04x", (int) val);
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
		CDebug::DebugInfo("CMagiC::SendKeyboardShift() --- Exited critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendMousePosition() --- Enter critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendMousePosition() --- Exited critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendMouseButton() --- Enter critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::SendMouseButton() --- Exited critical region m_KbCriticalRegionId");
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
		/*
		 is of no use, because guest calls: "jsr v_clswk" to close VDI, no more redraws!

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
		 */

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

UINT32 CMagiC::AtariInit(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariBIOSInit(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariVdiInit(UINT32 params, unsigned char *AdrOffset68k)
{
//#pragma unused(params)
//#pragma unused(AdrOffset68k)
	Point PtAtariMousePos;
	UInt16 *p_linea_BYTES_LIN;


	m_LineAVars = AdrOffset68k + params;
	// Aktuelle Mausposition: Bildschirmmitte
	PtAtariMousePos.h = (short) ((m_pMagiCScreen->m_PixMap.bounds_right - m_pMagiCScreen->m_PixMap.bounds_left) >> 1);
	PtAtariMousePos.v = (short) ((m_pMagiCScreen->m_PixMap.bounds_bottom - m_pMagiCScreen->m_PixMap.bounds_top) >> 1);
	m_MagiCMouse.Init(m_LineAVars, PtAtariMousePos);

	// Umgehe Fehler in Behnes MagiC-VDI. Bei Bildbreiten von 2034 Pixeln und true colour werden
	// fälschlicherweise 0 Bytes pro Bildschirmzeile berechnet. Bei größeren Bildbreiten werden
	// andere, ebenfalls fehlerhafte Werte berechnet.

	p_linea_BYTES_LIN = (UInt16 *) (m_LineAVars - 2);

#ifdef PATCH_VDI_PPC
	if	((CGlobals::s_Preferences.m_bPPC_VDI_Patch) &&
		 (CGlobals::s_PhysicalPixelSize == 32) &&
		 (CGlobals::s_pixelSize == 32) &&
		 (CGlobals::s_pixelSize2 == 32))
	{
		DebugInfo("CMagiC::AtariVdiInit() --- PPC");
//		DebugInfo("CMagiC::AtariVdiInit() --- (LINEA-2) = %u", *((UInt16 *) (m_LineAVars - 2)));
// Hier die Atari-Bildschirmbreite in Bytes eintragen, Behnes VDI kriegt hie ab 2034 Pixel Bildbreite
// immer Null raus, das führt zu Schrott.
//		*((UInt16 *) (m_LineAVars - 2)) = CFSwapInt16HostToBig(8136);	// 2034 * 4
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

UINT32 CMagiC::AtariExec68k(UINT32 params, unsigned char *AdrOffset68k)
{
#pragma unused(AdrOffset68k)
	char Old68kContext[128];
	UINT32 ret;
   	#pragma options align=packed
	struct New68Context
	{
		UINT32 regPC;
		UINT32 regSP;
		UINT32 arg;
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
	m68k_set_reg(M68K_REG_PC, CFSwapInt32BigToHost(pNew68Context->regPC));
	m68k_set_reg(M68K_REG_SP, CFSwapInt32BigToHost(pNew68Context->regSP));
	m68k_set_reg(M68K_REG_A0, CFSwapInt32BigToHost(pNew68Context->arg));
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

UINT32 CMagiC::AtariDOSFn(UINT32 params, unsigned char *AdrOffset68k)
{
  	#pragma options align=packed
	struct AtariDOSFnParm
	{
		UInt16 dos_fnr;
		UInt32 parms;
	};
   	#pragma options align=reset

#ifdef _DEBUG
    AtariDOSFnParm *theAtariDOSFnParm = (AtariDOSFnParm *) (AdrOffset68k + params);
#endif
    DebugInfo("CMagiC::AtariDOSFn(fn = 0x%x)", CFSwapInt16BigToHost(theAtariDOSFnParm->dos_fnr));
	return((UINT32) EINVFN);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Gettime
*
**********************************************************************/

UINT32 CMagiC::AtariGettime(UINT32 params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
/*
	DateTimeRec dtr;
	GetTime (&dtr);
	return (dtr.second>>1) + (dtr.minute<<5) + ((unsigned short)dtr.hour<<11) + (((UINT32)dtr.day)<<16) + (((UINT32)dtr.month)<<21) + ((((UINT32)dtr.year)-1980)<<25);
*/

	CFAbsoluteTime at = CFAbsoluteTimeGetCurrent();
	CFTimeZoneRef tz = CFTimeZoneCopyDefault();
	CFGregorianDate d = CFAbsoluteTimeGetGregorianDate(at, tz);
	CFRelease(tz);
	return (((unsigned) d.second) >> 1) +
		   (d.minute << 5) +
		   ((unsigned short) d.hour << 11) +
		   (((UINT32) d.day) << 16) +
		   (((UINT32) d.month) << 21) +
		   ((((UINT32) d.year) - 1980) << 25);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Settime
*
**********************************************************************/

UINT32 CMagiC::AtariSettime(UINT32 params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	UINT32 time;
	DateTimeRec dtr;


	time = CFSwapInt32BigToHost(*((UINT32 *) params));
	dtr.second = (short) ((time&31)<<1);
	dtr.minute = (short) ((time>>5)&63);
	dtr.hour = (short) ((time>>11)&31);
	dtr.day = (short) ((time>>16)&31);
	dtr.month = (short) ((time>>21)&15);
	dtr.year = (short) ((time>>25)+1980);
	SetTime(&dtr);
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: XBIOS Setpalette
*
**********************************************************************/

UINT32 CMagiC::AtariSetpalette(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariSetcolor(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariVsetRGB(UINT32 params, unsigned char *AdrOffset68k)
{
	int i,j;
	UInt32 c;
	UInt32 *pColourTable;
  	#pragma options align=packed
	struct VsetRGBParm
	{
		UInt16 index;
		UInt16 cnt;
		UInt32 pValues;
	};
   	#pragma options align=reset

	VsetRGBParm *theVsetRGBParm = (VsetRGBParm *) (AdrOffset68k + params);
	const UInt8 *pValues = (const UInt8 *) (AdrOffset68k + CFSwapInt32BigToHost(theVsetRGBParm->pValues));
	UInt16 index = CFSwapInt16BigToHost(theVsetRGBParm->index);
	UInt16 cnt = CFSwapInt16BigToHost(theVsetRGBParm->cnt);
	DebugInfo("CMagiC::AtariVsetRGB(index=%u, cnt=%u, 0x%02x%02x%02x%02x)",
			  (unsigned) index, (unsigned) cnt,
			  (unsigned) pValues[0], (unsigned) pValues[1], (unsigned) pValues[2], (unsigned) pValues[3]);

	// durchlaufe alle zu ändernden Farben
	pColourTable = pTheMagiC->m_pMagiCScreen->m_pColourTable;
	j = min(MAGIC_COLOR_TABLE_LEN, index + cnt);
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

UINT32 CMagiC::AtariVgetRGB(UINT32 params, unsigned char *AdrOffset68k)
{
	int i,j;
	UInt32 *pColourTable;
   	#pragma options align=packed
	struct VgetRGBParm
	{
		UInt16 index;
		UInt16 cnt;
		UInt32 pValues;
	};
    	#pragma options align=reset

 	VgetRGBParm *theVgetRGBParm = (VgetRGBParm *) (AdrOffset68k + params);
 	UInt8 *pValues = (UInt8 *) (AdrOffset68k + CFSwapInt32BigToHost(theVgetRGBParm->pValues));
	UInt16 index = CFSwapInt16BigToHost(theVgetRGBParm->index);
	UInt16 cnt = CFSwapInt16BigToHost(theVgetRGBParm->cnt);
	DebugInfo("CMagiC::AtariVgetRGB(index=%d, cnt=%d)", index, cnt);

	// durchlaufe alle zu ändernden Farben
	pColourTable = pTheMagiC->m_pMagiCScreen->m_pColourTable;
	j = min(MAGIC_COLOR_TABLE_LEN, index + cnt);
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

void CMagiC::SendMessageToMainThread( bool bAsync, UInt32 command )
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

UINT32 CMagiC::AtariSysHalt(UINT32 params, unsigned char *AdrOffset68k)
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


UINT32 CMagiC::AtariSysErr(UINT32 params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
	UINT32 act_pd;
	UInt32 m68k_pc;
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
	m68k_pc = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + proc_stk + 2)));

	DebugInfo("CMagiC::AtariSysErr() -- act_pd = 0x%08lx", act_pd);
	DebugInfo("CMagiC::AtariSysErr() -- Prozeßpfad = %s", (AtariPrgFname) ? AtariPrgFname : "<unknown>");
#if defined(_DEBUG)
	if (m68k_pc < pTheMagiC->m_RAM68ksize - 8)
	{
		UInt16 opcode1 = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + m68k_pc)));
		UInt16 opcode2 = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + m68k_pc + 2)));
		UInt16 opcode3 = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + m68k_pc + 4)));
		DebugInfo("CMagiC::AtariSysErr() -- opcode = 0x%04x 0x%04x 0x%04x", (unsigned) opcode1, (unsigned) opcode2, (unsigned) opcode3);
	}
#endif

	Send68kExceptionData(
				(UInt16) (AdrOffset68k[proc_pc /*0x3c4*/]),		// Exception-Nummer
				pTheMagiC->m_BusErrorAddress,
				pTheMagiC->m_BusErrorAccessMode,
				m68k_pc,															// pc
				CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + proc_stk))),		// sr
				CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + proc_usp))),		// usp
				(UInt32 *) (AdrOffset68k + proc_regs /*0x384*/),					// Dx (big endian)
				(UInt32 *) (AdrOffset68k + proc_regs + 32),							// Ax (big endian)
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

UINT32 CMagiC::AtariColdBoot(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariExit(UINT32 params, unsigned char *AdrOffset68k)
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
		CDebug::DebugInfo("### VideoRamWriteCounter(%2d) = %d", i, WriteCounters[i]);
#endif
#endif
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Debug-Ausgaben
*
**********************************************************************/

UINT32 CMagiC::AtariDebugOut(UINT32 params, unsigned char *AdrOffset68k)
{
#pragma unused(params)
#pragma unused(AdrOffset68k)
	DebugInfo("CMagiC::AtariDebugOut(%s)", AdrOffset68k + params);
	return(0);
}


/**********************************************************************
*
* Callback des Emulators: Fehler-Alert
*
* zur Zeit nur Dummy
*
**********************************************************************/

UINT32 CMagiC::AtariError(UINT32 params, unsigned char *AdrOffset68k)
{
	UInt16 errorCode = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + params)));

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

UINT32 CMagiC::AtariPrtOs(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariPrtIn(UINT32 params, unsigned char *AdrOffset68k)
{
	unsigned char c;
	UINT32 n;

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

UINT32 CMagiC::AtariPrtOut(UINT32 params, unsigned char *AdrOffset68k)
{
	UInt32 ret;

	DebugInfo("CMagiC::AtariPrtOut()");
	ret = pThePrint->Write(AdrOffset68k + params + 1, 1);
	// Zeitpunkt (200Hz) des letzten Druckerzugriffs merken
	s_LastPrinterAccess = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + _hz_200)));
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

UINT32 CMagiC::AtariPrtOutS(UINT32 params, unsigned char *AdrOffset68k)
{
	struct PrtOutParm
	{
		UInt32 buf;
		UInt32 cnt;
	};
 	PrtOutParm *thePrtOutParm = (PrtOutParm *) (AdrOffset68k + params);
 	UInt32 ret;


//	CDebug::DebugInfo("CMagiC::AtariPrtOutS()");
	ret = pThePrint->Write(AdrOffset68k + CFSwapInt32BigToHost(thePrtOutParm->buf), CFSwapInt32BigToHost(thePrtOutParm->cnt));
	// Zeitpunkt (200Hz) des letzten Druckerzugriffs merken
	s_LastPrinterAccess = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + _hz_200)));
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

UINT32 CMagiC::OpenSerialBIOS(void)
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
		return((UInt32) ERROR);
	}

	if	(-1 == (int) m_MagiCSerial.Open(CGlobals::s_Preferences.m_szAuxPath))
	{
		DebugInfo("CMagiC::OpenSerialBIOS() -- kann \"%s\" nicht öffnen.", CGlobals::s_Preferences.m_szAuxPath);
		return((UInt32) ERROR);
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

UINT32 CMagiC::AtariSerConf(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerConfParm
	{
		UInt16 baud;
		UInt16 ctrl;
		UInt16 ucr;
		UInt16 rsr;
		UInt16 tsr;
		UInt16 scr;
		UInt32 xtend_magic;	// ist ggf. 'iocl'
		UInt16 biosdev;		// Ioctl-Bios-Gerät
		UInt16 cmd;			// Ioctl-Kommando
		UInt32 parm;		// Ioctl-Parameter
		UInt32 ptr2zero;		// deref. und auf ungleich 0 setzen!
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
		return((UInt32) ERROR);
	}

	SerConfParm *theSerConfParm = (SerConfParm *) (AdrOffset68k + params);

	// Rsconf(-2,-2,-1,-1,-1,-1, 'iocl', dev, cmd, parm) macht Fcntl

	if	((CFSwapInt16BigToHost(theSerConfParm->baud) == 0xfffe) &&
		 (CFSwapInt16BigToHost(theSerConfParm->ctrl) == 0xfffe) &&
		 (CFSwapInt16BigToHost(theSerConfParm->ucr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->rsr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->tsr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->scr) == 0xffff) &&
		 (CFSwapInt32BigToHost(theSerConfParm->xtend_magic) == 'iocl'))
	{
		UInt32 grp;
		UInt32 mode;
		UInt32 ret;
		bool bSet;
		UInt32 NewBaudrate, OldBaudrate;
		UInt16 flags;

		bool bXonXoff;
		bool bRtsCts;
		bool bParityEnable;
		bool bParityEven;
		unsigned int nBits;
		unsigned int nStopBits;


		DebugInfo("CMagiC::AtariSerConf() -- Fcntl(dev=%d, cmd=0x%04x, parm=0x%08x)", CFSwapInt16BigToHost(theSerConfParm->biosdev), CFSwapInt16BigToHost(theSerConfParm->cmd), CFSwapInt32BigToHost(theSerConfParm->parm));
		*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->ptr2zero))) = CFSwapInt32HostToBig(0xffffffff);	// wir kennen Fcntl
		switch(CFSwapInt16BigToHost(theSerConfParm->cmd))
		{
			case TIOCBUFFER:
				// Inquire/Set buffer settings
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCBUFFER) -- nicht unterstützt");
				ret = (UInt32) EINVFN;
				break;

			case TIOCCTLMAP:
				// Inquire I/O-lines and signaling capabilities
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLMAP) -- nicht unterstützt");
				ret = (UInt32) EINVFN;
				break;

			case TIOCCTLGET:
				// Inquire I/O-lines and signals
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLGET) -- nicht unterstützt");
				ret = (UInt32) EINVFN;
				break;

			case TIOCCTLSET:
				// Set I/O-lines and signals
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLSET) -- nicht unterstützt");
				ret = (UInt32) EINVFN;
				break;

			case TIOCGPGRP:
				//get terminal process group
				DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCGPGRP) -- nicht unterstützt");
				ret = (UInt32) EINVFN;
				break;

			case TIOCSPGRP:
				//set terminal process group
				grp = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->parm))));
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCSPGRP, %d)", (UInt32) grp);
				ret = (UInt32) EINVFN;
				break;

			case TIOCFLUSH:
				// Leeren der seriellen Puffer
				mode = CFSwapInt32BigToHost(theSerConfParm->parm);
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
						ret = (UInt32) EINVFN;
						break;
				}
				break;

			case TIOCIBAUD:
			case TIOCOBAUD:
				// Eingabegeschwindigkeit festlegen
				NewBaudrate = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->parm))));
				bSet = ((int) NewBaudrate != -1) && (NewBaudrate != 0);
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(%s, %d)", (CFSwapInt16BigToHost(theSerConfParm->cmd) == TIOCIBAUD) ? "TIOCIBAUD" : "TIOCOBAUD", NewBaudrate);

				if	(CFSwapInt16BigToHost(theSerConfParm->cmd) == TIOCIBAUD)
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

				*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->parm))) = CFSwapInt32HostToBig(OldBaudrate);
				if	((int) ret == -1)
					ret = (UInt32) ATARIERR_ERANGE;
				break;

			case TIOCGFLAGS:
				// Übertragungsprotokolleinstellungen erfragen

				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCGFLAGS, %d)", CFSwapInt32BigToHost(theSerConfParm->parm));
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
				*((UInt16 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->parm))) = CFSwapInt16HostToBig(flags);
				ret = (UInt32) E_OK;
				break;

			case TIOCSFLAGS:
				// Übertragungsprotokolleinstellungen setzen
				flags = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerConfParm->parm))));
				DebugInfo("CMagiC::AtariSerConf() -- Fcntl(TIOCSFLAGS, 0x%04x)", (UInt32) flags);
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
					return((UInt32) ATARIERR_ERANGE);
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
					ret = (UInt32) ATARIERR_ERANGE;
				break;

			default:
				DebugError("CMagiC::AtariSerConf() -- Fcntl(0x%04x -- unbekannt", CFSwapInt16BigToHost(theSerConfParm->cmd) & 0xffff);
				ret = (UInt32) EINVFN;
				break;
		}
		return(ret);
	}

	// Rsconf(-2,-1,-1,-1,-1,-1) gibt aktuelle Baudrate zurück

	if	((CFSwapInt16BigToHost(theSerConfParm->baud) == 0xfffe) &&
		 (CFSwapInt16BigToHost(theSerConfParm->ctrl) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->ucr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->rsr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->tsr) == 0xffff) &&
		 (CFSwapInt16BigToHost(theSerConfParm->scr) == 0xffff))
	{
//		unsigned long OldInputBaudrate;
//		return((UINT32) pTheSerial->GetBaudRate());
	}

	if	(CFSwapInt16BigToHost(theSerConfParm->baud) >= sizeof(baudtable)/sizeof(baudtable[0]))
	{
		DebugError("CMagiC::AtariSerConf() -- ungültige Baudrate von Rsconf()");
		return((UINT32) ATARIERR_ERANGE);
	}

	nBits = nBitsTable[(CFSwapInt16BigToHost(theSerConfParm->ucr) >> 5) & 3];
	nStopBits = (unsigned int) (((CFSwapInt16BigToHost(theSerConfParm->ucr) >> 3) == 3) ? 2 : 1);

	return(pTheSerial->Config(
					true,						// Input-Rate ändern
					baudtable[CFSwapInt16BigToHost(theSerConfParm->baud)],	// neue Input-Rate
					NULL,						// alte Input-Rate egal
					true,						// Output-Rate ändern
					baudtable[CFSwapInt16BigToHost(theSerConfParm->baud)],	// neue Output-Rate
					NULL,						// alte Output-Rate egal
					true,						// Xon/Xoff ändern
					(CFSwapInt16BigToHost(theSerConfParm->ctrl) & 1) != 0,	// neuer Wert
					NULL,						// alter Wert egal
					true,						// Rts/Cts ändern
					(CFSwapInt16BigToHost(theSerConfParm->ctrl) & 2) != 0,
					NULL,
					true,						// parity enable ändern
					(CFSwapInt16BigToHost(theSerConfParm->ucr) & 4) != 0,
					NULL,
					true,						// parity even ändern
					(CFSwapInt16BigToHost(theSerConfParm->ucr) & 2) != 0,
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

UINT32 CMagiC::AtariSerIs(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariSerOs(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariSerIn(UINT32 params, unsigned char *AdrOffset68k)
{
	char c;
	UInt32 ret;
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
		return((UINT32) c & 0x000000ff);
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

UINT32 CMagiC::AtariSerOut(UINT32 params, unsigned char *AdrOffset68k)
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

UINT32 CMagiC::AtariSerOpen(UINT32 params, unsigned char *AdrOffset68k)
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
		return((UINT32) EACCDN);
	}

	if	(-1 == (int) pTheMagiC->m_MagiCSerial.Open(CGlobals::s_Preferences.m_szAuxPath))
	{
		DebugInfo("CMagiC::AtariSerOpen() -- kann \"%s\" nicht öffnen.", CGlobals::s_Preferences.m_szAuxPath);
		return((UInt32) ERROR);
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

UINT32 CMagiC::AtariSerClose(UINT32 params, unsigned char *AdrOffset68k)
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
		return((UINT32) EACCDN);
	}

	if	(pTheMagiC->m_MagiCSerial.Close())
	{
		return((UInt32) ERROR);
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

UINT32 CMagiC::AtariSerRead(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerReadParm
	{
		UInt32 buf;
		UInt32 len;
	};
    	#pragma options align=reset
	UInt32 ret;

	SerReadParm *theSerReadParm = (SerReadParm *) (AdrOffset68k + params);
//	DebugInfo("CMagiC::AtariSerRead(buflen = %d)", theSerReadParm->len);

	ret = pTheSerial->Read(CFSwapInt32BigToHost(theSerReadParm->len),
						(char *) (AdrOffset68k +  CFSwapInt32BigToHost(theSerReadParm->buf)));
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

UINT32 CMagiC::AtariSerWrite(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerWriteParm
	{
		UInt32 buf;
		UInt32 len;
	};
    	#pragma options align=reset
	UInt32 ret;

	SerWriteParm *theSerWriteParm = (SerWriteParm *) (AdrOffset68k + params);
//	DebugInfo("CMagiC::AtariSerWrite(buflen = %d)", theSerWriteParm->len);

	ret = pTheSerial->Write(CFSwapInt32BigToHost(theSerWriteParm->len),
							(char *) (AdrOffset68k +  CFSwapInt32BigToHost(theSerWriteParm->buf)));
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

UINT32 CMagiC::AtariSerStat(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerStatParm
	{
		UInt16 rwflag;
	};
    	#pragma options align=reset

//	DebugInfo("CMagiC::AtariSerWrite()");
	SerStatParm *theSerStatParm = (SerStatParm *) (AdrOffset68k + params);

	return((CFSwapInt16BigToHost(theSerStatParm->rwflag)) ?
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

UINT32 CMagiC::AtariSerIoctl(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct SerIoctlParm
	{
		UInt16 cmd;
		UInt32 parm;
	};
    	#pragma options align=reset

//	DebugInfo("CMagiC::AtariSerWrite()");
	SerIoctlParm *theSerIoctlParm = (SerIoctlParm *) (AdrOffset68k + params);

	UInt32 grp;
	UInt32 mode;
	UInt32 ret;
	bool bSet;
	UInt32 NewBaudrate, OldBaudrate;
	UInt16 flags;

	bool bXonXoff;
	bool bRtsCts;
	bool bParityEnable;
	bool bParityEven;
	unsigned int nBits;
	unsigned int nStopBits;


	DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(cmd=0x%04x, parm=0x%08x)", CFSwapInt16BigToHost(theSerIoctlParm->cmd), CFSwapInt32BigToHost(theSerIoctlParm->parm));
	switch(CFSwapInt16BigToHost(theSerIoctlParm->cmd))
	{
		case TIOCBUFFER:
			// Inquire/Set buffer settings
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCBUFFER) -- nicht unterstützt");
			ret = (UInt32) EINVFN;
			break;

		case TIOCCTLMAP:
			// Inquire I/O-lines and signaling capabilities
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLMAP) -- nicht unterstützt");
			ret = (UInt32) EINVFN;
			break;

		case TIOCCTLGET:
			// Inquire I/O-lines and signals
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLGET) -- nicht unterstützt");
			ret = (UInt32) EINVFN;
			break;

		case TIOCCTLSET:
			// Set I/O-lines and signals
			DebugWarning("CMagiC::AtariSerConf() -- Fcntl(TIOCCTLSET) -- nicht unterstützt");
			ret = (UInt32) EINVFN;
			break;

		case TIOCGPGRP:
			//get terminal process group
			DebugWarning("CMagiC::AtariSerIoctl() -- Fcntl(TIOCGPGRP) -- nicht unterstützt");
			ret = (UInt32) EINVFN;
			break;

		case TIOCSPGRP:
			//set terminal process group
			grp = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerIoctlParm->parm))));
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCSPGRP, %d)", (UInt32) grp);
			ret = (UInt32) EINVFN;
			break;

		case TIOCFLUSH:
			// Leeren der seriellen Puffer
			mode = CFSwapInt32BigToHost(theSerIoctlParm->parm);
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
					ret = (UInt32) EINVFN;
					break;
			}
			break;

		case TIOCIBAUD:
		case TIOCOBAUD:
			// Eingabegeschwindigkeit festlegen
			NewBaudrate = CFSwapInt32BigToHost(*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerIoctlParm->parm))));
			bSet = ((int) NewBaudrate != -1) && (NewBaudrate != 0);
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(%s, %d)", (CFSwapInt16BigToHost(theSerIoctlParm->cmd) == TIOCIBAUD) ? "TIOCIBAUD" : "TIOCOBAUD", NewBaudrate);

			if	(CFSwapInt16BigToHost(theSerIoctlParm->cmd) == TIOCIBAUD)
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

			*((UInt32 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerIoctlParm->parm))) = CFSwapInt32HostToBig(OldBaudrate);
			if	((int) ret == -1)
				ret = (UInt32) ATARIERR_ERANGE;
			break;

		case TIOCGFLAGS:
			// Übertragungsprotokolleinstellungen erfragen

			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCGFLAGS, %d)", CFSwapInt32BigToHost(theSerIoctlParm->parm));
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
			*((UInt16 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerIoctlParm->parm))) = CFSwapInt16HostToBig(flags);
			ret = (UInt32) E_OK;
			break;

		case TIOCSFLAGS:
			// Übertragungsprotokolleinstellungen setzen
			flags = CFSwapInt16BigToHost(*((UInt16 *) (AdrOffset68k + CFSwapInt32BigToHost(theSerIoctlParm->parm))));
			DebugInfo("CMagiC::AtariSerIoctl() -- Fcntl(TIOCSFLAGS, 0x%04x)", (UInt32) flags);
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
				return((UInt32) ATARIERR_ERANGE);
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
				ret = (UInt32) ATARIERR_ERANGE;
			break;

		default:
			DebugError("CMagiC::AtariSerIoctl() -- Fcntl(0x%04x -- unbekannt", CFSwapInt16BigToHost(theSerIoctlParm->cmd) & 0xffff);
			ret = (UInt32) EINVFN;
			break;
	}

	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Idle Task
* params		Zeiger auf UINT32
* Rückgabe:
*
**********************************************************************/

UINT32 CMagiC::AtariYield(UINT32 params, unsigned char *AdrOffset68k)
{
   	#pragma options align=packed
	struct YieldParm
	{
		UInt32 num;
	};
    	#pragma options align=reset
	OSStatus err;
	MPEventFlags EventFlags;


	// zuerst testen, ob während des letzten Assembler-Befehls gerade
	// Ereignisse eingetroffen sind, die im Interrupt bearbeitet worden
	// sind. Wenn ja, hier nicht warten, sondern gleich weitermachen.

	YieldParm *theYieldParm = (YieldParm *) (AdrOffset68k + params);
	if	(CFSwapInt32BigToHost(theYieldParm->num))
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

UINT32 CMagiC::AtariGetKeyboardOrMouseData(UINT32 params, unsigned char *AdrOffset68k)
{
	UINT32 ret;
	char buf[3];
#pragma unused(AdrOffset68k)

#ifdef _DEBUG_KB_CRITICAL_REGION
	CDebug::DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Enter critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
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
		CDebug::DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
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
	CDebug::DebugInfo("CMagiC::AtariGetKeyboardOrMouseData() --- Exited critical region m_KbCriticalRegionId");
	#endif
	return(ret);
}


/**********************************************************************
*
* Callback des Emulators: Programmstart aus Apple-Events abholen
*
**********************************************************************/

UINT32 CMagiC::MmxDaemon(UINT32 params, unsigned char *AdrOffset68k)
{
	UINT32 ret;
   	#pragma options align=packed
	struct MmxDaemonParm
	{
		UInt16 cmd;
		UInt32 parm;
	};
	unsigned char *pBuf;
    	#pragma options align=reset


//	CDebug::DebugInfo("CMagiC::MmxDaemon()");
	MmxDaemonParm *theMmxDaemonParm = (MmxDaemonParm *) (AdrOffset68k + params);

	switch(CFSwapInt16BigToHost(theMmxDaemonParm->cmd))
	{
		// ermittle zu startende Programme/Dateien aus AppleEvent 'odoc'
		case 1:
			OS_EnterCriticalRegion(m_AECriticalRegionId, kDurationForever);
			if	(m_iNoOfAtariFiles)
			{
				// Es liegen Anforderungen vor.
				// Zieladresse:
				pBuf = AdrOffset68k + CFSwapInt32BigToHost(theMmxDaemonParm->parm);
				// von Quelladresse kopieren
				strcpy((char *) pBuf, m_szStartAtariFiles[m_iOldestAtariFile]);
				ret = E_OK;
				m_iNoOfAtariFiles--;
			}
			else
				ret = (UINT32) EFILNF;
			OS_ExitCriticalRegion(m_AECriticalRegionId);
			break;

		// ermittle shutdown-Status
		case 2:
			ret = (UINT32) m_bShutdown;
			break;

		default:
			ret = (UINT32) EUNCMD;
	}

	return(ret);
}
