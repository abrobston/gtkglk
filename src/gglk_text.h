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

#ifndef GGLK_TEXT_H
#define GGLK_TEXT_H

#include "gglk.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktextview.h>

G_BEGIN_DECLS

#define GGLK_TEXT_TYPE (gglk_text_get_type ())
#define GGLK_TEXT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GGLK_TEXT_TYPE, GglkText))
#define GGLK_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GGLK_TEXT_TYPE, GglkTextClass))
#define IS_GGLK_TEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GGLK_TEXT_TYPE))
#define IS_GGLK_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GGLK_TEXT_TYPE))


typedef struct _GglkText   GglkText;
typedef struct _GglkTextClass  GglkTextClass;

struct _GglkText 
{
    GtkTextView view;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    int xunits, yunits;  /* Approximate size of a character */

    gboolean request_char;  /* True if a character request is in progress */
    int line_maxlen;     /* nonzero if a line request is in progress */
    GtkTextMark *startedit, *endedit;  /* Boundaries of the line edit region */
    gulong cursor_handler, update_handler;
    GList *command_history, *cur_hist_item;

    GtkTextMark *scrollmark, *endmark;
    gboolean should_scroll;
    GtkAdjustment *vadjust;
    gulong scroll_handler;

    struct glk_stylehint_struct *stylehints;
    GtkTextTag *style_tag;   /* The current style tag */
    glui32 hyperval;         /* The current hyperlink value */
    GtkTextMark *hypermark;  /* Marks the beginning of the current hyperlink */

    int click_x, click_y;
};

struct _GglkTextClass
{
    GtkTextViewClass parent_class;
    void (*gglk_text)(GglkText *tb);

    /* event signals */
    void (*char_input_event)(GtkWidget *widget, int ch);
    void (*line_input_event)(GtkWidget *widget);
    void (*hyper_input_event)(GtkWidget *widget, int linkval);
    void (*mouse_input_event)(GtkWidget *widget, int x, int y);

    /* action signals */
    void (*clear_action)(GtkWidget *widget);
    void (*put_buffer)(GtkWidget *widget, const gunichar *buf, int len);
    int (*orig_button_press)(GtkWidget *widget, GdkEventButton *event);

};

GType gglk_text_get_type(void) PRIVATE;

void gglk_text_init_tags(GglkText *tb) PRIVATE;

gboolean gglk_text_can_scroll(GglkText *tb) PRIVATE;
void gglk_text_auto_scroll(GglkText *tb) PRIVATE;
void gglk_text_update_scrollmark(GglkText *tb) PRIVATE;

void gglk_text_clear(GglkText *tb) PRIVATE;
void gglk_text_put_buffer(GglkText *tb, const gunichar *buf, int len) PRIVATE;
void gglk_text_set_style(GglkText *tb, GtkTextTag *tag) PRIVATE;
void gglk_text_set_hyperlink(GglkText *tb, glui32 linkval) PRIVATE;


void gglk_text_set_char_input(GglkText *tb, gboolean requested) PRIVATE;
void gglk_text_set_mouse_input(GglkText *tb, gboolean requested) PRIVATE;

void gglk_text_line_input_set(GglkText *tb, int length,
			      const gunichar *text) PRIVATE;
gunichar *gglk_text_line_input_get(GglkText *tb) PRIVATE;
void gglk_text_line_input_end(GglkText *tb) PRIVATE;
void gglk_text_line_input_accept(GglkText *tb) PRIVATE;


G_END_DECLS

#endif /* GGLK_TEXT_H */
