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

#include <stdlib.h>
#include <stdio.h>
#include "gglk.h"
#include "glkstart.h"
#include "interface.h"
#include "support.h"
#include "g_bindable.h"

GtkWidget *gglk_win;
GtkWidget *gglk_root;
static GtkWidget *gglk_statusbar;
int gglk_statusbar_context_sglk, gglk_statusbar_context_exit,
    gglk_statusbar_context_more;

static GtkIMContext* gglk_context;

glui32 gglk_age = 1;
gboolean gglk_frame = FALSE;

GHashTable *gglk_bindings;



gboolean gglk_isvalid(const void *obj,
		      glui32 goodmagic, const glui32 *realmagic,
		      const char *function)
{
    if(obj == NULL) {
	g_warning("%s: NULL %s", function, gglk_magic_to_typename(goodmagic));
	return FALSE;
    }
    if(*realmagic != goodmagic) {
	if(*realmagic == GGLK_MAGIC_FREE)
	    g_warning("%s: using a freed object", function);
	else
	    g_warning("%s: %s object not a %s.", function,
		      gglk_magic_to_typename(*realmagic),
		      gglk_magic_to_typename(goodmagic));
	return FALSE;
    }
    return TRUE;
}


gboolean gglk_isvalidtypes(const void *obj,
			   glui32 goodmagic, const glui32 *realmagic,
			   const char *function, const glui32 *realtype,
			   glui32 goodtype1, glui32 goodtype2)
{
    if(!gglk_isvalid(obj, goodmagic, realmagic, function))
	return FALSE;
    if(*realtype != goodtype1 && *realtype != goodtype2) {
	if(goodtype2 != 0xffffffff)
	    g_warning("%s: expected %s or %s but found %s", function,
		      gglk_subtype_names(goodmagic, goodtype1),
		      gglk_subtype_names(goodmagic, goodtype2),
		      gglk_subtype_names(goodmagic, *realtype));
	else
	    g_warning("%s: expected %s but found %s", function,
		      gglk_subtype_names(goodmagic, goodtype1),
		      gglk_subtype_names(goodmagic, *realtype));
	return FALSE;
    }
    return TRUE;
}


static gint gglk_really_die_now(GtkWidget *unused_widget, ...)
{
    gtk_main_quit();
    gglk_free_pic_cache();
    gglk_free_blorb();
    if(glk_window_get_root())
	glk_window_close(glk_window_get_root(), NULL);
    if(gglk_win)
	gtk_widget_destroy(gglk_win);
    exit(0);
}


void glk_exit(void)
{
    if(gglk_win) {
	event_t ev;
	sglk_window_set_title("Hit any key to exit");
	gglk_status_set_mesg("Hit any key to exit",
			     gglk_statusbar_context_exit);
	gglk_auto_scroll();
	g_signal_connect(gglk_win, "key-press-event",
			 GTK_SIGNAL_FUNC(gglk_really_die_now), NULL);
	g_signal_connect(gglk_win, "delete_event",
			 GTK_SIGNAL_FUNC(gglk_really_die_now), NULL);
	for(;;) {
	    glk_select(&ev);
	}
    }
    gglk_really_die_now(NULL);
}

static void (*user_interrupt)(void);

void glk_set_interrupt_handler(void (*func)(void))
{
    user_interrupt = func;
}


static gint gglk_byebye_event(GtkWidget *unused_widget, ...)
{
    if(user_interrupt)
	user_interrupt();
    gglk_really_die_now(NULL);
    return TRUE;
}

static gboolean gglk_window_resize(GtkWidget *unused_widget,
				   GdkEventConfigure *event,
				   gpointer unused)
{
    static int oldwidth, oldheight;

    if(event->width != oldwidth || event->height != oldheight) {
	gglk_trigger_event(evtype_Arrange, glk_window_get_root(), 0, 0);
    }
    oldwidth  = event->width;
    oldheight = event->height;

    return FALSE;
}



void sglk_window_set_title(const char *title)
{
    gtk_window_set_title(GTK_WINDOW(gglk_win), title);    
}


void gglk_status_set_mesg(const char *message, int context)
{
    gtk_statusbar_pop(GTK_STATUSBAR(gglk_statusbar), context);
    if(message)
	gtk_statusbar_push(GTK_STATUSBAR(gglk_statusbar), context, message);
}

void sglk_status_set_mesg(const char *message)
{
    gglk_status_set_mesg(message, gglk_statusbar_context_sglk);
}




int sglk_init(int argc, char **argv, glkunix_startup_t *data)
{
    GtkWidget *widget;
    
    g_thread_init(NULL);
    gdk_threads_init();

    if(!gtk_init_check(&argc, &argv))
	return FALSE;
    
    data->argc = argc;
    data->argv = argv;

    gglk_win = create_gglk_win();
    gglk_root = lookup_widget(gglk_win, "gglk_root");

    gglk_statusbar = lookup_widget(gglk_win, "statusbar");
    gglk_statusbar_context_sglk =
	gtk_statusbar_get_context_id(GTK_STATUSBAR(gglk_statusbar),
				     "sglk message");
    gglk_statusbar_context_exit =
	gtk_statusbar_get_context_id(GTK_STATUSBAR(gglk_statusbar),
				     "exit message");
    gglk_statusbar_context_more =
	gtk_statusbar_get_context_id(GTK_STATUSBAR(gglk_statusbar),
				     "more message");
    
    gtk_window_set_default_size(GTK_WINDOW(gglk_win), 600, 400);
    gtk_widget_show(gglk_win);
    gtk_widget_show(gglk_root);

    gglk_bindings = g_hash_table_new(NULL, NULL);
    gglk_init_styles();
    gglk_init_fref();
    gglk_init_gestalt();
    gglk_prefs_init();

    sglk_window_set_title(g_get_prgname());

    gglk_make_popup_menu();

    gglk_context = gtk_im_multicontext_new();
    widget = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(lookup_widget(gglk_root,
							  "input_methods1")),
			      widget);
    gtk_im_multicontext_append_menuitems(GTK_IM_MULTICONTEXT(gglk_context),
					 GTK_MENU_SHELL(widget));
    
    g_signal_connect(gglk_win, "delete_event",
		     GTK_SIGNAL_FUNC(gglk_byebye_event), NULL);
    g_signal_connect(gglk_win, "configure-event",
		     GTK_SIGNAL_FUNC(gglk_window_resize), NULL);

    return TRUE;
}
