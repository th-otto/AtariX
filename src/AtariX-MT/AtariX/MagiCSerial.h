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

#ifndef _MAGICSERIAL_INCLUDED_
#define _MAGICSERIAL_INCLUDED_

//#define USE_SERIAL_SELECT

#ifndef USE_SERIAL_SELECT
#define SERIAL_IBUFLEN	32
#endif

class CMagiCSerial
{
   public:
	// Konstruktor
	CMagiCSerial();
	// Destruktor
	~CMagiCSerial();
	// Initialisierung
	UInt32 Open(const char *BsdPath);
	// Schließen
	UInt32 Close();
	// Geöffnet?
	bool IsOpen();
	// Ausgabe
	UInt32 Write(unsigned int cnt, const char *pBuffer);
	// Eingabe
	UInt32 Read(unsigned int cnt, char *pBuffer);
	// Ausgabestatus
	bool WriteStatus(void);
	// Eingabestatus
	bool ReadStatus(void);
	// Konfiguration
	UInt32 Config(
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
			unsigned int *pOldnStopBits);

	// wartet, bis der Ausgabepuffer geleert ist.
	UInt32 Drain(void);
	// löscht Ein-/Ausgangspuffer
	UInt32 Flush(bool bInputBuffer, bool bOutputBuffer);

   private:
   	// Unix-Dateideskriptor für Modem
   	int m_fd;
	// Eingangspuffer
#ifndef USE_SERIAL_SELECT
	char m_InBuffer[SERIAL_IBUFLEN];
	unsigned int m_InBufFill;
#endif
};

#endif
