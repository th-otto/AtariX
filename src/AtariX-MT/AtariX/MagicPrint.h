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

// System-Header
// Programm-Header
#include "osd_cpu.h"
//#include "MagiCScreen.h"
#include "XCmd.h"
#include "MacXFS.h"
#include "MagiCKeyboard.h"
#include "MagiCMouse.h"
#include "MagiCSerial.h"
// Schalter

#define KEYBOARDBUFLEN	32
#define N_ATARI_FILES		8

class CMagiCPrint
{
   public:
	// Konstruktor
	CMagiCPrint();
	// Destruktor
	~CMagiCPrint();

	UInt32 GetOutputStatus(void);
	UInt32 Read(unsigned char *pBuf, UInt32 NumOfBytes);
	UInt32 Write(const unsigned char *pBuf, UInt32 NumOfBytes);
	UInt32 ClosePrinterFile(void);

   private:
	short m_PrintFileRefNum;
	int m_PrintFileCounter;
	static bool bTempFileCreated;
};
