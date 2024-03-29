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

//
//  missing.h
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 07.10.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef SDLOpenGLIntro_missing_h
#define SDLOpenGLIntro_missing_h

#include "resource.h"
#include "Atari.h"

#define MAGIC_COLOR_TABLE_LEN 256

class CMagiCScreen
{
public:
	// Konstruktor
	CMagiCScreen();
	// Destruktor
	~CMagiCScreen();
	// Initialisierung
	int Init(void);

	// Vordergrund-Bildspeicher
	MXVDI_PIXMAP m_PixMap;					// der Atari-Bildschirm in Mac-Koordinaten
	void *hostScreen;
	uint32_t m_pColourTable[MAGIC_COLOR_TABLE_LEN];			// 256 Farben: Farbtabelle
};

extern "C" int GuiMyAlert(CFStringRef msg_text, CFStringRef info_txt, int nButtons);
extern "C" void GuiAtariCrash
(
	uint16_t exc,
	uint32_t ErrAddr,
	const char *AccessMode,
	uint32_t pc,
	uint16_t sr,
	uint32_t usp,
	const uint32_t *pDx,
	const uint32_t *pAx,
	const char *ProcPath,
	uint32_t pd
 );
extern "C" void GuiShowMouse(void);

#endif
