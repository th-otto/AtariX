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
#include <CoreFoundation/CoreFoundation.h>
// Programm-Header
#include "osd_cpu.h"
#include "missing.h"
#include "XCmd.h"
#include "MacXFS.h"
#include "MagiCKeyboard.h"
#include "MagiCMouse.h"
#include "MagiCSerial.h"
#include "MagiCPrint.h"
// Schalter

#define KEYBOARDBUFLEN	32
#define N_ATARI_FILES		8

class CMagiC
{
   public:
	// Konstruktor
	CMagiC();
	// Destruktor
	~CMagiC();
	// Initialisierung
	int Init(CMagiCScreen *pMagiCScreen, CXCmd *pXCmd);
	int CreateThread( void );			// create emulator thread
	void StartExec( void );				// ... let it run
	void StopExec( void );				// ... pause it
	void TerminateThread( void );		// terminate it

//	int SendKeyboard(UInt32 message, bool KeyUp);
	int SendSdlKeyboard(int sdlScanCode, bool KeyUp);
	int SendKeyboardShift(UInt32 modifiers);
	int SendMousePosition(int x, int y);
	int SendMouseButton(unsigned int NumOfButton, bool bIsDown);
	int SendHz200(void);
	int SendVBL(void);
	void SendBusError(UInt32 addr, const char *AccessMode);
	void SendAtariFile(const char *pBuf);
	void SendShutdown(void);
	void ChangeXFSDrive(short drvNr);
	static void GetActAtariPrg(const char **pName, UINT32 *pact_pd);
	bool m_bEmulatorIsRunning;
	bool m_bAtariWasRun;
	bool m_bShutdown;
	void DumpAtariMem(const char *filename);

	char bVideoBufChanged;
   private:
   	// Typdefinitionen
   	#pragma options align=packed

	void UpdateAtariDoubleBuffer(void);

	struct Atari68kData
	{
		MXVDI_PIXMAP m_PixMap;	// der Atari-Bildschirm, baseAddr ist virtuelle 68k-Adresse.
		MgMxCookieData m_CookieData;
	};

   	#pragma options align=reset

	// private Funktionen
	void Init_CookieData(MgMxCookieData *pCookieData);
   	OSErr LoadReloc (CFURLRef fileUrl,
					long stackSize,
					long reladdr,
					BasePage **basePage);
	int GetKbBufferFree( void );
	void PutKeyToBuffer(unsigned char key);
	static OSStatus _EmuThread(void *param);
	OSStatus EmuThread(void);
#if defined(USE_ASGARD_PPC_68K_EMU)
	static int IRQCallback(int IRQLine, void *thisPtr);
#else
	static int IRQCallback(int IRQLine);
#endif
	UINT32 AtariInit( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 AtariBIOSInit( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 AtariVdiInit( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 AtariExec68k( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 OpenSerialBIOS(void);
	static void SendMessageToMainThread( bool bAsync, UInt32 command );
	static UINT32 AtariDOSFn( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariGettime( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSettime( UINT32 params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariSysHalt( void *param );
	static UINT32 AtariSysHalt( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSetpalette( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSetcolor( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariVsetRGB( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariVgetRGB( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSysErr( UINT32 params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariSysErr( void *param );
	static UINT32 AtariColdBoot( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariExit( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariDebugOut( UINT32 params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariError( void *param );
	static UINT32 AtariError( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariPrtOs( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariPrtIn( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariPrtOut( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariPrtOutS( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerConf( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerIs( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerOs( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerIn( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerOut( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerOpen( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerClose( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerRead( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerWrite( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerStat( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariSerIoctl( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 AtariGetKeyboardOrMouseData( UINT32 params, unsigned char *AdrOffset68k );
	UINT32 MmxDaemon( UINT32 params, unsigned char *AdrOffset68k );
	static UINT32 AtariYield( UINT32 params, unsigned char *AdrOffset68k );

	// private Attribute
	CMagiCScreen *m_pMagiCScreen;		// Bildschirmdaten
	unsigned char *m_RAM68k;			// Zeiger auf den emulierten Speicher
	size_t m_RAM68ksize;				// Größe dieses Blocks
	size_t m_Video68ksize;				// Größe dieses Blocks
	unsigned char *m_AtariKbData;		// [0] = kbshift, [1] = kbrepeat
	UINT32 *m_pAtariActPd;
	UINT32 *m_pAtariActAppl;
	BasePage *m_BasePage;			// geladener MagiC-Kernel
	MPQueueID m_EmuNotifQID;			// Notification-Queue für den Thread
	MPTaskID m_EmuTaskID;			// Der Thread
	CMacXFS m_MacXFS;				// das XFS
	CMagiCKeyboard m_MagiCKeyboard;	// Atari-Tastatur
	CMagiCMouse m_MagiCMouse;		// Atari-Maus
	CMagiCSerial m_MagiCSerial;			// serielle Schnittstelle
	CMagiCPrint m_MagiCPrint;			// serielle Schnittstelle
	UInt32 m_CurrModifierKeys;			// aktueller Zustand von Shift/Cmd/...
	bool m_bBIOSSerialUsed;
	bool m_bBusErrorPending;
	UInt32 m_BusErrorAddress;
	char m_BusErrorAccessMode[32];

	bool m_bInterrupt200HzPending;
	bool m_bInterruptVBLPending;
	bool m_bInterruptMouseKeyboardPending;
	Point m_InterruptMouseWhere;
	bool m_bInterruptMouseButton[2];
	bool m_bInterruptPending;
	bool m_bWaitEmulatorForIRQCallback;

	#define EMU_EVNT_RUN			0x00000001
	#define EMU_EVNT_TERM		0x00000002
	MPEventID m_EventId;
	bool m_bCanRun;
	#define EMU_INTPENDING_KBMOUSE	0x00000001
	#define EMU_INTPENDING_200HZ	0x00000002
	#define EMU_INTPENDING_VBL		0x00000004
	#define EMU_INTPENDING_OTHER	0x00000008
	MPEventID m_InterruptEventsId;

	bool m_bSpecialExec;
	unsigned char *m_LineAVars;

	// Ringpuffer für Tastatur/Maus
	unsigned char m_cKeyboardOrMouseData[KEYBOARDBUFLEN];
	unsigned char *m_pKbRead;		// Lesezeiger
	unsigned char *m_pKbWrite;		// Schreibzeiger
	MPCriticalRegionID m_KbCriticalRegionId;
	MPCriticalRegionID m_AECriticalRegionId;
	// Apple Events (Atari-Programm direkt starten)
	int m_iNoOfAtariFiles;
	int m_iOldestAtariFile;	// für den Ringpuffer
	char m_szStartAtariFiles[N_ATARI_FILES][256];

	// Bildschirmdaten
	bool m_bScreenBufferChanged;
	unsigned char *m_pFgBuffer;
	unsigned long m_FgBufferLineLenInBytes;
//	unsigned char *m_pBgBuffer;
//	unsigned long m_BgBufferLineLenInBytes;
	MPCriticalRegionID m_ScrCriticalRegionId;

	// fürs Drucken (leider statisch)
	static UInt32 s_LastPrinterAccess;

	UInt32 m_AtariShutDownDelay;		// added for AtariX
};