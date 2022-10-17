/*
 *  prefs_editor.h - Preferences editor
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

#ifndef PREFS_EDITOR_H
#define PREFS_EDITOR_H

#ifdef __BEOS__
extern void PrefsEditor(uint32 msg);
#else
extern bool PrefsEditor(void);
#endif

#ifdef ENABLE_GTK
static GtkWidget *create_tree_view (void);
static void cb_add_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void cb_create_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void cb_remove_volume (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static GList *add_serial_names(void);
static GList *add_ether_names(void);
static void save_volumes(void);
static void get_graphics_settings(void);
static bool is_jit_capable(void);
static void hide_widget(GtkWidget *widget);
extern "C" {
void dl_quit(GtkWidget *dialog);
void cb_swap_opt_cmd (GtkWidget *widget);
void cb_infobar_show (GtkWidget *widget);
}
#endif
#endif
