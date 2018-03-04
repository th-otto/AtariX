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
* Enthält die Debugger-Ausgaben
*
*/

#ifdef _DEBUG
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Debug.h"

// Schalter

short CDebug::RefNum = 0;
int CDebug::GeneralPurposeVariable = 0;


/**********************************************************************
*
* Initialisierung
*
**********************************************************************/

void CDebug::_DebugInit(const unsigned char *DebugFileName)
{
	if (DebugFileName)
	{
		FSSpec spec;
		OSErr err;

		err = FSMakeFSSpec(0, 0, DebugFileName, &spec);
		if	((err != 0) && (err != fnfErr))
			return;		// Fehler
		// Bestehende Datei löschen
		FSpDelete(&spec);
		// Neue Datei anlegen
		err = FSpCreate(&spec, 0, 0, smSystemScript);
		// Neue Datei öffnen
		if	(!err)
			err = FSpOpenDF(&spec, fsWrPerm, &RefNum);
	}
}


/**********************************************************************
*
* Haupt-Ausgabe-Routine
*
**********************************************************************/

void CDebug::_DebugPrint(const char *head, const char *format, va_list arglist)
{
	char line[1024];
	char *s;
//	unsigned long SystemTime;		// in 60stel Sekunden
	DateTimeRec dtr;

	//return;

//	SystemTime = TickCount();
//	sprintf(line, "(%ld) %s", SystemTime, head);
	GetTime(&dtr);
	sprintf(line, "(%02d:%02d:%02d) %s", dtr.hour, dtr.minute, dtr.second, head);

	s = line + strlen(line);
	vsprintf(s, format, arglist);
	// Steuerzeichen entfernen
	while((s = strchr(line, '\r')) != NULL)
	{
		memmove(s, s+1, strlen(s+1) + 1);
	}

	while((s = strchr(line, '\n')) != NULL)
	{
		memmove(s, s+1, strlen(s+1) + 1);
	}

	if (RefNum)
	{
		// Zeilenende
		strcat(line, "\r\n");
		long count;
		count = (long) strlen(line);
		(void) FSWrite(RefNum, &count, line);
	}
	else
	{
		fprintf(stderr, "%s\n", line);
	}
}


/**********************************************************************
*
* Ausgaben
*
**********************************************************************/

void CDebug::_DebugInfo(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	_DebugPrint( "DBG-INF ", format, arglist);
}

void CDebug::_DebugWarning(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	_DebugPrint( "DBG-WRN ", format, arglist);
}

void CDebug::_DebugError(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	_DebugPrint( "DBG-ERR ", format, arglist);
}
#endif