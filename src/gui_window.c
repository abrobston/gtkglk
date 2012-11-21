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

/* #included from window.c */
#include "g_bindable.h"
#include "support.h"
#include <string.h>

static gboolean gglk_window_do_framed(gboolean framed, gboolean write,
				      gpointer data[2])
{
    winid_t win = data[0];

    if(write) {
	if(framed) gglk_win_add_frame(win, FALSE);
	else       gglk_win_remove_frame(win);
	if(win->parent) {
	    /*glk_window_set_arrangement(win->parent, win->parent->method,
	      win->parent->size, win->parent->key);*/
	    gglk_trigger_event(evtype_Arrange, win, 0, 0);
	}
    }

    return win->frame != NULL;
}

static gint gglk_window_do_size(gint size, gboolean write,
				gpointer data[2])
{
    winid_t win = data[0];
    
    if(write) {
	glk_window_set_arrangement(win, win->method, size, win->key);
	gtk_widget_queue_draw(win->widget);
	gglk_trigger_event(evtype_Arrange, win, 0, 0);
    }

    return win->size;
}

static gint gglk_window_do_dir(gint dir, gboolean write,
			       gpointer data[2])
{
    winid_t win = data[0];

    if(write) {
	glk_window_set_arrangement(win, (win->method & ~winmethod_DirMask)
				   | dir,
				   win->size, win->key);
	gtk_widget_queue_draw(win->widget);
	gglk_trigger_event(evtype_Arrange, win, 0, 0);
    }

    return win->method & winmethod_DirMask;
}


static gboolean gglk_window_do_fixed(gboolean v, gboolean write,
				     gpointer data[2])
{
    winid_t win = data[0];
    
    if(write) {
	glk_window_set_arrangement(win, (win->method & ~winmethod_DivisionMask)
				   | (v ? winmethod_Fixed :
				      winmethod_Proportional),
				   win->size, win->key);
	gtk_widget_queue_draw(win->widget);
	gglk_trigger_event(evtype_Arrange, win, 0, 0);
    }

    return (win->method & winmethod_DivisionMask) == winmethod_Fixed;    
}


static gboolean gglk_drawing_split_expose(GtkWidget *widget, 
					  GdkEventExpose *event,
					  gpointer user_data)
{
    winid_t win = user_data;
    GdkRectangle b[2];
    int i;

    for(i = 0; i < 2; i++) {
	gglk_window_calc_size_from_parent(win->child[i],
					  &b[i].width, &b[i].height,
					  widget->allocation.width,
					  widget->allocation.height);
    }

    b[0].x = 0;
    b[0].y = 0;

    switch(win->method & winmethod_DirMask) {
    case winmethod_Left:
    case winmethod_Right:
	b[1].x = b[0].width;
	b[1].y = 0;
	break;
    case winmethod_Above:
    case winmethod_Below:
	b[1].x = 0;
	b[1].y = b[0].height;
	break;
    }
    
    for(i = 0; i < 2; i++) {
	char buf[100];
	int text_width, text_height;
	PangoLayout *layout;
	sprintf(buf, "%d", win->child[i]->age);
	layout = gtk_widget_create_pango_layout(widget, buf);
	pango_layout_get_pixel_size(layout, &text_width, &text_height);
	gtk_paint_layout(widget->style, widget->window, widget->state,
			 FALSE, &event->area, widget, NULL,
			 b[i].x + (b[i].width - text_width) / 2,
			 b[i].y + (b[i].height - text_height) / 2,
			 layout);
	/* FIXME: do I need to destroy the layout??? */
	gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL,
			 GTK_SHADOW_IN, &event->area, widget, NULL,
			 b[i].x, b[i].y, b[i].width, b[i].height);
    }
    return TRUE;
}



