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
#include "gglk_graphics.h"

#include <gtk/gtk.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtkmarshal.h>

enum {
    CLICKED_SIGNAL,
    LAST_SIGNAL
};



static void gglk_graphics_class_init(GglkGraphicsClass *klass);
static void gglk_graphics_init(GglkGraphics *gg);
static void gglk_graphics_destroy(GtkObject *object,
				  gpointer unused_data);

static gboolean gglk_graphics_mouse(GtkWidget *unused_widget,
				    GdkEventButton *event,
				    gpointer user_data);
static gboolean gglk_graphics_configure(GtkWidget *unused_widget,
					GdkEventConfigure *event,
					gpointer user_data);
static gboolean gglk_graphics_expose(GtkWidget *widget,
				     GdkEventExpose *event,
				     gpointer user_data);




static guint gglk_graphics_signals[LAST_SIGNAL] = { 0 };


GType gglk_graphics_get_type(void)
{
    static GType gg_type = 0;

    if (!gg_type) {
	static const GTypeInfo gg_info = {
	    sizeof(GglkGraphicsClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) gglk_graphics_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GglkGraphics),
	    0,
	    (GInstanceInitFunc) gglk_graphics_init,
	    NULL
	};

	gg_type = g_type_register_static(GTK_TYPE_DRAWING_AREA, "GglkGraphics", &gg_info, 0);
    }

    return gg_type;
}

static void gglk_graphics_class_init(GglkGraphicsClass *klass)
{
    klass->mouse_input_event = NULL;
    gglk_graphics_signals[CLICKED_SIGNAL] =
	g_signal_new("mouse-input-event",
		     G_OBJECT_CLASS_TYPE(klass),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (GglkGraphicsClass, mouse_input_event),
		     NULL, NULL,
		     gtk_marshal_VOID__INT_INT,
		     G_TYPE_NONE, 2,
		     G_TYPE_INT, G_TYPE_INT);

}

static void gglk_graphics_init(GglkGraphics *gg)
{
    gg->pixbuf = NULL;
    gg->backcolor = 0xffffff;

    g_signal_connect(gg, "configure-event",
		     GTK_SIGNAL_FUNC(gglk_graphics_configure), NULL);
    g_signal_connect(gg, "expose-event",
		     GTK_SIGNAL_FUNC(gglk_graphics_expose), NULL);
    g_signal_connect(gg, "button-press-event",
		     GTK_SIGNAL_FUNC(gglk_graphics_mouse), NULL);
    g_signal_connect(gg, "destroy",
		     GTK_SIGNAL_FUNC(gglk_graphics_destroy), NULL);
    gtk_widget_add_events(GTK_WIDGET(gg), GDK_BUTTON_PRESS_MASK);

}


static void gglk_graphics_destroy(GtkObject *object,
				  gpointer unused_data)
{
    GglkGraphics *gg = GGLK_GRAPHICS(object);
    if(gg->pixbuf)
	g_object_unref(gg->pixbuf);
}


GtkWidget *gglk_graphics_new(void)
{
    return GTK_WIDGET(g_object_new(gglk_graphics_get_type(), NULL));
}


static gboolean gglk_normalize_coords(int winx, int winy,
				      glsi32 * restrict left,
				      glsi32 * restrict top,
				      glsi32 * restrict width,
				      glsi32 * restrict height)
{
    if(*left >= winx || *top >= winy)
	return FALSE;
    if(*left < 0) {
	if(-*left >= *width)
	    return FALSE;
	*width = MIN(*left + *width, winx);
	*left = 0;
    } else {
	*width = MIN(*width, winx - *left);
    }
    if(*top < 0) {
	if(-*top >= *height)
	    return FALSE;
	*height = MIN(*top + *height, winy);
	*top = 0;
    } else {
	*height = MIN(*height, winy - *top);
    }
    if(*width <= 0 || *height <= 0)
	return FALSE;
    return TRUE;
}


