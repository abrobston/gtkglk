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

#ifndef GGLK_TEXTBUFFER_H
#define GGLK_TEXTBUFFER_H

#include "gglk.h"
#include <glib.h>
#include <glib-object.h>
#include "gglk_text.h"

G_BEGIN_DECLS

#define GGLK_TEXTBUFFER_TYPE (gglk_textbuffer_get_type ())
#define GGLK_TEXTBUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GGLK_TEXTBUFFER_TYPE, GglkTextBuffer))
#define GGLK_TEXTBUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GGLK_TEXTBUFFER_TYPE, GglkTextBufferClass))
#define IS_GGLK_TEXTBUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GGLK_TEXTBUFFER_TYPE))
#define IS_GGLK_TEXTBUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GGLK_TEXTBUFFER_TYPE))


typedef struct _GglkTextBuffer   GglkTextBuffer;
typedef struct _GglkTextBufferClass  GglkTextBufferClass;

struct _GglkTextBuffer 
{
    GglkText base;
};

struct _GglkTextBufferClass
{
    GglkTextClass parent_class;
    void (*gglk_textbuffer)(GglkTextBuffer *tb);
};

GType gglk_textbuffer_get_type(void) PRIVATE;
GtkWidget *gglk_textbuffer_new(struct glk_stylehint_struct *stylehints) PRIVATE;

void gglk_textbuffer_put_image(GglkTextBuffer *tb, GdkPixbuf *pic, glui32 align) PRIVATE;
void gglk_textbuffer_put_flowbreak(GglkTextBuffer *tb) PRIVATE;

G_END_DECLS

#endif /* GGLK_TEXTBUFFER_H */
