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

#include "glkpaned.h"
#include "gtkhpaned.h"
#include "gtkvpaned.h"

#include "gglk.h"
#include "support.h"

#include "gglk_graphics.h"
#include "gglk_text.h"
#include "gglk_textbuffer.h"
#include "gglk_textgrid.h"


struct glk_window_struct {
    glui32 magic, rock, age, type;
    GList *node;
    gidispatch_rock_t disprock;

    GtkWidget *widget;   /* Will be a Box containing frame or content */
    GtkWidget *frame;    /* NULL or a GtkFrame or a GtkHandleBox */
    GtkWidget *content;  /* A scrolled window, or gbuffer or ggraphics */
    GglkText *gbuffer;   /* For a TextBuffer or TextGrid */
    GglkGraphics *ggraphics;

    winid_t parent;
    strid_t stream;
    int units[2];

    gboolean request[9];

    char *line_buffer_latin1; glui32 *line_buffer_ucs4;
    glui32 line_maxlen;
    gidispatch_rock_t line_rock;

    /* For pair windows: */
    winid_t child[2];
    glui32 method, size;
    winid_t key;
};

static winid_t root_window = NULL;

#define gidisp_Class_window gidisp_Class_Window
MAKE_ROCK(winid_t, window, gglk_validwin)
MAKE_ITER(winid_t, window)


static const int interp_mode = GDK_INTERP_HYPER;
static GtkPolicyType scroll_policy = GTK_POLICY_AUTOMATIC;


static void gglk_pair_arrange(GtkWidget *widget, gpointer user_data);

static void gglk_mouse_event(GtkWidget *unused_widget, gint x, gint y,
			     gpointer user_data);
static void gglk_char_input_event(GtkWidget *unused_widget,
				  int ch, gpointer user_data);
static void gglk_line_input_event(GtkWidget *unused_widget,
				  gpointer user_data);
static void gglk_hyper_input_event(GtkWidget *unused_widget,
				   int linkval, gpointer user_data);


static inline void gglk_set_focus_win(winid_t win)
{
    if(win->gbuffer) {
#if ! GTK_CHECK_VERSION(2,2,2)	
        GTK_TEXT_VIEW(win->gbuffer)->disable_scroll_on_focus = TRUE;
#endif
	gtk_widget_grab_focus(GTK_WIDGET(win->gbuffer));
    }
}


/* Figures out which window to give focus */
void gglk_set_focus(void)
{
    winid_t focus = NULL, scroll_win = NULL, line_win = NULL, char_win = NULL;
    winid_t win;
    win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	gboolean allow_focus = FALSE;

	if(win->gbuffer) {

	    if(gglk_text_can_scroll(win->gbuffer)) {
		/* If the window is less than one line tall, or one chanacter
		   wide, no amount of scrolling will help us out. */
		glui32 width, height;
		glk_window_get_size(win, &width, &height);
		if(width >= 1 && height >= 1) {
		    scroll_win = win;
		    allow_focus = TRUE;
		}
	    }
	    if(win->request[evtype_CharInput]) {
		char_win = win;
		allow_focus = TRUE;
	    }
	    if(win->request[evtype_LineInput]) {
		line_win = win;
		allow_focus = TRUE;
	    }
	    if(gtk_widget_is_focus(GTK_WIDGET(win->gbuffer)) && allow_focus) {
		focus = win;
	    }

	    if(allow_focus)
		GTK_WIDGET_SET_FLAGS(win->gbuffer, GTK_CAN_FOCUS);
	    else
		GTK_WIDGET_UNSET_FLAGS(win->gbuffer, GTK_CAN_FOCUS);
	}
    }

    /* If currently focused window can scroll, stay here */
    if(focus && focus->gbuffer && gglk_text_can_scroll(focus->gbuffer))
	scroll_win = focus;
    
    /* If some other window can scroll, go there */
    if(scroll_win) {
	gglk_set_focus_win(scroll_win);

	gglk_status_set_mesg("[MORE]", gglk_statusbar_context_more);

	return;
    }

    gglk_status_set_mesg(NULL, gglk_statusbar_context_more);

    /* If currently focused window can accept input, stay here */
    if(focus != NULL && (focus->request[evtype_CharInput] ||
			 focus->request[evtype_LineInput]))
	return;

    /* If somewhere can accept line input, go there */
    if(line_win) {
	gglk_set_focus_win(line_win);
	return;
    }

    /* If somewhere can accept character input, go there */
    if(char_win) {
	gglk_set_focus_win(char_win);
	return;
    }
}


void gglk_auto_scroll(void)
{
    winid_t win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	if(win->gbuffer)
	    gglk_text_auto_scroll(win->gbuffer);
    }
}


static void gglk_update_scroll(void)
{
    winid_t win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	if(win->gbuffer) {
	    gglk_text_update_scrollmark(win->gbuffer);
	    win->gbuffer->should_scroll = TRUE;
	}
    }
}

void gglk_win_set_scroll_policy(GtkPolicyType p)
{
    winid_t win = NULL;
    
    scroll_policy = p;
    
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	if(win->type == wintype_TextBuffer && win->content) {
	    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win->content),
					   GTK_POLICY_NEVER,
					   scroll_policy);
	}
    }
}



