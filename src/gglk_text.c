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

#include "gglk.h"
#include "gglk_text.h"

#include <gtk/gtk.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtkmarshal.h>

enum {
    CHAR_SIGNAL,
    LINE_SIGNAL,
    HYPER_SIGNAL,
    MOUSE_SIGNAL,

    CLEAR_SIGNAL,
    PUT_BUFFER_SIGNAL,

    LAST_SIGNAL
};



static void gglk_text_class_init(GglkTextClass *klass);
static void gglk_text_init(GglkText *tb);
static void gglk_text_destroy(GtkObject *object,
				  gpointer unused_data);
static gboolean gglk_text_keypress(GtkWidget *widget,
					 GdkEventKey *event,
					 gpointer unused_data);
static void gglk_text_setscroll(GtkTextView *textview,
				      GtkAdjustment *hadjust,
				      GtkAdjustment *vadjust,
				      gpointer user_data);
static void gglk_text_paste_clipboard(GtkTextView *view);
static gint gglk_text_button_press_event(GtkWidget *widget,
					 GdkEventButton *event);


static guint gglk_text_signals[LAST_SIGNAL] = { 0 };


GType gglk_text_get_type(void)
{
    static GType gg_type = 0;

    if (!gg_type) {
	static const GTypeInfo gg_info = {
	    sizeof(GglkTextClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) gglk_text_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GglkText),
	    0,
	    (GInstanceInitFunc) gglk_text_init,
	    NULL
	};

	gg_type = g_type_register_static(GTK_TYPE_TEXT_VIEW, "GglkText",
					 &gg_info, 0);
    }

    return gg_type;
}

static void gglk_text_class_init(GglkTextClass *klass)
{
    GtkTextViewClass *view_class = GTK_TEXT_VIEW_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    view_class->paste_clipboard = gglk_text_paste_clipboard;
    klass->orig_button_press = widget_class->button_press_event;
    widget_class->button_press_event = gglk_text_button_press_event;

    klass->char_input_event = NULL;
    gglk_text_signals[CHAR_SIGNAL] =
	g_signal_new("char-input-event",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, char_input_event),
		     NULL, NULL,
		     gtk_marshal_VOID__INT,
		     G_TYPE_NONE, 1,
		     G_TYPE_INT);
    klass->line_input_event = NULL;
    gglk_text_signals[LINE_SIGNAL] =
	g_signal_new("line-input-event",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, line_input_event),
		     NULL, NULL,
		     gtk_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
    klass->hyper_input_event = NULL;
    gglk_text_signals[HYPER_SIGNAL] =
	g_signal_new("hyper-input-event",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, hyper_input_event),
		     NULL, NULL,
		     gtk_marshal_VOID__INT,
		     G_TYPE_NONE, 1,
		     G_TYPE_INT);
    klass->mouse_input_event = NULL;
    gglk_text_signals[MOUSE_SIGNAL] =
	g_signal_new("mouse-input-event",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, mouse_input_event),
		     NULL, NULL,
		     gtk_marshal_VOID__INT_INT,
		     G_TYPE_NONE, 2,
		     G_TYPE_INT, G_TYPE_INT);
    klass->clear_action = NULL;
    gglk_text_signals[CLEAR_SIGNAL] =
	g_signal_new("clear",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, clear_action),
		     NULL, NULL,
		     gtk_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
    klass->put_buffer = NULL;
    gglk_text_signals[PUT_BUFFER_SIGNAL] =
	g_signal_new("put-buffer",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkTextClass, put_buffer),
		     NULL, NULL,
		     gtk_marshal_VOID__POINTER_INT,
		     G_TYPE_NONE, 2,
		     G_TYPE_POINTER, G_TYPE_INT);

}

