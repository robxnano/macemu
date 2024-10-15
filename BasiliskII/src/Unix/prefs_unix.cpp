/*
 *  prefs_unix.cpp - Preferences handling, Unix specific stuff
 *
 *  Basilisk II, SheepShaver (C) 1997-2008 Christian Bauer and Marc Hellwig
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
#include <sys/stat.h>
#include <errno.h>
#include <string>
using std::string;

#include "prefs.h"

// Platform-specific preferences items
prefs_desc platform_prefs_items[] = {
#ifdef SHEEPSHAVER
	{"ether", TYPE_STRING, false,          "device name of Mac ethernet adapter"},
	{"etherconfig", TYPE_STRING, false,    "path of network config script"},
	{"keycodes", TYPE_BOOLEAN, false,      "use keycodes rather than keysyms to decode keyboard"},
	{"keycodefile", TYPE_STRING, false,    "path of keycode translation file"},
	{"mousewheelmode", TYPE_INT32, false,  "mouse wheel support mode (0=page up/down, 1=cursor up/down)"},
	{"mousewheellines", TYPE_INT32, false, "number of lines to scroll in mouse wheel mode 1"},
#else
	{"fbdevicefile", TYPE_STRING, false,   "path of frame buffer device specification file"},
#endif
	{"dsp", TYPE_STRING, false,            "audio output (dsp) device name"},
	{"mixer", TYPE_STRING, false,          "audio mixer device name"},
	{"idlewait", TYPE_BOOLEAN, false,      "sleep when idle"},
#ifdef USE_SDL_VIDEO
	{"sdlrender", TYPE_STRING, false,      "SDL_Renderer driver (\"auto\", \"software\" (may be faster), etc.)"},
#endif
	{NULL, TYPE_END, false, NULL} // End of list
};


// Standard file names and paths
#ifdef SHEEPSHAVER
static const char PREFS_FILE_NAME[] = ".sheepshaver_prefs";
static const char XDG_PREFS_FILE_NAME[] = "prefs";
static const char XPRAM_FILE_NAME[] = ".sheepshaver_nvram";
static const char XDG_XPRAM_FILE_NAME[] = "nvram";
static const char XDG_CONFIG_SUBDIR[] = "SheepShaver";
#else
static const char PREFS_FILE_NAME[] = ".basilisk_ii_prefs";
static const char XDG_PREFS_FILE_NAME[] = "prefs";
static const char XPRAM_FILE_NAME[] = ".basilisk_ii_xpram";
static const char XDG_XPRAM_FILE_NAME[] = "xpram";
static const char XDG_CONFIG_SUBDIR[] = "BasiliskII";
#endif

// Prefs file name and path
string UserPrefsPath;
extern string xpram_name;

static const string& get_home_dir(void)
{
	static string home_dir;
	if (home_dir.empty()) {
		char *env;
		if ((env = getenv("HOME")))
			home_dir = string(env);
		else
			home_dir = "."; // last resort, use the current directory
	}
	return home_dir;
}

static const string& get_xdg_config_dir(void)
{
	static string xdg_config_dir;
	if (xdg_config_dir.empty()) {
		char *env;
		if ((env = getenv("XDG_CONFIG_HOME")))
			xdg_config_dir = string(env) + '/' + XDG_CONFIG_SUBDIR;
		else
			xdg_config_dir = get_home_dir() + "/.config/" + XDG_CONFIG_SUBDIR;
	}
	return xdg_config_dir;
}

static string get_dir(const string& path)
{
	size_t pos = path.find_last_of('/');
	if (pos == 0)
		return ""; // file is in root folder
	if (pos == std::string::npos)
		return "."; // file is in current folder
	return path.substr(0, pos);
}

static void exit_if_dir(const string& path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0){
		return;
	}
	if ((info.st_mode & S_IFDIR) != 0)
	{
		fprintf(stderr, "ERROR: Cannot open %s (Is a directory)\n", path.c_str());
		exit(1);
	}
}

static bool load_prefs_file(const string& path, bool exit_on_failure)
{
	exit_if_dir(path);
	FILE *prefs = fopen(path.c_str(), "r");
	if (prefs != NULL)
	{
		LoadPrefsFromStream(prefs);
		fclose(prefs);
		printf("Using prefs file at %s\n", path.c_str());
		return true;
	}
	else if (exit_on_failure)
	{
		fprintf(stderr, "ERROR: Could not load prefs file from %s (%s)\n",
		        path.c_str(), strerror(errno));
		exit(1);
	}
	return false;
}

/*
 *  Look for prefs file in the following locations (in order of priority):
 *  1. From vmdir/.basilisk_ii_prefs if a vmdir has been specified
 *  2. From path specified with --config command line
 *  3. From $HOME/.basilisk_ii_prefs if it exists
 *  4. From $XDG_CONFIG_HOME/BasiliskII/prefs if it exists
 *  5. Create a new prefs file at $XDG_CONFIG_HOME/BasiliskII/prefs
 *  (or the equivalent paths for SheepShaver)
 *  If $XDG_CONFIG_HOME doesn't exist, $HOME/.config is used instead,
 *  in accordance with XDG Base Directory Specification:
 *  https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */

void LoadPrefs(const char* vmdir)
{
	// vmdir was specified on the command line
	if (vmdir)
	{
		UserPrefsPath = string(vmdir) + '/' + XDG_PREFS_FILE_NAME;
		xpram_name = string(vmdir) + '/' + XDG_XPRAM_FILE_NAME;
		if (load_prefs_file(UserPrefsPath, true))
			return;
	}

	// --config was specified
	if (!UserPrefsPath.empty())
	{
		xpram_name = get_dir(UserPrefsPath) + '/' + XPRAM_FILE_NAME;
		if (load_prefs_file(UserPrefsPath, true))
			return;
	}

	// Load .basilisk_ii_prefs from $HOME if it exists
	UserPrefsPath = get_home_dir() + '/' + PREFS_FILE_NAME;
	xpram_name = get_home_dir() + '/' + XPRAM_FILE_NAME;
	if (load_prefs_file(UserPrefsPath, false))
		return;

	// If no other prefs file exists, try the $XDG_CONFIG_HOME directory
	UserPrefsPath = get_xdg_config_dir() + '/' + XDG_PREFS_FILE_NAME;
	xpram_name = get_xdg_config_dir() + '/' + XDG_XPRAM_FILE_NAME;
	if (load_prefs_file(UserPrefsPath, false))
		return;

	// No prefs file, save defaults in $XDG_CONFIG_HOME directory
#ifdef __linux__
	PrefsAddString("cdrom", "/dev/cdrom");
#endif
	printf("No prefs file found, creating new one at %s\n", UserPrefsPath.c_str());
	SavePrefs();
}

static bool is_dir(const string& path)
{
	struct stat info;
	if (stat(path.c_str(), &info) != 0){
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
}

static bool create_directories(const string& path, mode_t mode)
{
	if (mkdir(path.c_str(), mode) == 0)
		return true;

	switch (errno)
	{
		case ENOENT:
			{
				size_t pos = path.find_last_of('/');
				if (pos == std::string::npos)
					return false;
				if (!create_directories(path.substr(0, pos), mode))
					return false;
			}
			return 0 == mkdir(path.c_str(), mode);

		case EEXIST:
			return is_dir(path);
		default:
			return false;
	}
}

/*
 *  Save preferences to settings file
 */


void SavePrefs(void)
{
	FILE *f;
	string prefs_dir = get_dir(UserPrefsPath);
	if (!prefs_dir.empty() && !is_dir(prefs_dir))
	{
		create_directories(prefs_dir, 0700);
	}
	if ((f = fopen(UserPrefsPath.c_str(), "w")) != NULL)
	{
		SavePrefsToStream(f);
		fclose(f);
	}
	else
	{
		fprintf(stderr, "WARNING: Unable to save %s (%s)\n",
		        UserPrefsPath.c_str(), strerror(errno));
	}
}


/*
 *  Add defaults of platform-specific prefs items
 *  You may also override the defaults set in PrefsInit()
 */

void AddPlatformPrefsDefaults(void)
{
	PrefsAddBool("keycodes", false);
	PrefsReplaceString("extfs", "/");
	PrefsReplaceInt32("mousewheelmode", 1);
	PrefsReplaceInt32("mousewheellines", 3);
#ifdef __linux__
	if (access("/dev/sound/dsp", F_OK) == 0)
	{
		PrefsReplaceString("dsp", "/dev/sound/dsp");
	}
	else
	{
		PrefsReplaceString("dsp", "/dev/dsp");
	}
	if (access("/dev/sound/mixer", F_OK) == 0)
	{
		PrefsReplaceString("mixer", "/dev/sound/mixer");
	}
	else
	{
		PrefsReplaceString("mixer", "/dev/mixer");
	}
#elif defined (__NetBSD__)
	PrefsReplaceString("dsp", "/dev/audio");
	PrefsReplaceString("mixer", "/dev/mixer");
#else
	PrefsReplaceString("dsp", "/dev/dsp");
	PrefsReplaceString("mixer", "/dev/mixer");
#endif
	PrefsAddBool("idlewait", true);
}