GtkWidget *gglk_get_line_input_view(void)
{
    winid_t focus = NULL, line_win = NULL;
    
    winid_t win;
    win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL) {
	if(win->gbuffer && win->request[evtype_LineInput]) {
	    if(gtk_widget_is_focus(GTK_WIDGET(win->gbuffer)))
		focus = win;
	    line_win = win;
	}
    }
    if(focus)
	return GTK_WIDGET(focus->gbuffer);
    if(line_win)
	return GTK_WIDGET(line_win->gbuffer);
    return NULL;
}


GtkWidget *gglk_get_selection_view(void)
{
    winid_t win = NULL;
    while((win = glk_window_iterate(win, NULL)) != NULL){
	if(win->gbuffer) {
	    GtkTextBuffer *buffer;
	    GtkTextIter sel_start, sel_end;
	    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->gbuffer));
	    if(gtk_text_buffer_get_selection_bounds(buffer,
						    &sel_start, &sel_end)) {
		return GTK_WIDGET(win->gbuffer);
	    }
	}
    }
    return NULL;
}



winid_t glk_window_get_root(void) 
{
    return root_window;
}


static winid_t gglk_create_win(glui32 wintype, glui32 rock)
{
    winid_t win  = g_malloc(sizeof(struct glk_window_struct));
    int i;
    
    win->magic   = GGLK_MAGIC_WIN;
    win->rock    = rock;
    win->age     = gglk_age++;
    win->type    = wintype;

    win->widget  = NULL;
    win->frame   = NULL;
    win->content = NULL;
    win->gbuffer = NULL;
    win->ggraphics = NULL;

    win->stream = NULL;

    for(i = 0; i < 9; i++)
	win->request[i] = FALSE;
    win->request[evtype_Arrange] = TRUE;

    win->line_rock = dispa_bad;
    win->line_buffer_latin1 = NULL; win->line_buffer_ucs4 = NULL;

    win->units[0] = win->units[1] = 1;
    win->parent = win->child[0] = win->child[1] = win->key = NULL;
    win->method = win->size = 0;

    win->node = gglk_window_add(win);
    return win;
}


gboolean gglk_win_contains(winid_t a, winid_t b)
{
    if(a == NULL) return FALSE;
    if(a == b) return TRUE;
    return gglk_win_contains(a->child[0], b)
	|| gglk_win_contains(a->child[1], b);
}


static inline gboolean gglk_win_method_left_above(winid_t win) G_GNUC_PURE;
static inline gboolean gglk_win_method_left_right(winid_t win) G_GNUC_PURE;
static inline gboolean gglk_win_method_proportional(winid_t win) G_GNUC_PURE;

static inline gboolean gglk_win_method_left_above(winid_t win)
{
    glui32 method = win->method & winmethod_DirMask;
    return method == winmethod_Left || method == winmethod_Above;
}

static inline gboolean gglk_win_method_left_right(winid_t win)
{
    glui32 method = win->method & winmethod_DirMask;
    return method == winmethod_Left || method == winmethod_Right;
}

static inline gboolean gglk_win_method_proportional(winid_t win)
{
    return (win->method & winmethod_DivisionMask) == winmethod_Proportional;
}



static void gglk_window_place(winid_t parent,
			      winid_t placed_child)
{
    glui32 size;

    if(parent == NULL) {
	gtk_container_add(GTK_CONTAINER(gglk_root), placed_child->widget);
	return;
    }

    size = parent->size;
    gtk_widget_set_size_request(placed_child->widget, 0, 0);
    
    if(gglk_win_method_proportional(parent)) {

	if(!gglk_win_method_left_above(parent))
	    size = 100 - parent->size;

	if(placed_child == parent->child[0])
	    gtk_paned_pack1(GTK_PANED(parent->widget), placed_child->widget,
			    TRUE, TRUE);
	else
	    gtk_paned_pack2(GTK_PANED(parent->widget), placed_child->widget,
			    TRUE, TRUE);
	g_object_set_data(G_OBJECT(parent->widget), "percent",
			  GINT_TO_POINTER(size));
    } else {
	int coord[2];
	GtkAttachOptions opt[2];

	gboolean h = gglk_win_method_left_right(parent);
	
	coord[h] = 0;
	opt[h]  = GTK_SHRINK | GTK_EXPAND | GTK_FILL;

	coord[!h] = (placed_child == parent->child[1]);

	if(gglk_win_method_left_above(parent) == coord[!h]) {
	    opt[!h] = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
	} else {
	    opt[!h] = GTK_SHRINK;
	    
	    size *= parent->key->units[!h];
	    if(placed_child->gbuffer && h == 1)
		size += 5; /* For margins */
	    if(placed_child->frame) {
		if(h == 1)
		    size += placed_child->frame->style->xthickness * 2;
		else
		    size += placed_child->frame->style->ythickness * 2;
	    }
	    if(h == 1)
		gtk_widget_set_size_request(placed_child->widget, size, 0);
	    else
		gtk_widget_set_size_request(placed_child->widget, 0, size);
	}
	
	gtk_table_attach(GTK_TABLE(parent->widget), placed_child->widget,
			 coord[0], coord[0]+1,
			 coord[1], coord[1]+1, opt[0], opt[1], 0, 0);
    }
}


