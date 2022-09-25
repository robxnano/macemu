/*
 *  prefs_editor_linux.cpp - Preferences editor, Linux implementation using GTK+
 *
 *  SheepShaver (C) 1997-2008 Christian Bauer and Marc Hellwig
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "user_strings.h"
#include "version.h"
#include "cdrom.h"
#include "xpram.h"
#include "prefs.h"
#include "prefs_editor.h"
#include "vm_alloc.h"
#include "g_resources.h"

#if defined(USE_SDL_AUDIO) || defined(USE_SDL_VIDEO)
#include <SDL.h>
#endif

#define DEBUG 0
#include "debug.h"


// Global variables
static GtkBuilder *builder;
static GtkWidget *win;				// Preferences window
static bool start_clicked = false;	// Return value of PrefsEditor() function
static int screen_width, screen_height; // Screen dimensions


static GtkComboBox *screen_mode;
static GtkComboBox *screen_x;
static GtkComboBox *screen_y;

static GtkWidget *volumes_view;
static GtkTreeModel *volume_store;
static GtkTreeIter toplevel;
static GtkTreeSelection *selection;

// Prototypes

static GtkWidget *create_tree_view (void);
static void cb_add_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void cb_create_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void cb_remove_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);

#if REAL_ADDRESSING
const char* INFO_ADDRESSING = "Addressing mode: Real";
#else
const char* INFO_ADDRESSING = "Addressing mode: Direct";
#endif

#if defined(USE_SDL_AUDIO) && defined(USE_SDL_VIDEO)
#if SDL_VERSION_ATLEAST(2,0,0)
const char* INFO_VIDEO = "SDL 2 audio";
#else
const char* INFO_VIDEO = "SDL 1.2 audio";
#endif
const char* INFO_AUDIO = "video";
#else
#ifdef USE_SDL_AUDIO
#if SDL_VERSION_ATLEAST(2,0,0)
const char* INFO_AUDIO = "SDL 2 audio";
#else
const char* INFO_AUDIO = "SDL 1.2 audio";
#endif
#elif defined(ESD_AUDIO)
const char* INFO_AUDIO = "ESD audio";
#else
const char* INFO_AUDIO = "no audio";
#endif
#ifdef USE_SDL_VIDEO
#if SDL_VERSION_ATLEAST(2,0,0)
const char* INFO_VIDEO = "SDL 2 video";
#else
const char* INFO_VIDEO = "SDL 1.2 video";
#endif
#else
const char* INFO_VIDEO = "X11 video";
#endif
#endif


/*
 *  Utility functions
 */

#if ! GLIB_CHECK_VERSION(2,0,0)
#define G_OBJECT(obj)							GTK_WIDGET(obj)
#define g_object_get_data(obj, key)				gtk_object_get_data((obj), (key))
#define g_object_set_data(obj, key, data)		gtk_object_set_data((obj), (key), (data))
#endif

struct opt_desc {
	int label_id;
	GCallback func;
};

struct combo_desc {
	int label_id;
};

// User closed the file chooser dialog, possibly selecting a file
static void cb_browse_response(GtkWidget *chooser, int response, GtkEntry *entry)
{
	if (response == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_entry_set_text(GTK_ENTRY(entry), filename);
		g_free (filename);
	}
	gtk_widget_destroy (chooser);
}

// Open the file chooser dialog to select a file
extern "C" void cb_browse(GtkWidget *button, GtkWidget *entry)
{
	GtkWidget *chooser = gtk_file_chooser_dialog_new(GetString(STR_BROWSE_TITLE),
							GTK_WINDOW(win),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							"Cancel", GTK_RESPONSE_CANCEL,
							"Open", GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(chooser), GTK_RESPONSE_ACCEPT);
	gtk_window_set_transient_for(GTK_WINDOW(chooser), GTK_WINDOW(win));
	gtk_window_set_modal(GTK_WINDOW(chooser), true);
	g_signal_connect(chooser, "response", G_CALLBACK(cb_browse_response), GTK_ENTRY(entry));
	gtk_widget_show(chooser);
}

// Open the file chooser dialog to select a folder
extern "C" void cb_browse_dir(GtkWidget *button, GtkWidget *entry)
{
	GtkWidget *chooser = gtk_file_chooser_dialog_new(GetString(STR_BROWSE_FOLDER_TITLE),
							GTK_WINDOW(win),
							GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
							"Cancel", GTK_RESPONSE_CANCEL,
							"Select", GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(chooser), GTK_RESPONSE_ACCEPT);
	gtk_window_set_transient_for(GTK_WINDOW(chooser), GTK_WINDOW(win));
	gtk_window_set_modal(GTK_WINDOW(chooser), true);
	g_signal_connect(chooser, "response", G_CALLBACK(cb_browse_response), GTK_WIDGET(entry));
	gtk_widget_show(chooser);
}

// User changed a file entry setting
static void cb_file_entry(GtkEntry *entry, char *pref)
{
	PrefsReplaceString(pref, gtk_entry_get_text(entry));
}

// User changed one of the screen mode settings
extern "C" void cb_screen_mode(GtkWidget *widget)
{
	const char *str_x = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(screen_x));
	int int_x = atoi(str_x);
	if (int_x < 0) int_x = 0;

	const char *str_y = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(screen_y));
	int int_y = atoi(str_y);
	if (int_y < 0) int_y = 0;

	char screen[256];
#ifdef ENABLE_FBDEV_DGA
	const char *str = gtk_combo_box_get_active(screen_mode) ? "fbdev/%d/%d" : "win/%d/%d";
#else
	const char *str = gtk_combo_box_get_active(screen_mode) ? "dga/%d/%d" : "win/%d/%d";
#endif
	sprintf(screen, str, int_x, int_y);
	PrefsReplaceString("screen", screen);
	// Old prefs are now migrated
	PrefsRemoveItem("windowmodes");
	PrefsRemoveItem("screenmodes");
}

