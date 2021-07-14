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
//  missing.c
//  SDLOpenGLIntro
//
//  Created by Andreas Kromke on 07.10.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "config.h"
// System-Header
#include <CoreFoundation/CoreFoundation.h>
// Programm-Header
#include "Globals.h"
#include "Debug.h"
#include "missing.h"
#include "TextConversion.h"

#define _(x) CFCopyLocalizedString(CFSTR(x), NULL)

int MyAlert(int alertID, int nButtons)
{
	CFStringRef msgtxt;
	CFStringRef inftxt;

	switch (alertID)
	{
	case ALRT_NO_VIDEO_DRIVER:
		msgtxt = _("No screen driver found.");
		inftxt = _("The Atari Root-Fs is incomplete, and the screen driver is missing.");
		break;

	case ALRT_NO_MAGICMAC_OS:
		msgtxt = _("The Atari kernel is missing.");
		inftxt = _("No file with name \"MagicMacX.OS\" found in the application directory.");
		break;

	case ALRT_NOT_ENOUGH_MEM:
		msgtxt = _("Not enough memory.");
		inftxt = NULL;
		break;

	case ALRT_INVALID_MAGICMAC_OS:
		msgtxt = _("The Atari kernel is damaged.");
		inftxt = _("The file \"MagicMacX.OS\" has been modified.");
		break;

	case ALRT_RESTART_APPLICATION:
		msgtxt = _("The Atari emulator has to be restarted.");
		inftxt = _("The changes to the settings cannot be applied at runtime.");
		break;

	/* not used anywhere? */
	case ALRT_QUIT_INSTANTLY:
		msgtxt = _("The Atari emulator will be terminated immediately.");
		inftxt = NULL;
		break;

	case ALRT_DEMO:
		msgtxt = _("The Atari emulator is a Demo-version.");
		inftxt = _("Write access is disabled in the Demo-version.");
		break;

	case ALRT_MIN_ATARI_SCREEN_SIZE:
		msgtxt = _("The screen size has been adjusted automatically.");
		inftxt = _("The Atari emulator needs a minimum screen size of 320x200 pixel.");
		break;

	case ALRT_ILLEGAL_FUNC:
		msgtxt = _("Illegal function call");
		inftxt = _("The atari kernel issued an illegal function call.\n"
		         "Maybe the kernel is damaged.\n"
		         "Emulation will be stopped.");
		break;

	case ALRT_INVALID_MONITOR:
	case ALRT_DISPLAY_CHANGED:
	default:
		DebugError("%s: invalid alertID %d", __FUNCTION__, alertID);
		return 0;
	}

	GuiMyAlert(msgtxt, inftxt, nButtons);
	// CFRelease(inftxt);
	// CFRelease(msgtxt);
	return 0;
}
void Send68kExceptionData(
						 uint16_t exc,
						 uint32_t ErrAddr,
						 const char *AccessMode,
						 uint32_t pc,
						 uint16_t sr,
						 uint32_t usp,
						 uint32_t *pDx,
						 uint32_t *pAx,
						 const char *ProcPath,
						 uint32_t pd)
{
	GuiAtariCrash(exc, ErrAddr, AccessMode, pc, sr, usp, pDx, pAx, ProcPath, pd);
}


/*
 * possible (english) strings:
 * '*** FATAL ERROR IN DOS-XFS:'
 * '*** FATAL ERROR IN MEMORY MANAGEMENT:'
 * '*** FATAL ERROR IN GEMDOS:'
 * '*** FATAL ERROR IN AES:'
 * '*** INSUFFICIENT MEMORY IN WINDOW MANAGER:'
 * '*** FATAL ERROR IN WINDOW MANAGER:'
 * '*** SYSTEM STACK OVERFLOW::'
 *
 * They are already localized, but in atari encoding.
 */
void SendSysHaltReason(const char *Reason)
{
	char chars[2048];
	CFStringRef reason;

	CFStringRef title = _("Emulation has been stopped");
	CTextConversion::Atari2HostUtf8Copy(chars, Reason, sizeof(chars));
	reason = CFStringCreateWithCString(NULL, chars, kCFStringEncodingUTF8);
	GuiMyAlert(title, reason, 1);
	// CFRelease(reason); crashes?
	// CFRelease(title);
}

void MMX_BeginDialog(void)
{
	//GuiShowMouse();
}
void MMX_EndDialog(void)
{
}

CMagiCScreen::CMagiCScreen(void)
{
	memset(this, 0, sizeof(*this));
}
CMagiCScreen::~CMagiCScreen(void)
{
}