static void gglk_window_close_recurse(winid_t win)
{
    if(win == NULL)
	return;
    
    if(win->type == wintype_Pair) {
	gglk_window_close_recurse(win->child[0]); win->child[0] = NULL;
	gglk_window_close_recurse(win->child[1]); win->child[1] = NULL;
    }

    if(win->stream) {
	glk_stream_close(win->stream, NULL);
	win->stream = NULL;
    }

    gglk_window_del(win);
    win->magic = GGLK_MAGIC_FREE;

    g_free(win);
}


static inline void gglk_win_child_replace(winid_t win, winid_t oldchild, winid_t newchild)
{
    if(win->child[0] == oldchild)
	win->child[0] = newchild;
    else
	win->child[1] = newchild;
}


static inline void gglk_window_remove_split(winid_t pair,
					    winid_t goodchild)
{
    GtkWidget *pairparentwidget;
    
    gglk_win_child_replace(pair, goodchild, NULL);

    goodchild->parent = pair->parent;
    if(pair->parent) {
	pairparentwidget = pair->parent->widget;
	gglk_win_child_replace(pair->parent, pair, goodchild);
    } else {
	pairparentwidget = gglk_root;
	root_window = goodchild;
    }
    
    g_object_ref(G_OBJECT(goodchild->widget));
    gtk_container_remove(GTK_CONTAINER(pair->widget), goodchild->widget);
    gtk_container_remove(GTK_CONTAINER(pairparentwidget), pair->widget);

    gglk_window_place(pair->parent, goodchild);
    g_object_unref(G_OBJECT(goodchild->widget));

    gglk_window_close_recurse(pair);
    gglk_window_resize_all_in(goodchild);
}



void glk_window_close(winid_t win, stream_result_t *result)
{
    gglk_validwin(win, return);
    if(win->stream) {
	glk_stream_close(win->stream, result);
	win->stream = NULL;
    }
    
    if(win->parent) {
	gglk_window_remove_split(win->parent, glk_window_get_sibling(win));
    } else {
	if(win->widget)
	    gtk_container_remove(GTK_CONTAINER(gglk_root), win->widget);
	gglk_window_close_recurse(win);
	root_window = NULL;
    }

    gglk_object_tree_update();
}



void glk_window_set_echo_stream(winid_t win, strid_t str)
{
    gglk_validwin(win, return);
    gglk_stream_set_echo(win->stream, str);
}
strid_t glk_window_get_echo_stream(winid_t win)
{
    gglk_validwin(win, return NULL);
    return gglk_stream_get_echo(win->stream);
}


static void gglk_line_input_event(GtkWidget *unused_widget,
				  gpointer user_data)
{
    winid_t win = user_data;
    event_t ev;

    glk_cancel_line_event(win, &ev);
    gglk_update_scroll();
    gglk_trigger_event(evtype_LineInput, win, ev.val1, ev.val2);
}

static void gglk_check_char_line_events(winid_t win, const char *called_from)
{
    if(win->request[evtype_CharInput]) {
	g_warning("%s: char event already requested", called_from);
	glk_cancel_char_event(win);
    }
    if(win->request[evtype_LineInput]) {
	g_warning("%s: line event already requested", called_from);
	glk_cancel_line_event(win, NULL);
    }
}


void glk_request_line_event(winid_t win, char *buf,
			    glui32 maxlen,
			    glui32 initlen)
{
    glui32 i;

    GGLK_WIN_TYPES(win, return, wintype_TextBuffer, wintype_TextGrid);
    gglk_check_char_line_events(win, __func__);
    
    win->request[evtype_LineInput] = TRUE;
    if(initlen > maxlen)
	initlen = maxlen;
    win->line_buffer_latin1 = buf; win->line_maxlen = maxlen;
    
    {
	gunichar buf_ucs4[initlen + 1];
	for(i = 0; i < initlen; i++)
	    buf_ucs4[i] = buf[i];
	buf_ucs4[i] = 0;
	gglk_text_line_input_set(win->gbuffer, maxlen, buf_ucs4);
    }

    win->line_rock = gglk_dispa_newmem(win->line_buffer_latin1, win->line_maxlen,
				       "&+#!Cn");
}

 void glk_request_line_event_ucs4(winid_t win, glui32 *buf, glui32 maxlen,
				  glui32 initlen)
{
    glui32 i;

    GGLK_WIN_TYPES(win, return, wintype_TextBuffer, wintype_TextGrid);
    gglk_check_char_line_events(win, __func__);
    
    win->request[evtype_LineInput] = TRUE;
    if(initlen > maxlen)
	initlen = maxlen;
    win->line_buffer_ucs4 = buf; win->line_maxlen = maxlen;
    
    {
	glui32 buf_ucs4[initlen + 1];
	
	for(i = 0; i < initlen; i++)
	    buf_ucs4[i] = buf[i];
	buf_ucs4[i] = 0;
	gglk_text_line_input_set(win->gbuffer, maxlen, buf_ucs4);
    }

    win->line_rock = gglk_dispa_newmem(win->line_buffer_ucs4, win->line_maxlen,
				       "&+#!Iu");
}


