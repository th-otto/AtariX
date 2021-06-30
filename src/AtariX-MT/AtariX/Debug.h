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

// ANSI-Header
#include <stdarg.h>
// System-Header
#include <stdio.h>
// Programm-Header

// Schalter


#ifdef __cplusplus
extern "C" {
#endif
	void _DebugInit(const char *DebugFileName);
	void _DebugTrace(const char *format, ...);
	void _DebugInfo(const char *format, ...);
	void _DebugWarning(const char *format, ...);
	void _DebugError(const char *format, ...);
	
	void _DebugPrint(const char *head, const char *format, va_list arglist);
#ifdef __cplusplus
}
#endif

#if defined(_DEBUG)

#define DebugInit _DebugInit
#define DebugTrace(...) _DebugTrace(__VA_ARGS__)
#define DebugInfo(...) _DebugInfo(__VA_ARGS__)
#define DebugWarning(...) _DebugWarning(__VA_ARGS__)
#define DebugError(...) _DebugError(__VA_ARGS__)

#else

#define DebugInit(...)
#define DebugTrace(...)
#define DebugInfo(...)
#define DebugWarning(...)
#define DebugError(...)

#endif
