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

#include "g_bindable.h"
#include <string.h>


/* Silence some gcc warnings */
#define unused_data     unused_data     G_GNUC_UNUSED
#define unused_value    unused_value    G_GNUC_UNUSED
#define unused_object   unused_object   G_GNUC_UNUSED
#define unused_pspec    unused_pspec    G_GNUC_UNUSED
#define unused_model    unused_model    G_GNUC_UNUSED
#define unused_path     unused_path     G_GNUC_UNUSED
#define unused_iter     unused_iter     G_GNUC_UNUSED
#define unused_client   unused_client   G_GNUC_UNUSED
#define unused_cnxn_id  unused_cnxn_id  G_GNUC_UNUSED
#define unused_entry    unused_entry    G_GNUC_UNUSED


static GHashTable *g_bindable_cache;


enum {
  PROP_0,
  PROP_WRITABLE,
  PROP_SENSITIVE,
  PROP_MODEL
};

static void g_bindable_class_init(GBindableClass *klass);
static void g_bindable_init(GBindable *bindable);
static void g_bindable_set_property(GObject *object, guint prop_id,
				    const GValue *value, GParamSpec *pspec);
static void g_bindable_get_property(GObject *object, guint prop_id,
				    GValue *value, GParamSpec *pspec);
static void g_bindable_finalize(GObject *object);




static void g_bindable_pass_to_children(GBindable *root);



GType g_bindable_get_type(void)
{
    static GType type = 0;
    if(!type) {
	static const GTypeInfo info = {
	    sizeof(GBindableClass),
	    NULL,
	    NULL,
	    (GClassInitFunc) g_bindable_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GBindable),
	    0,    /* n_preallocs */
	    (GInstanceInitFunc) g_bindable_init,
	    NULL  /* value_table */
	};

	type = g_type_register_static(G_TYPE_OBJECT, "GBindable",
				      &info, 0);
    }

    return type;
}


static void g_bindable_class_init(GBindableClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    object_class->set_property = g_bindable_set_property;
    object_class->get_property = g_bindable_get_property;
    object_class->finalize = g_bindable_finalize;

    g_object_class_install_property(
	object_class, PROP_WRITABLE,
	g_param_spec_boolean("writable", "Writable",
			     "True if the associated item is writable",
			     TRUE,
			     G_PARAM_READABLE | G_PARAM_WRITABLE));
    g_object_class_install_property(
	object_class, PROP_SENSITIVE,
	g_param_spec_boolean("sensitive", "Sensitive",
			     "True if writes will be written to the model",
			     TRUE,
			     G_PARAM_READABLE));
    g_object_class_install_property(
	object_class, PROP_MODEL,
	g_param_spec_object("model", "Model",
			    "The model this bindable is bound to",
			    G_TYPE_BINDABLE,
			    G_PARAM_READABLE | G_PARAM_WRITABLE));
}



static void g_bindable_init(GBindable *bindable)
{
    bindable->floating = TRUE;
    bindable->writable = TRUE;
    bindable->init_level = 0;

    memset(&bindable->value, 0, sizeof(bindable->value));
    bindable->current = FALSE;

    bindable->views = NULL;
    bindable->model = NULL;
    bindable->model_views_node = NULL;
}