void glk_cancel_line_event(winid_t win, event_t *event)
{
    if(event)
	event->type = evtype_None;
    gglk_validwin(win, return);
    if(win->request[evtype_LineInput] && win->gbuffer) {
	strid_t echo = gglk_stream_get_echo(win->stream);
	gunichar *line_ucs4;
	glui32 length;

	line_ucs4 = gglk_text_line_input_get(win->gbuffer);
	for(length = 0; line_ucs4[length] != 0; length++)
	    continue;
	if(length > win->line_maxlen)
	    length = win->line_maxlen;

	gglk_text_line_input_end(win->gbuffer);

	if(win->line_buffer_latin1 != NULL) {
	    glui32 i;
	    for(i = 0; i < length; i++)
		win->line_buffer_latin1[i] = line_ucs4[i];
	    if(echo)
		glk_put_buffer_stream(echo, win->line_buffer_latin1, length);
	    glk_put_char_stream(win->stream, '\n');

	    gglk_dispa_endmem(win->line_buffer_latin1, win->line_maxlen,
			      "&+#!Cn", win->line_rock);
	    win->line_buffer_latin1 = NULL;
	} else if(win->line_buffer_ucs4 != NULL) {
	    glui32 i;
	    for(i = 0; i < length; i++)
		win->line_buffer_ucs4[i] = line_ucs4[i];
	    if(echo)
		glk_put_buffer_stream_ucs4(echo, win->line_buffer_ucs4,length);
	    glk_put_char_stream_ucs4(win->stream, '\n');

	    gglk_dispa_endmem(win->line_buffer_ucs4, win->line_maxlen,
			      "&+#!Iu", win->line_rock);
	    win->line_buffer_ucs4 = NULL;
	} else {
	    g_warning("%s: nowhere to put line event", __func__);
	}

	g_free(line_ucs4); line_ucs4 = NULL;

	if(event) {
	    event->type = evtype_LineInput;
	    event->win = win;
	    event->val1 = length;
	    event->val2 = 0;
	}
	win->request[evtype_LineInput] = FALSE;
    }    
}

static void gglk_char_input_event(GtkWidget *unused_widget,
				  int ch, gpointer user_data)
{
    winid_t win = user_data;
    gglk_update_scroll();
    gglk_trigger_event(evtype_CharInput, win, ch, 0);
}
void glk_request_char_event(winid_t win)
{
    GGLK_WIN_TYPES(win, return, wintype_TextBuffer, wintype_TextGrid);
    gglk_check_char_line_events(win, __func__);
    if(win->gbuffer) {
	gglk_text_set_char_input(win->gbuffer, TRUE);
    }
    win->request[evtype_CharInput] = TRUE;
}
void glk_request_char_event_ucs4(winid_t win)
{
    GGLK_WIN_TYPES(win, return, wintype_TextBuffer, wintype_TextGrid);
    gglk_check_char_line_events(win, __func__);
    if(win->gbuffer) {
	gglk_text_set_char_input(win->gbuffer, TRUE);
    }
    /* FIXME */
    win->request[evtype_CharInput] = TRUE;
}
void glk_cancel_char_event(winid_t win)
{
    gglk_validwin(win, return);
    win->request[evtype_CharInput] = FALSE;
    if(win->gbuffer) {
	gglk_text_set_char_input(win->gbuffer, FALSE);
    }
}

static void gglk_mouse_event(GtkWidget *unused_widget, gint x, gint y,
			     gpointer user_data)
{
    winid_t win = user_data;
    gglk_update_scroll();
    gglk_trigger_event(evtype_MouseInput, win, x, y);
}
void glk_request_mouse_event(winid_t win)
{
    gglk_validwin(win, return);
    win->request[evtype_MouseInput] = TRUE;
}
void glk_cancel_mouse_event(winid_t win)
{
    gglk_validwin(win, return);
    win->request[evtype_MouseInput] = FALSE;
}
static void gglk_hyper_input_event(GtkWidget *unused_widget,
				   int linkval, gpointer user_data)
{
    winid_t win = user_data;
    gglk_update_scroll();
    gglk_trigger_event(evtype_Hyperlink, win, linkval, 0);
}
void glk_request_hyperlink_event(winid_t win)
{
    gglk_validwin(win, return);
    win->request[evtype_Hyperlink] = TRUE;
}
void glk_cancel_hyperlink_event(winid_t win)
{
    gglk_validwin(win, return);
    win->request[evtype_Hyperlink] = FALSE;
}

gboolean gglk_win_validate_event(glui32 evtype, winid_t win)
{
    gglk_validwin(win, return FALSE);

    /* FIXME: line event has already been canceled by the time we get here. */
    if(!win->request[evtype] && evtype != evtype_LineInput)
	return FALSE;

    switch(evtype) {
    case evtype_CharInput:  glk_cancel_char_event(win); break;
    case evtype_LineInput:  glk_cancel_line_event(win, NULL); break;
    case evtype_MouseInput: glk_cancel_mouse_event(win); break;
    case evtype_Hyperlink:  glk_cancel_hyperlink_event(win); break;
    }

    return TRUE;
}



