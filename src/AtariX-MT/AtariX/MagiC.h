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
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h> /* needed for MultiProcessing functions */
// Programm-Header
#include "missing.h"
#include "XCmd.h"
#include "MacXFS.h"
#include "MagiCKeyboard.h"
#include "MagiCMouse.h"
#include "MagiCSerial.h"
#include "MagicPrint.h"
#include <stdatomic.h>
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

//	int SendKeyboard(uint32_t message, bool KeyUp);
	int SendSdlKeyboard(int sdlScanCode, bool KeyUp);
	int SendKeyboardShift(uint32_t modifiers);
	int SendMousePosition(int x, int y);
	int SendMouseButton(unsigned int NumOfButton, bool bIsDown);
	int SendHz200(void);
	int SendVBL(void);
	void SendBusError(uint32_t addr, const char *AccessMode);
	void SendAtariFile(const char *pBuf);
	void SendShutdown(void);
	void ChangeXFSDrive(short drvNr);
	static void GetActAtariPrg(const char **pName, uint32_t *pact_pd);
	bool m_bEmulatorIsRunning;
	bool m_bShutdown;
	void DumpAtariMem(const char *filename);

	atomic_char bVideoBufChanged;
   private:
   	// Typdefinitionen
   	#pragma options align=packed

	static uint32_t thunk_AtariInit(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_AtariBIOSInit(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_AtariVdiInit(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_AtariExec68k(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_AtariGetKeyboardOrMouseData(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_MmxDaemon(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_XFSFunctions(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_XFSDevFunctions(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_Drv2DevCode(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_RawDrvr(uint32_t params, unsigned char *AdrOffset68k);
	static uint32_t thunk_XCmdCommand(uint32_t params, unsigned char *AdrOffset68k);
	CXCmd *m_pXCmd;

	void UpdateAtariDoubleBuffer(void);

	struct Atari68kData
	{
		MXVDI_PIXMAP m_PixMap;	// der Atari-Bildschirm, baseAddr ist virtuelle 68k-Adresse.
		MgMxCookieData m_CookieData;
	};

   	#pragma options align=reset

	// private Funktionen
	void Init_CookieData(MgMxCookieData *pCookieData);
   	int LoadReloc (CFURLRef fileUrl,
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
	uint32_t AtariInit( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t AtariBIOSInit( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t AtariVdiInit( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t AtariExec68k( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t OpenSerialBIOS(void);
	static void SendMessageToMainThread( bool bAsync, uint32_t command );
	static uint32_t AtariEnosys( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariDOSFn( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariGettime( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSettime( uint32_t params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariSysHalt( void *param );
	static uint32_t AtariSysHalt( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSetpalette( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSetcolor( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariVsetRGB( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariVgetRGB( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSysErr( uint32_t params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariSysErr( void *param );
	static uint32_t AtariColdBoot( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariExit( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariDebugOut( uint32_t params, unsigned char *AdrOffset68k );
	static void *_Remote_AtariError( void *param );
	static uint32_t AtariError( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariPrtOs( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariPrtIn( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariPrtOut( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariPrtOutS( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerConf( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerIs( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerOs( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerIn( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerOut( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerOpen( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerClose( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerRead( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerWrite( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerStat( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariSerIoctl( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t AtariGetKeyboardOrMouseData( uint32_t params, unsigned char *AdrOffset68k );
	uint32_t MmxDaemon( uint32_t params, unsigned char *AdrOffset68k );
	static uint32_t AtariYield( uint32_t params, unsigned char *AdrOffset68k );

	// private Attribute
	CMagiCScreen *m_pMagiCScreen;		// Bildschirmdaten
	unsigned char *m_RAM68k;			// Zeiger auf den emulierten Speicher
	size_t m_RAM68ksize;				// Größe dieses Blocks
	size_t m_Video68ksize;				// Größe dieses Blocks
	unsigned char *m_AtariKbData;		// [0] = kbshift, [1] = kbrepeat
	uint32_t *m_pAtariActPd;
	uint32_t *m_pAtariActAppl;
	BasePage *m_BasePage;			// geladener MagiC-Kernel
	MPQueueID m_EmuNotifQID;			// Notification-Queue für den Thread
	MPTaskID m_EmuTaskID;			// Der Thread
	CMacXFS m_MacXFS;				// das XFS
	CMagiCKeyboard m_MagiCKeyboard;	// Atari-Tastatur
	CMagiCMouse m_MagiCMouse;		// Atari-Maus
	CMagiCSerial m_MagiCSerial;			// serielle Schnittstelle
	CMagiCPrint m_MagiCPrint;			// serielle Schnittstelle
	uint32_t m_CurrModifierKeys;			// aktueller Zustand von Shift/Cmd/...
	bool m_bBIOSSerialUsed;
	bool m_bBusErrorPending;
	uint32_t m_BusErrorAddress;
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
	static uint32_t s_LastPrinterAccess;

	uint32_t m_AtariShutDownDelay;		// added for AtariX
};