static void set_ramsize_combo_box(void)
{
	const char *name = "ramsize";
	int size_mb = PrefsFindInt32(name) >> 20;
	GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, name));
	char id[32];
	snprintf(id, 32, "%d", size_mb);
	if(!gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id))
	{
		gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(combo), id, id);
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id);
	}
}

static void set_hotkey_buttons(void)
{
	GtkToggleButton *ctrl = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "ctrl-hotkey"));
	GtkToggleButton *alt = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "alt-hotkey"));
	GtkToggleButton *super = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "super-hotkey"));
	int32 hotkey = PrefsFindInt32("hotkey");
	if (hotkey == 0)
		hotkey = 1;
	gtk_toggle_button_set_active(ctrl, hotkey & 1);
	gtk_toggle_button_set_active(alt, hotkey & 2);
	gtk_toggle_button_set_active(super, hotkey & 4);
}

static void set_cpu_combo_box (void)
{
	GtkComboBox *combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cpu"));
	int32 cpu = PrefsFindInt32("cpu");
	bool fpu = PrefsFindBool("fpu");
	char id[32];
	sprintf(id, "%d", (cpu << 1 | fpu));
	gtk_combo_box_set_active_id(combo, id);
}

static GtkWidget *add_combo_box_values(GList *entries, const char *pref)
{
	GtkWidget *combo = GTK_WIDGET(gtk_builder_get_object(builder, pref));
	const char *current = PrefsFindString(pref);
	GList *list;
	list = entries;
	while (list)
	{
		gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(combo), ((gchar *) list->data), ((gchar *) list->data));
		list = list->next;
	}

	if (pref != NULL && !gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), current))
	{
		gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(combo), current, current);
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), current);
	}
	return combo;
}

/*
 *  Show preferences editor
 *  Returns true when user clicked on "Start", false otherwise
 */

// Window closed
static gint window_closed(void)
{
	return FALSE;
}

// Window destroyed
static void window_destroyed(void)
{
	gtk_main_quit();
}

// Emulator is ready to start
static void run_emulator()
{
	start_clicked = true;
	save_volumes();
	SavePrefs();
	gtk_widget_destroy(win);
}
#if REAL_ADDRESSING && defined(__linux__)

// Checks if mmap from zero is possible
static bool can_mmap_real()
{
	bool ret = false;
	if (vm_init() < 0) return false;
	if (vm_acquire_fixed(0, 0x3000) >= 0)
		ret = true;
	vm_release(0, 0x3000);
	vm_exit();
	return ret;
}

/* Attempts to get superuser permissions to change security settings
 * preventing mmap from zero */
static int elevate_for_mmap()
{
	int ret = 0;
	char *child_stdout;
	const char *exec;
	char cmds[64] = "";
	const char *cmd1;
	const char *cmd2;
	// Find a suitable superuser program
	if (g_find_program_in_path("pkexec"))
	{
		exec = "pkexec";
		cmd1 = "sh";
		cmd2 = "-c";
	}
	else if (g_find_program_in_path("gksudo"))
	{
		exec = "gksudo";
		cmd1 = "-D";
		cmd2 = GetString(STR_WINDOW_TITLE);
		strcat(cmds, "sh -c ");
	}
	else return 0;

	// Find out which security mechanisms are in use
	if(access("/proc/sys/vm/mmap_min_addr", F_OK) == 0)
		strcat(cmds, "sysctl vm.mmap_min_addr=0;");
	if(g_find_program_in_path("setsebool"))
		strcat(cmds, "setsebool mmap_low_allowed 1;");

	// Run the command
	GError *g_error = NULL;
	const gchar *argv[] = {exec, cmd1, cmd2, cmds, NULL};
		ret = g_spawn_sync(NULL, (gchar **)argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
		                   NULL, &child_stdout, NULL, NULL, &g_error);
		if(ret == 0)
			printf("Elevation Error: %s\n", g_error->message);
	return ret;
}

// User responded to the pkexec dialog
extern "C" void cb_elevate_response(GtkWidget *dialog, int response)
{
	if (response == GTK_RESPONSE_OK)
	{
		if(elevate_for_mmap() && can_mmap_real())
			run_emulator();
		else
			cb_infobar_show(GTK_WIDGET(gtk_builder_get_object(builder, "elevate-error-bar")));
	}
	gtk_widget_destroy(dialog);
}
#endif

// "Start" button clicked
static void cb_start (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	if(access(PrefsFindString("rom"), F_OK) != 0)
	{
		cb_infobar_show(GTK_WIDGET(gtk_builder_get_object(builder, "rom-error-bar")));
		return;
	}
#if REAL_ADDRESSING && defined(__linux__)
	if(!can_mmap_real())
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win),
		                    (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		                    GTK_MESSAGE_QUESTION,
		                    GTK_BUTTONS_CANCEL,
		                    GetString(STR_ELEVATE_DIALOG_TITLE));
	    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), GetString(STR_ELEVATE_DIALOG_TEXT));
		GtkWidget *button = gtk_dialog_add_button(GTK_DIALOG(dialog), GetString(STR_CONTINUE_BUTTON), GTK_RESPONSE_OK);
		gtk_style_context_add_class(gtk_widget_get_style_context(button), "suggested-action");
		g_signal_connect(dialog, "response", G_CALLBACK(cb_elevate_response), NULL);
		gtk_widget_show(dialog);
	}
	else
#endif
		run_emulator();
}

// "Save Settings" menu item clicked
static void
cb_save_settings (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	save_volumes();
	SavePrefs();
}

// "Quit" button clicked
static void cb_quit (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	start_clicked = false;
	gtk_widget_destroy(win);
}

// Dialog box has been closed
extern "C" void dl_quit(GtkWidget *dialog)
{
	gtk_widget_destroy(dialog);
	g_object_unref(dialog);
}