glui32 glk_window_get_type(winid_t win)
{
    gglk_validwin(win, return 0);
    return win->type;
}

winid_t glk_window_get_parent(winid_t win)
{
    gglk_validwin(win, return NULL);
    return win->parent;
}

winid_t glk_window_get_sibling(winid_t win)
{
    winid_t pair;
    gglk_validwin(win, return NULL);
    pair = win->parent;
    if(!pair)
	return NULL;
    return (pair->child[0] == win) ? pair->child[1] : pair->child[0];
}


static GtkWidget *gglk_window_newpairwidget(winid_t win)
{
    GtkWidget *widget;
    
    if(gglk_win_method_proportional(win)) {
	if(gglk_win_method_left_right(win))
	    widget = gtk_hglkpaned_new();
	else
	    widget = gtk_vglkpaned_new();
        g_signal_connect(widget, "position-changed",
                         GTK_SIGNAL_FUNC(gglk_pair_arrange), win);
    } else {
	if(gglk_win_method_left_right(win))
            widget = gtk_table_new(1, 2, FALSE);
	else
            widget = gtk_table_new(2, 1, FALSE);
    }

    return widget;
}



static inline void gglk_win_add_frame(winid_t win, gboolean handle)
{
    if(win->frame)
	return;

    g_object_ref(G_OBJECT(win->content));
    gtk_container_remove(GTK_CONTAINER(win->widget), win->content);

    if(handle) {  /* ** FIXME **: Doesn't quite work right */
	win->frame = gtk_handle_box_new();
	gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(win->frame),
				       GTK_SHADOW_IN);
    } else {
	win->frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(win->frame), GTK_SHADOW_IN);
    }

    gtk_widget_show(win->frame);
    gtk_box_pack_start_defaults(GTK_BOX(win->widget), win->frame);
    gtk_container_add(GTK_CONTAINER(win->frame), win->content);
    g_object_unref(G_OBJECT(win->content));
}

static inline void gglk_win_remove_frame(winid_t win)
{
    if(!win->frame)
	return;
    
    g_object_ref(G_OBJECT(win->content));
    gtk_container_remove(GTK_CONTAINER(win->frame), win->content);

    gtk_container_remove(GTK_CONTAINER(win->widget), win->frame);
    win->frame = NULL;
    
    gtk_container_add(GTK_CONTAINER(win->widget), win->content);
    g_object_unref(G_OBJECT(win->content));
}



winid_t glk_window_open_with_styles(winid_t split, glui32 method, glui32 size,
				    glui32 wintype, 
				    struct glk_stylehint_struct *stylehints,
				    glui32 rock)
{
    winid_t win, pair, parent;
    GtkWidget *parent_widget;

    if(split == NULL && root_window != NULL) {
	g_warning("%s: root window already exists", __func__);
	return NULL;
    }

    win = gglk_create_win(wintype, rock);
    
    if(wintype == wintype_TextBuffer) {
	win->content = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win->content),
				       GTK_POLICY_NEVER,
				       scroll_policy);
	win->gbuffer = GGLK_TEXT(gglk_textbuffer_new(stylehints));
	gtk_container_add(GTK_CONTAINER(win->content), GTK_WIDGET(win->gbuffer));
    } else if(wintype == wintype_TextGrid) {
	win->content = gglk_textgrid_new(stylehints);
	win->gbuffer = GGLK_TEXT(win->content);

	g_signal_connect(win->gbuffer, "mouse-input-event",
			 GTK_SIGNAL_FUNC(gglk_mouse_event), win);

    } else if(wintype == wintype_Graphics) {
	win->content = gglk_graphics_new();
	win->ggraphics = GGLK_GRAPHICS(win->content);
	
	g_signal_connect(win->content, "mouse-input-event",
			 GTK_SIGNAL_FUNC(gglk_mouse_event), win);

    } else if(wintype == wintype_Blank) {
	win->content = gtk_drawing_area_new();
    } else {
	glk_window_close(win, NULL);
	return NULL;
    }

    if(!win->content) {
	glk_window_close(win, NULL);
	return NULL;
    }
    win->widget = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(win->widget), win->content);

    if(gglk_frame && (wintype == wintype_TextBuffer ||
		      wintype == wintype_TextGrid ||
		      wintype == wintype_Graphics)) {
	gglk_win_add_frame(win, FALSE);
    }


    if(win->gbuffer) {
	win->units[0] = win->gbuffer->xunits;
	win->units[1] = win->gbuffer->yunits;
	win->stream = gglk_strid_from_gglk_text(win->gbuffer, win, wintype);

	g_signal_connect(win->gbuffer, "char-input-event",
			 GTK_SIGNAL_FUNC(gglk_char_input_event), win);
	g_signal_connect(win->gbuffer, "line-input-event",
			 GTK_SIGNAL_FUNC(gglk_line_input_event), win);
	g_signal_connect(win->gbuffer, "hyper-input-event",
			 GTK_SIGNAL_FUNC(gglk_hyper_input_event), win);

    }
    

    if(!win->stream)
	win->stream = gglk_strid_dummy();

    if(split == NULL) {
	root_window = win;
	win->parent = NULL;
	gglk_window_place(split, win);
	gglk_window_resize_all_in(win);
	return win;
    }

    gglk_validwin(split, return NULL);
    pair = gglk_create_win(wintype_Pair, 0);
    pair->method = method;
    pair->size = size;
    pair->stream = gglk_strid_dummy();
    win->parent = pair;
    parent = split->parent;

    if(split == root_window)
	root_window = pair;

    if(gglk_win_method_left_above(pair)) {
	pair->child[0] = win;
	pair->child[1] = split;
    } else {
	pair->child[0] = split;
	pair->child[1] = win;
    }

    pair->parent = parent;
    pair->key = win;
    
    if(parent) {
	gglk_win_child_replace(parent, split, pair);
	parent_widget = parent->widget;
    } else {
	parent_widget = gglk_root;
    }
    
    g_object_ref(split->widget);
    gtk_container_remove(GTK_CONTAINER(parent_widget), split->widget);
    split->parent = pair;

    pair->widget = gglk_window_newpairwidget(pair);
    gglk_window_place(pair, pair->child[0]);
    gglk_window_place(pair, pair->child[1]);
    
    gglk_window_place(parent, pair);
    g_object_unref(split->widget);

    gglk_window_resize_all_in(pair);

    gglk_object_tree_update();
    return win;
}


