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
* Enth�lt alle ben�tigten Routinen aus MoreFilesExtras.cpp
*
*/

// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "MyMoreFiles.h"


/**********************************************************************
*
* Ermittelt die dirID und die vRefnum des Verzeichnisses,
* in dem die ausf�hrbare Datei bzw. das Bundle liegt
*
* Im Fall "Bundle" wird der aktuelle Pfad auf dieses Verzeichnis
* gesetzt.
*
**********************************************************************/

void GetExeLocation
(
	ProcessSerialNumber *pPsn,
	ProcessInfoRec *pProcInfo,
	FSSpec *pProcDirFsp,
	long *pProc_dirID,			// hier liegt das Bundle
	long *pExecutableDirID		// hier liegt die ausgef�hrte Programmdatei
)
{
	FSSpec FSpApp;
	OSErr err;
	OSStatus errs;
	FSRef fsref;

	// get process information

	err = GetCurrentProcess (pPsn);
	if	(err)
		return;
	pProcInfo->processInfoLength = sizeof (*pProcInfo);
	pProcInfo->processName = nil;
	pProcInfo->processAppSpec = &FSpApp;
	err = GetProcessInformation (pPsn, pProcInfo);
	if	(err)
		return;
	*pExecutableDirID = FSpApp.parID;

	// get directory the executable resp. the bundle lies in

#if TARGET_RT_MAC_MACHO
	errs = GetProcessBundleLocation(pPsn, &fsref);
	if	(err)
		return;
	// convert FSRef to FSSpec
	err = FSGetCatalogInfo(
			&fsref,
			kFSCatInfoNone, // kFSCatInfoVolume+kFSCatInfoParentDirID,
			NULL, //catalogInfo,
			NULL,	// name
			&FSpApp,
			NULL		// FSRef *parent
			);
	if	(err)
		return;

	// make directory the executable resp. the bundle lies in as current directory

	err = HSetVol(NULL, FSpApp.vRefNum, FSpApp.parID);
	if	(err)
		return;
#endif
	*pProc_dirID = FSpApp.parID;

	// Der Pfad, in dem die ausgef�hrte Datei liegt
	FSMakeFSSpec(FSpApp.vRefNum, *pProc_dirID, nil, pProcDirFsp);
}


/**********************************************************************
*
* Aus MM
*
* Pr�ft, ob die Datei ein Alias ist und l�st den ggf. auf.
* Liefert Fehlercode, z.B., wenn ein Login-Dialog vom User abgebrochen wurde.
*
**********************************************************************/

pascal OSErr FSpResolveAlias (FSSpec *pSpec)
{
	Boolean isFolder, wasAlias;
	return ResolveAliasFile (pSpec, true, &isFolder, &wasAlias);
}

/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

pascal OSErr GetCatInfoNoName(
				short vRefNum,
				long dirID,
				ConstStr255Param name,
				CInfoPBPtr pb)
{
	Str31 tempName;
	OSErr error;
	
	/* Protection against File Sharing problem */
	if ( (name == NULL) || (name[0] == 0) )
	{
		tempName[0] = 0;
		pb->dirInfo.ioNamePtr = tempName;
		pb->dirInfo.ioFDirIndex = -1;	/* use ioDirID */
	}
	else
	{
		pb->dirInfo.ioNamePtr = (StringPtr)name;
		pb->dirInfo.ioFDirIndex = 0;	/* use ioNamePtr and ioDirID */
	}
	pb->dirInfo.ioVRefNum = vRefNum;
	pb->dirInfo.ioDrDirID = dirID;
	error = PBGetCatInfoSync(pb);
	pb->dirInfo.ioNamePtr = NULL;
	return ( error );
}


/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

