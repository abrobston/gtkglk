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

/* #included from fileref.c */
#include "support.h"
#include "g_bindable.h"

static void gglk_tree_fref_display(frefid_t fref, GtkWidget *widget)
{
    GtkWidget *w;
    char buf[256];

    w = lookup_widget(widget, "notebook1");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 3);

    w = lookup_widget(widget, "object_ID");
    sprintf(buf, "%d", fref->age);
    gtk_label_set_text(GTK_LABEL(w), buf);

    g_bindable_bind(
	g_bindable_new_from_integer_collect(&fref->rock, gglk_bindings),
	g_bindable_new_from_glade(widget, "rock"),
	NULL);

    g_bindable_bind(
	g_bindable_new_from_integer_collect(&fref->disprock.num,gglk_bindings),
	g_bindable_new_from_glade(widget, "disprock"),
	NULL);

    g_bindable_bind(
	g_bindable_new_from_integer_collect(&fref->orig_fmode, gglk_bindings),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "fmode_Read"),
	    filemode_Read),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "fmode_Write"),
	    filemode_Write),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "fmode_ReadWrite"),
	    filemode_ReadWrite),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "fmode_WriteAppend"),
	    filemode_WriteAppend),
	NULL);
}



void gglk_tree_store_populate_fref(frefid_t fref, 
				  GtkTreeStore *store, GtkTreeIter *parent)
{
    GtkTreeIter iter;
    if(fref == NULL)
	return;
    gtk_tree_store_append(store, &iter, parent);
    gtk_tree_store_set(store, &iter,
		       0, fref->age,
		       1, "fref",
		       3, gglk_tree_fref_display,
		       4, fref,
		       -1);
}
