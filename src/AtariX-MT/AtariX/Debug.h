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


#ifdef _DEBUG
class CDebug
{
	public:
	static void _DebugInit(const unsigned char *DebugFileName);
	static void _DebugInfo(const char *format, ...);
	static void _DebugWarning(const char *format, ...);
	static void _DebugError(const char *format, ...);

	static int GeneralPurposeVariable;

	private:
	static void _DebugPrint(const char *head, const char *format, va_list arglist);
	static short RefNum;
};

#if 0
#define DebugInit(...)
#define DebugInfo(format, ...) fprintf(stderr, "INF: " format "\n", ##__VA_ARGS__)
#define DebugWarning(format, ...) fprintf(stderr, "WRN: " format "\n", ##__VA_ARGS__)
#define DebugError(format, ...) fprintf(stderr, "ERR: " format "\n", ##__VA_ARGS__)

#else
#define DebugInit CDebug::_DebugInit
#define DebugInfo(...) CDebug::_DebugInfo(__VA_ARGS__)
#define DebugWarning(...) CDebug::_DebugWarning(__VA_ARGS__)
#define DebugError(...) CDebug::_DebugError(__VA_ARGS__)
#endif
#else
/*
class CDebug
{
	public:
	inline static void DebugInit(unsigned char *DebugFileName)
	{
	};
	inline static void _DebugInfo(const char *format, ...)
	{
	};
	inline static void _DebugWarning(const char *format, ...)
	{
	};
	inline static void _DebugError(const char *format, ...)
	{
	};
	private:
};
*/
#define DebugInit(...)
#define DebugInfo(...)
#define DebugWarning(...)
#define DebugError(...)
#endif