static void g_bindable_set_property(GObject *object, guint prop_id,
				    const GValue *value, GParamSpec *pspec)
{
    GBindable *bindable = G_BINDABLE(object);
    switch(prop_id) {
    case PROP_WRITABLE:
	g_bindable_set_writable(bindable, g_value_get_boolean(value));
	break;
    case PROP_MODEL:
	g_bindable_bind(G_BINDABLE(g_value_get_object(value)),
			bindable,
			NULL);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void g_bindable_get_property(GObject *object, guint prop_id,
				    GValue *value, GParamSpec *pspec)
{
    GBindable *bindable = G_BINDABLE(object);
    switch(prop_id) {
    case PROP_WRITABLE:
	g_value_set_boolean(value, bindable->writable);
	break;
    case PROP_SENSITIVE:
	g_value_set_boolean(value, bindable->sensitive);
	break;
    case PROP_MODEL:
	g_value_set_object(value, bindable->model);
	break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void g_bindable_finalize(GObject *object)
{
    GBindable *bindable = G_BINDABLE(object);
    GList *l;
    
    for(l = bindable->views; l; l = g_list_next(l)) {
	GBindable *child = l->data;
	child->model = NULL;
	child->model_views_node = NULL;
    }
    g_list_free(bindable->views);
    bindable->views = NULL;

    g_bindable_unbind_view(bindable);

    g_hash_table_remove(g_bindable_cache, &bindable->key);
}



static void g_bindable_read_data(GBindable *bindable) 
{
    if(bindable->key.fun) {
	bindable->key.fun(&bindable->value, FALSE, bindable->key.data);
	bindable->current = TRUE;
    }
}
static inline void g_bindable_ensure_read_data(GBindable *bindable) 
{
    if(!bindable->current)
	g_bindable_read_data(bindable);
}
static void g_bindable_write_data(GBindable *bindable) 
{
    if(bindable->key.fun && bindable->writable) {
	bindable->key.fun(&bindable->value, TRUE, bindable->key.data);
	bindable->current = TRUE;
    }
}
static gboolean g_bindable_transform_value(GBindable *src, GBindable *dest)
{
    if(g_value_transform(&src->value, &dest->value)) {
	dest->current = TRUE;
	return TRUE;
    } else {
	g_warning("cannot transform %s %s to %s!",
		  G_VALUE_TYPE_NAME(&src->value),
		  g_strdup_value_contents(&src->value),
		  G_VALUE_TYPE_NAME(&dest->value));
	return FALSE;
    }
}




static guint g_bindable_key_hash(gconstpointer key)
{
    const struct g_bindable_key *k = key;
    return g_direct_hash(k->fun) + g_direct_hash(k->data[0]) +
	g_direct_hash(k->data[1]) + g_direct_hash(k->data[2]);
}

static gboolean g_bindable_key_equal(gconstpointer key1,
				     gconstpointer key2)
{
    const struct g_bindable_key *k1 = key1;
    const struct g_bindable_key *k2 = key2;

    return k1->type == k2->type && k1->fun == k2->fun &&
	k1->data[0] == k2->data[0] && k1->data[1] == k2->data[1] &&
	k1->data[2] == k2->data[2];
}


GBindable *g_bindable_new_from_func_gvalue(GType type,
					   void (*fun)(GValue *,gboolean,
						       gpointer[]),
					   gpointer data1, gpointer data2,
					   gpointer data3)
{
    struct g_bindable_key key;
    GBindable *bindable;

    if(!g_bindable_cache) {
	g_bindable_cache = g_hash_table_new(g_bindable_key_hash,
					    g_bindable_key_equal);
    }
    
    key.type = type;
    key.fun = fun;
    key.data[0] = data1;
    key.data[1] = data2;
    key.data[2] = data3;

    bindable = g_hash_table_lookup(g_bindable_cache, &key);
    if(bindable)
	return bindable;
    
    bindable = G_BINDABLE(g_object_new(G_TYPE_BINDABLE, NULL));
    
    bindable->key = key;

    g_value_init(&bindable->value, bindable->key.type);

    g_hash_table_insert(g_bindable_cache, &bindable->key, bindable);
    
    return bindable;
}


static void g_bindable_bind1(GBindable *model, GBindable *view)
{
    if(view->model)
	g_bindable_unbind_view(view);

    model->views = g_list_prepend(model->views, view);
    view->model = model;
    view->model_views_node = model->views;

    g_bindable_ensure_read_data(model);
    
    if(g_bindable_transform_value(model, view)) {
	g_bindable_write_data(view);
	g_bindable_pass_to_children(view);
    }
}

void g_bindable_bind(GBindable *model, GBindable *view, ...)
{
    va_list ap;
    va_start(ap, view);

    g_return_if_fail(G_IS_BINDABLE(model));

    while(view) {
	g_return_if_fail(G_IS_BINDABLE(view));
	g_bindable_bind1(model, view);
	view = va_arg(ap, GBindable *);
    }
    va_end(ap);
}

void g_bindable_unbind_view(GBindable *view)
{
    if(!view->model)
	return;
    view->model->views = g_list_delete_link(view->model->views,
					    view->model_views_node);
    view->model = NULL;
    view->model_views_node = NULL;
}


static void g_bindable_pass_to_children(GBindable *root)
{
    GList *l;
    for(l = root->views; l; l = g_list_next(l)) {
	GBindable *child = l->data;
	g_bindable_ensure_read_data(root);
	g_bindable_transform_value(root, child);
	g_bindable_write_data(child);
	
	if(child->views)
	    g_bindable_pass_to_children(child);
    }
}


void g_bindable_notify_changed(GBindable *gbindable)
{
    g_bindable_read_data(gbindable);

    /* Pass the new value up the tree */
    while(gbindable->model) {
	g_bindable_transform_value(gbindable, gbindable->model);
	gbindable = gbindable->model;
	g_bindable_write_data(gbindable);
    }

    /* Now pass the value back down the tree */
    g_bindable_pass_to_children(gbindable);
}


static gboolean g_bindable_value_equal(const GValue *a, const GValue *b)
{
    GType type;
    const gpointer *pa, *pb;
    
    if(a == b)
	return TRUE;
    if(a == NULL || b == NULL)
	return FALSE;

    type = G_VALUE_TYPE(a);
    if(type != G_VALUE_TYPE(b))
       return FALSE;
    
    switch(G_TYPE_FUNDAMENTAL(type)) {
    case G_TYPE_NONE:   return TRUE;
    case G_TYPE_INTERFACE: /* FIXME */ return FALSE;
    case G_TYPE_CHAR:   return g_value_get_char(a)   == g_value_get_char(b);
    case G_TYPE_UCHAR:  return g_value_get_uchar(a)  == g_value_get_uchar(b);
    case G_TYPE_BOOLEAN:return g_value_get_boolean(a)== g_value_get_boolean(b);
    case G_TYPE_INT:    return g_value_get_int(a)    == g_value_get_int(b);
    case G_TYPE_UINT:   return g_value_get_uint(a)   == g_value_get_uint(b);
    case G_TYPE_LONG:   return g_value_get_long(a)   == g_value_get_long(b);
    case G_TYPE_ULONG:  return g_value_get_ulong(a)  == g_value_get_ulong(b);
    case G_TYPE_INT64:  return g_value_get_int64(a)  == g_value_get_int64(b);
    case G_TYPE_UINT64: return g_value_get_uint64(a) == g_value_get_uint64(b);
    case G_TYPE_ENUM:   return g_value_get_enum(a)   == g_value_get_enum(b);
    case G_TYPE_FLAGS:  return g_value_get_flags(a)  == g_value_get_flags(b);
    case G_TYPE_FLOAT:  return g_value_get_float(a)  == g_value_get_float(b);
    case G_TYPE_DOUBLE: return g_value_get_double(a) == g_value_get_double(b);
    case G_TYPE_STRING: return !strcmp(g_value_get_string(a),
				       g_value_get_string(b));
    case G_TYPE_POINTER:return g_value_get_pointer(a)==g_value_get_pointer(b);

    case G_TYPE_BOXED:
	pa = g_value_get_boxed(a);
	pb = g_value_get_boxed(b);
	
	if(pa == pb)
	    return TRUE;
	if(pa == NULL || pb == NULL)
	    return FALSE;

	if(type == GDK_TYPE_COLOR)
	    return gdk_color_equal((const GdkColor *) pa,
				   (const GdkColor *) pb);

	if(type == GTK_TYPE_TEXT_ITER)
	    return gtk_text_iter_equal((const GtkTextIter *) pa,
				       (const GtkTextIter *) pb);

	if(type == PANGO_TYPE_FONT_DESCRIPTION)
	    return pango_font_description_equal(
		(const PangoFontDescription *) pa,
		(const PangoFontDescription *) pb);
#if 0
	if(type == PANGO_TYPE_ATTR_LIST) {
	    /* FIXME */
	    /* pango_attribute_equal( */
	}
	if(type == PANGO_TYPE_TAB_ARRAY) {
	    PangoTabArray *taba = (PangoTabArray *) pa;
	    PangoTabArray *tabb = (PangoTabArray *) pb;
	    int size, i;

	    size = pango_tab_array_get_size(taba);
	    if(size != pango_tab_array_get_size(tabb))
		return FALSE;
	    if(pango_tab_array_get_positions_in_pixels(taba) !=
	       pango_tab_array_get_positions_in_pixels(tabb))
		return FALSE;

	    for(i = 0; i < size; i++) {
		PangoTabAlign alignment_a, alignment_b;
		gint location_a, location_b;
		pango_tab_array_get_tab(taba, i, &alignment_a, &location_a);
		pango_tab_array_get_tab(tabb, i, &alignment_b, &location_b);
		if(alignment_a != alignment_b || location_a != location_b)
		    return FALSE;
	    }
	    return TRUE;
	}
#endif
	break;
	
    case G_TYPE_PARAM:   return g_value_get_param(a) == g_value_get_param(b);
    case G_TYPE_OBJECT:  return g_value_get_object(a) == g_value_get_object(b);
    
    }
    return FALSE;
}


void g_bindable_notify_maybe_changed(GBindable *bindable)
{
    GValue new_value;
    gboolean changed = FALSE;
    
    if(!bindable->key.fun)
	return;
    
    memset(&new_value, 0, sizeof(new_value));
    g_value_init(&new_value, bindable->key.type);
    bindable->key.fun(&new_value, FALSE, bindable->key.data);
    changed = !g_bindable_value_equal(&new_value, &bindable->value);

    g_value_unset(&new_value);

    if(changed)
	g_bindable_notify_changed(bindable);
}




/* Various wrapper functions */

static void g_bindable_func_boolean(GValue *value, gboolean write,
				    gpointer values[3])
{
    gboolean (*fun)(gboolean,gboolean,gpointer[]) = values[2];
    gboolean v = g_value_get_boolean(value);
    v = fun(v, write, values);
    g_value_set_boolean(value, v);
}

GBindable *g_bindable_new_from_func_boolean(gboolean (*fun)(gboolean,gboolean,
							    gpointer[]),
					    gpointer data1, gpointer data2)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_BOOLEAN,
					   g_bindable_func_boolean,
					   data1, data2, fun);
}

static void g_bindable_func_integer(GValue *value, gboolean write,
				    gpointer values[3])
{
    gint (*fun)(gint,gboolean,gpointer[]) = values[2];
    gint v = g_value_get_int(value);
    v = fun(v, write, values);
    g_value_set_int(value, v);
}

GBindable *g_bindable_new_from_func_integer(gint (*fun)(gint,gboolean,
							gpointer[]),
					    gpointer data1, gpointer data2)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_INT,
					   g_bindable_func_integer,
					   data1, data2, fun);
}

