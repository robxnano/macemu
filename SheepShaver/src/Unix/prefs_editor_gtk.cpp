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

#include <cerrno>

#include "user_strings.h"
#include "version.h"
#include "cdrom.h"
#include "xpram.h"
#include "prefs.h"
#include "prefs_editor.h"

#define DEBUG 0
#include "debug.h"


// Global variables
static GtkBuilder *menubuilder_;
static GtkWidget *win;				// Preferences window
static bool start_clicked = false;	// Return value of PrefsEditor() function
static int screen_width, screen_height; // Screen dimensions


// Prototypes
static void create_volumes_pane(GtkWidget *top);
static void create_graphics_pane(GtkWidget *top);
static void create_input_pane(GtkWidget *top);
static void create_serial_pane(GtkWidget *top);
static void create_memory_pane(GtkWidget *top);
static void create_jit_pane(GtkWidget *top);
static void read_settings(void);


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

struct file_req_assoc {
	file_req_assoc(GtkWidget *r, GtkWidget *e) : req(r), entry(e) {}
	GtkWidget *req;
	GtkWidget *entry;
};

static void on_browse_response(GtkWidget *chooser, int response, GtkEntry *entry)
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

static void cb_browse(GtkButton *button, GtkEntry *entry)
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
	g_signal_connect(chooser, "response", G_CALLBACK(on_browse_response), entry);
	gtk_widget_show(chooser);
}

static void cb_browse_dir(GtkButton *button, GtkEntry *entry)
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
	g_signal_connect(chooser, "response", G_CALLBACK(on_browse_response), entry);
	gtk_widget_show(chooser);
}

static GtkWidget *make_browse_button(GtkEntry *entry, bool only_dirs = false)
{
	GtkWidget *button;

	button = gtk_button_new_with_label(GetString(STR_BROWSE_CTRL));
	gtk_widget_show(button);
	if(only_dirs)
    	g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(cb_browse_dir), entry);
	else
	    g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(cb_browse), entry);
	return button;
}

static void add_menu_item(GtkWidget *menu, int label_id, GCallback func)
{
	GtkWidget *item = gtk_menu_item_new_with_label(GetString(label_id));
	gtk_widget_show(item);
	g_signal_connect(GTK_WIDGET(item), "activate", func, NULL);
	//gtk_menu_append(GTK_MENU(menu), item);
}

static GtkWidget *make_pane(GtkWidget *notebook, int title_id)
{
	GtkWidget *frame, *label, *box;

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 4);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(box), 4);
	gtk_container_add(GTK_CONTAINER(frame), box);

	gtk_widget_show_all(frame);

	label = gtk_label_new(GetString(title_id));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);
	return box;
}

static GtkWidget *make_button_box(GtkWidget *top, int border, const opt_desc *buttons)
{
	GtkWidget *bb, *button;

	bb = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(bb);
	gtk_container_set_border_width(GTK_CONTAINER(bb), border);
	gtk_box_pack_start(GTK_BOX(top), bb, FALSE, FALSE, 0);

	while (buttons->label_id) {
		button = gtk_button_new_with_label(GetString(buttons->label_id));
		gtk_widget_show(button);
		g_signal_connect(GTK_WIDGET(button), "clicked", buttons->func, NULL);
		gtk_box_pack_start(GTK_BOX(bb), button, TRUE, TRUE, 0);
		buttons++;
	}
	return bb;
}

static GtkWidget *make_separator(GtkWidget *top)
{
	GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(top), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);
	return sep;
}

static GtkWidget *make_grid(GtkWidget *top, int spacing)
{
	GtkWidget *grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), spacing);
	gtk_grid_set_column_spacing(GTK_GRID(grid), spacing);
	gtk_box_pack_start(GTK_BOX(top), grid, FALSE, FALSE, 0);
	return grid;
}

static GtkWidget *grid_make_combo_box(GtkWidget *grid, int row, bool with_entry, int label_id, const combo_desc *options, int default_value, GCallback func = NULL)
{
	GtkWidget *label, *combo, *box;
	char str[32];

	label = gtk_label_new(GetString(label_id));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
	if (with_entry)
	    combo = gtk_combo_box_text_new_with_entry();
	else
	    combo = gtk_combo_box_text_new();

	int i = 0;
	while (options->label_id) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), GetString(options->label_id), GetString(options->label_id));
		options++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), default_value);
	gtk_widget_show(combo);
	gtk_widget_set_hexpand(combo, with_entry);

    if(func)
        g_signal_connect(combo, "changed", G_CALLBACK(func), combo);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(box);
    gtk_widget_set_hexpand(box, with_entry);
    gtk_box_pack_start(GTK_BOX(box), combo, with_entry, with_entry, 0);
	gtk_grid_attach(GTK_GRID(grid), box, 1, row, 1, 1);

	return combo;
}