void glk_window_set_arrangement(winid_t win, glui32 method,
				glui32 size, winid_t keywin)
{
    GtkWidget *winparent, *winwidget;
    int i;
    
    GGLK_WIN_TYPE(win, return, wintype_Pair);

    if(keywin == NULL)
	keywin = win->key;

    if(!gglk_win_contains(win, keywin)) {
	g_warning("%s: key not in win", __func__);
	return;
    }    

    win->method = method;
    win->size = size;
    win->key = keywin;

    winparent = (win->parent) ? win->parent->widget : gglk_root;
    winwidget = win->widget;
    win->widget = gglk_window_newpairwidget(win);

    for(i = 0; i < 2; i++) {
	g_object_ref(G_OBJECT(win->child[i]->widget));
	gtk_container_remove(GTK_CONTAINER(winwidget), win->child[i]->widget);
	gglk_window_place(win, win->child[i]);
	g_object_unref(G_OBJECT(win->child[i]->widget));
    }

    gtk_container_remove(GTK_CONTAINER(winparent), winwidget);
    gglk_window_place(win->parent, win);

    gglk_window_resize_all_in(win);

    gtk_widget_show_all(win->widget);
}


void glk_window_get_arrangement(winid_t win,
				glui32 *methodptr,
				glui32 *sizeptr,
				winid_t *keywinptr)
{
    GGLK_WIN_TYPE(win, return, wintype_Pair);
    if(methodptr) *methodptr = win->method;
    if(sizeptr)   *sizeptr   = win->size;
    if(keywinptr) *keywinptr = win->key;
}


void glk_window_clear(winid_t win)
{
    gglk_validwin(win, return);

    if(win->request[evtype_LineInput]) {
	g_warning("%s: window has line input pending", __func__);
	return;
    }

    if(win->type == wintype_Graphics) {
	gglk_graphics_clear(win->ggraphics);
    } else if(win->gbuffer) {
	gglk_text_clear(win->gbuffer);
    }
}


strid_t glk_window_get_stream(winid_t win)
{
    gglk_validwin(win, return NULL);
    return win->stream;
}



void glk_window_move_cursor(winid_t win, glui32 xpos, glui32 ypos)
{
    GGLK_WIN_TYPE(win, return, wintype_TextGrid);
    gglk_textgrid_move_cursor(GGLK_TEXTGRID(win->gbuffer), xpos, ypos);
}



/* glk wants to know, immediately, the sizes of the windows it opens.
   GTK doesn't decide this until they are drawn.  gtkglk 0.1 waited
   for GTK to draw things when windows were opened/closed, but this was
   too slow.  So now, we calculate a good guess as to how large a window
   will be.
 */
static void gglk_window_calc_size_from_parent(winid_t win,
					      gint * restrict w,
					      gint * restrict h,
					      gint pwidth, gint pheight)
{
    gint border_width;
    winid_t child;
    int xsize, ysize;

    border_width = gtk_container_get_border_width(GTK_CONTAINER(win->parent->widget));

    pwidth  -= border_width * 2;
    pheight -= border_width * 2;

    *w = pwidth;
    *h = pheight;

    if(gglk_win_method_left_above(win->parent))
	child = win->parent->child[0];
    else
	child = win->parent->child[1];

    if(gglk_win_method_proportional(win->parent)) {
	gint handle_size;
	gint percent;

	gtk_widget_style_get(win->parent->widget,
			     "handle_size", &handle_size, NULL);
	percent = GPOINTER_TO_INT(g_object_get_data(
				      G_OBJECT(win->parent->widget),
				      "percent"));
	if(gglk_win_method_left_above(win->parent))
	    win->parent->size = percent;
	else
	    win->parent->size = 100 - percent;
		
	xsize = (*w - handle_size) * win->parent->size / 100;
	ysize = (*h - handle_size) * win->parent->size / 100;
    } else { /* winmethod_Fixed */
	xsize = win->parent->key->units[0] * win->parent->size;
	ysize = win->parent->key->units[1] * win->parent->size;

	if(win->gbuffer && gglk_win_method_left_right(win->parent))
	    xsize += 5; /* For margins */
    }

    if(gglk_win_method_left_right(win->parent)) {
	if(win == child)
	    *w = xsize;
	else
	    *w -= xsize;
    } else {
	if(win == child)
	    *h = ysize;
	else
	    *h -= ysize;
    }

    if(win->frame) {
	*w -= win->frame->style->xthickness * 2;
	*h -= win->frame->style->ythickness * 2;
    }

    if(*w < 0) *w = 0;
    else if(*w > pwidth) *w = pwidth;
    if(*h < 0) *h = 0;
    else if(*h > pheight)  *h = pheight;
}