// Set initial widget states
static void set_initial_prefs(void)
{
	const char *check_boxes[] = {
		"nocdrom", "nosound", "udptunnel", "keycodes", "nogui", "ignoresegv", "idlewait",
		"jit", "jitfpu", "jitinline", "jitlazyflush","jit68k", "gfxaccel", "swap_opt_cmd"};
	for (int i = 0; i < 14; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, check_boxes[i])),
		                             PrefsFindBool(check_boxes[i]));
	cb_swap_opt_cmd(NULL);

	const char *entries[] = {
		"extfs", "dsp", "mixer", "keycodefile", "scsi0", "scsi1", "scsi2", "scsi3", "scsi4", "scsi5", "scsi6", "rom"};
	for (int i = 0; i < 12; i++)
		if (PrefsFindString(entries[i]) != NULL)
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, entries[i])),
			                   PrefsFindString(entries[i]));

	const char *spin_buttons[] = {"mousewheellines", "udpport"};
	for (int i = 0; i < 2; i++)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, spin_buttons[i])),
		                          PrefsFindInt32(spin_buttons[i]));

	const char *id_combos[] = {"bootdriver", "frameskip", "mousewheelmode", "modelid"};
	for (int i = 0; i < 4; i++)
	{
		char str[32];
		sprintf(str, "%d", PrefsFindInt32(id_combos[i]));
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(gtk_builder_get_object(builder, id_combos[i])), str);
	}

	const char *text_combos[] = {"ramsize"};
	GtkComboBoxText *combo;
	for (int i = 0; i < 1; i++)
	{
		combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, text_combos[i]));
		if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), PrefsFindString(text_combos[i])))
		{
			gtk_combo_box_text_prepend(combo, PrefsFindString(text_combos[i]), PrefsFindString(text_combos[i]));
			gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), PrefsFindString(text_combos[i]));
		}
	}

	const char *int_combos[] = {"jitcachesize"};
	for (int i = 0; i < 1; i++)
	{
		char str[32];
		combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, int_combos[i]));
		sprintf(str, "%d", PrefsFindInt32(int_combos[i]));
		if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), str))
		{
			gtk_combo_box_text_prepend(combo, str, str);
			gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), str);
		}
	}
}

// "About" selected
static void mn_about (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	char sysinfo[512];
	sprintf(sysinfo, "%s\nBuilt with %s and %s\n\%s",
	        INFO_ADDRESSING,
	        INFO_VIDEO,
	        INFO_AUDIO,
	        GetString(STR_ABOUT_COPYRIGHT));

   	char version[64];
	sprintf(version, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
	const char *authors[512] = {"Christian Bauer <cb@cebix.net>", "Marc Hellwig", "Gwenole Beauchesne", NULL};
				gtk_show_about_dialog(GTK_WINDOW(win), "version", version,
				"copyright", sysinfo,
				"authors", authors,
				"comments", GetString(STR_ABOUT_COMMENTS),
				"website", GetString(STR_ABOUT_WEBSITE),
				"website-label", GetString(STR_ABOUT_WEBSITE_LABEL),
				"license", GetString(STR_ABOUT_LICENSE),
				"wrap-license", true,
#ifdef SHEEPSHAVER
				"logo-icon-name", "SheepShaver",
#else
				"logo-icon-name", "BasiliskII",
#endif
				NULL);
}

// "Zap NVRAM" selected
static void mn_zap_pram (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	ZapPRAM();
}

// "Keyboard Shortcuts" selected
static void cb_help_overlay (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	return;
}

// Action entries (used in menus and buttons)
static GActionEntry win_entries[] = {
	{ "add-volume", cb_add_volume },
	{ "create-volume", cb_create_volume },
	{ "remove-volume", cb_remove_volume },
	{ "start", cb_start },
	{ "save-settings", cb_save_settings },
	{ "zap-pram", mn_zap_pram },
	{ "quit", cb_quit },
	{ "about", mn_about },
};

// Hide widgets which aren't applicable to this emulator
static void hide_widget(const char *name)
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, name));
	gtk_widget_hide(widget);
}

// Bind the sensitivity of a widget to the active property of another
static void bind_sensitivity(const char *source_name, const char *target_name, GBindingFlags flags = G_BINDING_DEFAULT)
{
	GtkWidget *source = GTK_WIDGET(gtk_builder_get_object(builder, source_name));
	GtkWidget *target = GTK_WIDGET(gtk_builder_get_object(builder, target_name));
	g_object_bind_property(source, "active", target, "sensitive", GBindingFlags(flags | G_BINDING_SYNC_CREATE));
}

static void set_file_menu(GtkApplication *app)
{
	GtkBuilder *menubuilder = gtk_builder_new_from_file("ui/menu.ui");
	gtk_application_set_menubar(app, G_MENU_MODEL(gtk_builder_get_object(menubuilder, "prefs-editor-menu")));
	g_object_unref(menubuilder);
}

static void set_help_overlay (GtkApplicationWindow *win)
{
#if GTK_CHECK_VERSION(3,20,0)
	GtkBuilder *helpbuilder = gtk_builder_new_from_file("ui/help-overlay.ui");
	gtk_application_window_set_help_overlay(win,
	                                        GTK_SHORTCUTS_WINDOW(gtk_builder_get_object(helpbuilder, "emulator-shortcuts")));
	g_object_unref(helpbuilder);
#endif
}

// Main
bool PrefsEditor(void)
{
	GApplication *app = g_application_get_default();
	if (g_application_get_is_remote(app))
	{
		printf("ERROR: Another instance of Basilisk II is running, quitting...\n");
		return false;
	}
	builder = gtk_builder_new_from_file("ui/prefs-editor.ui");
	set_file_menu(GTK_APPLICATION(app));

	// Create window
	win = GTK_WIDGET(gtk_builder_get_object(builder, "prefs-editor"));
	g_assert(GTK_IS_APPLICATION_WINDOW (win));
	gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(win));
	set_initial_prefs();
	set_ramsize_combo_box();
	set_hotkey_buttons();
	set_cpu_combo_box();
	gtk_builder_connect_signals(builder, NULL);

	set_help_overlay(GTK_APPLICATION_WINDOW(win));

	gtk_window_set_title(GTK_WINDOW(win), GetString(STR_PREFS_TITLE));
	g_action_map_add_action_entries (G_ACTION_MAP (win),
					 win_entries, G_N_ELEMENTS (win_entries),
					 win);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP (win), "remove-volume")), false);
	g_signal_connect(GTK_WIDGET(win), "delete_event", G_CALLBACK(window_closed), NULL);
	g_signal_connect(GTK_WIDGET(win), "destroy", G_CALLBACK(window_destroyed), NULL);

	// Get screen dimensions