static GtkWidget *grid_make_file_entry(GtkWidget *grid, int row, int label_id, const char *prefs_item, bool only_dirs = false)
{
	GtkWidget *box, *label, *entry, *button;

	label = gtk_label_new(GetString(label_id));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

	const char *str = PrefsFindString(prefs_item);
	if (str == NULL)
		str = "";

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show(box);
	gtk_grid_attach(GTK_GRID(grid), box, 1, row, 1, 1);

	entry = gtk_entry_new();
	gtk_widget_set_hexpand(entry, true);
	gtk_entry_set_text(GTK_ENTRY(entry), str); 
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);

	button = make_browse_button(GTK_ENTRY(entry), only_dirs);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(entry), "chooser_button", button);
	return entry;
}

static GtkWidget *grid_make_entry(GtkGrid *grid, int row, int label_id, const char *prefs_item)
{
	GtkWidget *box, *label, *entry;

	label = gtk_label_new(GetString(label_id));
	gtk_widget_show(label);
	gtk_grid_attach(grid, label, 0, row, 1, 1);

	entry = gtk_entry_new();
	gtk_widget_show(entry);
	gtk_widget_set_hexpand(entry, true);
	const char *str = PrefsFindString(prefs_item);
	if (str == NULL)
		str = "";
	gtk_entry_set_text(GTK_ENTRY(entry), str); 
	gtk_grid_attach(grid, entry, 1, row, 1, 1);
	return entry;
}

static const gchar *get_file_entry_path(GtkWidget *entry)
{
	return gtk_entry_get_text(GTK_ENTRY(entry));
}

static GtkWidget *make_checkbox(GtkWidget *top, int label_id, const char *prefs_item, GCallback func)
{
	GtkWidget *button = gtk_check_button_new_with_label(GetString(label_id));
	gtk_widget_show(button);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), PrefsFindBool(prefs_item));
    g_signal_connect(GTK_WIDGET(button), "toggled", func, button);
	gtk_box_pack_start(GTK_BOX(top), button, FALSE, FALSE, 0);
	return button;
}

static GtkWidget *make_checkbox(GtkWidget *top, int label_id, bool active, GCallback func)
{
	GtkWidget *button = gtk_check_button_new_with_label(GetString(label_id));
	gtk_widget_show(button);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
	g_signal_connect(GTK_WIDGET(button), "toggled", func, button);
	gtk_box_pack_start(GTK_BOX(top), button, FALSE, FALSE, 0);
	return button;
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

// "Start" button clicked
static void
cb_start (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	start_clicked = true;
	read_settings();
	SavePrefs();
	gtk_widget_destroy(win);
}

// "Start" button clicked
static void
cb_save_settings (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	read_settings();
	SavePrefs();
}

// "Quit" button clicked
static void
cb_quit (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	start_clicked = false;
	gtk_widget_destroy(win);
}

// "OK" button of "About" dialog clicked
static void dl_quit(GtkWidget *dialog)
{
	gtk_widget_destroy(dialog);
}

// "About" selected
static void
mn_about (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{

    char version[64];
    sprintf(version, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
    const char *author[512] = {"Christian Bauer <cb@cebix.net>", "Marc Hellwig", NULL};
	GtkAboutDialog *dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
	gtk_about_dialog_set_version(dialog, version);
	gtk_about_dialog_set_copyright(dialog, "© 1997-2008 Christian Bauer and Marc Hellwig");
	gtk_about_dialog_set_authors(dialog, author);
	gtk_about_dialog_set_comments(dialog, "Open source PowerMac emulator");
	gtk_about_dialog_set_website(dialog, "http://sheepshaver.cebix.net");
	gtk_about_dialog_set_license(dialog, "SheepShaver comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under the terms of the GNU General Public License.");
	gtk_about_dialog_set_wrap_license(dialog, true);
	gtk_about_dialog_set_logo_icon_name(dialog, "SheepShaver");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

// "Zap NVRAM" selected
static void
mn_zap_pram (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	ZapPRAM();
}

static GActionEntry win_entries[] = {
  { "start", cb_start, NULL, NULL, cb_start },
  { "save-settings", cb_save_settings, NULL, NULL, cb_save_settings },
  { "zap-pram", mn_zap_pram, NULL, NULL, mn_zap_pram },
  { "quit", cb_quit, NULL, NULL, cb_quit },
  { "about", mn_about, NULL, NULL, mn_about }
};

bool PrefsEditor(void)
{
    GApplication *app = g_application_get_default();
    if (g_application_get_is_remote(app))
    {
        printf("ERROR: Another instance of SheepShaver is running, quitting...\n");
        return false;
    }
    menubuilder_ = gtk_builder_new_from_resource("/net/cebix/SheepShaver/data/menu.ui");
    gtk_application_set_menubar(GTK_APPLICATION(app),
                                 G_MENU_MODEL(gtk_builder_get_object(menubuilder_, "main-window-menu")));
	// Get screen dimensions
	screen_width = gdk_screen_width();
	screen_height = gdk_screen_height();

	// Create window
	win = gtk_application_window_new(GTK_APPLICATION(app));
	g_assert(GTK_IS_APPLICATION_WINDOW (win));

	gtk_window_set_title(GTK_WINDOW(win), GetString(STR_PREFS_TITLE));
    g_action_map_add_action_entries (G_ACTION_MAP (win),
                                 win_entries, G_N_ELEMENTS (win_entries),
                                 win);
	g_signal_connect(GTK_WIDGET(win), "delete_event", G_CALLBACK(window_closed), NULL);
	g_signal_connect(GTK_WIDGET(win), "destroy", G_CALLBACK(window_destroyed), NULL);

	// Create window contents
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER(win), box);

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(win), accel_group);

	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
	gtk_widget_realize(notebook);

	create_volumes_pane(notebook);
	create_graphics_pane(notebook);
	create_input_pane(notebook);
	create_serial_pane(notebook);
	create_memory_pane(notebook);
	create_jit_pane(notebook);
	gtk_widget_show(notebook);

	static const opt_desc buttons[] = {
		{STR_START_BUTTON, G_CALLBACK(cb_start)},
		{STR_QUIT_BUTTON, G_CALLBACK(cb_quit)},
		{0, NULL}
	};
	make_button_box(box, 4, buttons);

	// Show window and enter main loop
	gtk_widget_show(win);
	gtk_main();
	return start_clicked;
}


/*
 *  "Volumes" pane
 */

GtkTreeModel *volume_store;
GtkTreeIter toplevel;

enum {
  COLUMN_PATH,
  COLUMN_SIZE,
  N_COLUMNS
};

static GtkWidget *volume_view, *w_extfs, *w_boot_from;
static int selected_volume;

// Gets the size of the volume
static const char* get_file_size (const char* path)
{
    GFile *file;
    GFileInfo *info;
    file = g_file_new_for_path(path);
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
}

// Volume in list selected
static void cl_selected(GtkWidget *list, int row, int column)
{
	selected_volume = row;
}

static void on_add_volume_response (GtkWidget *chooser, int response)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        char *str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
        gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
                          COLUMN_PATH, str,
                          COLUMN_SIZE, get_file_size(str),
                          -1);
    }
	gtk_widget_destroy(chooser);
}