static void gglk_window_calc_size(winid_t win,
				  gint * restrict w,
				  gint * restrict h)
{
    gint pwidth, pheight;
    
    if(win == root_window) {
	int i;
	
	gtk_window_get_size(GTK_WINDOW(gglk_win), w, h);

	/* subtract size of menu and statusbar */
	for(i = 0; i < 2; i++) {
	    GtkWidget *widget = lookup_widget(gglk_win,
					      i ? "statusbar" : "menubar");
	    if(GTK_WIDGET_FLAGS(widget) & GTK_VISIBLE) {
		GtkRequisition req;
		gtk_widget_get_child_requisition(widget, &req);
		*h -= req.height;
	    }
	}

	return;
    }

    if(win->parent->type != wintype_Pair) {
	g_warning("%s: bad window hierarchy", __func__);
	*w = *h = 0;
	return;
    }

    gglk_window_calc_size(win->parent, &pwidth, &pheight);
    gglk_window_calc_size_from_parent(win, w, h, pwidth, pheight);
}


void glk_window_get_size(winid_t win, glui32 *widthptr, glui32 *heightptr)
{
    glui32 w, h;
    gglk_validwin(win, return);

    if(!widthptr) widthptr = &w;
    if(!heightptr) heightptr = &h;

    if(win->type == wintype_Blank || win->type == wintype_Pair) {
	*widthptr  = 0;
	*heightptr = 0;
    } else {
	gint width, height;
	gglk_window_calc_size(win, &width, &height);
	if(win->gbuffer)
	    width -= 5; /* For margins */
	
	*widthptr  = width / win->units[0];
	*heightptr = height / win->units[1];
    }
}


static inline void gglk_win_resize_textgrid(winid_t win)
{
    glui32 xsize, ysize;
    if(win->type != wintype_TextGrid)
	return;
    glk_window_get_size(win, &xsize, &ysize);
    gglk_textgrid_size(GGLK_TEXTGRID(win->gbuffer), xsize, ysize);
}

void gglk_window_resize_all_in(winid_t win)
{
    glui32 width, height;

    switch(win->type) {
    case wintype_Pair:
	gglk_window_resize_all_in(win->child[0]);
	gglk_window_resize_all_in(win->child[1]);
	break;
    case wintype_TextGrid:
	gglk_win_resize_textgrid(win);
	break;
    case wintype_Graphics:
	glk_window_get_size(win, &width, &height);
	gtk_widget_show_all(gglk_root);
	gglk_graphics_resize(win->ggraphics, width, height);
	break;
    }
}


static void gglk_pair_arrange(GtkWidget *unused_widget,
			      gpointer user_data)
{
    winid_t win = user_data;
    gglk_trigger_event(evtype_Arrange, win, 0, 0);
}



glui32 glk_image_draw(winid_t win, glui32 image, glsi32 val1, glsi32 val2)
{
    GdkPixbuf *pic;
    glui32 result = TRUE;
    GGLK_WIN_TYPES(win, return FALSE, wintype_Graphics, wintype_TextBuffer);
    if(!gglk_gestalt_claim[gestalt_Graphics] ||
       !gglk_gestalt_claim[gestalt_DrawImage]) return FALSE;
    if(!gglk_gestalt_support[gestalt_Graphics] ||
       !gglk_gestalt_support[gestalt_DrawImage]) return TRUE;

    pic = gglk_load_pic(image);
    if(!pic)
	return FALSE;
    switch(win->type) {
    case wintype_TextBuffer:
	gglk_textbuffer_put_image(GGLK_TEXTBUFFER(win->gbuffer), pic, val1);
	break;
    case wintype_Graphics:
	result = sglk_image_draw(win, pic, val1, val2);
    }
    g_object_unref(pic);
    return result;
}


glui32 glk_image_draw_scaled(winid_t win, glui32 image,
			     glsi32 val1, glsi32 val2,
			     glui32 width, glui32 height)
{
    GdkPixbuf *pic;
    glui32 result = TRUE;
    GGLK_WIN_TYPES(win, return FALSE, wintype_Graphics, wintype_TextBuffer);
    if(!gglk_gestalt_claim[gestalt_Graphics] ||
       !gglk_gestalt_claim[gestalt_DrawImage]) return FALSE;
    if(!gglk_gestalt_support[gestalt_Graphics] ||
       !gglk_gestalt_support[gestalt_DrawImage]) return TRUE;

    pic = gglk_load_pic(image);
    if(!pic)
	return FALSE;

    if(win->type == wintype_TextBuffer) {
	GdkPixbuf *pic2;
	pic2 = gdk_pixbuf_scale_simple(pic, width, height, interp_mode);
	gglk_textbuffer_put_image(GGLK_TEXTBUFFER(win->gbuffer), pic2, val1);
    } else if(win->type == wintype_Graphics) {
	result = sglk_image_draw_scaled(win, pic, val1, val2, width, height);
    }
    g_object_unref(pic);
    return result;
}


