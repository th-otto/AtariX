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
* Serielle Schnittstelle für MagicMacX
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Globals.h"
#include "Debug.h"
#include "MagiCSerial.h"
#include <unistd.h>
#include <termios.h>
#ifdef _DEBUG
//#define DEBUG_VERBOSE
#endif

// statische Attribute

#define ISPEED_ALWAYS_EQUAL_OSPEED	1

// Hilfskonstruktion
#define bzero(a,b) memset(a,0,b)


#if !TARGET_RT_MAC_MACHO
#define	FD_SET(n, p)	((p)->fds_bits[(n)/BSD_NFDBITS] |= (1 << ((n) % BSD_NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/BSD_NFDBITS] &= ~(1 << ((n) % BSD_NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/BSD_NFDBITS] & (1 << ((n) % BSD_NFDBITS)))
#define	FD_COPY(f, t)	bcopy(f, t, sizeof(*(f)))
#define	FD_ZERO(p)	bzero(p, sizeof(*(p)))
#endif

struct termios gOriginalTTYAttrs;
struct termios gActualTTYAttrs;


/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMagiCSerial::CMagiCSerial()
{
	m_fd = -1;
	gActualTTYAttrs.c_ospeed = gActualTTYAttrs.c_ispeed = 0xffffffff;	// ungültig
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMagiCSerial::~CMagiCSerial()
{
	if	(m_fd != -1)
		close(m_fd);
}


/**********************************************************************
*
* Initialisierung
*
**********************************************************************/

UInt32 CMagiCSerial::Open(const char *BsdPath)
{
#ifndef USE_SERIAL_SELECT
	m_InBufFill = 0;
#endif

	DebugInfo("CMagiCSerial::Open() -- Öffne Gerätedatei \"%s\"", BsdPath);

	m_fd = open(BsdPath, O_RDWR | O_NOCTTY | O_NDELAY);
	if (m_fd == -1)
	{
		DebugError("CMagiCSerial::Open() -- " "Fehler beim Öffnen der seriellen Schnittstelle %s - %s(%d).\n", BsdPath, strerror(errno), errno);
		return((UInt32) -1);
	}

	if	(fcntl(m_fd, F_SETFL, 0) == -1)
	{
		DebugError("CMagiCSerial::Open() -- " "Fehler beim Löschen von O_NDELAY %s - %s(%d).", BsdPath, strerror(errno), errno);
		return((UInt32) -2);
	}
    
	// Get the current options and save them for later reset
	if (tcgetattr(m_fd, &gOriginalTTYAttrs) == -1)
	{
		DebugError("CMagiCSerial::Open() -- " "Fehler beim Ermitteln der tty->Attribute %s - %s(%d).", BsdPath, strerror(errno), errno);
		return((UInt32) -3);
	}

	// Set raw input, one second timeout
	// These options are documented in the man page for termios
	// (in Terminal enter: man termios)
	gActualTTYAttrs = gOriginalTTYAttrs;
	gActualTTYAttrs.c_cflag |= (CLOCAL | CREAD);
	gActualTTYAttrs.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	gActualTTYAttrs.c_oflag &= ~OPOST;
	gActualTTYAttrs.c_cc[ VMIN ] = 0;
	gActualTTYAttrs.c_cc[ VTIME ] = 0;		// sofort zurück, wenn keine Zeichen da. War 10, d.h. 10*0,1s = 1s;

	// Default: Beide Richtungen gleich schnell
	gActualTTYAttrs.c_ospeed = gActualTTYAttrs.c_ispeed;

	// Default: keine Parität
	gActualTTYAttrs.c_cflag &= ~(PARENB);
    
	// Set the options
	if (tcsetattr(m_fd, TCSANOW, &gActualTTYAttrs) == -1)
	{
		DebugError("CMagiCSerial::Open() -- " "Fehler beim Setzen der tty->Attribute %s - %s(%d).", BsdPath, strerror(errno), errno);
		return((UInt32) -4);
	}

	return(0);
}


/**********************************************************************
*
* schließen
*
**********************************************************************/

UInt32 CMagiCSerial::Close()
{
	if	(m_fd != -1)
	{
		DebugInfo("CMagiCSerial::Open() -- Schließe Gerätedatei");
		close(m_fd);
		m_fd = -1;
	}
	return(0);
}


/**********************************************************************
*
* geöffnet?
*
**********************************************************************/

bool CMagiCSerial::IsOpen()
{
	return(m_fd != -1);
}


/**********************************************************************
*
* Ausgabe (schreiben)
*
**********************************************************************/

UInt32 CMagiCSerial::Write(unsigned int cnt, const char *pBuffer)
{
	int ret;

#ifdef DEBUG_VERBOSE
	DebugInfo("CMagiCSerial::Write() -- %d Zeichen schreiben:", cnt);
	for	(register int i = 0; i < cnt; i++)
	{
		DebugInfo("CMagiCSerial::Write() -- ==> c = 0x%02x (%c)", (unsigned int) pBuffer[i], ((pBuffer[i] >= ' ') && (pBuffer[i] <= 'z')) ? pBuffer[i] : '?');
	}
#endif
	ret = write(m_fd, pBuffer, cnt);
	if	(ret != -1)
		return((UInt32) ret);	// Anzahl Zeichen

	ret = errno;
	DebugError("CMagiCSerial::Write() -- "
				"Fehler %d", ret);
	return(0xffffffff);		// Atari-Fehlercode
}


/**********************************************************************
*
* Eingabe (lesen)
*
**********************************************************************/

UInt32 CMagiCSerial::Read(unsigned int cnt, char *pBuffer)
{
	int ret;
	int nBytesRead = 0;

#ifndef USE_SERIAL_SELECT
	nBytesRead = (int) min(m_InBufFill, cnt);
	if	(nBytesRead)
	{
		memcpy(pBuffer, m_InBuffer, (size_t) nBytesRead);
		memmove(m_InBuffer, m_InBuffer+nBytesRead, (size_t) (m_InBufFill - nBytesRead));
#ifdef DEBUG_VERBOSE
		if	(nBytesRead)
			DebugInfo("CMagiCSerial::Read() -- buflen = %d, %d Zeichen erhalten:", cnt, nBytesRead);

		for	(register int i = 0; i < nBytesRead; i++)
		{
			DebugInfo("CMagiCSerial::Read() -- <== c = 0x%02x (%c)", ((unsigned int) (pBuffer[i])) & 0xff, ((pBuffer[i] >= ' ') && (pBuffer[i] <= 'z')) ? pBuffer[i] : '?');
		}
#endif
		m_InBufFill -= nBytesRead;
		pBuffer += nBytesRead;
		cnt -= nBytesRead;
	}

	if	(cnt)
		ret = read(m_fd, pBuffer, cnt);
	else
		ret = 0;
#else
	ret = read(m_fd, pBuffer, cnt);
#endif

#ifdef DEBUG_VERBOSE
	if	(ret != -1)
	{
		if	(ret)
			DebugInfo("CMagiCSerial::Read() -- buflen = %d, %d Zeichen erhalten:", cnt, ret);

		for	(register int i = 0; i < ret; i++)
		{
			DebugInfo("CMagiCSerial::Read() -- <== c = 0x%02x (%c)", ((unsigned int) (pBuffer[i])) & 0xff, ((pBuffer[i] >= ' ') && (pBuffer[i] <= 'z')) ? pBuffer[i] : '?');
		}
	}
#endif
	if	(ret != -1)
		return((UInt32) ret + nBytesRead);	// Anzahl Zeichen

	ret = errno;
	DebugError("CMagiCSerial::Read() -- "
				"Fehler %d", ret);
	return(0xffffffff);		// Atari-Fehlercode
}


/**********************************************************************
*
* Ausgabestatus (schreiben)
*
**********************************************************************/

bool CMagiCSerial::WriteStatus(void)
{
	int ret;
	static struct timeval Timeout = {0,0};
	struct fd_set fdset;


	if	(m_fd == -1)
		return(false);

	FD_ZERO(&fdset);
	FD_SET(m_fd, &fdset);
	ret = select(1, NULL /*readfds*/, &fdset/*writefds*/, NULL, &Timeout);
	return(ret == 1);	// number of ready fds
}


/**********************************************************************
*
* Eingabestatus (lesen)
*
**********************************************************************/

bool CMagiCSerial::ReadStatus(void)
{
#ifdef USE_SERIAL_SELECT
	int ret;
	static struct bsd_timeval Timeout = {0,0};
	struct bsd_fd_set fdset;


	if	(m_fd == -1)
		return(false);

	FD_ZERO(&fdset);
	FD_SET(m_fd, &fdset);
	ret = CMachO::select(1, &fdset /*readfds*/, NULL/*writefds*/, NULL, &Timeout);
	return(ret == 1);	// number of ready fds
#else
	int ret;

	if	(m_InBufFill)
		return(true);

	ret = read(m_fd, m_InBuffer, SERIAL_IBUFLEN);
	if	((ret == -1) || (ret == 0))
		return(false);
	m_InBufFill = ret;
	return(true);
#endif
}


/**********************************************************************
*
* wartet, bis der Ausgabepuffer geleert ist.
*
**********************************************************************/
 
UInt32 CMagiCSerial::Drain(void)
{
	int ret;

	ret = tcdrain(m_fd);

	if	(ret)
		return((UInt32) -1);

	return(0);
}


/**********************************************************************
*
* Löscht Ein/Ausgangspuffer
*
**********************************************************************/
 
UInt32 CMagiCSerial::Flush(bool bInputBuffer, bool bOutputBuffer)
{
	int action,ret;

#ifndef USE_SERIAL_SELECT
	m_InBufFill = 0;
#endif

	if	(bInputBuffer && bOutputBuffer)
		action = TCIOFLUSH;
	else
	if	(bInputBuffer)
		action = TCIFLUSH;
	else
	if	(bOutputBuffer)
		action = TCOFLUSH;
	else
		action = 0;

	ret = tcflush(m_fd, action);

	if	(ret)
		return((UInt32) -1);

	return(0);
}


/**********************************************************************
*
* Baudrate oder Synchronisationsmodus holen/ändern
*
**********************************************************************/

UInt32 CMagiCSerial::Config(
		bool bSetInputBaudRate,
		unsigned long InputBaudRate,
		unsigned long *pOldInputBaudrate,
		bool bSetOutputBaudRate,
		unsigned long OutputBaudRate,
		unsigned long *pOldOutputBaudrate,
		bool bSetXonXoff,
		bool bXonXoff,
		bool *pbOldXonXoff,
		bool bSetRtsCts,
		bool bRtsCts,
		bool *pbOldRtsCts,
		bool bSetParityEnable,
		bool bParityEnable,
		bool *pbOldParityEnable,
		bool bSetParityEven,
		bool bParityEven,
		bool *pbOldParityEven,
		bool bSetnBits,
		unsigned int nBits,
		unsigned int *pOldnBits,
		bool bSetnStopBits,
		unsigned int nStopBits,
		unsigned int *pOldnStopBits)
{
	bool bSet;


	DebugInfo("CMagiCSerial::Config()");

	if	(pOldInputBaudrate)
		*pOldInputBaudrate = (unsigned long) gActualTTYAttrs.c_ispeed;
	if	(bSetInputBaudRate)
	{
		gActualTTYAttrs.c_ispeed = (long) InputBaudRate;
#if ISPEED_ALWAYS_EQUAL_OSPEED
		gActualTTYAttrs.c_ospeed = gActualTTYAttrs.c_ispeed;
#endif
		DebugInfo("   ispeed = %u, ospeed = %u", gActualTTYAttrs.c_ispeed, gActualTTYAttrs.c_ospeed);
		bSet = true;
	}

	if	(pOldOutputBaudrate)
		*pOldOutputBaudrate = (unsigned long) gActualTTYAttrs.c_ospeed;
	if	(bSetOutputBaudRate)
	{
		gActualTTYAttrs.c_ospeed = (long) OutputBaudRate;
#if ISPEED_ALWAYS_EQUAL_OSPEED
		gActualTTYAttrs.c_ispeed = gActualTTYAttrs.c_ospeed;
#endif
		DebugInfo("   ispeed = %u, ospeed = %u", gActualTTYAttrs.c_ispeed, gActualTTYAttrs.c_ospeed);
		bSet = true;
	}

	// Software-Synchronisation ein/aus

	if	(pbOldXonXoff)
	{
		*pbOldXonXoff = (gActualTTYAttrs.c_iflag & IXON + IXOFF) != 0;
	}
	if	(bSetXonXoff)
	{
		if	(bXonXoff)
		{
			gActualTTYAttrs.c_iflag |= IXON + IXOFF;
		}
		else
		{
			gActualTTYAttrs.c_iflag &= ~(IXON + IXOFF);
		}
		DebugInfo("   XON = %u, XOFF = %u", (gActualTTYAttrs.c_iflag & IXON) != 0, (gActualTTYAttrs.c_iflag & IXOFF) != 0);
		bSet = true;
	}

	// Hardware-Synchronisation ein/aus

	if	(pbOldRtsCts)
	{
		*pbOldRtsCts = (gActualTTYAttrs.c_cflag & CCTS_OFLOW + CRTS_IFLOW) != 0;
	}
	if	(bSetRtsCts)
	{
		if	(bRtsCts)
		{
			gActualTTYAttrs.c_cflag |= CCTS_OFLOW + CRTS_IFLOW;
		}
		else
		{
			gActualTTYAttrs.c_cflag &= ~(CCTS_OFLOW + CRTS_IFLOW);
		}
		DebugInfo("   RTS = %u, CTS = %u", (gActualTTYAttrs.c_cflag & CCTS_OFLOW) != 0, (gActualTTYAttrs.c_cflag & CRTS_IFLOW) != 0);
		bSet = true;
	}

	// Parität ein/aus

	if	(pbOldParityEnable)
	{
		*pbOldParityEnable = (gActualTTYAttrs.c_cflag & PARENB) != 0;
	}
	if	(bSetParityEnable)
	{
		if	(bParityEnable)
		{
			gActualTTYAttrs.c_cflag |= PARENB;
		}
		else
		{
			gActualTTYAttrs.c_cflag &= ~(PARENB);
		}
		DebugInfo("   parity enable = %u", gActualTTYAttrs.c_cflag & PARENB);
		bSet = true;
	}

	// Parität gerade/ungerade

	if	(pbOldParityEven)
	{
		*pbOldParityEven = (gActualTTYAttrs.c_cflag & PARODD) != 0;
	}
	if	(bSetParityEven)
	{
		if	(!bParityEven)
		{
			gActualTTYAttrs.c_cflag |= PARODD;
		}
		else
		{
			gActualTTYAttrs.c_cflag &= ~(PARODD);
		}
		DebugInfo("   parity odd = %u", gActualTTYAttrs.c_cflag & PARODD);
		bSet = true;
	}

	// Anzahl Bits

	if	(pOldnBits)
	{
		switch(gActualTTYAttrs.c_cflag & CSIZE)
		{
			case CS5:
				*pOldnBits = 5;
				break;

			case CS6:
				*pOldnBits = 6;
				break;

			case CS7:
				*pOldnBits = 7;
				break;

			case CS8:
				*pOldnBits = 8;
				break;

			default:
				*pOldnBits = 0;
				break;
		}
	}
	if	(bSetnBits)
	{
		gActualTTYAttrs.c_cflag &= ~CSIZE;
		switch(nBits)
		{
			case 5:
			gActualTTYAttrs.c_cflag |= CS5;
			DebugInfo("   5 data bits");
			break;

			case 6:
			gActualTTYAttrs.c_cflag |= CS6;
			DebugInfo("   6 data bits");
			break;

			case 7:
			gActualTTYAttrs.c_cflag |= CS7;
			DebugInfo("   7 data bits");
			break;

			case 8:
			default:
			gActualTTYAttrs.c_cflag |= CS8;
			DebugInfo("   8 data bits");
			break;
		}
		bSet = true;
	}

	// Stop-Bits 1 oder 2

	if	(pOldnStopBits)
	{
		*pOldnStopBits = (gActualTTYAttrs.c_cflag & CSTOPB) ? (unsigned int) 2 : (unsigned int) 1;
	}
	if	(bSetnStopBits)
	{
		if	(nStopBits == 2)
		{
			gActualTTYAttrs.c_cflag |= CSTOPB;
			DebugInfo("   2 stop bits");
		}
		else
		{
			gActualTTYAttrs.c_cflag &= ~(CSTOPB);
			DebugInfo("   1 stop bit");
		}
		bSet = true;
	}

	if	(bSet)
	{
		if (tcsetattr(m_fd, TCSANOW, &gActualTTYAttrs) == -1)
		{
			DebugError("CMagiCSerial::Config() -- " "Fehler %s(%d) beim Setzen der Einstellungen.", strerror(errno), errno);
			return((UInt32) -1);
		}
	}

	return(0);
}
