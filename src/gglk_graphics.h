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

#ifndef GGLK_GRAPHICS_H
#define GGLK_GRAPHICS_H


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkdrawingarea.h>


G_BEGIN_DECLS

#define GGLK_GRAPHICS_TYPE (gglk_graphics_get_type ())
#define GGLK_GRAPHICS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GGLK_GRAPHICS_TYPE, GglkGraphics))
#define GGLK_GRAPHICS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GGLK_GRAPHICS_TYPE, GglkGraphicsClass))
#define IS_GGLK_GRAPHICS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GGLK_GRAPHICS_TYPE))
#define IS_GGLK_GRAPHICS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GGLK_GRAPHICS_TYPE))


typedef struct _GglkGraphics   GglkGraphics;
typedef struct _GglkGraphicsClass  GglkGraphicsClass;

struct _GglkGraphics 
{
    GtkDrawingArea drawing;
    
    GdkPixbuf *pixbuf;
    glui32 backcolor;
};

struct _GglkGraphicsClass
{
    GtkDrawingAreaClass parent_class;
    void (*gglk_graphics)(GglkGraphics *gg);

    /* signals */
    void (*mouse_input_event)(GtkWidget *widget, gint x, gint y);
};


GType gglk_graphics_get_type(void) PRIVATE;
GtkWidget *gglk_graphics_new(void) PRIVATE;

void gglk_graphics_fill_rect(GglkGraphics *gg, glui32 color,
			     glsi32 left, glsi32 top,
			     glui32 width, glui32 height) PRIVATE;
GdkPixbuf *gglk_graphics_get_image(GglkGraphics *gg,
				   glsi32 left, glsi32 top,
				   glui32 width, glui32 height) PRIVATE;
glui32 gglk_graphics_image_draw(GglkGraphics *gg, GdkPixbuf *pic,
				glsi32 val1, glsi32 val2) PRIVATE;
glui32 gglk_graphics_image_draw_scaled(GglkGraphics *gg,
				       GdkPixbuf *pic, 
				       glsi32 val1, glsi32 val2,
				       glui32 width, glui32 height) PRIVATE;
void gglk_graphics_resize(GglkGraphics *gg, gint width, gint height) PRIVATE;
void gglk_graphics_clear(GglkGraphics *gg) PRIVATE;
void gglk_graphics_erase_rect(GglkGraphics *gg,
			      glsi32 left, glsi32 top, glui32 width, glui32 height) PRIVATE;
void gglk_graphics_background(GglkGraphics *gg, glui32 color) PRIVATE;


G_END_DECLS

#endif /* GGLK_GRAPHICS_H */