#if GTK_CHECK_VERSION (3,22,0)
	GdkMonitor *monitor = gdk_display_get_monitor(gdk_display_get_default(), 0);
	GdkRectangle *geometry = new GdkRectangle;
	gdk_monitor_get_geometry(monitor, geometry);
	screen_width = geometry->width;
	screen_height = geometry->height;
	g_free(geometry);
#else
	screen_width = gdk_screen_width();
	screen_height = gdk_screen_height();
#endif

	// Create window contents
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(win), accel_group);
	volumes_view = create_tree_view();
	GtkWidget *nosound = GTK_WIDGET(gtk_builder_get_object(builder, "nosound"));
	GtkWidget *jit = GTK_WIDGET(gtk_builder_get_object(builder, "jit"));
	GtkWidget *keycodes = GTK_WIDGET(gtk_builder_get_object(builder, "keycodes"));
	get_graphics_settings();
	bind_sensitivity("mousewheelmode", "mousewheellines");
	bind_sensitivity("keycodes", "keycodefile");
	bind_sensitivity("keycodes", "keycodefile-browse");
#ifdef SHEEPSHAVER
	hide_widget("scsi-expander");
	bind_sensitivity("jit", "jitfpu");
	hide_widget("udptunnel");
	hide_widget("udpport");
	hide_widget("udpport-label");
	hide_widget("cpu");
	hide_widget("cpu-label");
	hide_widget("modelid");
	hide_widget("modelid-label");
	hide_widget("jitcachesize");
	hide_widget("jitcachesize-label");
	hide_widget("jitfpu");
	hide_widget("jitlazyflush");
	hide_widget("jitinline");
#else
	hide_widget("gfxaccel");
	hide_widget("jit68k");
	bind_sensitivity("udptunnel", "udpport");
	bind_sensitivity("jit", "jitfpu");
	bind_sensitivity("jit", "jitlazyflush");
	bind_sensitivity("jit", "jitcachesize");
	bind_sensitivity("jit", "jitcachesize-label");
	bind_sensitivity("jit", "jitinline");
	bind_sensitivity("udptunnel", "ether", G_BINDING_INVERT_BOOLEAN);
#endif
#ifndef ENABLE_SDL_AUDIO
	hide_widget("dsp");
	hide_widget("mixer");
	hide_widget("dsp-label");
	hide_widget("mixer-label");
#endif
	if (!is_jit_capable())
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "jit-pane")), false);
	GList *glist = add_serial_names();
	add_combo_box_values(glist, "seriala");
	add_combo_box_values(glist, "serialb");
	glist = add_ether_names();
	add_combo_box_values(glist, "ether");
	bind_sensitivity("udptunnel", "udpport");

	// Show window and enter main loop
	gtk_widget_show(win);
	gtk_main();
	return start_clicked;
}

/*
 *  "Volumes" pane
 */

// Values for the columns of the list store
enum {
	COLUMN_PATH,
	COLUMN_SIZE,
	COLUMN_READONLY,
	N_COLUMNS
};


// Gets the size of the volume as a pretty string
static const char* get_file_size (GFile *file)
{
	GFileInfo *info;
	if (g_file_query_exists(file, NULL))
	{
		info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		guint64 size = guint64(g_file_info_get_size(info));
		char *sizestr = g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);
		return sizestr;
	}
	else
	{
		return "Not Found";
	}
	g_object_unref(info);
}

static bool has_file_ext (GFile *file, const char *ext)
{
	char *str = g_file_get_path(file);
	char *file_ext = strrchr(str, '.');
	if (file_ext == NULL)
		return 0;
	return (strncmp(file_ext, ext, 3) == 0);
}

// User selected a volume to add
static void cb_add_volume_response (GtkWidget *chooser, int response)
{
	if (response == GTK_RESPONSE_ACCEPT)
	{
		GFile *volume = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
		gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
		gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
				COLUMN_PATH, g_file_get_path(volume),
				COLUMN_SIZE, get_file_size(volume),
				COLUMN_READONLY, has_file_ext(volume, ".iso"),
				-1);
		g_object_unref(volume);
	}
	gtk_widget_destroy(chooser);
}

// "Add Volume" button clicked
static void cb_add_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWidget *chooser = gtk_file_chooser_dialog_new(GetString(STR_ADD_VOLUME_TITLE),
							GTK_WINDOW(win),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							"Cancel", GTK_RESPONSE_CANCEL,
							"Add", GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(chooser), GTK_RESPONSE_ACCEPT);
	gtk_window_set_modal(GTK_WINDOW(chooser), true);
	g_signal_connect(chooser, "response", G_CALLBACK(cb_add_volume_response), NULL);
	gtk_widget_show(chooser);
}