// "Add Volume" button clicked
static void cb_add_volume(...)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new(GetString(STR_ADD_VOLUME_TITLE),
                                                     GTK_WINDOW(win),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Add", GTK_RESPONSE_ACCEPT,
                                                     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(chooser), GTK_RESPONSE_ACCEPT);
	gtk_window_set_modal(GTK_WINDOW(chooser), true);
	g_signal_connect(chooser, "response", G_CALLBACK(on_add_volume_response), NULL);
	gtk_widget_show(chooser);
}

static void on_create_volume_response(GtkWidget *chooser, int response, GtkEntry *size_entry)
{

	if (response == GTK_RESPONSE_ACCEPT)
    {
        gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
	    const gchar *str = gtk_entry_get_text(GTK_ENTRY(size_entry));
	    int disk_size = atoi(str);
	    if (disk_size < 1 || disk_size > 2000) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_CLOSE,
                                               "Enter a valid size", NULL);
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "The volume size should be between 1 and 2000.");
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(chooser));
            gtk_window_set_modal(GTK_WINDOW(dialog), true);
	        g_signal_connect(dialog, "response", G_CALLBACK(dl_quit), NULL);
            gtk_widget_show(dialog);
		    return;
	    }
	    // The dialog asks to confirm overwrite, so no need to prevent overwriting if file already exists
	    int fd = open(file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	    ftruncate(fd, disk_size * 1024 * 1024);

	    close(fd);

        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
        gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
                          COLUMN_PATH, path,
                          COLUMN_SIZE, get_file_size(path),
                          -1);
    }
    gtk_widget_destroy (chooser);
}

// "Create Hardfile" button clicked
static void cb_create_volume(...)
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
	char str[32];
	sprintf(str, "%d", 512);
	gtk_entry_set_text(GTK_ENTRY(size_entry), str);
	gtk_box_pack_end(GTK_BOX(box), size_entry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_widget_set_hexpand(box, TRUE);

	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser), box);

	g_signal_connect(chooser, "response", G_CALLBACK(on_create_volume_response), size_entry);
	gtk_widget_show(chooser);
}

// "Remove Volume" button clicked
static void cb_remove_volume(...)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(volume_view));
    if (!gtk_tree_selection_get_selected(selection, &volume_store, &toplevel));
    {
        gtk_list_store_remove(GTK_LIST_STORE(volume_store), &toplevel);
    }
}

