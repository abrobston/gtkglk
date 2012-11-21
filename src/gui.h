/*  GtkGlk - Glk Implementation using Gtk+
    Copyright (C) 2003-2004  Evin Robertson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    The author can be reached at evin@users.sf.net  */

#ifndef GUI_H
#define GUI_H

#include "gglk.h"
#include <gconf/gconf-client.h>

extern GHashTable *gglk_bindings PRIVATE;
extern GConfChangeSet *gglk_style_changeset PRIVATE;

void gglk_update_bindings(void) PRIVATE;

void gglk_object_tree_update(void) PRIVATE;

void gglk_make_popup_menu(void) PRIVATE;

void gglk_do_glk_objects(GtkMenuItem *unused_menuitem,
			 gpointer unused_data) PRIVATE;
void gglk_do_styles(GtkMenuItem *unused_menuitem,
		    gpointer unused_data) PRIVATE;
void gglk_do_gestalt(GtkMenuItem *unused_menuitem,
		     gpointer unused_data) PRIVATE;
void gglk_do_about(GtkMenuItem *unused_menuitem,
		   gpointer unused_data) PRIVATE;
void gglk_do_preferences(GtkMenuItem *unused_menuitem,
		   gpointer unused_data) PRIVATE;

/* gui_window.c */
void gglk_tree_store_populate_win(winid_t win, 
				  GtkTreeStore *store, GtkTreeIter *parent) PRIVATE;
/* gui_stream.c */
void gglk_tree_store_populate_str(strid_t str, 
				  GtkTreeStore *store, GtkTreeIter *parent) PRIVATE;
/* gui_fileref.c */
void gglk_tree_store_populate_fref(frefid_t fref, 
				   GtkTreeStore *store, GtkTreeIter *parent) PRIVATE;
/* gui_style.c */
void gglk_style_toggle(GtkWidget *widget, gpointer unused_data) PRIVATE;
void gglk_style_gui(GtkWidget *widget, glui32 style) PRIVATE;

#endif