static void gglk_text_init(GglkText *tb)
{
    /* Set up the text view widget */
    GtkTextView *view = GTK_TEXT_VIEW(tb);
    gtk_text_view_set_editable(view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(view), 30, 30);

    tb->request_char = FALSE;
    tb->line_maxlen = 0;
    tb->cursor_handler = 0;
    tb->update_handler = 0;
    tb->command_history = tb->cur_hist_item = NULL;
    tb->hyperval = 0;

    tb->should_scroll = FALSE;
    tb->vadjust = NULL;
    tb->scroll_handler = 0;

    g_signal_connect(tb, "destroy",
		     GTK_SIGNAL_FUNC(gglk_text_destroy), NULL);

    g_signal_connect(view, "key-press-event",
		     GTK_SIGNAL_FUNC(gglk_text_keypress), NULL);

    g_signal_connect(view, "set-scroll-adjustments",
		     GTK_SIGNAL_FUNC(gglk_text_setscroll), tb);


    /*gtk_text_mark_set_visible(tb->scrollmark, TRUE);*/
}


void gglk_text_init_tags(GglkText *tb) 
{
    PangoContext *context;
    PangoFontMetrics *metrics;
    PangoFontDescription *font_desc = NULL;
    GtkTextTag *tag;
    
    tb->buffer = gtk_text_view_get_buffer(&tb->view);
    gtk_text_buffer_get_end_iter(tb->buffer, &tb->iter);

    tb->startedit=gtk_text_buffer_create_mark(tb->buffer,NULL,&tb->iter,TRUE);
    tb->endedit  =gtk_text_buffer_create_mark(tb->buffer,NULL,&tb->iter,FALSE);
    tb->scrollmark=gtk_text_buffer_create_mark(tb->buffer,NULL,&tb->iter,TRUE);
    tb->endmark  =gtk_text_buffer_create_mark(tb->buffer,NULL,&tb->iter,FALSE);
    tb->hypermark=gtk_text_buffer_create_mark(tb->buffer,NULL,&tb->iter,TRUE);

    tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(tb->buffer),
				    "Normal");
    if(!tag) return;

    gglk_text_set_style(tb, tag);

    /* Measure default font size */
    g_object_get(G_OBJECT(tag), "font-desc", &font_desc, NULL);

    font_desc = pango_font_description_copy(font_desc);
    pango_font_description_merge(font_desc,
				 GTK_WIDGET(tb)->style->font_desc,FALSE);

    context = gtk_widget_get_pango_context(GTK_WIDGET(tb));
    metrics = pango_context_get_metrics(context,
					font_desc,
					pango_context_get_language(context));

    pango_font_description_free(font_desc);

    tb->xunits = PANGO_PIXELS(pango_font_metrics_get_approximate_digit_width(
				  metrics));
    tb->yunits = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
			      pango_font_metrics_get_descent(metrics));
    pango_font_metrics_unref(metrics);
}

static void gglk_text_destroy(GtkObject *object,
			      gpointer unused_data)
{
    GglkText *tb = GGLK_TEXT(object);
    GList *p;

    for(p = tb->command_history; p; p=p->next)
	g_free(p->data);
    g_list_free(tb->command_history);
    tb->command_history = NULL;

    gglk_stylehint_unref(tb->stylehints);
    tb->stylehints = NULL;
}


static void gglk_text_scrolled_event(GtkAdjustment *unused, gpointer user_data)
{
    GglkText *tb = user_data;

    gglk_text_update_scrollmark(tb);
    tb->should_scroll = FALSE;
}

/* Set an adjustment to zero.  Connected to the value-changed signal to force
   it to always be zero. */
static void gglk_text_scrolled_zero(GtkAdjustment *adjustment,
				    gpointer unused_data)
{
    gtk_adjustment_set_value(adjustment, 0);
}

static void gglk_text_setscroll(GtkTextView *unused,
				      GtkAdjustment *hadjust,
				      GtkAdjustment *vadjust,
				      gpointer user_data)
{
    GglkText *tb = user_data;
    
    if(vadjust) {
	/* If the user manually scrolls the window, we don't want to jump
	   immediately to the bottom the next time glk_select is called,
	   as it could be doing timer stuff.  So keep track of this. */
	
	tb->vadjust = vadjust;
	tb->scroll_handler =
	    g_signal_connect(vadjust, "value-changed",
			     GTK_SIGNAL_FUNC(gglk_text_scrolled_event),
			     user_data);
    }
    if(hadjust) {
	/* we want to be scrolled all the way to the left, regardless
	   of whether there's some large picture which takes up too much
	   space, like in _Lock & Key_ ... you may want to remove /
	   improve this if you change the scrolled window to have a
	   horizontal scroll bar. */
	g_signal_connect(hadjust, "value-changed",
			 GTK_SIGNAL_FUNC(gglk_text_scrolled_zero),
			 user_data);
    }
}