static void g_bindable_func_double(GValue *value, gboolean write,
				   gpointer values[3])
{
    double (*fun)(double,gboolean,gpointer[]) = values[2];
    double v = g_value_get_double(value);
    v = fun(v, write, values);
    g_value_set_double(value, v);
}

GBindable *g_bindable_new_from_func_double(double (*fun)(double,gboolean,
							 gpointer[]),
					   gpointer data1, gpointer data2)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_DOUBLE,
					   g_bindable_func_double,
					   data1, data2, fun);
}

static void g_bindable_func_string(GValue *value, gboolean write,
				   gpointer values[3])
{
    gchar * (*fun)(const gchar *,gboolean,gpointer[]) = values[2];
    const gchar *ov = g_value_get_string(value);
    gchar *nv;
    nv = fun(ov, write, values);
    if(nv != ov)
	g_value_set_string_take_ownership(value, nv);
}

GBindable *g_bindable_new_from_func_string(gchar * (*fun)(const gchar *,
							  gboolean,
							  gpointer[]),
					   gpointer data1, gpointer data2)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_STRING,
					   g_bindable_func_string,
					   data1, data2, fun);
}

static void g_bindable_boolean(GValue *value, gboolean write,
			       gpointer values[3])
{
    gboolean *v = values[0];
    if(write)
	*v = g_value_get_boolean(value);
    else
	g_value_set_boolean(value, *v);
}