// "No CD-ROM Driver" button toggled
static void tb_nocdrom(GtkWidget *widget)
{
	PrefsReplaceBool("nocdrom", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}


// Read settings from widgets and set preferences
static void read_volumes_settings(void)
{
    GtkTreePath *path;
	while (PrefsFindString("disk"))
		PrefsRemoveItem("disk");
	if(gtk_tree_model_get_iter_first(volume_store, &toplevel))
	{
    do
    {
        char *item;
        gtk_tree_model_get(volume_store, &toplevel, 0, &item, -1);
        PrefsAddString("disk", item);
    }
    while (gtk_tree_model_iter_next(volume_store, &toplevel));

	PrefsReplaceString("extfs", gtk_entry_get_text(GTK_ENTRY(w_extfs)));
	}

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(w_boot_from)))
        PrefsReplaceInt32("bootdriver", CDROMRefNum);
    else
        PrefsReplaceInt32("bootdriver", 0);
}

static GtkTreeModel *
get_volumes (void)
{
    const char *str;
	int32 index = 0;
    volume_store = GTK_TREE_MODEL(gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING));
	while ((str = (const char *)PrefsFindString("disk", index++)) != NULL)
	{
		gtk_list_store_append (GTK_LIST_STORE(volume_store), &toplevel);
        gtk_list_store_set (GTK_LIST_STORE(volume_store), &toplevel,
                          COLUMN_PATH, str,
                          COLUMN_SIZE, get_file_size(str),
                          -1);
    }
    return volume_store;
}

static GtkWidget *
create_tree_view (void)
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkWidget *view;
    GtkTreeModel *model;

    view = gtk_tree_view_new();
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Location");
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
    gtk_tree_view_column_set_expand(col, true);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", COLUMN_PATH);

    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Size");
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", COLUMN_SIZE);

    model = get_volumes();
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
    g_object_unref(model); /* destroy model automatically with view */
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
                                                            GTK_SELECTION_BROWSE);

    return view;
}

// Create "Volumes" pane
static void create_volumes_pane(GtkWidget *top)
{
	GtkWidget *box, *scroll, *menu, *grid;
    GtkTreeIter iter;

	box = make_pane(top, STR_VOLUMES_PANE_TITLE);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scroll);
	volume_view = create_tree_view();
	gtk_widget_show(volume_view);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
	gtk_widget_show(scroll);
	gtk_container_add(GTK_CONTAINER(scroll), volume_view);
	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
	selected_volume = 0;

	static const opt_desc buttons[] = {
		{STR_ADD_VOLUME_BUTTON, G_CALLBACK(cb_add_volume)},
		{STR_CREATE_VOLUME_BUTTON, G_CALLBACK(cb_create_volume)},
		{STR_REMOVE_VOLUME_BUTTON, G_CALLBACK(cb_remove_volume)},
		{0, NULL},
	};
	make_button_box(box, 0, buttons);
	make_separator(box);

    grid = make_grid(box, 6);
	w_extfs = grid_make_file_entry(grid, 0, STR_EXTFS_CTRL, "extfs", true);

    int initial = (PrefsFindInt32("bootdriver") == CDROMRefNum) ? 1 : 0;
	combo_desc options[] = {STR_BOOT_ANY_LAB, STR_BOOT_CDROM_LAB};
	w_boot_from = grid_make_combo_box(grid, 1, false, STR_BOOTDRIVER_CTRL, options, initial);
	gtk_widget_set_hexpand(w_boot_from, false);

	make_checkbox(box, STR_NOCDROM_CTRL, "nocdrom", G_CALLBACK(tb_nocdrom));
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

// Set sensitivity of widgets
static void set_jit_sensitive(void)
{
	const bool jit_enabled = PrefsFindBool("jit");
}

