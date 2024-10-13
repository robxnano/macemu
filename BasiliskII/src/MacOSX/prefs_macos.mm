/*
 *  prefs_macos.mm - Preferences handling, macOS specific stuff
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
#include <Cocoa/Cocoa.h>
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
static const char PREFS_FILE_NAME[]    = ".sheepshaver_prefs";
static const char AS_PREFS_FILE_NAME[] = "sheepshaver_prefs";
static const char XPRAM_FILE_NAME[]    = ".sheepshaver_nvram";
static const char AS_XPRAM_FILE_NAME[] = "sheepshaver_nvram";
static const char APP_SUPPORT_SUBDIR[] = "net.cebix.SheepShaver";
#else
static const char PREFS_FILE_NAME[]    = ".basilisk_ii_prefs";
static const char AS_PREFS_FILE_NAME[] = "basilisk_ii_prefs";
static const char XPRAM_FILE_NAME[]    = ".basilisk_ii_xpram";
static const char AS_XPRAM_FILE_NAME[] = "basilisk_ii_xpram";
static const char APP_SUPPORT_SUBDIR[] = "net.cebix.BasiliskII";
#endif

// Prefs file name and path
string UserPrefsPath;
static string prefs_name;
extern string xpram_name;

static const string& get_app_support_dir(void)
{
	static string app_support_dir;
	if (app_support_dir.empty()) {
		NSArray *paths = 
NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, 
NSUserDomainMask, YES);
               NSString *dir = [paths firstObject];
               app_support_dir = string([dir UTF8String]);
               free(paths);
               free(dir);
       }
       return app_support_dir;
}

static const string& get_home_dir(void)
{
	static string home_dir;
	if (home_dir.empty()) {
		char *env;
		if ((env = getenv("HOME")))
			home_dir = string(env);
		else
			home_dir = ".";
	}
	return home_dir;
}

static string get_dir(const string& path)
{
	int pos = path.find_last_of('/');
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
		fprintf(stderr, "ERROR: Cannot open %s (Is a directory)\n", prefs_name.c_str());
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
		printf("Using prefs file at %s\n", prefs_name.c_str());
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
 *  4. From ~/Library/Application Support/net.cebix.BasiliskII/basilisk_ii_prefs if it exists
 *  5. Create a new prefs file at ~/Library/Application Support/net.cebix.BasiliskII/basilisk_ii_prefs
 *  (or the equivalent paths for SheepShaver)
 */

void LoadPrefs(const char* vmdir)
{
	// vmdir was specified on the command line
	if (vmdir)
	{
		prefs_name = string(vmdir) + '/' + AS_PREFS_FILE_NAME;
		xpram_name = string(vmdir) + '/' + AS_XPRAM_FILE_NAME;
		if (load_prefs_file(prefs_name, true))
			return;
	}

	// --config was specified
	if (!UserPrefsPath.empty())
	{
		prefs_name = UserPrefsPath;
		xpram_name = get_dir(prefs_name) + '/' + XPRAM_FILE_NAME;
		if (load_prefs_file(prefs_name, true))
			return;
	}

	// Load .basilisk_ii_prefs from $HOME if it exists

	prefs_name = get_home_dir() + '/' + PREFS_FILE_NAME;
	xpram_name = get_home_dir() + '/' + XPRAM_FILE_NAME;
	if (load_prefs_file(prefs_name, false))
		return;

	// If no other prefs file exists, try the Application Support directory
	prefs_name = get_app_support_dir() + '/' + APP_SUPPORT_SUBDIR + '/' + AS_PREFS_FILE_NAME;
	xpram_name = get_app_support_dir() + '/' + APP_SUPPORT_SUBDIR + '/' + AS_XPRAM_FILE_NAME;
	if (load_prefs_file(prefs_name, false))
		return;

	// No prefs file, save defaults in Application Support directory
	printf("No prefs file found, creating new one at %s\n", prefs_name.c_str());
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
				int pos = path.find_last_of('/');
				if (pos == std::string::npos)
					return false;
				if (!create_directories(path.substr(0,pos),mode))
					return false;
			}
			return 0 == mkdir(path.c_str(),mode);

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
	string prefs_dir = get_dir(prefs_name);
	if (!prefs_dir.empty() && !is_dir(prefs_dir))
	{
		create_directories(prefs_dir, 0700);
	}
	if ((f = fopen(prefs_name.c_str(), "w")) != NULL)
	{
		SavePrefsToStream(f);
		fclose(f);
	}
	else
	{
		fprintf(stderr, "WARNING: Unable to save %s (%s)\n",
		        prefs_name.c_str(), strerror(errno));
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
	PrefsReplaceString("dsp", "/dev/dsp");
	PrefsReplaceString("mixer", "/dev/mixer");
	PrefsAddBool("idlewait", true);
}
