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
* Enthält alles, was mit der Atari-Maus zu tun hat
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "osd_cpu.h"
#include "Debug.h"
#include "Globals.h"
#include "MagiCMouse.h"

// Schalter

#define	GCURX	-0x25a
#define	GCURY	-0x258
#define	M_HID_CT	-0x256
#define	CURX		-0x158
#define	CURY		-0x156


/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMagiCMouse::CMagiCMouse()
{
	m_bActAtariMouseButton[0] = m_bActAtariMouseButton[1] = false;
	m_bActMacMouseButton[0] = m_bActMacMouseButton[1] = false;
	m_pLineAVars = NULL;
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMagiCMouse::~CMagiCMouse()
{
}


/**********************************************************************
*
* Initialisierung
*
**********************************************************************/

void CMagiCMouse::Init(unsigned char *pLineAVars, Point PtPos)
{
	m_pLineAVars = pLineAVars;
	m_PtActAtariPos = PtPos;
}


/**********************************************************************
*
* Neue Mausposition übergeben
*
* Rückgabe: true = Mausbewegung notwendig false = keine Mausbewegung
*
**********************************************************************/

bool CMagiCMouse::SetNewPosition(Point PtPos)
{
	if	(m_pLineAVars)
	{
		m_PtActMacPos = PtPos;
		// get current Atari mouse position from Atari memory (big endian)
		m_PtActAtariPos.v = CFSwapInt16BigToHost(*((INT16 *) (m_pLineAVars + CURY)));
		m_PtActAtariPos.h = CFSwapInt16BigToHost(*((INT16 *) (m_pLineAVars + CURX)));
		return((m_PtActMacPos.h != m_PtActAtariPos.h) || (m_PtActMacPos.v != m_PtActAtariPos.v));
	}
	else	return(false);
}


/**********************************************************************
*
* Neuen Maustastenstatus übergeben
*
* Rückgabe: true = Maustasten-Aktualisierung notwendig / false = Maustasten unverändert
*
**********************************************************************/

bool CMagiCMouse::SetNewButtonState(unsigned int NumOfButton, bool bIsDown)
{
	if	(NumOfButton < 2)
	m_bActMacMouseButton[NumOfButton] = bIsDown;
	return(m_bActMacMouseButton[NumOfButton] != m_bActAtariMouseButton[NumOfButton]);
}


/**********************************************************************
*
* Mauspaket liefern
*
* Rückgabe: true = Mausbewegung notwendig false = keine Mausbewegung
*
**********************************************************************/

bool CMagiCMouse::GetNewPositionAndButtonState(char packet[3])
{
	int xdiff,ydiff;
	char packetcode;


	xdiff = m_PtActMacPos.h - m_PtActAtariPos.h;
	ydiff = m_PtActMacPos.v - m_PtActAtariPos.v;

	if	((!xdiff) && (!ydiff) &&
		 (m_bActAtariMouseButton[0] == m_bActMacMouseButton[0]) &&
		 (m_bActAtariMouseButton[1] == m_bActMacMouseButton[1]))
		return(false);	// keine Bewegung/Taste notwendig

	if	(packet)
		{
		packetcode = '\xf8';
		if	(m_bActMacMouseButton[0])
			packetcode += 2;
		if	(m_bActMacMouseButton[1])
			packetcode += 1;
		m_bActAtariMouseButton[0] = m_bActMacMouseButton[0];
		m_bActAtariMouseButton[1] = m_bActMacMouseButton[1];
		*packet++ = packetcode;

		if	(abs(xdiff) < 128)
			*packet = (char) xdiff;
		else	*packet = (xdiff > 0) ? (char) 127 : (char) -127;
		m_PtActAtariPos.h += *packet++;

		if	(abs(ydiff) < 128)
			*packet = (char) ydiff;
		else	*packet = (ydiff > 0) ? (char) 127 : (char) -127;
		m_PtActAtariPos.v += *packet++;
		}

	return(true);
}
