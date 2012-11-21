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

#include <stdio.h>
#include <string.h>
#include "gglk.h"
#include "gui.h"
#include "interface.h"
#include "support.h"
#include "g_bindable.h"
#include "g_intbits.h"

static void gglk_objects_tree_selection_changed(GtkTreeSelection *selection,
						gpointer data)
{   
    GtkTreeIter iter;
    GtkTreeModel *model;

    if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	void (*display)(winid_t, GtkWidget *);
	winid_t win;
	GtkWidget *widget = GTK_WIDGET(data);
	
	gtk_tree_model_get(model, &iter,
			   3, &display,
			   4, &win,
			   -1);
	display(win, widget);
    }
}

static GtkWidget *win_glk_objects;

void gglk_object_tree_update(void)
{
    GtkTreeView *tree;
    GtkTreeStore *store;
    GtkTreeSelection *select;
    GtkTreeIter iter;
    winid_t win;
    strid_t str;
    frefid_t fref;
    GHashTable *streams;

    if(!win_glk_objects)
	return;

    gglk_bindings = g_hash_table_new(NULL, NULL);
    
    tree = GTK_TREE_VIEW(lookup_widget(win_glk_objects, "treeview1"));
    store = GTK_TREE_STORE(gtk_tree_view_get_model(tree));
    gtk_tree_store_clear(store);
    
    gglk_tree_store_populate_win(glk_window_get_root(), store, NULL);

    streams = g_hash_table_new(NULL, NULL);
    win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	str = glk_window_get_stream(win);
	g_hash_table_insert(streams, str, win);
    }
    str = NULL;
    while((str = glk_stream_iterate(str, NULL)) != NULL) {
	win = g_hash_table_lookup(streams, str);
	if(!win)
	    gglk_tree_store_populate_str(str, store, NULL);
    }
    g_hash_table_destroy(streams);

    fref = NULL;
    while((fref = glk_fileref_iterate(fref, NULL)) != NULL) {
	gglk_tree_store_populate_fref(fref, store, NULL);
    }
    gtk_tree_view_expand_all(tree);

    select = gtk_tree_view_get_selection(tree);
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	gtk_tree_selection_select_iter(select, &iter);
}


static void gglk_key_render(GtkTreeViewColumn *tree_column,
			    GtkCellRenderer *cell,
			    GtkTreeModel *tree_model,
			    GtkTreeIter *iter,
			    gpointer data)
{
    gint val;
    char buf[100] = "";
    gtk_tree_model_get(tree_model, iter, GPOINTER_TO_INT(data), &val, -1);
    
    if(val != 0)
	sprintf(buf, "%d", val);

    g_object_set(GTK_CELL_RENDERER(cell), "text", buf, NULL);
}



void gglk_do_glk_objects(GtkMenuItem *unused_menuitem,
			 gpointer unused_data)
{
    
    GtkTreeView *tree;
    GtkTreeStore *store;
    GtkTreeSelection *select;

    if(win_glk_objects) {
	gtk_window_present(GTK_WINDOW(win_glk_objects));
	return;
    }
    
    win_glk_objects = create_win_glk_objects();
    g_signal_connect(win_glk_objects, "destroy",
		     G_CALLBACK(gtk_widget_destroyed),
		     &win_glk_objects);
    
    {
	GtkSizeGroup *group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	int i;
	const char *p;
	static const char group_widget_names[] =
	    "char" "\0"     "line" "\0"
	    "mouse" "\0"    "hyper" "\0"
	    "arrange" "\0"  "redraw" "\0";
	
	for(p = group_widget_names; *p; p += strlen(p) + 1) {
	    char name[30];
	    sprintf(name, "request_%s", p);
	    gtk_size_group_add_widget(group,
				      lookup_widget(win_glk_objects, name));
	}

	for(i = 1; i <= 15; i++) {
	    char labelname[30];
	    sprintf(labelname, "glabel%d", i);
	    gtk_size_group_add_widget(group, 
				      lookup_widget(win_glk_objects,
						    labelname));
	}

	g_object_unref(group);
    }


    tree = GTK_TREE_VIEW(lookup_widget(win_glk_objects, "treeview1"));
    store = gtk_tree_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT,
			          G_TYPE_POINTER, G_TYPE_POINTER);
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(store));

    gtk_tree_view_insert_column_with_attributes(tree, -1, "ID",
						gtk_cell_renderer_text_new(),
						"text", 0,
						NULL);
    gtk_tree_view_insert_column_with_attributes(tree, -1, "Type",
						gtk_cell_renderer_text_new(),
						"text", 1,
						NULL);
    gtk_tree_view_insert_column_with_data_func(tree, -1, "Key",
					       gtk_cell_renderer_text_new(),
					       gglk_key_render,
					       GINT_TO_POINTER(2), NULL);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(select, "changed",
		     G_CALLBACK(gglk_objects_tree_selection_changed),
		     win_glk_objects);

    gglk_object_tree_update();
    
    gtk_widget_show(win_glk_objects);
}




