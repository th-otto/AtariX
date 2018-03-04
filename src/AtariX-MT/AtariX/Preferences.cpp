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
* Enthält alle Kommunikation mit den "Preferences"
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Globals.h"
#include "Debug.h"
#include "PascalStrings.h"
//#include "Preferences.h"
extern "C" {
#include "MyMoreFiles.h"
}

// Schalter
#define OLDOSVERSIONS	1


// statische Attribute:
FSSpec CPreferences::s_PrefsFolderFspec;

/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CPreferences::CPreferences()
{
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CPreferences::~CPreferences()
{
	Close();
}


/**********************************************************************
*
* Initialisierung: Ist seit CFPreferences überflüssig.
*
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

int CPreferences::Init(const char *name, OSType Creator, OSType PrefsType)
{
	return(noErr);
}


/**********************************************************************
*
* Öffnet die Preferences-Datei: Überflüssig seit CFPreferences
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

int CPreferences::Open(void)
{
	return(0);
}


/**********************************************************************
*
* Aktualisiert die Preferences-Datei, ohne sie zu schließen
* => 0 = OK, sonst = Fehler
*
**********************************************************************/

void CPreferences::Update(void)
{
	CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}


/**********************************************************************
*
* Schließt die Preferences-Datei: Seit CFPreferences überflüssig
*
**********************************************************************/

void CPreferences::Close(void)
{
}


/**********************************************************************
*
* Holt eine Zeichenkette (moderne Methode)
*
**********************************************************************/

void CPreferences::GetRsrcStr
(
	CFStringRef key,
	char *szData,
	const char *szDeflt,
	bool bAdd
)
{
	CFPropertyListRef thePropertyList;
	CFTypeID theType;
	const char *theString;
	CFStringRef theNewString;


	thePropertyList = CFPreferencesCopyAppValue(key,
								kCFPreferencesCurrentApplication);
	if (thePropertyList)
	{
		theType = CFGetTypeID(thePropertyList);
	}

	if ((thePropertyList) && (theType == CFStringGetTypeID()))
	{
		// the value has been found

		CFStringRef cfs = (CFStringRef) thePropertyList;
		theString = CFStringGetCStringPtr(cfs, kCFStringEncodingISOLatin1);

		if (theString)
			strcpy(szData, theString);
		else
		{
			// sometimes (?) we do not get the pointer and have to copy
			*szData = '\0';		// in case of failure
			(void) CFStringGetCString(cfs, szData, 64, kCFStringEncodingISOLatin1);
		}

		CFRelease(thePropertyList);
		return;
	}

	// The value has not been found or has the wrong type.
	// First, copy the default data as return value

	if (thePropertyList)
		CFRelease(thePropertyList);

	strcpy(szData, szDeflt);
	if (!bAdd)
		return;		// do not change preferences

	// update preferences, i.e. store default value

	// convert C string to CFString
	theNewString = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, szDeflt, kCFStringEncodingISOLatin1, kCFAllocatorNull);
	// store CFString
	CFPreferencesSetAppValue(key, (CFPropertyListRef) theNewString, kCFPreferencesCurrentApplication);

// CF_EXPORT CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
// CF_EXPORT CFStringRef CFStringCreateWithCStringNoCopy(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding, CFAllocatorRef contentsDeallocator);

// CF_EXPORT Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding);
// CF_EXPORT const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding);		/* May return NULL at any time; be prepared for NULL */

	CFRelease(theNewString);
}


/**********************************************************************
*
* Holt ein Alias, wenn vorhanden (moderne Methode)
*
**********************************************************************/

AliasHandle CPreferences::GetRsrcAlias(CFStringRef key)
{
	CFDataRef data;
	CFIndex dataSize;
	AliasHandle hdl;


	data = (CFDataRef) CFPreferencesCopyAppValue(
								key,
								kCFPreferencesCurrentApplication);

	if (!data)
		return(NULL);

	dataSize = CFDataGetLength(data);
	hdl = (AliasHandle) NewHandle(dataSize);
	if (hdl)
	{
		CFDataGetBytes(data,
					   CFRangeMake(0, dataSize),
					   (UInt8 *) *hdl);
	}

    CFRelease(data);
	return(hdl);
}


/**********************************************************************
*
* Setzt eine Zeichenkette (moderne Methode)
*
* Unterschied zur alten Methode: Die Zeichenkette wird auch dann
* gesetzt, wenn sie bisher nicht vorhanden war.
*
**********************************************************************/