gboolean gglk_text_can_scroll(GglkText *tb)
{
    GtkTextIter enditer;

    /* Move the endmark to the end of the buffer.  If it must be moved
       to get back onscreen, then the window might by scrollable. */
    gtk_text_buffer_get_end_iter(tb->buffer, &enditer);
    gtk_text_buffer_move_mark(tb->buffer, tb->endmark, &enditer);
    if(gtk_text_view_move_mark_onscreen(GTK_TEXT_VIEW(tb), tb->endmark))
	return TRUE;

    return FALSE;
}

void gglk_text_auto_scroll(GglkText *tb)
{
    if(!tb->should_scroll)
	return;
    tb->should_scroll = FALSE;

    if(tb->vadjust)
	g_signal_handler_block(tb->vadjust, tb->scroll_handler);

    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tb),
				 tb->scrollmark, 0.1,
				 TRUE, 0.0, 0.0);
    if(tb->vadjust)
	g_signal_handler_unblock(tb->vadjust, tb->scroll_handler);
}


void gglk_text_update_scrollmark(GglkText *tb)
{
    GtkTextIter olditer, pastiter, iter;

    gtk_text_buffer_get_iter_at_mark(tb->buffer, &olditer, tb->scrollmark);
    gtk_text_buffer_get_end_iter(tb->buffer, &iter);
    gtk_text_buffer_move_mark(tb->buffer, tb->scrollmark, &iter);

    gtk_text_view_move_mark_onscreen(GTK_TEXT_VIEW(tb),
				     tb->scrollmark);
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &pastiter, tb->scrollmark);

    iter = pastiter;
    gtk_text_view_backward_display_line(GTK_TEXT_VIEW(tb), &iter);

    /* If we're trying to scroll down, but the backward_display_line
       made us fail, undo that. */
    if(gtk_text_iter_compare(&pastiter, &olditer) >= 0 &&
       gtk_text_iter_compare(&iter, &olditer) <= 0) {
	iter = pastiter;
	gtk_text_view_forward_display_line(GTK_TEXT_VIEW(tb), &iter);
    }

    gtk_text_buffer_move_mark(tb->buffer, tb->scrollmark, &iter);
}




static void gglk_text_paste_clipboard_selection(GglkText *tb,
						GdkAtom selection,
						GtkTextIter *iter)
{
    GtkClipboard *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(tb),
						       selection);
    if(!gtk_text_iter_can_insert(iter, FALSE))
	gtk_text_buffer_get_iter_at_mark(tb->buffer, iter, tb->endedit);

    gtk_text_buffer_paste_clipboard(tb->buffer, clipboard, iter, FALSE);
}

static void gglk_text_paste_clipboard(GtkTextView *view)
{
    GglkText *tb = GGLK_TEXT(view);
    GtkTextMark *insert = gtk_text_buffer_get_insert(tb->buffer);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &iter, insert);
    
    gglk_text_paste_clipboard_selection(tb, GDK_SELECTION_CLIPBOARD, &iter);

    gtk_text_view_scroll_mark_onscreen(view, insert);
}



static gint gglk_text_button_press_event(GtkWidget *widget,
					 GdkEventButton *event)
{
    GglkText *tb = GGLK_TEXT(widget);
    GtkTextView *view = GTK_TEXT_VIEW(widget);
    
    if(event->type == GDK_BUTTON_PRESS) {
	if(event->button == 2) {
	    GtkTextIter iter;
	    gtk_text_view_get_iter_at_location(view, &iter,
					       event->x + view->xoffset,
					       event->y + view->yoffset);

	    gglk_text_paste_clipboard_selection(tb, GDK_SELECTION_PRIMARY,
						&iter);
	    return TRUE;
	}
	if(event->button == 3) /* Disable GTK's popup menu in favor of mine */
	    return FALSE;
    }
    return GGLK_TEXT_CLASS(G_OBJECT_GET_CLASS(tb))->
	orig_button_press(widget, event);
}