GBindable *g_bindable_new_from_boolean(gboolean *value)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_BOOLEAN,
					   g_bindable_boolean,
					   value, NULL, NULL);
}

static void g_bindable_integer(GValue *value, gboolean write,
			       gpointer values[3])
{
    gint *v = values[0];
    if(write)
	*v = g_value_get_int(value);
    else
	g_value_set_int(value, *v);
}

GBindable *g_bindable_new_from_integer(gint *value)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_INT,
					   g_bindable_integer,
					   value, NULL, NULL);
}

static void g_bindable_double(GValue *value, gboolean write,
			      gpointer values[3])
{
    double *v = values[0];
    if(write)
	*v = g_value_get_double(value);
    else
	g_value_set_double(value, *v);
}

GBindable *g_bindable_new_from_double(double *value)
{
    return g_bindable_new_from_func_gvalue(G_TYPE_DOUBLE,
					   g_bindable_double,
					   value, NULL, NULL);
}



/* Casts, modifications, etc. */
GBindable *g_bindable_new_model_cast(GType type, GBindable *child)
{
    GBindable *b;
    if(!child)
	return NULL;

    b = g_bindable_new_from_func_gvalue(type, NULL,
					child, GINT_TO_POINTER(type), NULL);
    g_bindable_bind(child, b, NULL);
    return b;
}
GBindable *g_bindable_new_view_cast(GType type, GBindable *child)
{
    GBindable *b;
    if(!child)
	return NULL;
    
    b = g_bindable_new_from_func_gvalue(type, NULL,
					child, GINT_TO_POINTER(type), NULL);
    g_bindable_bind(b, child, NULL);
    return b;
}


static void g_bindable_transform_forwards(GValue *value, gboolean write,
				 gpointer data[3])
{
    gboolean (*transformer)(GValue *value, GValue *tvalue,
			    gboolean forwards, gpointer data) = data[0];
    GBindable *other = data[3];
    
    if(write && transformer(value, &other->value, TRUE, data[2])) {
	g_bindable_notify_changed(other);
    }
}
static void g_bindable_transform_backwards(GValue *value, gboolean write,
				 gpointer data[3])
{
    gboolean (*transformer)(GValue *value, GValue *tvalue,
			    gboolean forwards, gpointer data) = data[0];
    GBindable *other = data[3];
    
    if(write && transformer(&other->value, value, FALSE, data[2])) {
	g_bindable_notify_changed(other);
    }
}
GBindable *g_bindable_new_view_transform(GBindable *child, GType newtype,
			 gboolean (*transformer)(GValue *value, GValue *tvalue,
						 gboolean forwards,
						 gpointer data),
					 gpointer data)
{
    GBindable *v, *t;
    if(!child)
	return NULL;

    v = g_bindable_new_from_func_gvalue(child->key.type,
					g_bindable_transform_forwards,
					transformer, child, data);
    t = g_bindable_new_from_func_gvalue(newtype,
					g_bindable_transform_backwards,
					transformer, child, data);
    v->key.data[3] = t;
    t->key.data[3] = v;
    
    g_bindable_bind(v, child, NULL);
    return t;
}
GBindable *g_bindable_new_model_transform(GBindable *child, GType newtype,
			 gboolean (*transformer)(GValue *value, GValue *tvalue,
						 gboolean forwards,
						 gpointer data),
					  gpointer data)
{
    GBindable *v, *t;
    if(!child)
	return NULL;

    v = g_bindable_new_from_func_gvalue(child->key.type,
					g_bindable_transform_forwards,
					transformer, child, data);
    t = g_bindable_new_from_func_gvalue(newtype,
					g_bindable_transform_backwards,
					transformer, child, data);
    v->key.data[3] = t;
    t->key.data[3] = v;
    
    g_bindable_bind(child, v, NULL);
    return t;
}