void CPreferences::SetRsrcStr
(
	CFStringRef key,
	const char *szData
)
{
	CFStringRef theNewString;

	if (!szData)
	{
		// remove key
		CFPreferencesSetAppValue(key, NULL, kCFPreferencesCurrentApplication);
		return;
	}

	// convert C string to CFString
	theNewString = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, szData, kCFStringEncodingISOLatin1, kCFAllocatorNull);
	// store CFString
	CFPreferencesSetAppValue(key, (CFPropertyListRef) theNewString, kCFPreferencesCurrentApplication);

// CF_EXPORT CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
// CF_EXPORT CFStringRef CFStringCreateWithCStringNoCopy(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding, CFAllocatorRef contentsDeallocator);

// CF_EXPORT Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding);
// CF_EXPORT const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding);		/* May return NULL at any time; be prepared for NULL */

	CFRelease(theNewString);
}


/**********************************************************************
*
* Setzt ein Alias (moderne Methode)
*
* See Apple Technical Q&A QA1350
*
**********************************************************************/

void CPreferences::SetRsrcAlias(CFStringRef key, AliasHandle alias)
{
	CFDataRef data;
	Size dataSize;


	if (alias)
	{
		dataSize = GetHandleSize((Handle) alias);
		data = CFDataCreate(kCFAllocatorDefault,
							(UInt8 *) *alias,
							dataSize);
		if (data)
		{
			CFPreferencesSetAppValue(key, data, kCFPreferencesCurrentApplication);
			Update();
			CFRelease(data);
		}
		else
		{
			DebugError("Preferences::SetRsrcAlias() - Error");
		}
	}
	else
	{
		// virtual drive unused

		// value is NULL, remove key from preferences
		CFPreferencesSetAppValue(key, NULL, kCFPreferencesCurrentApplication);
		Update();
	}
}


/**********************************************************************
*
* Holt eine Zahl (moderne Methode)
*
**********************************************************************/

long CPreferences::GetRsrcNum(CFStringRef key, long deflt, bool bAdd)
{
	char s[256], sdeflt[256];

	ultoa10((unsigned long) deflt, sdeflt);
	GetRsrcStr(key, s, sdeflt, bAdd);
	return(atol(s));
/*
	CFIndex theValue;
	Boolean keyExistsAndHasValidFormat;

	theValue = CFPreferencesGetAppIntegerValue(key,
						kCFPreferencesCurrentApplication,
						&keyExistsAndHasValidFormat);
	if (keyExistsAndHasValidFormat)
		return((long) theValue);

	if (!bAdd)
		return(deflt);
	exit(-1);
	return(0);
*/
}


/**********************************************************************
*
* Holt ein Zeichenketten-Feld
* Rückgabe: Anzahl der Zeichenketten
*
* DOCTYPES: "APP,PRG,TTP,TOS=MgMx/Gem1"
*
**********************************************************************/

/* no longer used
void CPreferences::GetRsrcStringArray
(
	const unsigned char *name,
	char ***str,
	UInt16 *len,
	void *pDeflt,
	short DfltLen
)
{
	Handle hdl;
	short oldres;
	SInt16 theID;
	ResType theType;
	Str255 theString;
	SInt16 index;


	oldres = CurResFile();			// aktuelle Resourcedatei retten
	UseResFile(m_PrefsFile);		// Preferences durchsuchen
	hdl = Get1NamedResource('STR#', name);	// Resource suchen

	*len = 0;
	index = 0;

	if	((!hdl) && (pDeflt))
	{
		AddRsrc(name, 'STR#', pDeflt, DfltLen);
		hdl = Get1NamedResource('STR#', name);	// Resource suchen
	}

	if	(hdl)
	{
		GetResInfo(hdl, &theID, &theType,  theString);
		do
		{
			GetIndString(theString, theID, (short) (index+1));
			if	(theString[0])
			{
				if	(!*str)
				{
					*str = (char **)malloc(MAX_STR_ARRAY * sizeof(char *));
					if	(!(*str))
						return;
				}

				(*str)[index] = (char *) malloc((size_t) (theString[0] + 1));
				if	(!((*str)[index - 1]))
					return;
				p2cstrcpy((*str)[index], theString);
				index++;
			}
		}
		while((theString[0]) && (index < MAX_STR_ARRAY));

		*len = (UInt16) index;
		if	(index)
			realloc(*str, index * sizeof(char *));
		ReleaseResource(hdl);
	}

	UseResFile(oldres);
}
*/


/**********************************************************************
*
* Setzt eine Zahl
*
**********************************************************************/

void CPreferences::SetRsrcNum(CFStringRef key, long l)
{
	char s[256];

	ultoa10((unsigned long) l, s);
	SetRsrcStr(key, s);
}