static gboolean gglk_text_keypress(GtkWidget *widget,
				   GdkEventKey *event,
				   gpointer unused_data)
{
    GglkText *tb = GGLK_TEXT(widget);
    glui32 c = gglk_keycode_gdk_to_glk(event->keyval);

    if(event->keyval == 0)
	return FALSE;
    if(c == keycode_Tab)
	return FALSE;

    if(c < 128 && event->state & GDK_CONTROL_MASK)
	c &= 0x1f;
    if(event->state & GDK_MOD1_MASK)
	c |= 0x100;

    /* Check to see if we should use the keypress to page down */
    if(gglk_text_can_scroll(tb)) {
	if(c == ' ' || c == keycode_Return) {
	    gglk_text_update_scrollmark(tb);
	    tb->should_scroll = TRUE;
	    gglk_auto_scroll();
	    while(gtk_events_pending())
		gtk_main_iteration();
	    gglk_set_focus();

	    return TRUE;
	} else if(c > ' ' && c <= 255) {
	    /* Scroll to end */
	    GtkTextIter iter;
	    gtk_text_buffer_get_end_iter(tb->buffer, &iter);
	    gtk_text_buffer_move_mark(tb->buffer, tb->scrollmark, &iter);
	    gglk_auto_scroll();
	    gglk_set_focus();
	    /* Don't return yet; check to see if we can use key */
	} else {
	    /* Let GTK handle the keypress */
	    return FALSE;
	}
    }
    
    if(tb->request_char) {
	if(c != keycode_Unknown) {
	    tb->request_char = FALSE;
	    g_signal_emit(tb, gglk_text_signals[CHAR_SIGNAL], 0, (int) c);
	    gglk_text_update_scrollmark(tb);
	    tb->should_scroll = TRUE;
	    return TRUE;
	}
    }
    if(tb->line_maxlen) {
	GtkTextIter b, e, current;
	    
	gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
	gtk_text_buffer_get_iter_at_mark(tb->buffer, &e, tb->endedit);
	gtk_text_buffer_get_iter_at_mark(tb->buffer, &current,
				       gtk_text_buffer_get_insert(tb->buffer));

	switch(c) {
	case keycode_Return:
	    gglk_text_line_input_accept(tb);
	    return TRUE;

	case keycode_Up:
	case 'P' & 0x1f:
	    if(gtk_text_iter_compare(&current, &b) < 0)
		return FALSE;

	    if(!tb->cur_hist_item)
		tb->cur_hist_item = g_list_last(tb->command_history);
	    else {
		GList *p = g_list_previous(tb->cur_hist_item);
		if(!p) return TRUE;
		tb->cur_hist_item = p;
	    }
	    if(tb->cur_hist_item)
		gglk_text_line_input_set(tb, tb->line_maxlen,
					 tb->cur_hist_item->data);
	    return TRUE;

	case keycode_Down:
	case 'N' & 0x1f:
	    if(gtk_text_iter_compare(&current, &b) < 0)
		return FALSE;

	    if(!tb->cur_hist_item)
		return TRUE;
	    tb->cur_hist_item = g_list_next(tb->cur_hist_item);
	    
	    {
		static const gunichar empty = 0;

		gglk_text_line_input_set(tb, tb->line_maxlen,
					 tb->cur_hist_item
					 ? tb->cur_hist_item->data
					 : &empty);
	    }
	    return TRUE;

	case keycode_Left:
	case keycode_Right:
	case keycode_Home:
	case keycode_End:
	case keycode_PageUp:
	case keycode_PageDown:
	case keycode_Unknown:
	    return FALSE;

	default:
	    /* If the user tries to type somewhere else in the buffer, move
	       the cursor to the end, so the input is placed there. */
	    if(gtk_text_iter_compare(&current, &b) < 0)
		gtk_text_buffer_place_cursor(tb->buffer, &e);
	    break;
	}
    }

    return FALSE;
}