static gboolean g_bindable_transformer_not(GValue *value, GValue *tvalue,
					   gboolean forwards,
					   gpointer unused_data)
{
    gboolean bvalue = g_value_get_boolean(value);
    gboolean btvalue = g_value_get_boolean(tvalue);
    
    if(bvalue != btvalue)
	return FALSE;

    g_value_set_boolean(forwards ? tvalue : value,
			!bvalue);
    return TRUE;
}
GBindable *g_bindable_new_view_not(GBindable *child)
{
    return g_bindable_new_view_transform(child, G_TYPE_BOOLEAN,
					 g_bindable_transformer_not,
					 NULL);
}


static gboolean g_bindable_transformer_bool_int(GValue *value, GValue *tvalue,
						gboolean forwards,
						gpointer data)
{
    gint bvalue = g_value_get_boolean(value);
    gint itvalue = g_value_get_int(tvalue);

    gint intval = GPOINTER_TO_INT(data);
    gboolean boolval = itvalue == intval;

    if(bvalue == boolval)
	return FALSE;
    
    if(forwards) {
	if(bvalue)
	    g_value_set_int(tvalue, intval);
	else
	    return FALSE;
    } else {
	g_value_set_boolean(value, boolval);
    }

    return TRUE;
}
GBindable *g_bindable_new_view_bool_int(GBindable *child, gint val)
{
    return g_bindable_new_view_transform(child, G_TYPE_INT,
					 g_bindable_transformer_bool_int,
					 GINT_TO_POINTER(val));
}

static gboolean g_bindable_transformer_add_const(GValue *value, GValue *tvalue,
						 gboolean forwards,
						 gpointer data)
{
    gint ivalue = g_value_get_int(value);
    gint itvalue = g_value_get_int(tvalue);
    gint constant = GPOINTER_TO_INT(data);
    
    if(ivalue + constant == itvalue)
	return FALSE;
    
    if(forwards)
	g_value_set_int(tvalue, ivalue + constant);
    else
	g_value_set_int(value, itvalue - constant);
    
    return TRUE;
}
GBindable *g_bindable_new_view_add_const(GBindable *child, gint addval)
{
    return g_bindable_new_view_transform(child, G_TYPE_INT,
					 g_bindable_transformer_add_const,
					 GINT_TO_POINTER(addval));
}


gboolean g_bindable_get_writable(GBindable *gbindable)
{
    return gbindable->writable;
}

void g_bindable_set_writable(GBindable *gbindable, gboolean writable)
{
    gbindable->writable = writable;
}


static void g_bindable_collect_weaknotify(gpointer data, GObject *object)
{
    g_hash_table_remove((GHashTable *) data, object);
}


GBindable *g_bindable_collect(GBindable *gbindable, GHashTable *collection)
{
    g_hash_table_insert(collection, gbindable, gbindable);
    g_object_weak_ref(G_OBJECT(gbindable), g_bindable_collect_weaknotify,
		      collection);
    return gbindable;
}

static void g_bindable_collect_help(gpointer bindable,
				    gpointer unused_value,
				    gpointer unused_data)
{
    g_bindable_notify_maybe_changed(bindable);
}

void g_bindable_collect_poll(GHashTable *collection)
{
    g_hash_table_foreach(collection, g_bindable_collect_help, NULL);
}




/**************************************************************/
/*                       GObject                              */
/**************************************************************/

static void g_bindable_gobject(GValue *value, gboolean write,
			       gpointer values[4])
{
    GObject *object = values[0];
    const gchar *prop = values[1];

    if(write)
	g_object_set_property(object, prop, value);
    else 
	g_object_get_property(object, prop, value);
}


static void g_bindable_gobject_notify(GObject *unused_object,
				      GParamSpec *unused_pspec,
				      gpointer user_data)
{
    GBindable *bindable = user_data;

    /* The g_param_values_cmp function is fairly useless ... */
#if 0
    GValue new_value;
    int comparison;
    /* To prevent infinite loops, check to see if the value really has
       changed */
    memset(&new_value, 0, sizeof(new_value));
    g_value_init(&new_value, bindable->key.type);
    g_object_get_property(obj, bindable->key.data[1], &new_value);
    comparison = g_param_values_cmp(pspec, &new_value, &bindable->value);
    g_value_unset(&new_value);
    
    if(comparison != 0)
#endif
	g_bindable_notify_maybe_changed(bindable);
}



