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
#include "gglk.h"
#include "g_bindable.h"
#include <gdk/gdkkeysyms.h>


glui32 glk_char_to_lower_ucs4(glui32 ch)
{
    return g_unichar_tolower(ch);
}
glui32 glk_char_to_upper_ucs4(glui32 ch)
{
    return g_unichar_toupper(ch);
}
glui32 glk_char_to_title_ucs4(glui32 ch)
{
    return g_unichar_totitle(ch);
}


/* Events may be added from other threads (e.g., SDL's audio thread).
   Use a lock here so they don't clobber each other. */
G_LOCK_DEFINE_STATIC(gglk_event_queue);

static GQueue *gglk_event_queue = NULL;

static gboolean gglk_timer_elapsed = FALSE;


static gint ev_compare(gconstpointer a, gconstpointer b)
{
    const event_t *av = a, *bv = b;
    if(av->type == bv->type && av->win == bv->win
       && av->val1 == bv->val1 && av->val2 == bv->val2)
	return 0;
    return 1;
}

static inline gboolean gglk_queue_find_custom(GQueue *q, gconstpointer data,
				       GCompareFunc func)
{
    if(q->head && g_list_find_custom(gglk_event_queue->head, data, func))
	return TRUE;
    if(q->tail && g_list_find_custom(gglk_event_queue->tail, data, func))
	return TRUE;
    return FALSE;
}


gboolean gglk_trigger_event(glui32 evtype, winid_t win, glui32 val1, glui32 val2)
{
#if 0
    static const char *evnames[] = {
	"None", "Timer", "CharInput", "LineInput", "MouseInput",
	"Arrange", "Redraw", "SoundNotify", "Hyperlink" 
    };
    g_message("%s: %d %d", evnames[evtype], val1, val2);
#endif

    G_LOCK(gglk_event_queue);

    if(evtype == evtype_Arrange) {
	gglk_window_resize_all_in(win);
    }
    
    if(evtype == evtype_Timer) {
	gglk_timer_elapsed = TRUE;
    } else if(gglk_event_queue) {
	event_t *ev = g_malloc(sizeof(event_t));
	ev->type = evtype;
	ev->win  = win;
	ev->val1 = val1;
	ev->val2 = val2;

	/* Check:
	   1. We should never have two identical events in queue
	   2. gestalt preferences permit this event 
	   3. the window requested this event (check may cancel the request) */
	if(gglk_queue_find_custom(gglk_event_queue, ev, ev_compare)
	   || !gglk_gestalt_validate_event(evtype)
	   || (win && !gglk_win_validate_event(evtype, win))) {

	    g_free(ev);
	    G_UNLOCK(gglk_event_queue);
	    return FALSE;
	}
	
	g_queue_push_tail(gglk_event_queue, ev);
    }
    if(gtk_main_level())
	gtk_main_quit();
    G_UNLOCK(gglk_event_queue);
    return TRUE;
}

static gboolean gglk_get_event(event_t *event)
{
    event_t moo;

    G_LOCK(gglk_event_queue);

    if(!gglk_event_queue)
	gglk_event_queue = g_queue_new();
    do {
	if(g_queue_is_empty(gglk_event_queue)) {
	    if(gglk_timer_elapsed) {
		moo.type = evtype_Timer;
		moo.win = NULL;
		moo.val1 = moo.val2 = 0;
		gglk_timer_elapsed = FALSE;
	    } else {
		G_UNLOCK(gglk_event_queue);
		return FALSE;
	    }
	} else {
	    event_t *ev = g_queue_pop_head(gglk_event_queue);
	    moo = *ev;
	    g_free(ev);
	}
	if(event)
	    *event = moo;

	/* Make sure we don't return events for nonexistent windows */
    } while(moo.win != NULL && !gglk_stillvalid(moo.win, gidisp_Class_Window));

    G_UNLOCK(gglk_event_queue);
    return TRUE;
}


void glk_select_poll(event_t *event)
{
    if(gglk_get_event(event))
	return;

    gtk_widget_show_all(gglk_root);

    gglk_auto_scroll();

    while (gtk_events_pending())
    	gtk_main_iteration();

    gglk_set_focus();

    gtk_main_iteration_do(FALSE);
    if(!gglk_get_event(event) && event) {
	event->type = evtype_None;
	event->win = NULL;
	event->val1 = event->val2 = 0;
    }
}

void glk_select(event_t *event)
{
    gglk_free_pic_cache();
    g_bindable_collect_poll(gglk_bindings);

    glk_select_poll(event);
    if(event->type != evtype_None)
	return;

    while(!gglk_get_event(event))
	gtk_main();
}



static gboolean hit_timer(gpointer unused)
{
    gglk_trigger_event(evtype_Timer, NULL, 0, 0);
    return TRUE;
}


static int was_timing = 0;
static guint timer_id;
void glk_request_timer_events(glui32 millisecs)
{
    if(was_timing) {
	g_source_remove(timer_id);
	was_timing = 0;
    }
    if(millisecs) {
	timer_id = g_timeout_add(millisecs, hit_timer, NULL);
	was_timing = 1;
    }
}



static const guint keytable[keycode_MAXVAL] = {
    GDK_VoidSymbol,
    GDK_Left,    GDK_Right,     GDK_Up,      GDK_Down,
    GDK_Return,  GDK_Delete,    GDK_Escape,  GDK_Tab,
    GDK_Page_Up, GDK_Page_Down, GDK_Home,    GDK_End,
    GDK_VoidSymbol, GDK_VoidSymbol, GDK_VoidSymbol,
    GDK_F1,      GDK_F2,        GDK_F3,      GDK_F4,
    GDK_F5,      GDK_F6,        GDK_F7,      GDK_F8,
    GDK_F9,      GDK_F10,       GDK_F11,     GDK_F12
};

glui32 gglk_keycode_gdk_to_glk(guint k)
{
    int i;
    if(k < 255)
	return k;
    for(i = 0; i < keycode_MAXVAL; i++)
	if(keytable[i] == k)
	    return keycode_Unknown - i;
    if(k == GDK_BackSpace)
	return keycode_Delete;
    return keycode_Unknown;
}

guint gglk_keycode_glk_to_gdk(glui32 k)
{
    if(k <= 255)
	return k;
    if(keycode_Unknown - k < keycode_MAXVAL)
	return keytable[keycode_Unknown - k];
    return GDK_VoidSymbol;
}


int gglk_printinfo(void)
{
    winid_t win = NULL; strid_t str = NULL;
    frefid_t fref = NULL; schanid_t chan = NULL;

    fprintf(stderr, PACKAGE_STRING " implementing Glk 0.6.1\n");

    /* check all objects */
    while((win = glk_window_iterate(win, NULL)) != NULL)
	glk_window_get_rock(win);
    while((str = glk_stream_iterate(str, NULL)) != NULL)
	glk_stream_get_rock(str);
    while((fref = glk_fileref_iterate(fref, NULL)) != NULL)
	glk_fileref_get_rock(fref);
    while((chan = glk_schannel_iterate(chan, NULL)) != NULL)
	glk_schannel_get_rock(chan);
    return 0;
}