// "Use JIT Compiler" button toggled
static void tb_jit(GtkWidget *widget)
{
	PrefsReplaceBool("jit", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	set_jit_sensitive();
}

// Read settings from widgets and set preferences
static void read_jit_settings(void)
{
	bool jit_enabled = is_jit_capable() && PrefsFindBool("jit");
}

// "Use built-in 68k DR emulator" button toggled
static void tb_jit_68k(GtkWidget *widget)
{
	PrefsReplaceBool("jit68k", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// Create "JIT Compiler" pane
static void create_jit_pane(GtkWidget *top)
{
	GtkWidget *box, *grid, *label, *menu;
	char str[32];
	
	box = make_pane(top, STR_JIT_PANE_TITLE);

	if (is_jit_capable()) {
		make_checkbox(box, STR_JIT_CTRL, "jit", G_CALLBACK(tb_jit));
		set_jit_sensitive();
	}

	make_checkbox(box, STR_JIT_68K_CTRL, "jit68k", G_CALLBACK(tb_jit_68k));
}


/*
 *  "Graphics/Sound" pane
 */

// Display types
enum {
	DISPLAY_WINDOW,
	DISPLAY_SCREEN
};

static GtkWidget *w_frameskip, *w_display_x, *w_display_y;
static GtkWidget *l_frameskip, *l_display_x, *l_display_y;
static GtkWidget *w_display_mode, *w_wheel_mode;
static int display_type;
static int dis_width, dis_height;
static bool is_fbdev_dga_mode = false;

static GtkWidget *w_dspdevice_file, *w_mixerdevice_file;

// Hide/show graphics widgets
static void hide_show_graphics_widgets(void)
{
	switch (display_type) {
		case DISPLAY_WINDOW:
			gtk_widget_set_sensitive(w_frameskip, true);
			break;
		case DISPLAY_SCREEN:
			gtk_widget_set_sensitive(w_frameskip, false);
			break;
	}
}

// Video type selected
static void mn_display_mode(GtkWidget *widget)
{
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == 1)
    	display_type = DISPLAY_SCREEN;
	else
    	display_type = DISPLAY_WINDOW;

	hide_show_graphics_widgets();
}

// "5 Hz".."60Hz" selected
static void mn_frameskip (GtkWidget *widget)
{
    int item = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    int frameskip = 1;
    switch (item)
    {
		case 0: frameskip = 12; break;
		case 1: frameskip = 8; break;
		case 2: frameskip = 6; break;
		case 3: frameskip = 4; break;
		case 4: frameskip = 2; break;
		case 5: frameskip = 1; break;
    }
    PrefsReplaceInt32("frameskip", frameskip);
}

// QuickDraw acceleration
static void tb_gfxaccel(GtkWidget *widget)
{
	PrefsReplaceBool("gfxaccel", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// Set sensitivity of widgets
static void set_graphics_sensitive(void)
{
	const bool sound_enabled = !PrefsFindBool("nosound");
	gtk_widget_set_sensitive(w_dspdevice_file, sound_enabled);
	gtk_widget_set_sensitive(w_mixerdevice_file, sound_enabled);
}

// "Disable Sound Output" button toggled
static void tb_nosound(GtkWidget *widget)
{
	PrefsReplaceBool("nosound", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	set_graphics_sensitive();
}

// Read and convert graphics preferences
static void parse_graphics_prefs(void)
{
	display_type = DISPLAY_WINDOW;
	dis_width = 640;
	dis_height = 480;

	const char *str = PrefsFindString("screen");
	if (str) {
		if (sscanf(str, "win/%d/%d", &dis_width, &dis_height) == 2)
			display_type = DISPLAY_WINDOW;
		else if (sscanf(str, "dga/%d/%d", &dis_width, &dis_height) == 2)
			display_type = DISPLAY_SCREEN;
#ifdef ENABLE_FBDEV_DGA
		else if (sscanf(str, "fbdev/%d/%d", &dis_width, &dis_height) == 2) {
			is_fbdev_dga_mode = true;
			display_type = DISPLAY_SCREEN;
		}
#endif
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
}

// Read settings from widgets and set preferences
static void read_graphics_settings(void)
{
	const char *str;

	str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_display_x));
	dis_width = atoi(str);

	str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_display_y));
	dis_height = atoi(str);

	char pref[256];
	bool use_screen_mode = true;
	switch (display_type) {
		case DISPLAY_WINDOW:
			sprintf(pref, "win/%d/%d", dis_width, dis_height);
			break;
		case DISPLAY_SCREEN:
			sprintf(pref, "dga/%d/%d", dis_width, dis_height);
			break;
		default:
			use_screen_mode = false;
			PrefsRemoveItem("screen");
			return;
	}
	if (use_screen_mode) {
		PrefsReplaceString("screen", pref);
		// Old prefs are now migrated
		PrefsRemoveItem("windowmodes");
		PrefsRemoveItem("screenmodes");
	}

	PrefsReplaceString("dsp", get_file_entry_path(w_dspdevice_file));
	PrefsReplaceString("mixer", get_file_entry_path(w_mixerdevice_file));
}

// Create "Graphics/Sound" pane
static void create_graphics_pane(GtkWidget *top)
{
	GtkWidget *box, *grid, *label, *opt;
	char str[32];

	parse_graphics_prefs();

	box = make_pane(top, STR_GRAPHICS_SOUND_PANE_TITLE);
	grid = make_grid(box, 6);
	label = gtk_label_new(GetString(STR_VIDEO_TYPE_CTRL));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	w_display_mode = gtk_combo_box_text_new();
	gtk_widget_show(w_display_mode);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_mode), GetString(STR_WINDOW_CTRL), GetString(STR_WINDOW_CTRL));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_mode), GetString(STR_FULLSCREEN_CTRL), GetString(STR_FULLSCREEN_CTRL));
	switch (display_type) {
		case DISPLAY_WINDOW:
			gtk_combo_box_set_active(GTK_COMBO_BOX(w_display_mode), 0);
			break;
		case DISPLAY_SCREEN:
			gtk_combo_box_set_active(GTK_COMBO_BOX(w_display_mode), 1);
			break;
	}
	g_signal_connect(w_display_mode, "changed", G_CALLBACK(mn_display_mode), w_display_mode);
	gtk_grid_attach(GTK_GRID(grid), w_display_mode, 1, 0, 1, 1);

	l_frameskip = gtk_label_new(GetString(STR_FRAMESKIP_CTRL));
	gtk_widget_show(l_frameskip);
	gtk_grid_attach(GTK_GRID(grid), l_frameskip, 0, 1, 1, 1);

	w_frameskip = gtk_combo_box_text_new();
	gtk_widget_show(w_frameskip);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "12",  GetString(STR_REF_5HZ_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "8",  GetString(STR_REF_7_5HZ_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "6", GetString(STR_REF_10HZ_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "4", GetString(STR_REF_15HZ_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "2", GetString(STR_REF_30HZ_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_frameskip), "1", GetString(STR_REF_60HZ_LAB));
	int frameskip = PrefsFindInt32("frameskip");
	int item = -1;
	switch (frameskip) {
		case 12: item = 0; break;
		case 8: item = 1; break;
		case 6: item = 2; break;
		case 4: item = 3; break;
		case 2: item = 4; break;
		case 1: item = 5; break;
		case 0: item = 5; break;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(w_frameskip), item);
	g_signal_connect(w_frameskip, "changed", G_CALLBACK(mn_frameskip), w_frameskip);
	l_display_x = gtk_label_new(GetString(STR_DISPLAY_X_CTRL));
	gtk_widget_show(l_display_x);

	gtk_grid_attach(GTK_GRID(grid), w_frameskip, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), l_display_x, 0, 2, 1, 1);

	w_display_x = gtk_combo_box_text_new_with_entry();
	gtk_widget_show(w_display_x);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_x), "512", GetString(STR_SIZE_512_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_x), "640", GetString(STR_SIZE_640_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_x), "800", GetString(STR_SIZE_800_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_x), "1024", GetString(STR_SIZE_1024_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_x), "0", GetString(STR_SIZE_MAX_LAB));
	if (dis_width >= 0)
		sprintf(str, "%d", dis_width);
	else
		sprintf(str, "%d", 0);
	if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_display_x), str))
	{
	    gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(w_display_x), str, str);
	    gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_display_x), str);
	}
	gtk_grid_attach(GTK_GRID(grid), w_display_x, 1, 2, 1, 1);

	l_display_y = gtk_label_new(GetString(STR_DISPLAY_Y_CTRL));
	gtk_widget_show(l_display_y);
	gtk_grid_attach(GTK_GRID(grid), l_display_y, 0, 3, 1, 1);

	w_display_y = gtk_combo_box_text_new_with_entry();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_y), "384", GetString(STR_SIZE_384_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_y), "480", GetString(STR_SIZE_480_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_y), "600", GetString(STR_SIZE_600_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_y), "768", GetString(STR_SIZE_768_LAB));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_display_y), "0", GetString(STR_SIZE_MAX_LAB));
	if (dis_height >= 0)
		sprintf(str, "%d", dis_height);
	else
		sprintf(str, "%d", 0);
	if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_display_y), str))
	{
	    gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(w_display_y), str, str);
	    gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_display_y), str);
	}
	gtk_widget_show(w_display_y);
	gtk_grid_attach(GTK_GRID(grid), w_display_y, 1, 3, 1, 1);
	make_checkbox(box, STR_GFXACCEL_CTRL, PrefsFindBool("gfxaccel"), G_CALLBACK(tb_gfxaccel));
	make_separator(box);
	make_checkbox(box, STR_NOSOUND_CTRL, "nosound", G_CALLBACK(tb_nosound));
	grid = make_grid(box, 6);
	w_dspdevice_file = grid_make_entry(GTK_GRID(grid), 0, STR_DSPDEVICE_FILE_CTRL, "dsp");
	w_mixerdevice_file = grid_make_entry(GTK_GRID(grid), 1, STR_MIXERDEVICE_FILE_CTRL, "mixer");

	set_graphics_sensitive();

	hide_show_graphics_widgets();
}


