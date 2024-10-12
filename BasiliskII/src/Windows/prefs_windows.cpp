/*
 *  prefs_windows.cpp - Preferences handling, Windows specific stuff
 *
 *  Basilisk II, SheepShaver (C) 1997-2008 Christian Bauer
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <shlobj.h>

#include <string>

#include "prefs.h"

// Platform-specific preferences items
prefs_desc platform_prefs_items[] = {
#ifdef SHEEPSHAVER
	{"ether", TYPE_STRING, false,          "device name of Mac ethernet adapter"},
#endif
	{"keycodes", TYPE_BOOLEAN, false,      "use keycodes rather than keysyms to decode keyboard"},
	{"keycodefile", TYPE_STRING, false,    "path of keycode translation file"},
	{"mousewheelmode", TYPE_INT32, false,  "mouse wheel support mode (0=page up/down, 1=cursor up/down)"},
	{"mousewheellines", TYPE_INT32, false, "number of lines to scroll in mouse wheel mode 1"},
	{"idlewait", TYPE_BOOLEAN, false,      "sleep when idle"},
	{"enableextfs", TYPE_BOOLEAN, false,   "enable extfs system"},
	{"debugextfs", TYPE_BOOLEAN, false,    "debug extfs system"},
	{"extdrives", TYPE_STRING, false,      "define allowed extfs drives"},
	{"pollmedia", TYPE_BOOLEAN, false,     "poll for new media (e.g. cd, floppy)"},
	{"etherguid", TYPE_STRING, false,      "GUID of the ethernet device to use"},
	{"etherpermanentaddress", TYPE_BOOLEAN, false,  "use permanent NIC address to identify itself"},
	{"ethermulticastmode", TYPE_INT32, false,       "how to multicast packets"},
	{"etherfakeaddress", TYPE_STRING, false,        "optional fake hardware address"},
	{"routerenabled", TYPE_BOOLEAN, false,          "enable NAT/Router module"},
	{"ftp_port_list", TYPE_STRING, false,           "FTP ports list"},
	{"tcp_port", TYPE_STRING, false,                "TCP ports list"},
	{"portfile0", TYPE_STRING, false,               "output file for serial port 0"},
	{"portfile1", TYPE_STRING, false,               "output file for serial port 1"},
#ifdef USE_SDL_VIDEO
	{"sdlrender", TYPE_STRING, false,      "SDL_Renderer driver (\"auto\", \"software\" (may be faster), etc.)"},
	{"sdl_vsync", TYPE_BOOLEAN, false,     "Make SDL_Renderer vertical sync frames to host (eg. with software renderer)"},
#endif
	{"reservewindowskey", TYPE_BOOLEAN, false,      "block Windows key from activating start menu"},

	{NULL, TYPE_END, false, NULL} // End of list
};


// Prefs file name and path
#ifdef SHEEPSHAVER
const wchar_t PREFS_FILE_NAME[] = L"SheepShaver_prefs";
const wchar_t APPDATA_SUBDIR[] = L"SheepShaver";
#else
const wchar_t PREFS_FILE_NAME[] = L"BasiliskII_prefs";
const wchar_t APPDATA_SUBDIR[] = L"BasiliskII";
#endif

std::wstring UserPrefsPath;

const std::wstring& get_appdata_dir(void)
{
	static std::wstring dir;
	if (dir.empty())
	{
		PWSTR wdir = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0,
		                                   nullptr, &wdir)))
		{
			dir = std::wstring(wdir);
		}
		CoTaskMemFree(wdir);
	}
	return dir;
}

static std::wstring get_dir(std::wstring& path)
{
	size_t pos = path.find_last_of(L'\\');
	if (pos == 0)
		return L""; // file is in root folder
	if (pos == std::wstring::npos)
		return L"."; // file is in current folder
	return path.substr(0, pos);
}

static bool is_dir(const std::wstring& path)
{
	struct _stat info;
	if (_wstat(path.c_str(), &info) != 0){
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
}

static bool create_directories(const std::wstring& path)
{
	if (_wmkdir(path.c_str()) == 0)
		return true;

	switch (errno)
	{
		case ENOENT:
			{
				size_t pos = path.find_last_of('\\');
				if (pos == std::wstring::npos)
					return false;
				if (!create_directories(path.substr(0, pos)))
					return false;
			}
			return 0 == _wmkdir(path.c_str());

		case EEXIST:
			return is_dir(path);
		default:
			return false;
	}
}

/*
 *  Look for prefs file in the following locations (in order of priority):
 *  1. From .\BasiliskII_prefs if it exists
 *  2. From %APPDATA%\BasiliskII\BasiliskII_prefs if it exists
 *  3. Create a new prefs file at %APPDATA%\BasiliskII\prefs
 *  (or the equivalent paths for SheepShaver)
 */

