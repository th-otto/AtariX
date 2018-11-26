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

static FILE *debug_file;
enum LOG_LEVEL {
	LOG_NONE,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_TRACE
};

static enum LOG_LEVEL debug_level = LOG_ERROR;

/**********************************************************************
*
* Initialisierung
*
**********************************************************************/

void _DebugInit(const char *DebugFileName)
{
	if (DebugFileName)
	{
		debug_file = fopen(DebugFileName, "w");
		if	(debug_file == NULL)
			return;		// Fehler
	}
}


/**********************************************************************
*
* Haupt-Ausgabe-Routine
*
**********************************************************************/

void _DebugPrint(const char *head, const char *format, va_list arglist)
{
	char line[1024];
	char *s;
	struct tm tm;
	time_t now;
	
	//return;

//	SystemTime = TickCount();
//	sprintf(line, "(%ld) %s", SystemTime, head);
	now = time(0);
	tm = *localtime(&now);
	sprintf(line, "(%02d:%02d:%02d) %s", tm.tm_hour, tm.tm_min, tm.tm_sec, head);

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

	if (debug_file)
	{
		fprintf(debug_file, "%s\n", line);
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

void _DebugTrace(const char *format, ...)
{
	va_list arglist;

	if (debug_level < LOG_TRACE)
		return;
	va_start(arglist, format);
	_DebugPrint( "", format, arglist);
}

void _DebugInfo(const char *format, ...)
{
	va_list arglist;

	if (debug_level < LOG_INFO)
		return;
	va_start(arglist, format);
	_DebugPrint( "DBG-INF ", format, arglist);
}

void _DebugWarning(const char *format, ...)
{
	va_list arglist;

	if (debug_level < LOG_WARNING)
		return;
	va_start(arglist, format);
	_DebugPrint( "DBG-WRN ", format, arglist);
}

void _DebugError(const char *format, ...)
{
	va_list arglist;

	if (debug_level < LOG_ERROR)
		return;
	va_start(arglist, format);
	_DebugPrint( "DBG-ERR ", format, arglist);
}
#endif
