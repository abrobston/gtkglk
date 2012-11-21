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

/* #included from style.c */
#include "support.h"
#include <string.h>
#include <stdio.h>
#include "g_bindable.h"
#include "gglk.h"

#if 0
void gglk_style_toggle(GtkWidget *widget, gpointer unused_data)
{
    int h;
    
    for(h = 0; h < stylehint_NUMHINTS + 2; h++) {
	char buf[50];
	GtkToggleButton *toggle;
	sprintf(buf, "important_%s", gglk_get_hint(h));
	toggle = GTK_TOGGLE_BUTTON(lookup_widget(widget, buf));
	gtk_toggle_button_set_active(toggle,
				     !gtk_toggle_button_get_active(toggle));
    }
}
#endif

static GtkWidget *gui_style_widget;

static struct glk_stylehint_struct preview_styles;
static struct glk_stylehint_important_struct preview_important_styles;

static void gglk_style_update_preview(int style, int hint)
{
    GtkTreeSelection *select;
    GtkTreeIter iter;
    GtkTreeModel *model;

#if !GTK_CHECK_VERSION(2,2,0)
    return;
#endif

    select = gtk_tree_view_get_selection(
	GTK_TREE_VIEW(lookup_widget(gui_style_widget, "treeview_styles")));
    if(gtk_tree_selection_get_selected (select, &model, &iter)) {
	GtkTextView *preview = GTK_TEXT_VIEW(
	    lookup_widget(gui_style_widget, "preview"));
	GtkTextTag *tag;
	
	gint disp_style;
	gtk_tree_model_get(model, &iter,
			   1, &disp_style,
			   -1);

	if(style != style_AllStyles && disp_style != style)
	    return;

	tag = gtk_text_tag_table_lookup(
	    gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer(preview)),
	    "default");

	gglk_adjust_tag(tag, hint, 
			gglk_get_stylehint_val(NULL,
					       &preview_styles,
					       &preview_important_styles,
					       style, hint));
    }
}

static void gglk_trigger_preview(GValue *unused_value, gboolean write,
				gpointer data[3])
{
    if(write) {
	int style = GPOINTER_TO_INT(data[0]);
	int hint  = GPOINTER_TO_INT(data[1]);
	gglk_style_update_preview(style, hint);
    }
}



void gglk_style_gui(GtkWidget *widget, glui32 style)
{
    int h;
    int i;
    GtkWidget *w;

    gui_style_widget = widget;

    for(h = 0; h < stylehint_NUMHINTS + 2; h++) {
	char buf[50];
	char key[200];
	GBindable *model, *view;
	
	sprintf(buf, "style_%s", gglk_get_hint(h));
	sprintf(key, "styles/%s/%s",
		style==style_AllStyles ? "AllStyles" : gglk_get_tag(style),
		gglk_get_hint(h));
	model = g_bindable_new_from_gconf_changeset(gglk_style_changeset,
						    key);
	view = NULL;
	
	switch(h) {
	case stylehint_Justification:
	    model = g_bindable_new_model_cast(
		GLK_TYPE_JUSTIFICATION_TYPE, model);
	    
	    {
		static const char names[] =
		    "style_justdefault" "\0"/* -1 */
		    "style_justleft" "\0"   /* 0 = stylehint_just_LeftFlush */
		    "style_justfill" "\0"   /* 1 = stylehint_just_LeftRight */
		    "style_justcenter" "\0" /* 2 = stylehint_just_Centered */
		    "style_justright" "\0"; /* 3 = stylehint_just_RightFlush */
		const char *p;
		
		i = -1;
		for(p = names; *p; p += strlen(p) + 1) {
		    g_bindable_bind(model,
				    g_bindable_new_view_bool_int(
					g_bindable_new_from_glade(widget, p),
					i),
				    NULL);
		    i++;
		}
	    }
	    break;

	case stylehint_Proportional:
	    w = lookup_widget(widget, "style_family");
	    if(!gtk_option_menu_get_menu(GTK_OPTION_MENU(w))) {
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_items;

		menu_items = gtk_menu_item_new_with_label("(Default)");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_items);
		gtk_widget_show(menu_items);

		for(i = 0; i < num_families; i++) {
		    menu_items = gtk_menu_item_new_with_label(
			pango_font_family_get_name(family_list[i]));
		    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_items);
		    gtk_widget_show(menu_items);
		}

		gtk_option_menu_set_menu(GTK_OPTION_MENU(w), menu);
	    }
	    model = g_bindable_new_model_transform(model,
						   G_TYPE_INT,
						   gglk_font_transform,
						   NULL);
	    view = g_bindable_new_view_add_const(
		g_bindable_new_from_gtkobject(GTK_OBJECT(w)), -1);
	    break;

	case stylehint_TextColor:
	case stylehint_BackColor:
	    model = g_bindable_new_model_cast(GDK_TYPE_COLOR, model);
	    view = g_bindable_new_from_gtkobject_prop(
		GTK_OBJECT(lookup_widget(widget, buf)), "color");
	    break;

	default:
	    view = g_bindable_new_from_glade(widget, buf);
	    break;
	}

	if(model && view)
	    g_bindable_bind(model, view, NULL);


	sprintf(buf, "important_%s", gglk_get_hint(h));
	strcat(key, "_Important");
	g_bindable_bind(g_bindable_new_from_gconf_changeset(
			    gglk_style_changeset,
			    key),
			g_bindable_new_from_glade(widget, buf),
			NULL);

    }
    gglk_styles_bind_gconf(gglk_style_changeset, &preview_styles,
			   &preview_important_styles, gglk_trigger_preview);
}