static void gglk_win_emit(GtkButton *button, gpointer user_data)
{
    winid_t win = user_data;
    int evtype =GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "evtype"));
    glui32 val1 = 0, val2 = 0;
    
    switch(evtype) {
    case evtype_CharInput:
	val1 = gtk_spin_button_get_value_as_int(
	    GTK_SPIN_BUTTON(lookup_widget(GTK_WIDGET(button), "val1_char")));
	break;
    case evtype_LineInput:
	if(win->line_buffer_latin1) 
	    ;
	/* FIXME */
	break;
    case evtype_MouseInput:
	val1 = gtk_spin_button_get_value_as_int(
	    GTK_SPIN_BUTTON(lookup_widget(GTK_WIDGET(button), "val1_mouse")));
	val2 = gtk_spin_button_get_value_as_int(
	    GTK_SPIN_BUTTON(lookup_widget(GTK_WIDGET(button), "val2_mouse")));
	break;
    case evtype_Redraw:
	if(win->type == wintype_Graphics)
	    glk_window_clear(win);
	break;
    }

    gglk_trigger_event(evtype, win, val1, val2);
}


static void gglk_win_name_char(GtkSpinButton *spinbutton,
			       gpointer user_data)
{
    GtkLabel *char_name = GTK_LABEL(user_data);
    glui32 v = gtk_spin_button_get_value_as_int(spinbutton);
    static const char keynames[keycode_MAXVAL][9] = {
	"Unknown",
	"Left",    "Right",   "Up",      "Down",
	"Return",  "Delete",  "Escape",  "Tab",
	"PageUp",  "PageDown","Home",    "End",
	"Unknown", "Unknown", "Unknown",
	"Func1",   "Func2",   "Func3",   "Func4",
	"Func5",   "Func6",   "Func7",   "Func8",
	"Func9",   "Func10",  "Func11",  "Func12"
    };
    
    char name[64];
    
    if (v >= 1 && v <= 26) {
	sprintf(name, "(Control-%c)", 'A' + v - 1);
    } else if(v == 32) {
	strcpy(name, "(space)");
    } else if(v > 32 && v <= 255) {
	char *utf = g_ucs4_to_utf8(&v, 1, NULL, NULL, NULL);
	sprintf(name, "(%s)", utf);
	g_free(utf);
    } else {
	if(keycode_Unknown - v >= keycode_MAXVAL)
	    v = keycode_Unknown;
	sprintf(name, "(%s)", keynames[keycode_Unknown - v]);
    }

    gtk_label_set_text(char_name, name);
}