void gglk_text_clear(GglkText *tb)
{
    glui32 linkval = tb->hyperval;
    gglk_text_set_hyperlink(tb, 0);
    
    g_signal_emit(tb, gglk_text_signals[CLEAR_SIGNAL], 0);

    gtk_text_buffer_get_start_iter(tb->buffer, &tb->iter);
    gtk_text_buffer_place_cursor(tb->buffer, &tb->iter);
    gtk_text_buffer_move_mark(tb->buffer, tb->scrollmark, &tb->iter);
    gtk_text_buffer_move_mark(tb->buffer, tb->endmark, &tb->iter);
    gtk_text_buffer_move_mark(tb->buffer, tb->hypermark, &tb->iter);

    gglk_text_set_hyperlink(tb, linkval);
}

void gglk_text_put_buffer(GglkText *tb, const gunichar *buf, int len)
{
    gboolean cancelled_char = FALSE;
    
    g_return_if_fail(IS_GGLK_TEXT(tb));
    if(len == 0)
	return;
    if(tb->request_char || tb->line_maxlen) {
	g_warning("%s: Attempting output during input", __func__);

	if(tb->request_char) {
	    gglk_text_set_char_input(tb, FALSE);
	    cancelled_char = TRUE;
	}
	if(tb->line_maxlen)
	    gglk_text_line_input_end(tb);
    }
    g_signal_emit(tb, gglk_text_signals[PUT_BUFFER_SIGNAL], 0, buf, len);

    /* Resume the cancelled input */
    if(cancelled_char) {
	gglk_text_set_char_input(tb, TRUE);
    }
    /* FIXME: what about line input? */
}


void gglk_text_set_style(GglkText *tb, GtkTextTag *tag)
{
    tb->style_tag = tag;
}

void gglk_text_set_hyperlink(GglkText *tb, glui32 linkval)
{
    if(tb->hyperval == linkval)
	return;
    if(tb->hyperval) {
	GtkTextTag *hypertag;
	GtkTextIter iter;
	hypertag = gtk_text_buffer_create_tag(tb->buffer, NULL, NULL);
	g_signal_connect(hypertag, "event",
			 G_CALLBACK(gglk_text_mouse),
			 GINT_TO_POINTER(tb->hyperval));

	/* compensate for GTK not sending through click on images if they're
	   not followed by text */
	iter = tb->iter;
	gtk_text_iter_backward_char(&iter);
	if(gtk_text_iter_get_pixbuf(&iter)) {
	    const gunichar space = ' ';
	    gglk_text_put_buffer(tb, &space, 1);
	}

	gtk_text_buffer_get_iter_at_mark(tb->buffer, &iter, tb->hypermark);
	gtk_text_buffer_apply_tag(tb->buffer, hypertag, &iter, &tb->iter);
	gtk_text_buffer_apply_tag_by_name(tb->buffer,
					  gglk_get_tag(style_Hyperlinks),
					  &iter, &tb->iter);
    }

    tb->hyperval = linkval;

    if(linkval) {
	gtk_text_buffer_move_mark(tb->buffer, tb->hypermark, &tb->iter);
    }
}

void gglk_text_set_char_input(GglkText *tb, gboolean requested)
{
    tb->request_char = requested;
}



