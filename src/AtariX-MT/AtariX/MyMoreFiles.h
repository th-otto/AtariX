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
* Enthält alle benötigten Routinen aus MoreFilesExtras.h
*
*/

#ifndef _INCLUDED_MYMOREFILES_H
#define _INCLUDED_MYMOREFILES_H

// System-Header
// Programm-Header

// Schalter

extern pascal OSErr FSpResolveAlias (FSSpec *pSpec);

extern pascal OSErr GetCatInfoNoName(
				short vRefNum,
				long dirID,
				ConstStr255Param name,
				CInfoPBPtr pb);

extern pascal OSErr GetDirectoryID(
			short vRefNum,
			long dirID,
			ConstStr255Param name,
			long *theDirID,
			Boolean *isDirectory);

extern pascal OSErr FSpGetDirectoryID(
			const FSSpec *spec,
			long *theDirID,
			Boolean *isDirectory);

extern pascal OSErr GetVolumeInfoNoName(
				ConstStr255Param pathname,
				short vRefNum,
				HParmBlkPtr pb);

extern pascal OSErr DetermineVRefNum(
			ConstStr255Param pathname,
			short vRefNum,
			short *realVRefNum);

extern pascal OSErr GetDirName(
			short vRefNum,
			long dirID,
			Str31 name);

#endif