static void gglk_style_selection_changed(GtkTreeSelection *selection,
					 gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	GtkWidget *widget = GTK_WIDGET(data);
	gint style;
	
	gtk_tree_model_get(model, &iter,
			   1, &style,
			   -1);
	gglk_style_gui(widget, style);
    }
}

GConfChangeSet *gglk_style_changeset;


static void gglk_style_response(GtkDialog *dialog,
				gint response,
				gpointer unused_data)
{
    switch(response) {
    case GTK_RESPONSE_APPLY:
	if(gglk_style_changeset)
	    gconf_client_commit_change_set(gconf_client_get_default(),
					   gglk_style_changeset,
					   TRUE,
					   NULL);
	break;
    case GTK_RESPONSE_OK:
	if(gglk_style_changeset)
	    gconf_client_commit_change_set(gconf_client_get_default(),
					   gglk_style_changeset,
					   TRUE,
					   NULL);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	break;
    case GTK_RESPONSE_CANCEL:
	gtk_widget_destroy(GTK_WIDGET(dialog));
	break;
    }
}

void gglk_do_styles(GtkMenuItem *unused_menuitem,
		    gpointer unused_data)
{
    static GtkWidget *dialog_styles;

    GtkTreeView *tree;
    GtkTreeStore *store;
    GtkTreeSelection *select;
    GtkTreeIter parent_iter, iter;
    int i;

    if(dialog_styles) {
	gtk_window_present(GTK_WINDOW(dialog_styles));
	return;
    }

    dialog_styles = create_dialog_styles();
    g_signal_connect(dialog_styles, "destroy",
		     G_CALLBACK(gtk_widget_destroyed),
		     &dialog_styles);
    g_signal_connect(dialog_styles, "response",
		     G_CALLBACK(gglk_style_response),
		     NULL);

    if(!gglk_style_changeset) {
	gglk_style_changeset = gconf_change_set_new();
    } else {
	gconf_change_set_clear(gglk_style_changeset);
    }

    {
	GtkTextBuffer *buffer;
	GtkTextIter text_start, text_end;

	buffer = gtk_text_view_get_buffer(
	    GTK_TEXT_VIEW(lookup_widget(dialog_styles, "preview")));
	gtk_text_buffer_create_tag(buffer, "default", NULL);
	gtk_text_buffer_get_bounds(buffer, &text_start, &text_end);
	gtk_text_buffer_apply_tag_by_name(buffer, "default",
					  &text_start, &text_end);
    }

    tree = GTK_TREE_VIEW(lookup_widget(dialog_styles, "treeview_styles"));
    store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(store));

    gtk_tree_view_insert_column_with_attributes(tree, -1, "Style",
						gtk_cell_renderer_text_new(),
						"text", 0,
						NULL);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    g_signal_connect(select, "changed",
		     G_CALLBACK(gglk_style_selection_changed),
		     dialog_styles);

    gtk_tree_store_append(store, &parent_iter, NULL);
    gtk_tree_store_set(store, &parent_iter,
		       0, gglk_get_tag(style_AllStyles),
		       1, style_AllStyles,
		       -1);
    gtk_tree_selection_select_iter(select, &parent_iter);

    for(i = 0; i < style_NUMSTYLES; i++) {
	gtk_tree_store_append(store, &iter, &parent_iter);
	gtk_tree_store_set(store, &iter,
			   0, gglk_get_tag(i),
			   1, i,
			   -1);
    }

    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter,
		       0, gglk_get_tag(style_TextGrid),
		       1, style_TextGrid,
		       -1);

    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter,
		       0, gglk_get_tag(style_Hyperlinks),
		       1, style_Hyperlinks,
		       -1);

    gtk_tree_view_expand_all(tree);