static void gglk_tree_window_display(winid_t win, GtkWidget *widget)
{
    GtkWidget *w;
    char buf[256];
    int i;
    static gulong gglk_drawing_split_expose_handler = 0;

    w = lookup_widget(widget, "notebook1");

    gtk_notebook_set_current_page(GTK_NOTEBOOK(w),
				  (win->type == wintype_Pair) ? 0 : 1);

    w = lookup_widget(widget, "object_ID");
    sprintf(buf, "%d", win->age);
    gtk_label_set_text(GTK_LABEL(w), buf);

    g_bindable_bind(
	g_bindable_new_from_integer_collect(&win->rock, gglk_bindings),
	g_bindable_new_from_glade(widget, "rock"),
	NULL);

    g_bindable_bind(
	g_bindable_new_from_integer_collect(&win->disprock.num, gglk_bindings),
	g_bindable_new_from_glade(widget, "disprock"),
	NULL);
    
    if(win->type == wintype_Pair) {
	g_bindable_bind(
	    g_bindable_collect(g_bindable_new_from_func_integer(
				   gglk_window_do_size,
				   win, NULL),
			       gglk_bindings),
	    g_bindable_new_from_glade(widget, "size"),
	    NULL);

	g_bindable_bind(
	    g_bindable_collect(g_bindable_new_from_func_integer(
				   gglk_window_do_dir,
				   win, NULL),
			       gglk_bindings),
	    g_bindable_new_view_bool_int(
		g_bindable_new_from_glade(widget, "method_left"),
		winmethod_Left),
	    g_bindable_new_view_bool_int(
		g_bindable_new_from_glade(widget, "method_right"),
		winmethod_Right),
	    g_bindable_new_view_bool_int(
		g_bindable_new_from_glade(widget, "method_above"),
		winmethod_Above),
	    g_bindable_new_view_bool_int(
		g_bindable_new_from_glade(widget, "method_below"),
		winmethod_Below),
	    NULL);
	g_bindable_bind(
	    g_bindable_collect(g_bindable_new_from_func_boolean(
				   gglk_window_do_fixed,
				   win, GINT_TO_POINTER(winmethod_Fixed)),
			       gglk_bindings),
	    g_bindable_new_from_glade(widget, "method_fixed"),
	    NULL);

	w = lookup_widget(widget, "drawingarea1");
	if(gglk_drawing_split_expose_handler && 
	   g_signal_handler_is_connected(w,
					 gglk_drawing_split_expose_handler))
	    g_signal_handler_disconnect(w,
					gglk_drawing_split_expose_handler);
	gglk_drawing_split_expose_handler = g_signal_connect(
	    w, "expose_event",
	    G_CALLBACK(gglk_drawing_split_expose), win);
	gtk_widget_queue_draw(w);
	
    } else {
	static const char event_names[] =
	    "char" "\0"     "line" "\0"    "mouse" "\0"    "arrange" "\0"
	    "redraw" "\0"   "sound" "\0"   "hyper" "\0";
	const char *p;

	g_bindable_bind(
	    g_bindable_collect(g_bindable_new_from_func_boolean(
				   gglk_window_do_framed,
				   win, NULL),
			       gglk_bindings),
	    g_bindable_new_from_glade(widget, "framed"),
	    NULL);

	i = evtype_CharInput;

	for(p = event_names; *p; p += strlen(p) + 1, i++) {
	    GtkObject *obj;
	    
	    if(i == evtype_SoundNotify)
		continue;
	    sprintf(buf, "request_%s", p);
	    obj = GTK_OBJECT(lookup_widget(widget, buf));

	    if(obj)
		g_bindable_bind(
		    g_bindable_new_from_boolean_collect(&win->request[i],
							gglk_bindings),
		    g_bindable_new_from_gtkobject(obj),
		    NULL);
	    
	    sprintf(buf, "emit_%s", p);
	    obj = GTK_OBJECT(lookup_widget(widget, buf));

	    if(obj) {
		g_bindable_bind(
		    g_bindable_new_from_boolean_collect(&win->request[i],
							gglk_bindings),
		    g_bindable_new_from_gtkobject_prop(obj, "sensitive"),
		    NULL);
		
		g_object_set_data(G_OBJECT(obj), "evtype", GINT_TO_POINTER(i));
		g_signal_connect(obj, "clicked",
				 G_CALLBACK(gglk_win_emit), win);
	    }
	}
	    
	g_signal_connect(lookup_widget(widget, "val1_char"), "value_changed",
			 G_CALLBACK(gglk_win_name_char),
			 lookup_widget(widget, "name_char"));
    }
}


	
void gglk_tree_store_populate_win(winid_t win, 
				  GtkTreeStore *store, GtkTreeIter *parent)
{
    GtkTreeIter iter;
    char buf[100];
    if(win == NULL)
	return;

    sprintf(buf, "%s Window", gglk_subtype_names(GGLK_MAGIC_WIN, win->type));

    gtk_tree_store_append(store, &iter, parent);
    gtk_tree_store_set(store, &iter,
		       0, win->age,
		       1, buf,
		       2, win->key ? win->key->age : 0,
		       3, gglk_tree_window_display,
		       4, win,
		       -1);

    gglk_tree_store_populate_str(win->stream, store, &iter);
    gglk_tree_store_populate_win(win->child[0], store, &iter);
    gglk_tree_store_populate_win(win->child[1], store, &iter);
}