// User selected to create a new volume
static void cb_create_volume_response (GtkWidget *chooser, int response, GtkEntry *size_entry)
{
	if (response == GTK_RESPONSE_ACCEPT)
	{
		GFile *volume = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
		const gchar *str = gtk_entry_get_text(GTK_ENTRY(size_entry));
		int disk_size = atoi(str);
		if (disk_size < 1 || disk_size > 2000)
		{
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win),
							(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							GTK_MESSAGE_WARNING,
							GTK_BUTTONS_CLOSE,
							"Enter a valid size", NULL);
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "The volume size should be between 1 and 2000.");
			gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(chooser));
			g_signal_connect(dialog, "response", G_CALLBACK(dl_quit), NULL);
			gtk_widget_show(dialog);
			return; // Don't close the file chooser dialog
		}
		// The dialog asks to confirm overwrite, so no need to prevent overwriting if file already exists
		GFileOutputStream *fd = g_file_replace(volume, NULL, false, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
		if (fd != NULL && g_seekable_can_truncate(G_SEEKABLE(fd)))
		{
			g_seekable_truncate(G_SEEKABLE(fd), disk_size * 1024 * 1024, NULL, NULL);
		}
		g_object_unref(fd);

		gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
		gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
				COLUMN_PATH, g_file_get_path(volume),
				COLUMN_SIZE, get_file_size(volume),
				-1);
		g_object_unref(volume);
	}
	gtk_widget_destroy (chooser);
}

// "Create" button clicked
static void cb_create_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWidget *chooser = gtk_file_chooser_dialog_new(GetString(STR_CREATE_VOLUME_TITLE),
							GTK_WINDOW(win),
							GTK_FILE_CHOOSER_ACTION_SAVE,
							"Cancel", GTK_RESPONSE_CANCEL,
							"Create", GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
	gtk_dialog_set_default_response(GTK_DIALOG(chooser), GTK_RESPONSE_ACCEPT);
	gtk_window_set_transient_for(GTK_WINDOW(chooser), GTK_WINDOW(win));
	gtk_window_set_modal(GTK_WINDOW(chooser), true);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_show(box);
	GtkWidget *label = gtk_label_new(GetString(STR_HARDFILE_SIZE_CTRL));
	gtk_widget_show(label);
	GtkWidget *size_entry = gtk_entry_new();
	gtk_widget_show(size_entry);
	gtk_entry_set_text(GTK_ENTRY(size_entry), "512");
	gtk_box_pack_end(GTK_BOX(box), size_entry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_widget_set_hexpand(box, TRUE);

	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser), box);

	g_signal_connect(chooser, "response", G_CALLBACK(cb_create_volume_response), size_entry);
	gtk_widget_show(chooser);
}

// Enables and disables the "Remove" button depending on whether a volume is selected
static void cb_remove_enable(GtkWidget *widget)
{
	bool enable = false;
	if (selection != NULL && gtk_tree_selection_count_selected_rows(selection))
		enable = true;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP (win), "remove-volume")), enable);
}

// "Remove" button clicked
static void cb_remove_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	if (gtk_tree_selection_count_selected_rows(selection))
	{
		gtk_tree_selection_get_selected(selection, &volume_store, &toplevel);
		gtk_list_store_remove(GTK_LIST_STORE(volume_store), &toplevel);
	}
}

static void cb_readonly (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean read_only;
	gboolean new_value = true;
	char *name;

	gtk_tree_model_get_iter (volume_store, &iter, path);
	gtk_tree_model_get (volume_store, &iter, COLUMN_READONLY, &read_only, -1);

	read_only ^= 1;

	gtk_list_store_set (GTK_LIST_STORE (volume_store), &iter, COLUMN_READONLY, read_only, -1);
	gtk_tree_path_free (path);
}

// Save volumes from list store to prefs
static void save_volumes (void)
{
	GtkTreePath *path;
	while (PrefsFindString("disk"))
		PrefsRemoveItem("disk");
	if(gtk_tree_model_get_iter_first(volume_store, &toplevel))
	{
		const char *path;
		do {
			gboolean read_only = true;
			gtk_tree_model_get(volume_store, &toplevel, COLUMN_READONLY, &read_only, -1);
			gtk_tree_model_get(volume_store, &toplevel, COLUMN_PATH, &path, -1);
			if (read_only)
			{
				char str[256];
				snprintf(str, 255, "*%s", path);
				PrefsAddString("disk", str);
			}
			else
			{
				PrefsAddString("disk", path);
			}
		}
		while (gtk_tree_model_iter_next(volume_store, &toplevel));
	}
}

// Gets the list of volumes from the prefs file and adds them to the list store
static GtkTreeModel *get_volumes (void)
{
	const char *str;
	int32 index = 0;
	volume_store = GTK_TREE_MODEL(gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN));
	while ((str = (const char *)PrefsFindString("disk", index++)) != NULL)
	{
		int read_only = 0;
		if (strncmp(str, "*", 1) == 0)
		{
			read_only = 1;
		}
		gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
			gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
				COLUMN_PATH, str + read_only,
				COLUMN_SIZE, get_file_size(g_file_new_for_path(str + read_only)),
				COLUMN_READONLY, read_only,
				-1);
	}
	return volume_store;
}

// Creates the tree view widget to display the volume list
static GtkWidget *create_tree_view (void)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;
	GtkTreeModel *model;

	view = GTK_WIDGET(gtk_builder_get_object(builder, "volumes-view"));
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Location");
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_set_expand(col, true);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COLUMN_PATH);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "ISO");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect (renderer, "toggled",
	                  G_CALLBACK (cb_readonly), NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "active", COLUMN_READONLY);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Size");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COLUMN_SIZE);

	model = get_volumes();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model); // destroy model automatically with view

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(selection, "changed", G_CALLBACK(cb_remove_enable), NULL);

	return view;
}

/*
 *  "JIT Compiler" pane
 */

// Are we running a JIT capable CPU?
static bool is_jit_capable(void)
{
#if USE_JIT
	return true;
#elif defined __APPLE__ && defined __MACH__
	// XXX run-time detect so that we can use a PPC GUI prefs editor
	static char cpu[10];
	if (cpu[0] == 0) {
		FILE *fp = popen("uname -p", "r");
		if (fp == NULL)
			return false;
		fgets(cpu, sizeof(cpu) - 1, fp);
		fclose(fp);
	}
	if (cpu[0] == 'i' && cpu[2] == '8' && cpu[3] == '6') // XXX assuming i?86
		return true;
#endif
	return false;
}