#if 0
    g_signal_connect(lookup_widget(dialog_styles, "important_toggle"),
		     "clicked", G_CALLBACK(gglk_style_toggle), NULL);
#endif
    
    gtk_widget_show(dialog_styles);

}





static void gglk_radio_toggled(GtkCellRendererToggle *cell, gchar *path_str,
			       gpointer user_data)
{

  GtkTreeModel *model = GTK_TREE_MODEL(user_data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;

  gint *column;

  column = g_object_get_data (G_OBJECT (cell), "column");

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);

  /* toggle the item on */
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, TRUE, -1);

  /* clean up */
  gtk_tree_path_free (path);
}



static void gglk_do_gestalt_line(GtkTreeStore *store, GtkTreeIter *iter,
				 const char *text, glui32 gestnum)
{
    GtkTreeModel *model = GTK_TREE_MODEL(store);
    GtkTreePath *path;
    GIntbits *enable_status;
    const char *key_base = gglk_gestalt_get_name(gestnum);
    char key[100];

    gtk_tree_store_set(store, iter,
		       0, text,
		       1, gestnum,
		       -1);
    path = gtk_tree_model_get_path(model, iter);
    enable_status = g_intbits_new(); /* FIXME: memory leak */
    sprintf(key, "gestalt/%s_Enable", key_base);
    g_bindable_bind(
	g_bindable_new_from_gconf(key),
	g_bindable_new_from_gobject(G_OBJECT(enable_status), "bit1"),
	NULL);
    sprintf(key, "gestalt/%s_Claim", key_base);
    g_bindable_bind(
	g_bindable_new_from_gconf(key),
	g_bindable_new_from_gobject(G_OBJECT(enable_status), "bit0"),
	NULL);
    g_bindable_bind(
	g_bindable_new_from_gobject(G_OBJECT(enable_status), "value"),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_gtktreemodel(model, path, 2), 3),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_gtktreemodel(model, path, 3), 0),
	g_bindable_new_view_bool_int(
	    g_bindable_new_from_gtktreemodel(model, path, 4), 1),
	NULL);
}



void gglk_do_gestalt(GtkMenuItem *unused_menuitem,
		     gpointer unused_data)
{
    static GtkWidget *dialog_gestalt;
    GtkTreeView *tree;
    GtkTreeStore *store;

    if(dialog_gestalt) {
	gtk_window_present(GTK_WINDOW(dialog_gestalt));
	return;
    }

    dialog_gestalt = create_dialog_gestalt();
    g_signal_connect(dialog_gestalt, "destroy",
		     G_CALLBACK(gtk_widget_destroyed),
		     &dialog_gestalt);

    g_bindable_bind(
	g_bindable_new_from_gconf("gestalt/Version"),
	g_bindable_new_from_glade(dialog_gestalt, "gestalt_claim_version"),
	NULL);

    tree = GTK_TREE_VIEW(lookup_widget(dialog_gestalt,
				       "enable_disable_pretend"));
    store = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_INT,
			       G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(store));

    gtk_tree_view_insert_column_with_attributes(tree, -1, "Gestalt",
						gtk_cell_renderer_text_new(),
						"text", 0,
						NULL);
    
    {
	int i;
	static const char column_names[] =
	    "Enable" "\0" "Disable" "\0" "Pretend" "\0";
	const char *p;

	i = 2;
	for(p = column_names; *p; p += strlen(p) + 1) {
	    GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
	    gtk_cell_renderer_toggle_set_radio(
		GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
	    g_object_set_data(G_OBJECT(renderer), "column",GINT_TO_POINTER(i));
	    g_signal_connect(renderer, "toggled",
			     G_CALLBACK(gglk_radio_toggled), store);
	    gtk_tree_view_insert_column_with_attributes(tree, -1, p,
							renderer,
							"active", i,
							NULL);
	    i++;
	}
    }
    {
	static const char line_names[] =
	    "Mouse Input" "\0"
	    "Timer Events" "\0"
	    "Unicode" "\0"
	    "Graphics" "\0"
	    " Draw Image" "\0"
	    " Transparency" "\0"
	    "Sound" "\0"
	    " Volume" "\0"
	    " Notify Events" "\0"
	    " Music Resources" "\0"
	    "Hyperlinks" "\0"
	    " Input Events" "\0";
	static const glui32 line_nums[] = {
	    gestalt_MouseInput,
	    gestalt_Timer,
	    gestalt_Unicode,
	    gestalt_Graphics,
	    gestalt_DrawImage,
	    gestalt_GraphicsTransparency,
	    gestalt_Sound,
	    gestalt_SoundVolume,
	    gestalt_SoundNotify,
	    gestalt_SoundMusic,
	    gestalt_Hyperlinks,
	    gestalt_HyperlinkInput
	};
	glui32 i;
	const char *p;
	GtkTreeIter parent_iter, iter;
	i = 0;
	for(p = line_names; *p; p += strlen(p) + 1) {
	    if(*p == ' ') {
		gtk_tree_store_append(store, &iter, &parent_iter);
		gglk_do_gestalt_line(store, &iter, p+1, line_nums[i]);
	    } else {
		gtk_tree_store_append(store, &parent_iter, NULL);
		gglk_do_gestalt_line(store, &parent_iter, p, line_nums[i]);
	    }
	    i++;
	}
    }

    gtk_tree_view_expand_all(tree);
    
    gtk_widget_show(dialog_gestalt);
}