pascal OSErr GetDirectoryID(
			short vRefNum,
			long dirID,
			ConstStr255Param name,
			long *theDirID,
			Boolean *isDirectory)
{
	CInfoPBRec pb;
	OSErr error;

	error = GetCatInfoNoName(vRefNum, dirID, name, &pb);
	if ( error == noErr )
	{
		*isDirectory = (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0;
		if ( *isDirectory )
		{
			*theDirID = pb.dirInfo.ioDrDirID;
		}
		else
		{
			*theDirID = pb.hFileInfo.ioFlParID;
		}
	}
	
	return ( error );
}


/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

pascal OSErr FSpGetDirectoryID(
			const FSSpec *spec,
			long *theDirID,
			Boolean *isDirectory)
{
	return ( GetDirectoryID(spec->vRefNum, spec->parID, spec->name,
			 theDirID, isDirectory) );
}


/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

/*
**	GetVolumeInfoNoName uses pathname and vRefNum to call PBHGetVInfoSync
**	in cases where the returned volume name is not needed by the caller.
**	The pathname and vRefNum parameters are not touched, and the pb
**	parameter is initialized by PBHGetVInfoSync except that ioNamePtr in
**	the parameter block is always returned as NULL (since it might point
**	to the local tempPathname).
**
**	I noticed using this code in several places, so here it is once.
**	This reduces the code size of MoreFiles.
*/
pascal OSErr GetVolumeInfoNoName(
				ConstStr255Param pathname,
				short vRefNum,
				HParmBlkPtr pb)
{
	Str255 tempPathname;
	OSErr error;
	
	/* Make sure pb parameter is not NULL */ 
	if ( pb != NULL )
	{
		pb->volumeParam.ioVRefNum = vRefNum;
		if ( pathname == NULL )
		{
			pb->volumeParam.ioNamePtr = NULL;
			pb->volumeParam.ioVolIndex = 0;		/* use ioVRefNum only */
		}
		else
		{
			BlockMoveData(pathname, tempPathname, pathname[0] + 1);	/* make a copy of the string and */
			pb->volumeParam.ioNamePtr = (StringPtr)tempPathname;	/* use the copy so original isn't trashed */
			pb->volumeParam.ioVolIndex = -1;	/* use ioNamePtr/ioVRefNum combination */
		}
		error = PBHGetVInfoSync(pb);
		pb->volumeParam.ioNamePtr = NULL;	/* ioNamePtr may point to local	tempPathname, so don't return it */
	}
	else
	{
		error = paramErr;
	}
	return ( error );
}


/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

pascal OSErr DetermineVRefNum(
			ConstStr255Param pathname,
			short vRefNum,
			short *realVRefNum)
{
	HParamBlockRec pb;
	OSErr error;

	error = GetVolumeInfoNoName(pathname,vRefNum, &pb);
	if ( error == noErr )
	{
		*realVRefNum = pb.volumeParam.ioVRefNum;
	}
	return ( error );
}


/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/
#if 0
pascal OSErr GetParentID(
			short vRefNum,
			long dirID,
			ConstStr255Param name,
			long *parID)
{
	CInfoPBRec pb;
	Str31 tempName;
	OSErr error;
	short realVRefNum;
	
	/* Protection against File Sharing problem */
	if ( (name == NULL) || (name[0] == 0) )
	{
		tempName[0] = 0;
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioFDirIndex = -1;	/* use ioDirID */
	}
	else
	{
		pb.hFileInfo.ioNamePtr = (StringPtr)name;
		pb.hFileInfo.ioFDirIndex = 0;	/* use ioNamePtr and ioDirID */
	}
	pb.hFileInfo.ioVRefNum = vRefNum;
	pb.hFileInfo.ioDirID = dirID;
	error = PBGetCatInfoSync(&pb);
	if ( error == noErr )
	{
		/*
		**	There's a bug in HFS where the wrong parent dir ID can be
		**	returned if multiple separators are used at the end of a
		**	pathname. For example, if the pathname:
		**		'volumeName:System Folder:Extensions::'
		**	is passed, the directory ID of the Extensions folder is
		**	returned in the ioFlParID field instead of fsRtDirID. Since
		**	multiple separators at the end of a pathname always specifies
		**	a directory, we only need to work-around cases where the
		**	object is a directory and there are multiple separators at
		**	the end of the name parameter.
		*/
		if ( (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )
		{
			/* Its a directory */
			
			/* is there a pathname? */
			if ( pb.hFileInfo.ioNamePtr == name )	
			{
				/* could it contain multiple separators? */
				if ( name[0] >= 2 )
				{
					/* does it contain multiple separators at the end? */
					if ( (name[name[0]] == ':') && (name[name[0] - 1] == ':') )
					{
						/* OK, then do the extra stuff to get the correct parID */
						
						/* Get the real vRefNum (this should not fail) */
						error = DetermineVRefNum(name, vRefNum, &realVRefNum);
						if ( error == noErr )
						{
							/* we don't need the parent's name, but add protect against File Sharing problem */
							tempName[0] = 0;
							pb.dirInfo.ioNamePtr = tempName;
							pb.dirInfo.ioVRefNum = realVRefNum;
							/* pb.dirInfo.ioDrDirID already contains the */
							/* dirID of the directory object */
							pb.dirInfo.ioFDirIndex = -1;	/* get information about ioDirID */
							error = PBGetCatInfoSync(&pb);
							/* now, pb.dirInfo.ioDrParID contains the correct parID */
						}
					}
				}
			}
		}
		
		if ( error == noErr )
		{
			/* if no errors, then pb.hFileInfo.ioFlParID (pb.dirInfo.ioDrParID) */
			/* contains the parent ID */
			*parID = pb.hFileInfo.ioFlParID;
		}
	}
	
	return ( error );
}
#endif

/**********************************************************************
*
* Aus "MoreFilesExtras"
*
**********************************************************************/

pascal OSErr GetDirName(
			short vRefNum,
			long dirID,
			Str31 name)
{
	CInfoPBRec pb;
	OSErr error;

	if ( name != NULL )
	{
		pb.dirInfo.ioNamePtr = name;
		pb.dirInfo.ioVRefNum = vRefNum;
		pb.dirInfo.ioDrDirID = dirID;
		pb.dirInfo.ioFDirIndex = -1;	/* get information about ioDirID */
		error = PBGetCatInfoSync(&pb);
	}
	else
	{
		error = paramErr;
	}
	
	return ( error );
}

