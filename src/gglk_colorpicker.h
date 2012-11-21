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

#ifndef GGLK_COLORPICKER_H
#define GGLK_COLORPICKER_H

#include "gglk.h"
#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GGLK_COLORPICKER_TYPE     (gglk_colorpicker_get_type ())
#define GGLK_COLORPICKER(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GGLK_COLORPICKER_TYPE, GglkColorpicker))
#define GGLK_COLORPICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GGLK_COLORPICKER_TYPE, GglkColorpickerClass))
#define IS_GGLK_COLORPICKER(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GGLK_COLORPICKER_TYPE))
#define IS_GGLK_COLORPICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GGLK_COLORPICKER_TYPE))

typedef struct _GglkColorpicker GglkColorpicker;
typedef struct _GglkColorpickerClass GglkColorpickerClass;


struct _GglkColorpicker 
{
    GtkHBox hbox;
    GtkCheckButton *check;
    GtkButton *button;
    GtkDrawingArea *drawing;

    char *title;
    gboolean enabled;
    GdkColor color;
};

struct _GglkColorpickerClass
{
    GtkHBoxClass parent_class;
    void (*gglk_colorpicker)(GglkColorpicker *cp);
};

GType gglk_colorpicker_get_type(void) PRIVATE;
GtkWidget *gglk_colorpicker_new(const char *title) PRIVATE;

G_END_DECLS

#endif
