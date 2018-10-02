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
 *  C interface for C++ class EmulatorRunner.
 * To be called from Objective-C
 *
 */

#include "EmulationMain.h"
#include "EmulationRunner.h"
#include "m68k.h"
#include "nf_objs.h"



static int s_EmulationIsInit = 0;
static int s_EmulationIsRunning = 0;
static EmulationRunner theEmulation;



int EmulationIsRunning(void)
{
	return s_EmulationIsRunning;
}

int EmulationInit(void)
{
	if (!s_EmulationIsInit)
	{
		m68k_init();
		NFCreate();
		theEmulation.Init();
		s_EmulationIsInit = 1;
	}

	return 0;
}

int EmulationOpenWindow()
{
	return theEmulation.OpenWindow();
}

void EmulationRun(void)
{
	printf("%s()\n", __func__);
	if (s_EmulationIsInit && !s_EmulationIsRunning)
	{
		theEmulation.StartEmulatorThread();
		s_EmulationIsRunning = 1;
	}
	printf("%s() =>\n", __func__);
}

void EmulationRunSdl(void)
{
	theEmulation.EventLoop();
}

void EmulationExit(void)
{
	if (s_EmulationIsRunning)
	{
	}

	if (s_EmulationIsInit)
	{
		theEmulation.Cleanup();
		s_EmulationIsRunning = 0;
		s_EmulationIsInit = 0;
	}
}

void EmulationChangeAtariDrive(unsigned drvnr, CFURLRef drvUrl)
{
	theEmulation.ChangeAtariDrive(drvnr, drvUrl);
}

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
)
{
	theEmulation.Config(atariKernelPathUrl, atariRootfsPathUrl,
						atariMemorySize, atariScreenWidth, atariScreenHeight,
						atariScreenColourMode,
						atariScreenStretchX, atariScreenStretchY,
						atariLanguage,
						atariHideHostMouse, atariPrintCommand, atariSerialDevice);
}


/*
extern "C" int my_main(void)
{
	printf("%s()\n", __func__);

	theEmulation.EventLoop();

	printf("%s() =>\n", __func__);
    return 0;
}
*/