/*
 *  "Graphics/Sound" pane
 */

// Display types
enum {
	DISPLAY_WINDOW,
	DISPLAY_SCREEN
};

static int display_type;
static int dis_width, dis_height;
static bool is_fbdev_dga_mode = false;

// User changed the hotkey combination
extern "C" void cb_hotkey (GtkWidget *widget)
{
	GtkToggleButton *ctrl = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "ctrl-hotkey"));
	GtkToggleButton *alt = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "alt-hotkey"));
	GtkToggleButton *super = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "super-hotkey"));
	int hotkey = gtk_toggle_button_get_active(ctrl) |
	             gtk_toggle_button_get_active(alt) << 1 |
	             gtk_toggle_button_get_active(super) << 2;
	if (hotkey == 0)
		return;
	PrefsReplaceInt32("hotkey", hotkey);
}

// Saves the new ram size preference
extern "C" void cb_ramsize (GtkWidget *widget)
{
	int size_bytes = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget))) << 20;
	PrefsReplaceInt32("ramsize", size_bytes);
}

// Saves the new cpu and fpu preferences
extern "C" void cb_cpu (GtkWidget *widget)
{
	int value = atoi(gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));
	int cpu = value >> 1;
	bool fpu = false;
	if (value & 1)
		fpu = true;
	PrefsReplaceInt32("cpu", cpu);
	PrefsReplaceBool("fpu", fpu);
}

// Save the id of the selected combo box item to its associated preference
extern "C" void cb_combo_int (GtkWidget *widget)
{
	PrefsReplaceInt32(gtk_widget_get_name(widget), atoi(gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget))));
}

extern "C" void cb_combo_str (GtkWidget *widget)
{
	PrefsReplaceString(gtk_widget_get_name(widget), gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));
}

// Save the value of the combo box entry to its associated preference
extern "C" void cb_combo_entry_int (GtkWidget *widget)
{
	PrefsReplaceInt32(gtk_widget_get_name(widget),
	                  atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget))));
}

extern "C" void cb_combo_entry_str (GtkWidget *widget)
{
	PrefsReplaceString(gtk_widget_get_name(widget),
	                   gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
}

// Save the value of the spin button to its associated preference
extern "C" void cb_spin_button (GtkWidget *widget)
{
	PrefsReplaceInt32(gtk_widget_get_name(widget), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)));
}

// Save the value of the check box to its associated preference
extern "C" void cb_check_box (GtkWidget *widget)
{
	PrefsReplaceBool(gtk_widget_get_name(widget), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// Save the contents of the entry to its associated preference
extern "C" void cb_entry (GtkWidget *widget)
{
	if (gtk_entry_get_text_length(GTK_ENTRY(widget)))
		PrefsReplaceString(gtk_widget_get_name(widget), gtk_entry_get_text(GTK_ENTRY(widget)));
	else
		PrefsRemoveItem(gtk_widget_get_name(widget));
}

// Display the info bar
extern "C" void cb_infobar_show (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(3,22,29)
    /* Workaround for the fact that having the revealed property in the XML is not valid
    in older GTK versions. Instead we hide it until it is about to be revealed. */
    if (!gtk_widget_get_visible(widget))
	{
	    gtk_info_bar_set_revealed(GTK_INFO_BAR(widget), false);
	    gtk_widget_show(widget);
	}
	gtk_info_bar_set_revealed(GTK_INFO_BAR(widget), true);
#else
	gtk_widget_show(widget);
#endif
}

// Close the info bar
extern "C" void cb_infobar_hide (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(3,22,29)
	gtk_info_bar_set_revealed(GTK_INFO_BAR(widget), false);
#else
	gtk_widget_hide(GTK_WIDGET(widget));
#endif
}

extern "C" void cb_swap_opt_cmd (GtkWidget *widget)
{
	bool swapped = PrefsFindBool("swap_opt_cmd");
	GtkLabel *opt_label = GTK_LABEL(gtk_builder_get_object(builder, "opt-label"));
	GtkLabel *cmd_label = GTK_LABEL(gtk_builder_get_object(builder, "cmd-label"));
	gtk_label_set_text(opt_label, swapped ? "Super" : "Alt");
	gtk_label_set_text(cmd_label, swapped ? "Alt" : "Super");
}

// Read and convert graphics preferences
static void get_graphics_settings (void)
{
	display_type = DISPLAY_WINDOW;
	dis_width = 640;
	dis_height = 480;
	screen_mode = GTK_COMBO_BOX(gtk_builder_get_object(builder, "screen-mode"));
	screen_x = GTK_COMBO_BOX(gtk_builder_get_object(builder, "screen-x"));
	screen_y = GTK_COMBO_BOX(gtk_builder_get_object(builder, "screen-y"));

	const char *str = PrefsFindString("screen");
	if (str) {
		if (sscanf(str, "win/%d/%d", &dis_width, &dis_height) == 2)
			display_type = DISPLAY_WINDOW;
		else if (sscanf(str, "dga/%d/%d", &dis_width, &dis_height) == 2)
			display_type = DISPLAY_SCREEN;
		else if (sscanf(str, "fbdev/%d/%d", &dis_width, &dis_height) == 2) {
#ifdef ENABLE_FBDEV_DGA
			is_fbdev_dga_mode = true;
#endif
			display_type = DISPLAY_SCREEN;
		}
	}
	else {
		uint32 window_modes = PrefsFindInt32("windowmodes");
		uint32 screen_modes = PrefsFindInt32("screenmodes");
		if (screen_modes) {
			display_type = DISPLAY_SCREEN;
			static const struct {
				int id;
				int width;
				int height;
			}
			modes[] = {
				{  1,	 640,	 480 },
				{  2,	 800,	 600 },
				{  4,	1024,	 768 },
				{ 64,	1152,	 768 },
				{  8,	1152,	 900 },
				{ 16,	1280,	1024 },
				{ 32,	1600,	1200 },
				{ 0, }
			};
			for (int i = 0; modes[i].id != 0; i++) {
				if (screen_modes & modes[i].id) {
					if (modes[i].width <= screen_width && modes[i].height <= screen_height) {
						dis_width = modes[i].width;
						dis_height = modes[i].height;
					}
				}
			}
		}
		else if (window_modes) {
			display_type = DISPLAY_WINDOW;
			if (window_modes & 1)
				dis_width = 640, dis_height = 480;
			if (window_modes & 2)
				dis_width = 800, dis_height = 600;
		}
	}
	if (dis_width == screen_width)
		dis_width = 0;
	if (dis_height == screen_height)
		dis_height = 0;

	if(display_type == DISPLAY_SCREEN)
		gtk_combo_box_set_active(screen_mode, 1);
	char val[32];
	sprintf(val, "%d", dis_width);
	if(!gtk_combo_box_set_active_id(screen_x, val))
	{
		gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(screen_x), val, val);
		gtk_combo_box_set_active_id(screen_x, val);
	}
	sprintf(val, "%d", dis_height);
	if(!gtk_combo_box_set_active_id(screen_y, val))
	{
		gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(screen_y), val, val);
		gtk_combo_box_set_active_id(screen_y, val);
	}

}

