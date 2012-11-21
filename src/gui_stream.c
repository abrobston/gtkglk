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

/* #included from stream.c */
#include "support.h"
#include "g_bindable.h"




static void gglk_tree_stream_display(strid_t str, GtkWidget *widget)
{
    GtkWidget *w;
    char buf[256];

    static const char names[] = 
	"rock" "\0"       "disprock" "\0"
	"read_count" "\0" "write_count" "\0"
	"length" "\0"     "index" "\0";
    int *ints[] = {
	&str->rock,       &str->disprock.num,
	&str->readcount,  &str->writecount,
	&str->length,     &str->index 
    };
    const char *p;
    int i;
    
    w = lookup_widget(widget, "notebook1");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 2);

    w = lookup_widget(widget, "object_ID");
    sprintf(buf, "%d", str->age);
    gtk_label_set_text(GTK_LABEL(w), buf);

    i = 0;
    for(p = names; *p; p += strlen(p)) {
	g_bindable_bind(
	    g_bindable_new_from_integer_collect(ints[i], gglk_bindings),
	    g_bindable_new_from_glade(widget, p),
	    NULL);
	i++;
    }
    g_bindable_bind(
	g_bindable_new_from_integer_collect(&str->fmode, gglk_bindings),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "str_fmode_Read"),
	    filemode_Read),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "str_fmode_Write"),
	    filemode_Write),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "str_fmode_ReadWrite"),
	    filemode_ReadWrite),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_glade(widget, "str_fmode_WriteAppend"),
	    filemode_WriteAppend),
	NULL);


    w = lookup_widget(widget, "scrolledwindow3");
    if(str->gbuffer) {
	gtk_widget_show(w);
	w = lookup_widget(widget, "textview1");
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(w),
				 gtk_text_view_get_buffer(
				     GTK_TEXT_VIEW(str->gbuffer)));
    } else {
	gtk_widget_hide(w);
    }
}

void gglk_tree_store_populate_str(strid_t str, 
				  GtkTreeStore *store, GtkTreeIter *parent)
{
    GtkTreeIter iter;
    char buf[100];
    if(str == NULL)
	return;

    sprintf(buf, "%s Stream", gglk_subtype_names(GGLK_MAGIC_STR, str->type));

    gtk_tree_store_append(store, &iter, parent);
    gtk_tree_store_set(store, &iter,
		       0, str->age,
		       1, buf,
		       2, str->echo ? str->echo->age : 0,
		       3, gglk_tree_stream_display,
		       4, str,
		       -1);
}