static void gglk_text_update_editable_region(GtkTextBuffer *unused_widget,
					     gpointer user_data)
{
    GglkText *tb = user_data;
    GtkTextIter b, e;
    gboolean fixup = FALSE;

    gtk_text_buffer_get_iter_at_mark(tb->buffer, &e, tb->endedit);

    if(gtk_text_iter_is_end(&e)) { /* deleted the end bit; add it back */
	fixup = TRUE;
    } else {
	gtk_text_iter_forward_char(&e);
	if(!gtk_text_iter_is_end(&e)) { /* user pasted extra stuff at end */
	    fixup = TRUE;
	}
    }

    if(fixup) {
	gtk_text_buffer_get_end_iter(tb->buffer, &e);
	gtk_text_buffer_insert(tb->buffer, &e, " ", -1);
	gtk_text_iter_backward_char(&e);
	gtk_text_buffer_move_mark(tb->buffer, tb->endedit, &e);
    }
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
    gtk_text_buffer_get_end_iter(tb->buffer, &e);

    gtk_text_buffer_apply_tag_by_name(tb->buffer, "Input", &b, &e);
    gtk_text_buffer_apply_tag_by_name(tb->buffer, "editable", &b, &e);
    gtk_text_buffer_get_end_iter(tb->buffer, &tb->iter);
}




/* Trap mark movement events in a Text, so that if line input is
   in progress, the user can use C-a or C-e to move to the beginning /
   end of the line input, not the beginning / end of the current line */
static void gglk_text_cursor_event(GtkTextBuffer *unused_widget,
				   GtkTextIter *unused_iter,
				   GtkTextMark *unused_mark,
				   gpointer user_data)
{
    GglkText *tb = user_data;
    GtkTextIter b, e;
    int i;
    gboolean moved[2] = { FALSE, FALSE };

    g_signal_handler_block(tb->buffer, tb->cursor_handler);

    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &e, tb->endedit);

    for(i = 0; i < 2; i++) {
	GtkTextMark *this_mark = gtk_text_buffer_get_mark(
	    tb->buffer, i ? "selection_bound" : "insert");
	GtkTextIter current;

	gtk_text_buffer_get_iter_at_mark(tb->buffer, &current, this_mark);
	
	/* If past the end mark, move back. */
	if(gtk_text_iter_compare(&current, &e) > 0) {
	    gtk_text_buffer_move_mark(tb->buffer, this_mark, &e);
	    moved[i] = TRUE;
	}

	/* If on the same line as the input, but before the region,
	 * move forward */

	if(gtk_text_iter_get_line(&current) == gtk_text_iter_get_line(&b) &&
	   gtk_text_iter_compare(&current, &b) < 0) {
	    
	    gtk_text_buffer_move_mark(tb->buffer, this_mark, &b);
	    moved[i] = TRUE;
	}
    }

    if(moved[0]) {
	/* This will make the cursor appear immediately */
	g_signal_emit_by_name(tb, "move-cursor",
			      GTK_MOVEMENT_LOGICAL_POSITIONS, 0, TRUE);
    }
    
    g_signal_handler_unblock(tb->buffer, tb->cursor_handler);
}



void gglk_text_line_input_end(GglkText *tb)
{
    GtkTextIter b, e;

    tb->line_maxlen = 0;

    /* Disable signal handlers */
    if(tb->cursor_handler)
	g_signal_handler_disconnect(tb->buffer, tb->cursor_handler);
    if(tb->update_handler)
	g_signal_handler_disconnect(tb->buffer, tb->update_handler);
    tb->cursor_handler = 0;
    tb->update_handler = 0;

    /* Remove end marker */
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->endedit);
    gtk_text_buffer_get_end_iter(tb->buffer, &e);
    gtk_text_buffer_delete(tb->buffer, &b, &e);
    
    /* Set text to be non-editable */
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
    gtk_text_buffer_get_end_iter(tb->buffer, &e);
    gtk_text_buffer_remove_tag_by_name(tb->buffer, "editable", &b, &e);

    gtk_text_buffer_get_end_iter(tb->buffer, &tb->iter);
}


/* Accept the current line as the input */
void gglk_text_line_input_accept(GglkText *tb)
{
    gunichar *buf = gglk_text_line_input_get(tb);
    tb->command_history = g_list_append(tb->command_history, buf);
    tb->cur_hist_item = NULL;

    gglk_text_line_input_end(tb);
    gglk_text_update_scrollmark(tb);
    tb->should_scroll = TRUE;
    g_signal_emit(tb, gglk_text_signals[LINE_SIGNAL], 0);
}