// Add names of serial devices
static GList *add_serial_names (void)
{
	GList *glist = NULL;

	// Search /dev for ttyS* and lp*
	DIR *d = opendir("/dev");
	if (d) {
		struct dirent *de;
		while ((de = readdir(d)) != NULL) {
#if defined(__linux__)
			if (strncmp(de->d_name, "ttyS", 4) == 0 || strncmp(de->d_name, "lp", 2) == 0) {
#elif defined(__FreeBSD__)
			if (strncmp(de->d_name, "cuaa", 4) == 0 || strncmp(de->d_name, "lpt", 3) == 0) {
#elif defined(__NetBSD__)
			if (strncmp(de->d_name, "tty0", 4) == 0 || strncmp(de->d_name, "lpt", 3) == 0) {
#elif defined(sgi)
			if (strncmp(de->d_name, "ttyf", 4) == 0 || strncmp(de->d_name, "plp", 3) == 0) {
#else
			if (false) {
#endif
				char *str = new char[64];
				sprintf(str, "/dev/%s", de->d_name);
				glist = g_list_append(glist, str);
			}
		}
		closedir(d);
	}
	if (!glist)
		glist = g_list_append(glist, (void *)"<none>");
	return glist;
}

// Add names of ethernet interfaces
static GList *add_ether_names (void)
{
	GList *glist = NULL;

	// Get list of all Ethernet interfaces
	int s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s >= 0) {
		char inbuf[8192];
		struct ifconf ifc;
		ifc.ifc_len = sizeof(inbuf);
		ifc.ifc_buf = inbuf;
		if (ioctl(s, SIOCGIFCONF, &ifc) == 0) {
			struct ifreq req, *ifr = ifc.ifc_req;
			for (int i=0; i<ifc.ifc_len; i+=sizeof(ifreq), ifr++) {
				req = *ifr;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(sgi)
				if (ioctl(s, SIOCGIFADDR, &req) == 0 && (req.ifr_addr.sa_family == ARPHRD_ETHER || req.ifr_addr.sa_family == ARPHRD_ETHER+1)) {
#elif defined(__linux__)
				if (ioctl(s, SIOCGIFHWADDR, &req) == 0 && req.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
#else
				if (false) {
#endif
					char *str = new char[64];
					strncpy(str, ifr->ifr_name, 63);
					glist = g_list_append(glist, str);
				}
			}
		}
		close(s);
	}
#ifdef HAVE_SLIRP
	static char s_slirp[] = "slirp";
	glist = g_list_append(glist, s_slirp);
#endif
	if (!glist)
		glist = g_list_append(glist, (void *)"<none>");
	return glist;
}

#ifdef STANDALONE_GUI
#include <errno.h>
#include <sys/wait.h>
#include "rpc.h"

/*
 *  Fake unused data and functions
 */

uint8 XPRAM[XPRAM_SIZE];
void MountVolume(void *fh) { }
void FileDiskLayout(loff_t size, uint8 *data, loff_t &start_byte, loff_t &real_size) { }

#if defined __APPLE__ && defined __MACH__
void DarwinSysInit(void) { }
void DarwinSysExit(void) { }
void DarwinAddFloppyPrefs(void) { }
void DarwinAddSerialPrefs(void) { }
bool DarwinCDReadTOC(char *, uint8 *) { }
#endif

static void dl_destroyed (void)
{
	gtk_main_quit();
}


/*
 *  Display alert
 */

static void display_alert (int title_id, int prefix_id, int button_id, const char *text)
{
	char str[256];
	sprintf(str, GetString(prefix_id), text);

	GtkWidget *dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), GetString(title_id));
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	gtk_widget_set_uposition(GTK_WIDGET(dialog), 100, 150);
	g_signal_connect(GTK_WIDGET(dialog), "destroy", G_CALLBACK(dl_destroyed), NULL);

	GtkWidget *label = gtk_label_new(str);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0);

	GtkWidget *button = gtk_button_new_with_label(GetString(button_id));
	gtk_widget_show(button);
	g_signal_connect_object(GTK_WIDGET(button), "clicked", G_CALLBACK(dl_quit), GTK_WIDGET(dialog));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	gtk_widget_set_receives_default(button, true);
	gtk_widget_grab_default(button);
	gtk_widget_show(dialog);

	gtk_main();
}


/*
 *  Display error alert
 */

void ErrorAlert (const char *text)
{
	display_alert(STR_ERROR_ALERT_TITLE, STR_GUI_ERROR_PREFIX, STR_QUIT_BUTTON, text);
}


/*
 *  Display warning alert
 */

void WarningAlert (const char *text)
{
	display_alert(STR_WARNING_ALERT_TITLE, STR_GUI_WARNING_PREFIX, STR_OK_BUTTON, text);
}


/*
 *  RPC handlers
 */

static GMainLoop *g_gui_loop;

static int handle_ErrorAlert(rpc_connection_t *connection)
{
	D(bug("handle_ErrorAlert\n"));

	int error;
	char *str;
	if ((error = rpc_method_get_args(connection, RPC_TYPE_STRING, &str, RPC_TYPE_INVALID)) < 0)
		return error;

	ErrorAlert(str);
	free(str);
	return RPC_ERROR_NO_ERROR;
}

static int handle_WarningAlert(rpc_connection_t *connection)
{
	D(bug("handle_WarningAlert\n"));

	int error;
	char *str;
	if ((error = rpc_method_get_args(connection, RPC_TYPE_STRING, &str, RPC_TYPE_INVALID)) < 0)
		return error;

	WarningAlert(str);
	free(str);
	return RPC_ERROR_NO_ERROR;
}

static int handle_Exit(rpc_connection_t *connection)
{
	D(bug("handle_Exit\n"));

	g_main_quit(g_gui_loop);
	return RPC_ERROR_NO_ERROR;
}


/*
 *  SIGCHLD handler
 */

static char g_app_path[PATH_MAX];
static rpc_connection_t *g_gui_connection = NULL;

static void sigchld_handler (int sig, siginfo_t *sip, void *)
{
	D(bug("Child %d exitted with status = %x\n", sip->si_pid, sip->si_status));

	// XXX perform a new wait because sip->si_status is sometimes not
	// the exit _value_ on MacOS X but rather the usual status field
	// from waitpid() -- we could arrange this code in some other way...
	int status;
	if (waitpid(sip->si_pid, &status, 0) < 0)
		status = sip->si_status;
	if (WIFEXITED(status))
		status = WEXITSTATUS(status);
	if (status & 0x80)
		status |= -1 ^0xff;

	if (status < 0) {	// negative -> execlp/-errno
		char str[256];
		sprintf(str, GetString(STR_NO_B2_EXE_FOUND), g_app_path, strerror(-status));
		ErrorAlert(str);
		status = 1;
	}

	if (status != 0) {
		if (g_gui_connection)
			rpc_exit(g_gui_connection);
		exit(status);
	}
}


/*
 *  Start standalone GUI
 */

int main (int argc, char *argv[])
{
	// Read preferences
	PrefsInit(0, argc, argv);

	// Show preferences editor
	bool start = PrefsEditor();

	// Exit preferences
	PrefsExit();

	// Transfer control to the executable
	if (start) {
		char gui_connection_path[64];
		sprintf(gui_connection_path, "/org/SheepShaver/GUI/%d", getpid());

		// Catch exits from the child process
		struct sigaction sigchld_sa, old_sigchld_sa;
		sigemptyset(&sigchld_sa.sa_mask);
		sigchld_sa.sa_sigaction = sigchld_handler;
		sigchld_sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
		if (sigaction(SIGCHLD, &sigchld_sa, &old_sigchld_sa) < 0) {
			char str[256];
			sprintf(str, GetString(STR_SIG_INSTALL_ERR), SIGCHLD, strerror(errno));
			ErrorAlert(str);
			return 1;
		}

		// Search and run the SheepShaver executable
		char *p;
		strcpy(g_app_path, argv[0]);
		if ((p = strstr(g_app_path, "SheepShaverGUI.app/Contents/MacOS")) != NULL) {
			strcpy(p, "SheepShaver.app/Contents/MacOS/SheepShaver");
			if (access(g_app_path, X_OK) < 0) {
				char str[256];
				sprintf(str, GetString(STR_NO_B2_EXE_FOUND), g_app_path, strerror(errno));
				WarningAlert(str);
				strcpy(g_app_path, "/Applications/SheepShaver.app/Contents/MacOS/SheepShaver");
			}
		} else {
			p = strrchr(g_app_path, '/');
			p = p ? p + 1 : g_app_path;
			strcpy(p, "SheepShaver");
		}

		int pid = fork();
		if (pid == 0) {
			D(bug("Trying to execute %s\n", g_app_path));
			execlp(g_app_path, g_app_path, "--gui-connection", gui_connection_path, (char *)NULL);
#ifdef _POSIX_PRIORITY_SCHEDULING
			// XXX get a chance to run the parent process so that to not confuse/upset GTK...
			sched_yield();
#endif
			_exit(-errno);
		}

		// Establish a connection to Basilisk II
		if ((g_gui_connection = rpc_init_server(gui_connection_path)) == NULL) {
			printf("ERROR: failed to initialize GUI-side RPC server connection\n");
			return 1;
		}
		static const rpc_method_descriptor_t vtable[] = {
			{ RPC_METHOD_ERROR_ALERT,			handle_ErrorAlert },
			{ RPC_METHOD_WARNING_ALERT,			handle_WarningAlert },
			{ RPC_METHOD_EXIT,					handle_Exit }
		};
		if (rpc_method_add_callbacks(g_gui_connection, vtable, sizeof(vtable) / sizeof(vtable[0])) < 0) {
			printf("ERROR: failed to setup GUI method callbacks\n");
			return 1;
		}
		int socket;
		if ((socket = rpc_listen_socket(g_gui_connection)) < 0) {
			printf("ERROR: failed to initialize RPC server thread\n");
			return 1;
		}

		g_gui_loop = g_main_new(TRUE);
		while (g_main_is_running(g_gui_loop)) {

			// Process a few events pending
			const int N_EVENTS_DISPATCH = 10;
			for (int i = 0; i < N_EVENTS_DISPATCH; i++) {
				if (!g_main_iteration(FALSE))
					break;
			}

			// Check for RPC events (100 ms timeout)
			int ret = rpc_wait_dispatch(g_gui_connection, 100000);
			if (ret == 0)
				continue;
			if (ret < 0)
				break;
			rpc_dispatch(g_gui_connection);
		}
		rpc_exit(g_gui_connection);
		return 0;
	}
	return 0;
}
#endif