void gglk_do_preferences(GtkMenuItem *unused_menuitem,
			 gpointer unused_data)
{
    static GtkWidget *dialog_prefs;
    
    if(dialog_prefs){
	gtk_window_present(GTK_WINDOW(dialog_prefs));
	return;
    }
    dialog_prefs = create_dialog_prefs();
    g_signal_connect(dialog_prefs, "destroy",
		     G_CALLBACK(gtk_widget_destroyed),
		     &dialog_prefs);

    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-menu"),
	g_bindable_new_from_glade(dialog_prefs, "showmenu"),
	NULL);
    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-statusbar"),
	g_bindable_new_from_glade(dialog_prefs, "showstatusbar"),
	NULL);
    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-scrollbars"),
	g_bindable_new_from_glade(dialog_prefs, "usescrollbars"),
	NULL);
    

    gtk_widget_show(dialog_prefs);
}



static char *gglk_about_extra = NULL;

void gglk_do_about(GtkMenuItem *unused_menuitem,
		     gpointer unused_data)
{
    static GtkWidget *dialog_about;

    if(dialog_about) {
	gtk_window_present(GTK_WINDOW(dialog_about));
	return;
    }
    dialog_about = create_dialog_about();
    g_signal_connect(dialog_about, "destroy",
		     G_CALLBACK(gtk_widget_destroyed),
		     &dialog_about);

    if(gglk_about_extra) {
	GtkWidget *textview_about = lookup_widget(dialog_about, "textview_about");
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview_about));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	gtk_text_buffer_insert(buffer, &iter, gglk_about_extra, -1);
    }

    gtk_widget_show(dialog_about);
}


/* Stolen from GTK documentation */
static gboolean my_popup_handler(GtkWidget *widget, GdkEvent *event)
{
    GtkMenu *menu;
    GdkEventButton *event_button;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(GTK_IS_MENU (widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    /* The "widget" is the menu that was supplied when
     * g_signal_connect_swapped() was called.
     */
    menu = GTK_MENU (widget);

    if(event->type == GDK_BUTTON_PRESS) {
	event_button = (GdkEventButton *) event;
	if(event_button->button == 3) {
	    gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
			    event_button->button, event_button->time);
	    return TRUE;
	}
    }
    return FALSE;
}


#include "glk-icon.h"

void gglk_make_popup_menu(void) 
{
    GtkWidget *menu = lookup_widget(gglk_win, "menu_glk_menu");
    GtkWidget *about;
    g_signal_connect_swapped(gglk_root, "button_press_event",
			     G_CALLBACK(my_popup_handler), menu);

    about = gtk_image_menu_item_new_with_mnemonic("_About");
    gtk_image_menu_item_set_image(
	GTK_IMAGE_MENU_ITEM(about),
	gtk_image_new_from_pixbuf(
	    gdk_pixbuf_new_from_inline(-1, glkicon, FALSE, NULL)));
    gtk_container_add(GTK_CONTAINER(menu), about);
    g_signal_connect(about, "activate", G_CALLBACK(gglk_do_about),
		     NULL);
    gtk_widget_show_all(about);
}