/* Set length to a positive number to enable line input; set to 0 to disable */
void gglk_text_line_input_set(GglkText *tb, int length,
			      const gunichar *text)
{
    GtkTextIter b, e;
    gchar *line_utf8;

    if(tb->line_maxlen != 0) {
	gglk_text_line_input_end(tb);

	/* Clear out old text there */
	gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
	gtk_text_buffer_get_iter_at_mark(tb->buffer, &e, tb->endedit);
	gtk_text_buffer_delete(tb->buffer, &b, &e);

	gtk_text_buffer_get_iter_at_mark(tb->buffer, &tb->iter, tb->startedit);
    }
    
    tb->line_maxlen = length;
    if(length == 0) {
	gglk_text_line_input_end(tb);
	return;
    }

    /* Add new text */
    gtk_text_buffer_get_end_iter(tb->buffer, &b);
    gtk_text_buffer_move_mark(tb->buffer, tb->startedit, &b);
    line_utf8 = g_uni_to_utf8(text, -1, NULL, NULL, NULL);
    gtk_text_buffer_insert(tb->buffer, &b, line_utf8, -1);
    g_free(line_utf8);

    /* Extra character at end to maintain editability */
    gtk_text_buffer_get_end_iter(tb->buffer, &e);
    gtk_text_buffer_insert(tb->buffer, &e, " ", -1);

    /* Make editable */
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
    gtk_text_buffer_apply_tag_by_name(tb->buffer, "Input", &b, &e);
    gtk_text_buffer_apply_tag_by_name(tb->buffer, "editable", &b, &e);

    /* Set end mark */
    gtk_text_iter_backward_char(&e);
    gtk_text_buffer_move_mark(tb->buffer, tb->endedit, &e);

    gtk_text_buffer_place_cursor(tb->buffer, &e);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tb), tb->endedit, 0,FALSE,0,0);

    /* Add signals to handle user interaction */
    tb->cursor_handler = g_signal_connect(
	tb->buffer, "mark-set",
	GTK_SIGNAL_FUNC(gglk_text_cursor_event), tb);
    tb->update_handler = g_signal_connect(
	tb->buffer, "end-user-action",
	GTK_SIGNAL_FUNC(gglk_text_update_editable_region), tb);
}


gunichar *gglk_text_line_input_get(GglkText *tb)
{
    GtkTextIter b, e;
    gchar *line_utf8, *line_utf8_normal;
    gunichar *line_uni;
    glong len;
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &b, tb->startedit);
    gtk_text_buffer_get_iter_at_mark(tb->buffer, &e, tb->endedit);
    line_utf8 = gtk_text_buffer_get_text(tb->buffer, &b, &e, FALSE);
    line_utf8_normal = g_utf8_normalize(line_utf8, -1, G_NORMALIZE_NFC);
    line_uni = g_utf8_to_uni(line_utf8, -1, NULL, &len, NULL);
    g_free(line_utf8); line_utf8 = NULL;
    return line_uni;
}


gboolean gglk_text_mouse(GtkTextTag *unused,
                         GObject *object,
                         GdkEvent *event,
                         GtkTextIter *iter,
                         gpointer user_data)
{
    GglkText *tb;
    int x = gtk_text_iter_get_line_offset(iter);
    int y = gtk_text_iter_get_line(iter);
    
    if(!IS_GGLK_TEXT(object))
	return FALSE;

    tb = GGLK_TEXT(object);
    if(event->type == GDK_BUTTON_PRESS) {
	tb->click_x = x;
	tb->click_y = y;
    } else if(event->type == GDK_BUTTON_RELEASE) {
	if(tb->click_x == x && tb->click_y == y)
	    g_signal_emit(tb, gglk_text_signals[MOUSE_SIGNAL], 0, x, y);
    } else if(event->type == GDK_2BUTTON_PRESS && user_data) {
        int hyperval = GPOINTER_TO_INT(user_data);

	g_signal_emit(tb, gglk_text_signals[HYPER_SIGNAL], 0, hyperval);

	return TRUE;
    }

    return FALSE;
}