/*
 *  "Input" pane
 */

static GtkWidget *w_keycode_file;
static GtkWidget *w_mouse_wheel_lines;

// Set sensitivity of widgets
static void set_input_sensitive(void)
{
	const bool use_keycodes = PrefsFindBool("keycodes");
	gtk_widget_set_sensitive(w_keycode_file, use_keycodes);
	gtk_widget_set_sensitive(GTK_WIDGET(g_object_get_data(G_OBJECT(w_keycode_file), "chooser_button")), use_keycodes);
	gtk_widget_set_sensitive(w_mouse_wheel_lines, PrefsFindInt32("mousewheelmode") == 1);
}

// "Use Raw Keycodes" button toggled
static void tb_keycodes(GtkWidget *widget)
{
	PrefsReplaceBool("keycodes", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	set_input_sensitive();
}

// "Mouse Wheel Mode" selected
static void mn_wheel_mode(GtkWidget *widget)
{
    PrefsReplaceInt32("mousewheelmode", gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
    set_input_sensitive();
}

// Read settings from widgets and set preferences
static void read_input_settings(void)
{
	const char *str = get_file_entry_path(w_keycode_file);
	if (str && strlen(str))
		PrefsReplaceString("keycodefile", str);
	else
		PrefsRemoveItem("keycodefile");

	PrefsReplaceInt32("mousewheellines", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w_mouse_wheel_lines)));
}

