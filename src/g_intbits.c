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

#include "g_intbits.h"
#include <stdio.h>

enum {
    PROP_0,
    PROP_VALUE,
    PROP_BIT0,
    PROP_BIT31 = PROP_BIT0 + 32
};

static void g_intbits_class_init(GIntbitsClass *klass);
static void g_intbits_init(GIntbits *intbits);
static void g_intbits_set_property(GObject *object, guint prop_id,
				    const GValue *value, GParamSpec *pspec);
static void g_intbits_get_property(GObject *object, guint prop_id,
				    GValue *value, GParamSpec *pspec);

GType g_intbits_get_type(void)
{
    static GType type = 0;
    if(!type) {
	static const GTypeInfo info = {
	    sizeof(GIntbitsClass),
	    NULL,
	    NULL,
	    (GClassInitFunc) g_intbits_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GIntbits),
	    0,    /* n_preallocs */
	    (GInstanceInitFunc) g_intbits_init,
	    NULL  /* value_table */
	};

	type = g_type_register_static(G_TYPE_OBJECT, "GIntbits",
				      &info, 0);
    }

    return type;
}


static void g_intbits_class_init(GIntbitsClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    int i;
    
    object_class->set_property = g_intbits_set_property;
    object_class->get_property = g_intbits_get_property;
    

    g_object_class_install_property(object_class,
				    PROP_VALUE,
				    g_param_spec_int("value",
						     "Value",
						     "Integer value",
						     0, G_MAXINT, 0,
						     G_PARAM_READABLE | G_PARAM_WRITABLE));

    for(i = PROP_BIT0; i <= PROP_BIT31; i++) {
	char propname[100];
	char propnick[100];
	int bitnum = i - PROP_BIT0;
	sprintf(propname, "bit%d", bitnum);
	sprintf(propnick, "Bit %d", bitnum);
	g_object_class_install_property(object_class,
					i,
					g_param_spec_boolean(propname,
							     propnick,
							     "A single bit",
							     TRUE,
							     G_PARAM_READABLE | G_PARAM_WRITABLE));
    }
}



static void g_intbits_init(GIntbits *intbits)
{
    intbits->value = 0;
}

static void g_intbits_set_value(GIntbits *intbits, gint value)
{
    if(intbits->value != value) {
	int bit;
	for(bit = 0; bit <= 31; bit++) {
	    if((intbits->value & (1 << bit)) != (value & (1 << bit))) {
		char prop[100];
		sprintf(prop, "bit%d", bit);
		g_object_notify(G_OBJECT(intbits), prop);
	    }
	}
	intbits->value = value;
	g_object_notify(G_OBJECT(intbits), "value");
    }
}

static void g_intbits_set_bit(GIntbits *intbits, gint bitnum, gboolean value)
{
    int bitval = value << bitnum;
    if((intbits->value & (1 << bitnum)) != bitval) {
	char prop[100];
	intbits->value = (intbits->value & ~(1 << bitnum)) | bitval;
	sprintf(prop, "bit%d", bitnum);
	g_object_notify(G_OBJECT(intbits), prop);
	g_object_notify(G_OBJECT(intbits), "value");
    }
}



static void g_intbits_set_property(GObject *object, guint prop_id,
				    const GValue *value, GParamSpec *pspec)
{
    GIntbits *intbits = G_INTBITS(object);
    switch(prop_id) {
    case PROP_VALUE:
	g_intbits_set_value(intbits, g_value_get_int(value));
	break;
    default:
	if(prop_id >= PROP_BIT0 && prop_id <= PROP_BIT31) {
	    g_intbits_set_bit(intbits, prop_id - PROP_BIT0,
			      g_value_get_boolean(value));
	    break;
	}
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void g_intbits_get_property(GObject *object, guint prop_id,
				    GValue *value, GParamSpec *pspec)
{
    GIntbits *intbits = G_INTBITS(object);
    switch(prop_id) {
    case PROP_VALUE:
	g_value_set_int(value, intbits->value);
	break;
    default:
	if(prop_id >= PROP_BIT0 && prop_id <= PROP_BIT31) {
	    int bitnum = prop_id - PROP_BIT0;
	    g_value_set_boolean(value, (intbits->value & (1 << bitnum)) != 0);
	    break;
	}
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GIntbits *g_intbits_new(void)
{
    return G_INTBITS(g_object_new(G_TYPE_INTBITS, NULL));
}