static void g_bindable_gobject_weaknotify_w(gpointer object,
					    GObject *bindable_object);

/* Called when the object is destroyed.
   No need for the bindable to exist any more.
   Remove notification of the bindable being destroyed, then
   unref the bindable.  */
static void g_bindable_gobject_weaknotify(gpointer data,
					  GObject *object)
{
    GObject *bindable = G_OBJECT(data);
    g_object_weak_unref(bindable, g_bindable_gobject_weaknotify_w,
			object);
    g_object_unref(bindable);
}

/* Called when the bindable is destroyed, but the object still exists.
   Now we don't care if the object is destroyed, so remove our weak ref */
static void g_bindable_gobject_weaknotify_w(gpointer object,
					    GObject *bindable_object)
{
    g_object_weak_unref(G_OBJECT(object), g_bindable_gobject_weaknotify,
			bindable_object);
}

GBindable *g_bindable_new_from_gobject(GObject *object, const gchar *prop)
{
    GParamSpec *pspec;
    gchar *notify_signal;
    GBindable *bindable;
    
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(object),
					 prop);

    bindable = g_bindable_new_from_func_gvalue(G_PARAM_SPEC_VALUE_TYPE(pspec),
					       g_bindable_gobject,
					       object, pspec->name, NULL);

    g_bindable_set_writable(bindable, (pspec->flags & G_PARAM_WRITABLE));

    if(bindable->init_level < 1) {
	notify_signal = g_strdup_printf("notify::%s", prop);
	g_signal_connect(object, notify_signal,
			 G_CALLBACK(g_bindable_gobject_notify),
			 bindable);
	g_free(notify_signal);

	g_object_weak_ref(object, g_bindable_gobject_weaknotify,
			  bindable);
	g_object_weak_ref(G_OBJECT(bindable), g_bindable_gobject_weaknotify_w,
			  object);
	
	bindable->init_level = 1;
    }

    return bindable;
}


static gboolean g_bindable_bad_gtkobject_toggle(gboolean value, gboolean write,
						gpointer data[2])
{
    GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON(data[0]);

    if(write)
	gtk_toggle_button_set_active(togglebutton, value);
    return gtk_toggle_button_get_active(togglebutton);
}
static void g_bindable_bad_gtkobject_toggle_notify(GtkToggleButton *toggle,
						   gpointer user_data)
{
    GBindable *gbindable = user_data;
    if(gtk_toggle_button_get_active(toggle) !=
       g_value_get_boolean(&gbindable->value))
	g_bindable_notify_changed(gbindable);
}

static gint g_bindable_bad_gtkobject_option_menu(gint value, gboolean write,
						 gpointer data[2])
{
    GtkOptionMenu *optionmenu = GTK_OPTION_MENU(data[0]);
    if(write)
	gtk_option_menu_set_history(optionmenu, value);
    return gtk_option_menu_get_history(optionmenu);
}
static void g_bindable_bad_gtkobject_option_menu_notify(GtkOptionMenu *option_menu,
							gpointer user_data)
{
    GBindable *gbindable = user_data;
    if(gtk_option_menu_get_history(option_menu) !=
       g_value_get_int(&gbindable->value))
	g_bindable_notify_changed(gbindable);
}

static GBindable *g_bindable_new_from_bad_gtkobject(GObject *object)
{
    GBindable *bindable = NULL;
    GType type = G_OBJECT_TYPE(object);
    const char *signal_name;
    GCallback handler;
    
    if(g_type_is_a(type, GTK_TYPE_TOGGLE_BUTTON)) {
	bindable = g_bindable_new_from_func_boolean(
	    g_bindable_bad_gtkobject_toggle, object, NULL);
	signal_name = "toggled";
	handler = G_CALLBACK(g_bindable_bad_gtkobject_toggle_notify);
    } else if(g_type_is_a(type, GTK_TYPE_OPTION_MENU)) {
	bindable = g_bindable_new_from_func_integer(
	    g_bindable_bad_gtkobject_option_menu, object, NULL);
	signal_name = "changed";
	handler = G_CALLBACK(g_bindable_bad_gtkobject_option_menu_notify);
    }

    if(bindable && bindable->init_level < 1) {
	g_signal_connect(object, signal_name, handler, bindable);
	
	g_object_weak_ref(object, g_bindable_gobject_weaknotify,
			  bindable);
	g_object_weak_ref(G_OBJECT(bindable), g_bindable_gobject_weaknotify_w,
			  object);
	bindable->init_level = 1;
    }

    return bindable;
}