// Create "Input" pane
static void create_input_pane(GtkWidget *top)
{
	GtkWidget *box, *hbox, *menu, *label, *button, *grid;
	GtkAdjustment *adj;

	box = make_pane(top, STR_INPUT_PANE_TITLE);

	make_checkbox(box, STR_KEYCODES_CTRL, "keycodes", G_CALLBACK(tb_keycodes));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(GetString(STR_KEYCODE_FILE_CTRL));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	const char *str = PrefsFindString("keycodefile");
	if (str == NULL)
		str = "";

	w_keycode_file = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w_keycode_file), str); 
	gtk_widget_show(w_keycode_file);
	gtk_box_pack_start(GTK_BOX(hbox), w_keycode_file, TRUE, TRUE, 0);

	button = make_browse_button(GTK_ENTRY(w_keycode_file));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(w_keycode_file), "chooser_button", button);

	make_separator(box);
	grid = make_grid(box, 6);
	combo_desc options[] = { STR_MOUSEWHEELMODE_PAGE_LAB, STR_MOUSEWHEELMODE_CURSOR_LAB };
	int wheelmode = PrefsFindInt32("mousewheelmode");
    w_wheel_mode = grid_make_combo_box(grid, 0, false, STR_MOUSEWHEELMODE_CTRL, options, wheelmode, G_CALLBACK(mn_wheel_mode));
	gtk_widget_show(w_wheel_mode);

	label = gtk_label_new(GetString(STR_MOUSEWHEELLINES_CTRL));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	adj = gtk_adjustment_new(PrefsFindInt32("mousewheellines"), 1, 1000, 1, 5, 0);
	w_mouse_wheel_lines = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
	gtk_widget_show(w_mouse_wheel_lines);
	gtk_grid_attach(GTK_GRID(grid), w_mouse_wheel_lines, 1, 1, 1, 1);

	set_input_sensitive();
}


/*
 *  "Serial/Network" pane
 */

static GtkWidget *w_seriala, *w_serialb, *w_ether;

// Read settings from widgets and set preferences
static void read_serial_settings(void)
{
	const char *str;

	str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_seriala));
	PrefsReplaceString("seriala", str);

	str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_serialb));
	PrefsReplaceString("serialb", str);

	str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_ether));
	if (str && strlen(str))
		PrefsReplaceString("ether", str);
	else
		PrefsRemoveItem("ether");
}

// Add names of serial devices
static gint gl_str_cmp(gconstpointer a, gconstpointer b)
{
	return strcmp((char *)b, (char *)a);
}

static GList *add_serial_names(void)
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
static GList *add_ether_names(void)
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

static GtkWidget *make_combo_box(GList *entries, const char *pref)
{
    GtkWidget *combo;
    GList *list;
    list = entries;
    combo = gtk_combo_box_text_new_with_entry();
    gtk_widget_show(combo);
    while (list)
    {
        gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(combo), ((gchar *) list->data), ((gchar *) list->data));
        list = list->next;
    }

    if (pref != NULL && !gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), pref))
    {
        gtk_combo_box_text_prepend(GTK_COMBO_BOX_TEXT(combo), pref, pref);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), pref);
    }
    return combo;
}


