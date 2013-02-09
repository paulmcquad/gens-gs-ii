/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * W32U_mini.c: Win32 Unicode Translation Layer. (Mini Version)            *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2010 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "W32U_mini.h"

// C includes.
#include <stdlib.h>


/**
 * Indicates if the system is Unicode.
 * NOTE: Do NOT edit this variable outside of W32U!
 */
int W32U_IsUnicode = 0;


/**
 * Initialize the Win32 Unicode Translation Layer.
 * @return 0 on success; non-zero on error.
 */
int W32U_Init(void)
{
	W32U_IsUnicode = (GetModuleHandleW(nullptr) != nullptr);
	return 0;
}


/**
 * Shut down the Win32 Unicode Translation Layer.
 * @return 0 on success; non-zero on error.
 */
int W32U_End(void)
{
	W32U_IsUnicode = 0;
	return 0;
}


/**
 * Convert a null-terminated multibyte string to UTF-16.
 * @param mbs Multibyte string. (null-terminated)
 * @param codepage mbs codepage.
 * @return UTF-16 string, or NULL on error.
 */
wchar_t *W32U_mbs_to_UTF16(const utf8_str *mbs, unsigned int codepage)
{
	int cchWcs = MultiByteToWideChar(codepage, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	wchar_t *wcs = (wchar_t*)malloc(cchWcs * sizeof(wchar_t));
	MultiByteToWideChar(codepage, 0, mbs, -1, wcs, cchWcs);
	return wcs;
}


/**
 * Convert a null-terminated UTF-16 string to multibyte.
 * @param wcs UTF-16 string. (null-terminated)
 * @param codepage mbs codepage.
 * @return Multibyte string, or NULL on error.
 */
char *W32U_UTF16_to_mbs(const wchar_t *wcs, unsigned int codepage)
{
	int cbMbs = WideCharToMultiByte(codepage, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0)
		return nullptr;

	char *mbs = (char*)malloc(cbMbs);
	WideCharToMultiByte(codepage, 0, wcs, -1, mbs, cbMbs, nullptr, nullptr);
	return mbs;
}


// Make sure fopen() isn't redefined.
#ifdef fopen
#undef fopen
#endif

/**
 * Open a file.
 * @param filename Filename.
 * @param mode File mode.
 * @return File pointer, or NULL on error.
 */
FILE *W32U_fopen(const utf8_str *filename, const utf8_str *mode)
{
	// Convert the filename from UTF-8 to UTF-16.
	wchar_t *filenameW = W32U_mbs_to_UTF16(filename, CP_UTF8);
	if (!filenameW)
		return nullptr;

	// Convert the mode from UTF-8 to UTF-16.
	wchar_t *modeW = W32U_mbs_to_UTF16(mode, CP_UTF8);
	if (!modeW) {
		free(filenameW);
		return nullptr;
	}

	FILE *fRet = nullptr;
	if (W32U_IsUnicode) {
		// Unicode version.
		fRet = _wfopen(filenameW, modeW);
	} else {
		// ANSI version.

		// Convert the filename from UTF-16 to ANSI.
		char *filenameA = W32U_UTF16_to_mbs(filenameW, CP_ACP);
		if (!filenameA)
			goto fail;

		// Convert the mode from UTF-16 to ANSI.
		char *modeA = W32U_UTF16_to_mbs(modeW, CP_ACP);
		if (!modeA) {
			free(filenameA);
			goto fail;
		}

		// Open the file.
		fRet = fopen(filenameA, modeA);
		free(filenameA);
		free(modeA);
	}

fail:
	free(filenameW);
	free(modeW);
	return fRet;
}