static const gchar *g_bindable_widget_to_propname(GtkObject *widget) G_GNUC_PURE;

static const gchar *g_bindable_widget_to_propname(GtkObject *widget)
{
    GType type = G_OBJECT_TYPE(G_OBJECT(widget));
    const GType typelist[] = {
	GTK_TYPE_LABEL,                 GTK_TYPE_IMAGE,
	GTK_TYPE_PROGRESS_BAR,          GTK_TYPE_TOGGLE_BUTTON,
	GTK_TYPE_WINDOW,                GTK_TYPE_SPIN_BUTTON,
	GTK_TYPE_ENTRY,                 GTK_TYPE_RULER,
	GTK_TYPE_RANGE,                 GTK_TYPE_ADJUSTMENT,
	GTK_TYPE_OPTION_MENU,           GTK_TYPE_CELL_RENDERER_PIXBUF,
	GTK_TYPE_CELL_RENDERER_TEXT,    GTK_TYPE_CELL_RENDERER_TOGGLE,
	GTK_TYPE_TREE_VIEW_COLUMN
    };
    static const char proplist[] = {
	"label" "\0"                    "pixbuf" "\0"
	"fraction" "\0"                 "active" "\0"
	"title" "\0"                    "value" "\0"
	"text" "\0"                     "position" "\0"
	"value" "\0"                    "value" "\0"
	"history" "\0"                  "pixbuf" "\0"
	"text" "\0"                     "active" "\0"
	"title" "\0"
    };
    const char *p;
    int i = 0;
    for(p = proplist; *p; p += strlen(p) + 1) {
	if(g_type_is_a(type, typelist[i]))
	    return p;
	i++;
    }

    return NULL;
}


GBindable *g_bindable_new_from_gtkobject_prop(GtkObject *object,
					      const char *prop)
{
    GBindable *gbindable;

    if((GTK_IS_RADIO_BUTTON(object) && !strcmp(prop, "active")) ||
       (GTK_IS_OPTION_MENU(object) && !strcmp(prop, "history"))) {
	gbindable = g_bindable_new_from_bad_gtkobject(G_OBJECT(object));
    } else {
	gbindable = g_bindable_new_from_gobject(G_OBJECT(object), prop);
    }
    
    /* FIXME: add "sensitive" support */

    return gbindable;
}


GBindable *g_bindable_new_from_gtkobject(GtkObject *object)
{
    const char *prop = g_bindable_widget_to_propname(object);
    return g_bindable_new_from_gtkobject_prop(object, prop);
}

/**************************************************************/
/*                       GtkTreeModel                         */
/**************************************************************/

static void g_bindable_gtktreemodel(GValue *value, gboolean write,
				    gpointer data[4])
{
    GtkTreeModel *model = data[0];
    GtkTreePath *path = data[1];
    gint column = GPOINTER_TO_INT(data[2]);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    if(write) {
	if(GTK_IS_LIST_STORE(model))
	    gtk_list_store_set_value(GTK_LIST_STORE(model), &iter,
				     column, value);
	else if(GTK_IS_TREE_STORE(model))
	    gtk_tree_store_set_value(GTK_TREE_STORE(model), &iter,
				     column, value);
    } else {
	g_value_unset(value);
	gtk_tree_model_get_value(model, &iter, column, value);
    }
}

static void g_bindable_gtktreemodel_notify(GtkTreeModel *unused_model,
					   GtkTreePath *unused_path,
					   GtkTreeIter *unused_iter,
					   gpointer user_data)
{
    GBindable *bindable = user_data;
    g_bindable_notify_maybe_changed(bindable);
}

GBindable *g_bindable_new_from_gtktreemodel(GtkTreeModel *model,
					    GtkTreePath *path,
					    gint column)
{
    GBindable *bindable;
    bindable = g_bindable_new_from_func_gvalue(
	gtk_tree_model_get_column_type(model, column),
	g_bindable_gtktreemodel,
	model, path, GINT_TO_POINTER(column));
    if(bindable->init_level < 1) {
	g_signal_connect(model, "row-changed",
			 G_CALLBACK(g_bindable_gtktreemodel_notify),
			 bindable);
	bindable->init_level = 1;
    }
    return bindable;
}

					    




/**************************************************************/
/*                       GConfClient                          */
/**************************************************************/