void gglk_graphics_fill_rect(GglkGraphics *gg,
				 glui32 color,
				 glsi32 left, glsi32 top,
				 glui32 width, glui32 height)
{
    int winx, winy;
    GdkPixbuf *sub;

    if(!gg->pixbuf) return;
    winx = gdk_pixbuf_get_width(gg->pixbuf);
    winy = gdk_pixbuf_get_height(gg->pixbuf);

    if(!gglk_normalize_coords(winx, winy, &left, &top, &width, &height))
	return;

    /* Don't know how to use drawing primitives on pixbufs...
       so create a subpixbuf and fill it in */
    sub = gdk_pixbuf_new_subpixbuf(gg->pixbuf, left, top, width, height);
    gdk_pixbuf_fill(sub, (color << 8) | 0xff);
    g_object_unref(sub);
    
    gtk_widget_queue_draw(GTK_WIDGET(gg));
}


void gglk_graphics_clear(GglkGraphics *gg)
{
    if(!gg->pixbuf) return;
    gdk_pixbuf_fill(gg->pixbuf, (gg->backcolor << 8) | 0xff);
    gtk_widget_queue_draw(GTK_WIDGET(gg));
}


void gglk_graphics_erase_rect(GglkGraphics *gg,
				  glsi32 left, glsi32 top, glui32 width, glui32 height)
{
    gglk_graphics_fill_rect(gg, gg->backcolor, left, top, width, height);
}


GdkPixbuf *gglk_graphics_get_image(GglkGraphics *gg,
				  glsi32 left, glsi32 top,
				  glui32 width, glui32 height)
{
	int winx, winy;
    GdkPixbuf *result;
    if(!gg->pixbuf) return NULL;
    winx = gdk_pixbuf_get_width(gg->pixbuf);
    winy = gdk_pixbuf_get_height(gg->pixbuf);

    if(!gglk_normalize_coords(winx, winy, &left, &top, &width, &height))
	return NULL;
    
    result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
				       width, height);
    gdk_pixbuf_copy_area(gg->pixbuf, left, top, width, height, result, 0, 0);
    return result;
}


void *sglk_get_blorbimage(glui32 image)
{
    return gglk_load_pic(image);
}


void sglk_delete_image(void *b)
{
    g_object_unref(GDK_PIXBUF(b));
}


glui32 gglk_graphics_image_draw(GglkGraphics *gg,
				    GdkPixbuf *pic, glsi32 val1, glsi32 val2)
{
    glui32 width, height;
    glsi32 top = val1, left = val2;
    int winx, winy;
    if(!gg->pixbuf) return FALSE;
    winx = gdk_pixbuf_get_width(gg->pixbuf);
    winy = gdk_pixbuf_get_height(gg->pixbuf);
    width = gdk_pixbuf_get_width(pic);
    height = gdk_pixbuf_get_height(pic);

    if(!gglk_normalize_coords(winx, winy, &top, &left, &width, &height))
	return FALSE;
	
    if(gglk_gestalt_support[gestalt_GraphicsTransparency])
	gdk_pixbuf_composite(pic, gg->pixbuf,
			     top, left, width, height,
			     val1, val2, 1, 1,
			     GDK_INTERP_NEAREST, 255);
    else
	gdk_pixbuf_composite_color(pic, gg->pixbuf,
				   top, left, width, height,
				   val1, val2, 1, 1,
				   GDK_INTERP_NEAREST, 255,
				   0, 0, 1, 0, 0);
	    
    gtk_widget_queue_draw(GTK_WIDGET(gg));
    return TRUE;
}


glui32 gglk_graphics_image_draw_scaled(GglkGraphics *gg,
				       GdkPixbuf *pic, 
				       glsi32 val1, glsi32 val2,
				       glui32 width, glui32 height)
{
    int winx, winy;
    glsi32 top = val1, left = val2;
    double scalex, scaley;
    if(!gg->pixbuf) return FALSE;
    scalex = ((double) width) / gdk_pixbuf_get_width(pic);
    scaley = ((double) height) / gdk_pixbuf_get_height(pic);
    winx = gdk_pixbuf_get_width(gg->pixbuf);
    winy = gdk_pixbuf_get_height(gg->pixbuf);

    if(!gglk_normalize_coords(winx, winy, &top, &left, &width, &height))
	return FALSE;

    if(gglk_gestalt_support[gestalt_GraphicsTransparency])
	gdk_pixbuf_composite(pic, gg->pixbuf,
			     top, left, width, height,
			     val1, val2, scalex, scaley,
			     GDK_INTERP_HYPER, 255);
    else
	gdk_pixbuf_composite_color(pic, gg->pixbuf,
				   top, left, width, height,
				   val1, val2, scalex, scaley,
				   GDK_INTERP_HYPER, 255,
				   0, 0, 1, 0, 0);
    gtk_widget_queue_draw(GTK_WIDGET(gg));
    return TRUE;
}


