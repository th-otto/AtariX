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
* Druckerausgabe
*
*/

#include "config.h"
// System-Header
#include <Carbon/Carbon.h>
#include <machine/endian.h>
// Programm-Header
#include "Debug.h"
#include "Globals.h"
#include "MagicPrint.h"

// Schalter

// statische Variablen

bool CMagiCPrint::bTempFileCreated = false;


/**********************************************************************
*
* Konstruktor
*
**********************************************************************/

CMagiCPrint::CMagiCPrint()
{
	m_PrintFile = 0;
	m_PrintFileCounter = 0;
}


/**********************************************************************
*
* Destruktor
*
**********************************************************************/

CMagiCPrint::~CMagiCPrint()
{
	if (bTempFileCreated)
	{
		char command[1024];
		int ierr;

		// Lösche alle Druckdateien. Da das "rm"-Kommando versagt, wenn der
		// Pfad Leerzeichen enthält, muß der Pfad in Anführungszeichen
		// eingeschlossen werden. Dann wiederum funktioniert die Expansion
		// der Fragezeichen nicht. Folglich werden zwei Kommandos abgesetzt,
		// erst ein "cd" zum Wechsel in das Verzeichnis, in dem MagicMacX liegt,
		// dann ein rm auf alle Druckdateien im Verzeichnis "PrintQueue".

		sprintf(command, "cd \"%s\";rm .PrintQueue.MagiCPrintFile????????",
							CGlobals::s_ThisPathNameUnix);
		// Ausgabe-Umlenkung (stdout und stderr)
		strcat(command, " >&\"");
		strcat(command, CGlobals::s_ThisPathNameUnix);
		strcat(command, ".PrintQueue.PrintCommand_rm.txt\"");

		DebugInfo("CMagiCPrint::~CMagiCPrint() --- executing command: %s", command);
		// Lösch-Kommando absetzen
		ierr = system(command);
		if	(ierr)
		{
			DebugError("CMagiCPrint::~CMagiCPrint() --- error %d while deleting print files", ierr);
		}
	}
}


/**********************************************************************
*
* Ausgabestatus des Druckers abfragen
* Rückgabe: -1 = bereit 0 = nicht bereit
*
**********************************************************************/

UInt32 CMagiCPrint::GetOutputStatus(void)
{
	return(0xffffffff);		// bereit
}


/**********************************************************************
*
* Daten von Drucker lesen
* Rückgabe: Anzahl gelesener Zeichen
*
**********************************************************************/

UInt32 CMagiCPrint::Read(unsigned char *pBuf, UInt32 NumOfBytes)
{
#pragma unused(pBuf)
#pragma unused(NumOfBytes)
	return(0);
}


/**********************************************************************
*
* Mehrere Zeichen auf Drucker ausgeben
* Rückgabe: Anzahl geschriebener Zeichen
*
**********************************************************************/

UInt32 CMagiCPrint::Write(const unsigned char *pBuf, UInt32 cnt)
{
	char PrintFileName[256];
	long OutCnt;


	if	(!m_PrintFile)
	{
		// Druckdatei nicht mehr geöffnet => Neu anlegen

		sprintf(PrintFileName, ".PrintQueue.MagiCPrintFile%08d", m_PrintFileCounter++);
		DebugInfo("CMagiCPrint::Write() --- creating print file \"%s\" an", PrintFileName);
		m_PrintFile = creat(PrintFileName, 0644);
		if	(m_PrintFile <= 0)
		{
			m_PrintFile = 0;
			DebugError("CMagiCPrint::Write() --- error creating file: %s", strerror(errno));
			return(0);		// Fehler
		}
		bTempFileCreated = true;
	}

	// Zeichen in Druckdatei schreiben

	OutCnt = write(m_PrintFile, pBuf, cnt);

//	DebugInfo("CMagiCPrint::Write() --- s_LastPrinterAccess = %u", s_LastPrinterAccess);

	// Rückgabe: Anzahl geschriebener Zeichen

	return(cnt);
}


/**********************************************************************
*
* Aktuelle Druckdatei schließen und absenden.
* Rückgabe: Fehlercode
*
**********************************************************************/

UInt32 CMagiCPrint::ClosePrinterFile(void)
{
	char szPrintFileNameUnix[1024];
	char command2[4096];
	char command[4096];
	int ierr;
	char *src,*dst;


	if	(m_PrintFile == 0 || m_PrintFileCounter == 0)
		return(0);

	// Datei schließen

	close(m_PrintFile);
	m_PrintFile = 0;

	// Datei drucken

	// Dateinamen generieren. Dabei muß blöderweise der absolute Pfad eingesetzt
	// werden, da system() immer im Wurzelverzeichnis läuft.
	// Dateinamen in Anführungszeichen setzen, da der Pfadname Leerzeichen enthalten kann.
	sprintf(szPrintFileNameUnix, "\"%s.PrintQueue.MagiCPrintFile%08d\"",
						CGlobals::s_ThisPathNameUnix,
						m_PrintFileCounter - 1);

	// unseren Pfad ggf. in das Benutzerkommando einsetzen (%APPDIR%)

	dst = command2;
	src = CGlobals::s_Preferences.m_szPrintingCommand;
	do
	{
		if	(!strncmp(src, "%APPDIR%", 8))
		{
			strcpy(dst, CGlobals::s_ThisPathNameUnix);
			dst += strlen(dst);
			src += 8;
		}
		else
			*dst++ = *src++;
	}
	while(*src);
	*dst = '\0';

	// Druck-Dateinamen in das Benutzerkommando einsetzen
	sprintf(command, command2, szPrintFileNameUnix);

	// Ausgabe-Umlenkung (stdout und stderr)
	strcat(command, " >&\"");
	strcat(command, CGlobals::s_ThisPathNameUnix);
	strcat(command, ".PrintQueue.PrintCommand_stdout.txt\"");

	DebugInfo("CMagiCPrint::ClosePrinterFile() --- executing command: %s", command);
	// Test. ob MacOS-Fehler noch drin ist.
	// (void) system("pwd >/tmp/hallo.txt");
	// Druck-Kommando absetzen
	ierr = system(command);
	if	(ierr)
	{
		DebugError("CMagiCPrint::ClosePrinterFile() --- error %d while printing", ierr);
	}
	return(0);
}
