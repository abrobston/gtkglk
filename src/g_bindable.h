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

#ifndef G_BINDABLE_H
#define G_BINDABLE_H

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define G_TYPE_BINDABLE            (g_bindable_get_type())
#define G_BINDABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), G_TYPE_BINDABLE, GBindable))
#define G_BINDABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BINDABLE, GBindableCLass))
#define G_IS_BINDABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), G_TYPE_BINDABLE))
#define G_IS_BINDABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), G_TYPE_BINDABLE))
#define G_BINDABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), G_TYPE_BINDABLE, GBindableClass))


typedef struct _GBindable GBindable;
typedef struct _GBindableClass GBindableClass;

struct g_bindable_key {
    GType type;
    void (*fun)(GValue *, gboolean, gpointer[]);
    gpointer data[4];           /* Only data[0], data[1], data[2] are public */
};

struct _GBindable 
{
    GObject parent_class;
    gboolean floating;          /* Is there is a floating reference to me */

    struct g_bindable_key key;         /* Information unique to me */

    GValue value;
    gboolean current, writable, sensitive;
    int init_level;

    GList *views;               /* I am a model for these views */
    GBindable *model;           /* I am a view for this model */
    GList *model_views_node;    /* This is the node in my model's "views" list
				   which points to me */
};

struct _GBindableClass
{
    GObjectClass parent_class;
};


GType g_bindable_get_type(void) G_GNUC_CONST;

GBindable *g_bindable_new_from_func_gvalue(GType type,
					   void (*fun)(GValue *,gboolean,
						       gpointer[]),
					   gpointer data1, gpointer data2,
					   gpointer data3);

GBindable *g_bindable_new_from_func_boolean(gboolean (*fun)(gboolean,gboolean,
							    gpointer[]),
					    gpointer data1, gpointer data2);
GBindable *g_bindable_new_from_func_integer(gint (*fun)(gint,gboolean,
							gpointer[]),
					    gpointer data1, gpointer data2);
GBindable *g_bindable_new_from_func_double(double (*fun)(double,gboolean,
							 gpointer[]),
					   gpointer data1, gpointer data2);
GBindable *g_bindable_new_from_func_string(gchar * (*fun)(const gchar *,gboolean,
							  gpointer[]),
					   gpointer data1, gpointer data2);

GBindable *g_bindable_new_from_boolean(gboolean *value);
GBindable *g_bindable_new_from_integer(gint *value);
GBindable *g_bindable_new_from_double(double *value);

GBindable *g_bindable_collect(GBindable *gbindable, GHashTable *collection);
void g_bindable_collect_poll(GHashTable *collection);

#define g_bindable_new_from_boolean_collect(val, collect) g_bindable_collect(g_bindable_new_from_boolean(val), collect)
#define g_bindable_new_from_integer_collect(val, collect) g_bindable_collect(g_bindable_new_from_integer(val), collect)
#define g_bindable_new_from_double_collect(val, collect) g_bindable_collect(g_bindable_new_from_double(val), collect)


void g_bindable_gconf_set_gconf_default(GConfClient *client,
					const char *path);
GBindable *g_bindable_new_from_gconf(const gchar *key);
GBindable *g_bindable_new_from_gconf_changeset(GConfChangeSet *changeset,
					       const gchar *key);
GBindable *g_bindable_new_from_gobject(GObject *object, const gchar *prop);
GBindable *g_bindable_new_from_gtkobject_prop(GtkObject *object,
					      const char *prop);
GBindable *g_bindable_new_from_gtkobject(GtkObject *object);
GBindable *g_bindable_new_from_gtktreemodel(GtkTreeModel *model,
					    GtkTreePath *path,
					    gint column);

GBindable *g_bindable_new_model_cast(GType type, GBindable *gbindable);
GBindable *g_bindable_new_view_cast(GType type, GBindable *gbindable);
GBindable *g_bindable_new_view_transform(GBindable *child, GType newtype,
			 gboolean (*transformer)(GValue *value, GValue *tvalue,
						 gboolean forwards,
						 gpointer data),
					 gpointer data);
GBindable *g_bindable_new_view_not(GBindable *gbindable);
GBindable *g_bindable_new_view_bool_int(GBindable *child, gint val);
GBindable *g_bindable_new_view_add_const(GBindable *child, gint addval);

GBindable *g_bindable_new_model_transform(GBindable *child, GType newtype,
			 gboolean (*transformer)(GValue *value, GValue *tvalue,
						 gboolean forwards,
						 gpointer data),
					  gpointer data);
GBindable *g_bindable_new_model_not(GBindable *gbindable);
GBindable *g_bindable_new_model_bool_int(GBindable *child, gint val);
GBindable *g_bindable_new_model_add_const(GBindable *child, gint addval);

gboolean g_bindable_get_writable(GBindable *gbindable);
void g_bindable_set_writable(GBindable *gbindable, gboolean writable);

void g_bindable_notify_changed(GBindable *gbindable);
void g_bindable_notify_maybe_changed(GBindable *bindable);

void g_bindable_bind(GBindable *model, GBindable *view, ...);
void g_bindable_unbind_view(GBindable *view);


#ifdef GLADE_XML_H
#define g_bindable_new_from_glade(self, name) g_bindable_new_from_gtkobject(GTK_OBJECT(glade_xml_get_widget(self, name)))
#else
#define g_bindable_new_from_glade(self, name) g_bindable_new_from_gtkobject(GTK_OBJECT(lookup_widget(self, name)))
#endif

G_END_DECLS

#endif