// Create "Serial/Network" pane
static void create_serial_pane(GtkWidget *top)
{
	GtkWidget *box, *grid, *label, *combo;
	GList *glist = add_serial_names();

	box = make_pane(top, STR_SERIAL_NETWORK_PANE_TITLE);
	grid = make_grid(box, 6);

	label = gtk_label_new(GetString(STR_SERPORTA_CTRL));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	const char *str = PrefsFindString("seriala");
    w_seriala = make_combo_box(glist, str);
    gtk_widget_set_hexpand(w_seriala, true);
	gtk_grid_attach(GTK_GRID(grid), w_seriala, 2, 1, 1, 1);

	label = gtk_label_new(GetString(STR_SERPORTB_CTRL));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);

	str = PrefsFindString("serialb");
	w_serialb = make_combo_box(glist, str);
	gtk_widget_show(w_serialb);
	gtk_grid_attach(GTK_GRID(grid), w_serialb, 2, 2, 1, 1);

	label = gtk_label_new(GetString(STR_ETHERNET_IF_CTRL));
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);

	glist = add_ether_names();
	str = PrefsFindString("ether");
	w_ether = make_combo_box(glist, str);
	gtk_grid_attach(GTK_GRID(grid), w_ether, 2, 3, 1, 1);
}

/*
 *  "Memory/Misc" pane
 */

static GtkWidget *w_ramsize;
static GtkWidget *w_rom_file;
static GtkWidget *l_nogui;

// Don't use CPU when idle?
static void tb_idlewait(GtkWidget *widget)
{
	PrefsReplaceBool("idlewait", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// Don't show settings window before launching?
static void tb_nogui(GtkWidget *widget)
{
	PrefsReplaceBool("nogui", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	gtk_widget_set_visible(l_nogui, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// "Ignore SEGV" button toggled
static void tb_ignoresegv(GtkWidget *widget)
{
	PrefsReplaceBool("ignoresegv", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

// Read settings from widgets and set preferences
static void read_memory_settings(void)
{
	const char *str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_ramsize));
	PrefsReplaceInt32("ramsize", atoi(str) << 20);

	str = gtk_entry_get_text(GTK_ENTRY(w_rom_file));
	if (str && strlen(str))
		PrefsReplaceString("rom", str);
	else
		PrefsRemoveItem("rom");
}

// Create "Memory/Misc" pane
static void create_memory_pane(GtkWidget *top)
{
	GtkWidget *box, *hbox, *grid, *label, *menu;

	box = make_pane(top, STR_MEMORY_MISC_PANE_TITLE);
	grid = make_grid(box, 6);

	static const combo_desc options[] = {
		STR_RAMSIZE_16MB_LAB,
		STR_RAMSIZE_32MB_LAB,
		STR_RAMSIZE_64MB_LAB,
		STR_RAMSIZE_128MB_LAB,
		STR_RAMSIZE_256MB_LAB,
		STR_RAMSIZE_512MB_LAB,
		STR_RAMSIZE_1024MB_LAB,
		0
	};
	char default_ramsize[16];
	sprintf(default_ramsize, "%d", PrefsFindInt32("ramsize") >> 20);
	w_ramsize = grid_make_combo_box(grid, 0, true, STR_RAMSIZE_CTRL, options, 3);
	if(!gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_ramsize), default_ramsize))
	    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w_ramsize), default_ramsize, default_ramsize);
	    gtk_combo_box_set_active_id(GTK_COMBO_BOX(w_ramsize), default_ramsize);
    gtk_widget_show(w_ramsize);
	w_rom_file = grid_make_file_entry(grid, 1, STR_ROM_FILE_CTRL, "rom");

	make_checkbox(box, STR_IGNORESEGV_CTRL, "ignoresegv", G_CALLBACK(tb_ignoresegv));
	make_checkbox(box, STR_IDLEWAIT_CTRL, "idlewait", G_CALLBACK(tb_idlewait));
	make_checkbox(box, STR_NOGUI_CTRL, "nogui", G_CALLBACK(tb_nogui));
	l_nogui = gtk_label_new(GetString(STR_NOGUI_TIP2));
	gtk_widget_set_halign(l_nogui, GTK_ALIGN_START);
	gtk_widget_set_margin_start(l_nogui, 24);
	gtk_widget_set_visible(l_nogui, PrefsFindBool("nogui"));
	gtk_box_pack_start(GTK_BOX(box), l_nogui, FALSE, FALSE, 0);
}


/*
 *  Read settings from widgets and set preferences
 */

static void read_settings(void)
{
	read_volumes_settings();
	read_graphics_settings();
	read_input_settings();
	read_serial_settings();
	read_memory_settings();
	read_jit_settings();
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


/*
 *  Display alert
 */

static void dl_destroyed(void)
{
	gtk_main_quit();
}

static void display_alert(int title_id, int prefix_id, int button_id, const char *text)
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

void ErrorAlert(const char *text)
{
	display_alert(STR_ERROR_ALERT_TITLE, STR_GUI_ERROR_PREFIX, STR_QUIT_BUTTON, text);
}


/*
 *  Display warning alert
 */

void WarningAlert(const char *text)
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

static void sigchld_handler(int sig, siginfo_t *sip, void *)
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

int main(int argc, char *argv[])
{
	// Init GTK
	gtk_set_locale();
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

		return 0;	}


	return 0;}
#endif