void LoadPrefs(const char *vmdir)
{
	std::wstring prefs_path;
	// Construct prefs path
	if (UserPrefsPath.empty()) {
		uint32_t pwd_len = GetCurrentDirectoryW(0, NULL);
		wchar_t *pwd = new wchar_t[pwd_len];
		if (GetCurrentDirectoryW(pwd_len, pwd) == pwd_len - 1)
			prefs_path = std::wstring(pwd) + L'\\';
		delete[] pwd;

		prefs_path += PREFS_FILE_NAME;
	} else
		prefs_path = UserPrefsPath;

	// Read preferences from settings file
	FILE *f = _wfopen(prefs_path.c_str(), L"r");
	if (f != NULL) {

		wprintf(L"Using prefs file at %ls\n", prefs_path.c_str());
		UserPrefsPath = prefs_path;
		// Prefs file found, load settings
		LoadPrefsFromStream(f);
		fclose(f);
		return;

	}

	prefs_path = get_appdata_dir() + L'\\' + APPDATA_SUBDIR + L'\\' + PREFS_FILE_NAME;
	f = _wfopen(prefs_path.c_str(), L"r");
	if (f != NULL) {

		wprintf(L"Using prefs file at %ls\n", prefs_path.c_str());
		// Prefs file found, load settings
		UserPrefsPath = prefs_path;
		LoadPrefsFromStream(f);
		fclose(f);
		return;

	} else {
		// No prefs file, save defaults
		wprintf(L"No prefs file found, creating new one at %ls\n", prefs_path.c_str());
		UserPrefsPath = prefs_path;
		SavePrefs();
	}
}


/*
 *  Save preferences to settings file
 */

void SavePrefs(void)
{
	FILE *f;
	std::wstring prefs_dir = get_dir(UserPrefsPath);
	if (!prefs_dir.empty() && !is_dir(prefs_dir))
	{
		create_directories(prefs_dir);
	}
	if ((f = _wfopen(UserPrefsPath.c_str(), L"w")) != NULL) {
		SavePrefsToStream(f);
		fclose(f);
	}
}


/*
 *  Add defaults of platform-specific prefs items
 *  You may also override the defaults set in PrefsInit()
 */

void AddPlatformPrefsDefaults(void)
{
#ifdef SHEEPSHAVER
	PrefsAddInt32("windowmodes", 3);
	PrefsAddInt32("screenmodes", 0x3f);
	PrefsReplaceBool("routerenabled", false);
#endif
	PrefsAddBool("keycodes", false);
	PrefsReplaceBool("pollmedia", true);
	PrefsReplaceBool("enableextfs", false);
	PrefsReplaceString("extfs", "");
	PrefsReplaceString("extdrives", "CDEFGHIJKLMNOPQRSTUVWXYZ");
	PrefsReplaceInt32("mousewheelmode", 1);
	PrefsReplaceInt32("mousewheellines", 3);
	PrefsAddBool("idlewait", true);
	PrefsReplaceBool("etherpermanentaddress", true);
	PrefsReplaceInt32("ethermulticastmode", 0);
	PrefsReplaceString("ftp_port_list", "21");
	PrefsReplaceString("seriala", "COM1");
	PrefsReplaceString("serialb", "COM2");
	PrefsReplaceString("portfile0", "C:\\B2TEMP0.OUT");
	PrefsReplaceString("portfile1", "C:\\B2TEMP1.OUT");
#ifdef USE_SDL_VIDEO
	PrefsReplaceString("sdlrender", "software");
	PrefsReplaceBool("sdl_vsync", false);
#endif
	PrefsAddBool("reservewindowskey", false);
}
