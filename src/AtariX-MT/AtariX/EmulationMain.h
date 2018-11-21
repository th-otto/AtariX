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
//  EmulationMain.h
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 21.11.11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#ifndef _EmulationMain_h
#define _EmulationMain_h

#include <CoreFoundation/CFURL.h>


#if defined (__cplusplus)
extern "C"
{
#endif

int EmulationInit(void);
int EmulationOpenWindow(void);
void EmulationCloseWindow(void);
void EmulationRun(void);
void EmulationRunSdl(void);
int EmulationIsRunning(void);
void EmulationExit(void);
void EmulationStop(void);
void EmulationConfig
(
	const char *atariKernelPathUrl,
	const char *atariRootfsPathUrl,
	unsigned atariMemorySize,
	unsigned atariScreenWidth,
	unsigned atariScreenHeight,
	unsigned atariScreenColourMode,
	bool atariScreenStretchX,
	bool atariScreenStretchY,
	unsigned atariLanguage,
	bool atariHideHostMouse,
	const char *atariPrintCommand,
	const char *atariSerialDevice
);
void EmulationChangeAtariDrive(unsigned drvnr, CFURLRef drvUrl);

#if defined (__cplusplus)
}
#endif

#endif
