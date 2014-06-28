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
#include "gglk_textgrid.h"

#include <gtk/gtk.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtkmarshal.h>



static void gglk_textgrid_class_init(GglkTextGridClass *klass);
static void gglk_textgrid_init(GglkTextGrid *tb);
static void gglk_textgrid_clear(GtkWidget *widget);
static void gglk_textgrid_put_buffer(GtkWidget *widget, const glui32 *buf,
				     int len);


GType gglk_textgrid_get_type(void)
{
    static GType gg_type = 0;

    if (!gg_type) {
	static const GTypeInfo gg_info = {
	    sizeof(GglkTextGridClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) gglk_textgrid_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GglkTextGrid),
	    0,
	    (GInstanceInitFunc) gglk_textgrid_init,
	    NULL
	};

	gg_type = g_type_register_static(GGLK_TEXT_TYPE, "GglkTextGrid",
					 &gg_info, 0);
    }

    return gg_type;
}

static void gglk_textgrid_class_init(GglkTextGridClass *klass)
{
    GglkTextClass *text_class = (GglkTextClass *) klass;
    
    text_class->clear_action = gglk_textgrid_clear;
    text_class->put_buffer = gglk_textgrid_put_buffer;
}

static void gglk_textgrid_init(GglkTextGrid *tb)
{
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tb), GTK_WRAP_NONE);
    tb->maxx = tb->maxy = 0;
}

/* Create a new GglkTextGrid */
GtkWidget *gglk_textgrid_new(struct glk_stylehint_struct *stylehints)
{
    GglkText *tb = GGLK_TEXT(g_object_new(gglk_textgrid_get_type(), NULL));
    gglk_stylehint_ref(stylehints);
    tb->stylehints = stylehints;
    gglk_set_tags(GTK_TEXT_VIEW(tb), tb->stylehints);
    gglk_text_init_tags(tb);
    gglk_textgrid_clear(GTK_WIDGET(tb));

    return GTK_WIDGET(tb);
}


static void gglk_textgrid_clear(GtkWidget *widget)
{
    GglkTextGrid *tb = GGLK_TEXTGRID(widget);

    if(tb->maxx != 0 && tb->maxy != 0) {
	int xsize = tb->maxx, ysize = tb->maxy;
	gtk_text_buffer_set_text(tb->base.buffer, "", 0);
	tb->maxx = tb->maxy = tb->curx = tb->cury = 0;
	gglk_textgrid_size(tb, xsize, ysize);
    }
}

void gglk_textgrid_move_cursor(GglkTextGrid *tb, guint xpos, guint ypos)
{
    glui32 oldlink = tb->base.hyperval;
    gglk_text_set_hyperlink(GGLK_TEXT(tb), 0);
    
    if(tb->maxx == 0 || tb->maxy == 0)
	return;
    if(xpos > tb->maxx || ypos >= tb->maxy) {
	xpos = tb->maxx;
	ypos = tb->maxy-1;
    }
    tb->curx = xpos;
    tb->cury = ypos;
    gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer, &tb->base.iter,
					    tb->cury, tb->curx);
    gtk_text_buffer_place_cursor(tb->base.buffer, &tb->base.iter);

    gglk_text_set_hyperlink(GGLK_TEXT(tb), oldlink);
}


static void gglk_textgrid_put_buffer_raw(GglkTextGrid *tb, const glui32 *buf,
					 int len, int cell_len) 
{
    GtkTextIter b;
    gchar *utfstr;
   
    /* Delete text from the grid to make room for the replacement */
    gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer, &tb->base.iter,
					    tb->cury, tb->curx);
    gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer, &b,
					    tb->cury, tb->curx + cell_len);
    gtk_text_buffer_delete(tb->base.buffer, &tb->base.iter, &b);
    gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer, &tb->base.iter,
					    tb->cury, tb->curx);
    tb->curx += cell_len;

    utfstr = g_ucs4_to_utf8(buf, len, NULL, NULL, NULL);

    gtk_text_buffer_insert_with_tags(tb->base.buffer, &tb->base.iter,
				     utfstr, -1,
				     tb->base.style_tag,
				     NULL);
    g_free(utfstr);
}



