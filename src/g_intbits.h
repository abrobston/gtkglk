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

#ifndef G_INTBITS_H
#define G_INTBITS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_INTBITS            (g_intbits_get_type())
#define G_INTBITS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), G_TYPE_INTBITS, GIntbits))
#define G_INTBITS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_INTBITS, GIntbitsCLass))
#define G_IS_INTBITS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), G_TYPE_INTBITS))
#define G_IS_INTBITS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), G_TYPE_INTBITS))
#define G_INTBITS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), G_TYPE_INTBITS, GIntbitsClass))


typedef struct _GIntbits GIntbits;
typedef struct _GIntbitsClass GIntbitsClass;

struct _GIntbits 
{
    GObject parent_class;
    gint value;
};

struct _GIntbitsClass
{
    GObjectClass parent_class;
};


GType g_intbits_get_type(void) G_GNUC_CONST;
GIntbits *g_intbits_new(void);

G_END_DECLS

#endif
