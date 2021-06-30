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

#ifndef _MAGICMOUSE_INCLUDED_
#define _MAGICMOUSE_INCLUDED_


// System-Header
// Programm-Header

// Schalter

class CMagiCMouse
{
   public:
	// Konstruktor
	CMagiCMouse();
	// Destruktor
	~CMagiCMouse();
	// Initialisierung
	void Init(unsigned char *pLineAVars, Point PtPos);
	// Neue Mausposition übergeben
	bool SetNewPosition(Point PtPos);
	// Neuen Maustastenstatus übergeben
	bool SetNewButtonState(unsigned int NumOfButton, bool bIsDown);
	// packet abholen
	bool GetNewPositionAndButtonState(char packet[3]);

   private:
   	unsigned char *m_pLineAVars;
	Point m_PtActAtariPos;		// Ist
	Point m_PtActMacPos;		// Soll
	bool m_bActAtariMouseButton[2];		// Ist
	bool m_bActMacMouseButton[2];		// Soll
};

#endif
