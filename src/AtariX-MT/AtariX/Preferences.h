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

#define MAX_STR_ARRAY		100

class CPreferences
{
   public:
	// Konstruktor
	CPreferences();
	// Destruktor
	~CPreferences();
	// Initialisierung
	int Init(const char *name, OSType Creator, OSType PrefsType);
//	void SetFSSpec(FSSpec *spec);
	int Open(void);
	void Update(void);
	void Close(void);

	void GetRsrcStr(CFStringRef key, char *szData, const char *szDeflt, bool bAdd = true);
	long GetRsrcNum(CFStringRef key, long deflt, bool bAdd = true);

//	void GetRsrcStringArray(const unsigned char *name, char ***str, UInt16 *len, void *pDeflt, short DfltLen);
	AliasHandle GetRsrcAlias(CFStringRef key);
	void SetRsrcStr(CFStringRef key, const char *szData);
	void SetRsrcNum (CFStringRef key, long l);
	void SetRsrcAlias(CFStringRef key, AliasHandle alias);

   private:
	// Funktionen
	// Attribute
	OSType m_Creator;
	OSType m_PrefsType;
	// statische Attribute
	static FSSpec s_PrefsFolderFspec;
	// statische Funktionen
};