static void gglk_textgrid_put_buffer(GtkWidget *widget, const glui32 *buf,
				     int len)
{
    GglkTextGrid *tb = GGLK_TEXTGRID(widget);
    int cell_len, i;
    glui32 ch;
    glui32 char_len;

    if(tb->cury >= tb->maxy || tb->maxx == 0 || len == 0)
	return;

    cell_len = 0; /* How many textgrid cells we're going to overwrite */
    for(i = 0; i < len; i++) {
	switch(buf[i]) {
	case '\n':  /* Output text read so far, and move to next line */
	    gglk_textgrid_put_buffer_raw(tb, buf, i, cell_len);
	    gglk_textgrid_move_cursor(tb, 0, tb->cury + 1);
	    len -= i + 1;
	    buf += i + 1;
	    i = 0;
	    cell_len = 0;
	    break;
	case '\t': /* Output text read so far, then a space */
	    gglk_textgrid_put_buffer_raw(tb, buf, i, cell_len);
	    len -= i + 1;
	    buf += i + 1;
	    i = 0;
	    cell_len = 0;
	    /* Use gglk_textgrid_put_buffer because of possible wrapping */
	    ch = ' ';
	    gglk_textgrid_put_buffer(GTK_WIDGET(tb), &ch, 1);
	    break;
	case '\0':
	    g_warning("tried to print \\0 to TextGrid");
	    len = i;
	    break;
	default:  /* See how many cells are used, possibly wrapping */
	    glk_gestalt_ext(gestalt_CharOutput, buf[i], &char_len, 1);

	    if(tb->curx + cell_len + char_len > tb->maxx) {
		/* Output text read so far, go to next line */
		gglk_textgrid_put_buffer_raw(tb, buf, i, cell_len);
		gglk_textgrid_move_cursor(tb, 0, tb->cury + 1);
		len -= i;
		buf += i;
		i = 0;
		cell_len = 0;
	    }

	    cell_len += char_len;
	}
	if(tb->cury >= tb->maxy || len == 0 || tb->curx + len > tb->maxx)
	    return;
    }
    /* Final text portion */
    gglk_textgrid_put_buffer_raw(tb, buf, i, cell_len);
}





void gglk_textgrid_size(GglkTextGrid *tb, glui32 xsize, glui32 ysize)
{
    glui32 x, y;
    char t[xsize+2];

    if(xsize == 0 || ysize == 0) {
	gglk_text_clear(GGLK_TEXT(tb));
	tb->maxx = xsize;
	tb->maxy = ysize;
	return;
    }

    for(x = 0; x < xsize; x++)
	t[x] = ' ';
    t[x++] = '\n';
    t[x++] = 0;

    for(y = 0; y < ysize; y++) {
	int b = (y == ysize - 1);
	if(y < tb->maxy) {
	    GtkTextIter oldend, newend;
	    gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer,
						    &oldend, y, tb->maxx);
	    if(xsize < tb->maxx) {
		gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer,
							&newend, y, xsize);
		gtk_text_buffer_delete(tb->base.buffer, &newend, &oldend);
	    } else {
		gtk_text_buffer_insert_with_tags_by_name(
		    tb->base.buffer, &oldend, t, xsize - tb->maxx,
		    gglk_get_tag(style_Normal), NULL);
	    }
	} else {
	    GtkTextIter oldend;
	    gtk_text_buffer_get_end_iter(tb->base.buffer, &oldend);
	    gtk_text_buffer_insert_with_tags_by_name(
		tb->base.buffer, &oldend, t, xsize + 1 - b,
		gglk_get_tag(style_Normal), NULL);
	}
    }

    if(ysize < tb->maxy) {
	GtkTextIter oldend, newend;
	gtk_text_buffer_get_end_iter(tb->base.buffer, &oldend);
	gtk_text_buffer_get_iter_at_line_offset(tb->base.buffer,
						&newend, ysize-1, xsize);
	gtk_text_buffer_delete(tb->base.buffer, &newend, &oldend);
    }

    tb->maxx = xsize;
    tb->maxy = ysize;
    gglk_textgrid_move_cursor(tb, tb->curx, tb->cury);
}

