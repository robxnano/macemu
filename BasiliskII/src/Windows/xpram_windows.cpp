/*
 *  xpram_windows.cpp - XPRAM handling, Windows specific stuff
 *
 *  Basilisk II (C) 1997-2008 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"

#include <string>

#include "xpram.h"


// XPRAM file name and path
#if SHEEPSHAVER
const wchar_t XPRAM_FILE_NAME[] = L"SheepShaver_nvram.dat";
#else
const wchar_t XPRAM_FILE_NAME[] = L"BasiliskII_xpram.dat";
#endif
extern std::wstring UserPrefsPath; // From prefs_windows.cpp
static std::wstring xpram_path;

static std::wstring get_dir(std::wstring& path)
{
	size_t pos = path.find_last_of(L'\\');
	if (pos == 0)
		return L""; // file is in root folder
	if (pos == std::wstring::npos)
		return L"."; // file is in current folder
	return path.substr(0, pos);
}

/*
 *  Construct XPRAM path
 */

static void build_xpram_path(void)
{
	xpram_path.clear();
	xpram_path = get_dir(UserPrefsPath) + L'\\' += XPRAM_FILE_NAME;
}


/*
 *  Load XPRAM from settings file
 */

void LoadXPRAM(const char *vmdir)
{
	// Construct XPRAM path
	build_xpram_path();

	// Load XPRAM from settings file
	HANDLE fh = CreateFileW(xpram_path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fh != INVALID_HANDLE_VALUE) {
		DWORD bytesRead;
		ReadFile(fh, XPRAM, XPRAM_SIZE, &bytesRead, NULL);
		CloseHandle(fh);
	}
}


/*
 *  Save XPRAM to settings file
 */

void SaveXPRAM(void)
{
	HANDLE fh = CreateFileW(xpram_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (fh != INVALID_HANDLE_VALUE) {
		DWORD bytesWritten;
		WriteFile(fh, XPRAM, XPRAM_SIZE, &bytesWritten, NULL);
		CloseHandle(fh);
	}
}


/*
 *  Delete PRAM file
 */

void ZapPRAM(void)
{
	// Construct PRAM path
	build_xpram_path();

	// Delete file
	DeleteFileW(xpram_path.c_str());
}
