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
#include "gglk_textbuffer.h"

#include <gtk/gtk.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtkmarshal.h>



static void gglk_textbuffer_class_init(GglkTextBufferClass *klass);
static void gglk_textbuffer_init(GglkTextBuffer *tb);

static void gglk_textbuffer_clear(GtkWidget *widget);
static void gglk_textbuffer_put_buffer(GtkWidget *widget, const glui32 *buf,
				       int len);


GType gglk_textbuffer_get_type(void)
{
    static GType gg_type = 0;

    if (!gg_type) {
	static const GTypeInfo gg_info = {
	    sizeof(GglkTextBufferClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) gglk_textbuffer_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GglkTextBuffer),
	    0,
	    (GInstanceInitFunc) gglk_textbuffer_init,
	    NULL
	};

	gg_type = g_type_register_static(GGLK_TEXT_TYPE, "GglkTextBuffer",
					 &gg_info, 0);
    }

    return gg_type;
}


static void gglk_textbuffer_class_init(GglkTextBufferClass *klass)
{
    GglkTextClass *text_class = (GglkTextClass *) klass;
    
    text_class->clear_action = gglk_textbuffer_clear;
    text_class->put_buffer = gglk_textbuffer_put_buffer;
}

static void gglk_textbuffer_init(GglkTextBuffer *tb)
{
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tb), GTK_WRAP_WORD);
}

/* Create a new GglkTextBuffer */
GtkWidget *gglk_textbuffer_new(struct glk_stylehint_struct *stylehints)
{
    GglkText *tb = GGLK_TEXT(g_object_new(gglk_textbuffer_get_type(), NULL));
    gglk_stylehint_ref(stylehints);
    tb->stylehints = stylehints;
    gglk_set_tags(GTK_TEXT_VIEW(tb), tb->stylehints);
    gglk_text_init_tags(tb);
    return GTK_WIDGET(tb);
}

static void gglk_textbuffer_clear(GtkWidget *widget)
{
    GglkTextBuffer *tb = GGLK_TEXTBUFFER(widget);
    gtk_text_buffer_set_text(tb->base.buffer, "", 0);
}


static void gglk_textbuffer_put_buffer(GtkWidget *widget,
				       const glui32 *buf, int len)
{
    GglkTextBuffer *tb = GGLK_TEXTBUFFER(widget);
    char *utfstr;
    utfstr = g_uni_to_utf8(buf, len, NULL, NULL, NULL);
    gtk_text_buffer_insert_with_tags(tb->base.buffer, &tb->base.iter,
				     utfstr, -1,
				     tb->base.style_tag,
				     NULL);
    g_free(utfstr);
}



void gglk_textbuffer_put_image(GglkTextBuffer *tb,
			       GdkPixbuf *pic, glui32 align)
{
    int line_height = tb->base.yunits;
    int height = gdk_pixbuf_get_height(pic);

    if(align == imagealign_MarginLeft || align == imagealign_MarginRight) {
	/* FIXME */
	gglk_textbuffer_put_image(tb, pic, imagealign_InlineUp);
	
#if 0
	GdkRectangle rect;
	GtkWidget *child, *event_box;

	event_box = gtk_event_box_new();
	child = gtk_image_new_from_pixbuf(pic);
	gtk_container_add(GTK_CONTAINER(event_box), child);

	gtk_widget_show_all(event_box);

	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(tb),
					&tb->base.iter, &rect);
	gtk_text_view_add_child_in_window(GTK_TEXT_VIEW(tb),
					  event_box, GTK_TEXT_WINDOW_TEXT,
					  rect.x, rect.y);
#endif

    } else {
	GtkTextIter start, end;

	switch(align) {
	case imagealign_InlineUp:
	    height = 0;
	    break;
	case imagealign_InlineDown:
	    height -= line_height;
	    break;
	case imagealign_InlineCenter:
	    height = (height - line_height) / 2;
	    break;
	default:
	    height = 0;
	}

	gtk_text_buffer_insert_pixbuf(tb->base.buffer, &tb->base.iter, pic);

	start = end = tb->base.iter;
	gtk_text_iter_backward_char(&start);
	if(height != 0) {
	    GtkTextTag *tag = gtk_text_buffer_create_tag(tb->base.buffer, NULL,
							 "rise", PANGO_SCALE * (-height),
							 NULL);
	    gtk_text_buffer_apply_tag(tb->base.buffer, tag, &start, &end);
	}
	gtk_text_buffer_get_end_iter(tb->base.buffer, &tb->base.iter);
    }
}

void gglk_textbuffer_put_flowbreak(GglkTextBuffer *tb)
{
    glui32 newline = '\n';
    gglk_text_put_buffer(GGLK_TEXT(tb), &newline, 1);
    /* ** FIXME ** */
}