void glk_window_flow_break(winid_t win)
{
    GGLK_WIN_TYPE(win, return, wintype_TextBuffer);
    if(!gglk_gestalt_support[gestalt_Graphics]) return;

    gglk_textbuffer_put_flowbreak(GGLK_TEXTBUFFER(win->gbuffer));
}


/* Wrappers for gglk_graphics functions */
void glk_window_set_background_color(winid_t win, glui32 color)
{
    GGLK_WIN_TYPE(win, return, wintype_Graphics);
    if(!gglk_gestalt_support[gestalt_Graphics]) return;

    gglk_graphics_background(win->ggraphics, color);
}
void glk_window_erase_rect(winid_t win, glsi32 left, glsi32 top, glui32 width, glui32 height)
{
    GGLK_WIN_TYPE(win, return, wintype_Graphics);
    if(!gglk_gestalt_support[gestalt_Graphics]) return;

    gglk_graphics_erase_rect(win->ggraphics, left, top, width, height);
}
void glk_window_fill_rect(winid_t win, glui32 color, glsi32 left, glsi32 top, glui32 width, glui32 height)
{
    GGLK_WIN_TYPE(win, return, wintype_Graphics);
    if(!gglk_gestalt_support[gestalt_Graphics]) return;

    gglk_graphics_fill_rect(win->ggraphics, color, left, top, width, height);
}
void *sglk_get_image(winid_t win, glsi32 left, glsi32 top, glui32 width, glui32 height)
{
    GGLK_WIN_TYPE(win, return NULL, wintype_Graphics);
    if(!gglk_gestalt_support[gestalt_Graphics]) return NULL;

    return gglk_graphics_get_image(win->ggraphics, left, top, width, height);
}
glui32 sglk_image_draw(winid_t win, void *b, glsi32 val1, glsi32 val2)
{
    GGLK_WIN_TYPE(win, return FALSE, wintype_Graphics);
    if(!gglk_gestalt_claim[gestalt_Graphics] ||
       !gglk_gestalt_claim[gestalt_DrawImage]) return FALSE;
    if(!gglk_gestalt_support[gestalt_Graphics] ||
       !gglk_gestalt_support[gestalt_DrawImage]) return TRUE;

    return gglk_graphics_image_draw(win->ggraphics, b, val1, val2);
}
glui32 sglk_image_draw_scaled(winid_t win, void *b, glsi32 val1, glsi32 val2, glui32 width, glui32 height)
{
    GGLK_WIN_TYPE(win, return FALSE, wintype_Graphics);
    if(!gglk_gestalt_claim[gestalt_Graphics] ||
       !gglk_gestalt_claim[gestalt_DrawImage]) return FALSE;
    if(!gglk_gestalt_support[gestalt_Graphics] ||
       !gglk_gestalt_support[gestalt_DrawImage]) return TRUE;

    return gglk_graphics_image_draw_scaled(win->ggraphics, b, val1, val2, width, height);
}


glui32 glk_style_distinguish(winid_t win, glui32 styl1, glui32 styl2)
{
    /* Ideally this needs to ask the pango stuff whether the font *really*
       supports the attributes we'ved assigned it.  For example, I have an
       Andale Mono font which doesn't support bold and italic, and if
       I'm using it, this should say that bold preformatted is
       indistinguishable from normal preformatted...

       For now, just check to see if glk_style_measure reports any difference.
       Last time I checked, full justification was not supported by GtkTextView
       so that is thrown out.
    */

    int i;
    for(i = 0; i < stylehint_NUMHINTS; i++) {
	glsi32 a, b;
	glk_style_measure(win, styl1, i, &a);
	glk_style_measure(win, styl2, i, &b);
	if(a != b) {
	    if(i == stylehint_Justification && 
	       ((a == stylehint_just_LeftFlush &&
		 b == stylehint_just_LeftRight) ||
		(b == stylehint_just_LeftFlush &&
		 a == stylehint_just_LeftRight))) {
		continue;
	    }
	    return TRUE;
	}
    }
    return FALSE;
}

glui32 glk_style_measure(winid_t win, glui32 styl, glui32 hint, glsi32 *result)
{
    GtkTextView *view = GTK_TEXT_VIEW(win->gbuffer);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    return gglk_style_measure(gtk_text_buffer_get_tag_table(buffer),
			      styl, hint, result);
}

void gglk_window_set_background_color(winid_t win)
{
    if(win->gbuffer) {
	gglk_set_background_color(GTK_TEXT_VIEW(win->gbuffer),
				  win->gbuffer->stylehints);
    }
}


#include "gui_window.c"