static gboolean gglk_graphics_mouse(GtkWidget *widget,
				    GdkEventButton *event,
				    gpointer unused_data)
{
    GglkGraphics *gg = GGLK_GRAPHICS(widget);
    if(event->button == 1) {
	gint x, y;
	x = event->x;
	y = event->y;
	g_signal_emit(gg, gglk_graphics_signals[CLICKED_SIGNAL], 0, x, y);
	return TRUE;
    }
    return FALSE;
}


void gglk_graphics_resize(GglkGraphics *gg,
			      gint width, gint height)
{
    GdkPixbuf *oldpix = gg->pixbuf;

    /* If it's 1x1 or smaller, don't resize it.*/
    if(width <= 0 || height <= 0 || (width == 1 && height == 1)) {
	return;
    }

    gg->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
				 width, height);
    gdk_pixbuf_fill(gg->pixbuf, (gg->backcolor << 8) | 0xff);
    
    if(oldpix) {
	gdk_pixbuf_copy_area(oldpix, 0, 0,
			     MIN(width, gdk_pixbuf_get_width(oldpix)),
			     MIN(height, gdk_pixbuf_get_height(oldpix)),
			     gg->pixbuf, 0, 0);
	g_object_unref(oldpix);
    }
}


static gboolean gglk_graphics_configure(GtkWidget *widget,
					GdkEventConfigure *event,
					gpointer unused_data)
{
    gglk_graphics_resize(GGLK_GRAPHICS(widget), event->width, event->height);
    return TRUE;
}

static gboolean gglk_graphics_expose(GtkWidget *widget, GdkEventExpose *event,
				    gpointer unused_data)
{
    GglkGraphics *gg = GGLK_GRAPHICS(widget);
    if(!gg->pixbuf) return TRUE;
    
    gdk_gc_set_clip_rectangle(widget->style->fg_gc[widget->state],
			      &event->area);

#if GTK_CHECK_VERSION(2,2,0)
    gdk_draw_pixbuf(widget->window, widget->style->fg_gc[widget->state],
		    gg->pixbuf, 0, 0, 0, 0,
		    gdk_pixbuf_get_width(gg->pixbuf),
		    gdk_pixbuf_get_height(gg->pixbuf),
		    GDK_RGB_DITHER_NORMAL,
		    event->area.x, event->area.y);
#else
    gdk_pixbuf_render_to_drawable(gg->pixbuf, widget->window,
                                  widget->style->fg_gc[widget->state],
                                  0, 0, 0, 0,
                                  gdk_pixbuf_get_width(gg->pixbuf),
                                  gdk_pixbuf_get_height(gg->pixbuf),
                                  GDK_RGB_DITHER_NORMAL,
                                  event->area.x, event->area.y);
#endif

    gdk_gc_set_clip_rectangle(widget->style->fg_gc[widget->state],
			      NULL);
    return TRUE;
}


void gglk_graphics_background(GglkGraphics *gg,
				  glui32 backcolor)
{
    GdkColor color;
    GtkWidget *widget = GTK_WIDGET(gg);
    
    color.red   = ((backcolor & 0xff0000) >> 16) * 257;
    color.green = ((backcolor & 0xff00) >> 8) * 257;
    color.blue  = (backcolor & 0xff) * 257;

    gdk_colormap_alloc_color(gtk_widget_get_colormap(widget),
			     &color,
			     FALSE, TRUE);

    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &color);

    gg->backcolor = backcolor;
}