static void g_bindable_gconf(GValue *value, gboolean write,
			     gpointer data[3])
{
    GConfClient *client = data[0];
    const gchar *key = data[1];
    GConfChangeSet *changeset = data[2];
    GType type = G_VALUE_TYPE(value);
    GConfValue *confvalue = NULL;
    gboolean should_free = TRUE;
    
    if(write) {
	switch(type) {
	case G_TYPE_STRING:
	    confvalue = gconf_value_new(GCONF_VALUE_STRING);
	    gconf_value_set_string(confvalue, g_value_get_string(value));
	    break;
	case G_TYPE_INT:
	    confvalue = gconf_value_new(GCONF_VALUE_INT);
	    gconf_value_set_int(confvalue, g_value_get_int(value));
	    break;
	case G_TYPE_DOUBLE:
	    confvalue = gconf_value_new(GCONF_VALUE_FLOAT);
	    gconf_value_set_float(confvalue, g_value_get_double(value));
	    break;
	case G_TYPE_BOOLEAN:
	    confvalue = gconf_value_new(GCONF_VALUE_BOOL);
	    gconf_value_set_bool(confvalue, g_value_get_boolean(value));
	    break;
	}

	if(changeset)
	    gconf_change_set_set(changeset, key, confvalue);
	else
	    gconf_client_set(client, key, confvalue, NULL);

    } else {
	if(changeset) {
	    gconf_change_set_check_value(changeset, key, &confvalue);
	    if(confvalue)
		should_free = FALSE;  /* not a copy */
	}

	if(!confvalue)
	    confvalue = gconf_client_get(client, key, NULL);

	switch(type) {
	case G_TYPE_STRING:
	    g_value_set_string(value, gconf_value_get_string(confvalue));
	    break;
	case G_TYPE_INT:
	    g_value_set_int(value, gconf_value_get_int(confvalue));
	    break;
	case G_TYPE_DOUBLE:
	    g_value_set_double(value, gconf_value_get_float(confvalue));
	    break;
	case G_TYPE_BOOLEAN:
	    g_value_set_boolean(value, gconf_value_get_bool(confvalue));
	    break;
	}
    }

    if(confvalue && should_free)
	gconf_value_free(confvalue);
}

static void g_bindable_gconf_notify(GConfClient *unused_client,
				    guint unused_cnxn_id,
				    GConfEntry *unused_entry,
				    gpointer user_data)
{
    GBindable *gbindable = user_data;
    g_bindable_notify_changed(gbindable);
}
static void g_bindable_gconf_destroy(gpointer data)
{
    GBindable *gbindable = data;
    g_bindable_unbind_view(gbindable);
}


static GConfClient *g_bindable_gconf_client;
static gchar *g_bindable_gconf_path = NULL;

void g_bindable_gconf_set_gconf_default(GConfClient *client,
					const char *path)
{
    if(g_bindable_gconf_path) {
	gconf_client_remove_dir(g_bindable_gconf_client,
				g_bindable_gconf_path,
				NULL);
	g_free(g_bindable_gconf_path);
    }
    g_bindable_gconf_client = client;
    g_bindable_gconf_path = g_strdup(path);

    gconf_client_add_dir(g_bindable_gconf_client,
			 g_bindable_gconf_path,
			 GCONF_CLIENT_PRELOAD_NONE,
			 NULL);
}


GBindable *g_bindable_new_from_gconf_changeset(GConfChangeSet *changeset,
					       const gchar *qkey)
{
    GBindable *gbindable;
    GConfEntry *entry;
    GConfValue *value = NULL;
    const gchar *key;

    static const GType type_table[] = {
	G_TYPE_INVALID, /* GCONF_VALUE_INVALID */
	G_TYPE_STRING,  /* GCONF_VALUE_STRING */
	G_TYPE_INT,     /* GCONF_VALUE_INT */
	G_TYPE_DOUBLE,  /* GCONF_VALUE_FLOAT */
	G_TYPE_BOOLEAN, /* GCONF_VALUE_BOOL */
    };

    if(qkey[0] == '/') {
	key = g_quark_to_string(g_quark_from_string(qkey));
    } else {
	gchar *realkey = g_strconcat(g_bindable_gconf_path, "/",
				     qkey, NULL);
	key = g_quark_to_string(g_quark_from_string(realkey));
	g_free(realkey);
    }

    entry = gconf_client_get_entry(g_bindable_gconf_client,
				   key, NULL, TRUE, NULL);
    if(entry)
	value = gconf_entry_get_value(entry);
    if(!value) {
	g_warning("No value for \"%s\"", key);
	return NULL;
    }

    if(value->type > GCONF_VALUE_BOOL)
	return NULL;
    
    gbindable = g_bindable_new_from_func_gvalue(type_table[value->type],
						g_bindable_gconf,
						g_bindable_gconf_client,
						(char *) key,
						changeset);
    if(gbindable->init_level < 1) {
	g_bindable_set_writable(gbindable, gconf_entry_get_is_writable(entry));
	if(!changeset) {
	    gconf_client_notify_add(g_bindable_gconf_client, key,
				    g_bindable_gconf_notify,
				    gbindable,
				    g_bindable_gconf_destroy,
				    NULL);
	}
	gbindable->init_level = 1;
    }

    gconf_entry_unref(entry);
    
    return gbindable;
}


GBindable *g_bindable_new_from_gconf(const gchar *key)
{
    return g_bindable_new_from_gconf_changeset(NULL, key);
}
