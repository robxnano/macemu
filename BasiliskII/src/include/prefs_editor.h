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
static GList *add_serial_names(void);
static GList *add_ether_names(void);
static void save_volumes(void);
static void get_graphics_settings(void);
static bool is_jit_capable(void);
static void hide_widget(GtkWidget *widget);

extern "C" {
void cb_browse(GtkWidget *widget, GtkWidget *entry);
void cb_browse_dir(GtkWidget *widget, GtkWidget *entry);
void cb_entry(GtkWidget *widget);
void cb_check_box(GtkWidget *widget);
bool cb_switch(GtkWidget *widget);
bool cb_switch_inv(GtkWidget *widget);
void cb_spin_button(GtkWidget *widget);
void cb_combo_int(GtkWidget *widget);
void cb_combo_str(GtkWidget *widget);
void cb_combo_entry_int(GtkWidget *widget);
void cb_combo_entry_str(GtkWidget *widget);
void cb_ramsize(GtkWidget *widget);
void cb_screen_mode(GtkWidget *widget);
void cb_infobar_show(GtkWidget *widget);
void cb_hotkey(GtkWidget *widget);
void cb_cpu(GtkWidget *widget);
void cb_elevate_response(GtkWidget *widget, int response);
void cb_swap_opt_cmd(GtkWidget *widget);
void cb_format_mb(GtkWidget *widget);
void dl_quit(GtkWidget *widget);
gchar* ram_slider_fmt(GtkWidget *widget, double value);
}
#endif
#endif
