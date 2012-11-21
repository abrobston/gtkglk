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
#include "g_bindable.h"
#include "support.h"
#include <stdio.h>
#include <string.h>

#define GLK_TYPE_JUSTIFICATION_TYPE (glk_justification_type_get_type())

GType glk_justification_type_get_type(void)
{
    static GType etype = 0;
    if(etype == 0) {
	static const GEnumValue values[] = {
	    { -1,                       "",       "default",                 },
	    { stylehint_just_LeftFlush, "Left",   "stylehint_just_LeftFlush" },
	    { stylehint_just_LeftRight, "Fill",   "stylehint_just_LeftRight" },
	    { stylehint_just_Centered,  "Center", "stylehint_just_Centered"  },
	    { stylehint_just_RightFlush,"Right",  "stylehint_just_RightFlush"},
	    { 0, NULL, NULL }
	};
	etype = g_enum_register_static ("glk_justification_type", values);
    }
    return etype;
}

static void value_transform_string_gdkcolor(const GValue *src_value,
					    GValue *dest_value)
{
    GdkColor color;
    GdkColor *val;
    
    if(gdk_color_parse(g_value_get_string(src_value), &color))
	val = &color;
    else
	val = NULL;

    g_value_set_boxed(dest_value, val);
}

static void value_transform_gdkcolor_string(const GValue *src_value,
					    GValue *dest_value)
{
    GdkColor *color = g_value_get_boxed(src_value);
    char buf[10];

    if(color)
	sprintf(buf, "#%02X%02X%02X",
		color->red >> 8,
		color->green >> 8,
		color->blue >> 8);
    else
	*buf = 0;

    g_value_set_string(dest_value, buf);
}

static void value_transform_int_gdkcolor(const GValue *src_value,
					 GValue *dest_value)
{
    int i = g_value_get_int(src_value);
    GdkColor color;
    GdkColor *val;
    

    if(i == -1) {
	val = NULL;
    } else {
	color.red   = ((i & 0xff0000) >> 16) * 257;
	color.green = ((i & 0xff00) >> 8) * 257;
	color.blue  = (i & 0xff) * 257;
	val = &color;
    }

    g_value_set_boxed(dest_value, val);
}

static void value_transform_gdkcolor_int(const GValue *src_value,
					 GValue *dest_value)
{
    GdkColor *color = g_value_get_boxed(src_value);
    gint val;
    
    if(color)
	val = ((color->red >> 8) << 16) |
	    ((color->green >> 8) << 8) |
	    (color->blue >> 8);
    else
	val = -1;

    g_value_set_int(dest_value, val);
}

static void value_transform_string_enum(const GValue *src_value,
					GValue       *dest_value)
{
    GEnumClass *class = g_type_class_ref(G_VALUE_TYPE(dest_value));
    GEnumValue *enum_value;
    const char *str = g_value_get_string(src_value);

    enum_value = g_enum_get_value_by_name(class, str);
    if(!enum_value)
	enum_value = g_enum_get_value_by_nick(class, str);

    g_value_set_enum(dest_value, enum_value ? enum_value->value : 0);
}



static void gglk_gestalt_string(GValue *value, gboolean write,
				gpointer *unused_data)
{
    int major, minor, micro;
    if(write) {
	const char *buf = g_value_get_string(value);
	if(buf) {
	    sscanf(buf, "%d.%d.%d", &major, &minor, &micro);
	    gglk_gestalt_version = (major << 16) | (minor << 8) | micro;
	}
    } else {
	char buf[50];
	major = (gglk_gestalt_version >> 16) & 0xffff;
	minor = (gglk_gestalt_version >> 8) & 0xff;
	micro = gglk_gestalt_version & 0xff;
	sprintf(buf, "%d.%d.%d", major, minor, micro);
	g_value_set_string(value, buf);
    }
}


static inline void gglk_gestalt_bind_gconf(void)
{
    glui32 i;
    for(i = gestalt_MouseInput; i <= gestalt_Unicode; i++) {
	const char *name = gglk_gestalt_get_name(i);
	char key[100];
	sprintf(key, "gestalt/%s_Enable", name);
	g_bindable_bind(
	    g_bindable_new_from_gconf(key),
	    g_bindable_new_from_boolean(&gglk_gestalt_support[i]),
	    NULL);
	sprintf(key, "gestalt/%s_Claim", name);
	g_bindable_bind(
	    g_bindable_new_from_gconf(key),
	    g_bindable_new_from_boolean(&gglk_gestalt_claim[i]),
	    NULL);
    }

    g_bindable_bind(
	g_bindable_new_from_gconf("gestalt/Version"),
	g_bindable_new_from_func_gvalue(
	    G_TYPE_STRING, gglk_gestalt_string,
	    NULL, NULL, NULL),
	NULL);
}

static void gglk_trigger_styles(GValue *unused_value, gboolean write,
				gpointer data[3])
{

    if(write) {
	int style = GPOINTER_TO_INT(data[0]);
	int hint  = GPOINTER_TO_INT(data[1]);
	gglk_stylehints_update(style, hint);
    }
}


void gglk_styles_bind_gconf(GConfChangeSet *changeset,
			    struct glk_stylehint_struct *base,
			    struct glk_stylehint_important_struct *base_important,
			    void (*trigger)(GValue *, gboolean, gpointer *))
{
    glui32 style, hint;
    for(style = style_Normal; style <= style_AllStyles; style++)
	for(hint = 0; hint <= stylehint_Underline; hint++) {
	    char key[200];
	    GBindable *bindable;
	    sprintf(key, "styles/%s/%s_Important",
		    style==style_AllStyles ? "AllStyles" : gglk_get_tag(style),
		    gglk_get_hint(hint));
	    g_bindable_bind(g_bindable_new_from_gconf_changeset(changeset,
								key),
			    g_bindable_new_from_func_gvalue(
				G_TYPE_INT, trigger,
				GINT_TO_POINTER(style),
				GINT_TO_POINTER(hint), "important"),

			    g_bindable_new_from_boolean(
				&base_important->important[style][hint]),

			    NULL);

	    sprintf(key, "styles/%s/%s",
		    style==style_AllStyles ? "AllStyles" : gglk_get_tag(style),
		    gglk_get_hint(hint));

	    bindable = g_bindable_new_from_gconf_changeset(changeset, key);
	    
	    switch(hint) {
	    case stylehint_Proportional:
		bindable=g_bindable_new_model_transform(bindable,
							G_TYPE_INT,
							gglk_font_transform,
							NULL);
	    case stylehint_Justification:
		bindable=g_bindable_new_model_cast(GLK_TYPE_JUSTIFICATION_TYPE,
						  bindable);
		break;
	    case stylehint_TextColor:
	    case stylehint_BackColor:
		bindable=g_bindable_new_model_cast(GDK_TYPE_COLOR, bindable);
		break;
	    }
	    
	    g_bindable_bind(bindable,
			    g_bindable_new_from_func_gvalue(
				G_TYPE_INT, trigger,
				GINT_TO_POINTER(style),
				GINT_TO_POINTER(hint), NULL),
			    g_bindable_new_from_integer(
				&base->hints[style][hint]),
			    NULL);
	}
}


static void gglk_trigger_arrange(GValue *unused_value, gboolean write,
				 gpointer *unused_data) 
{
    if(write) {
	winid_t root = glk_window_get_root();
	if(root)
	    gglk_trigger_event(evtype_Arrange, root, 0, 0);
    }
}

static gboolean gglk_usescrollbars(gboolean value, gboolean write,
				   gpointer *unused_data)
{
    if(write) {
	gglk_win_set_scroll_policy(value ? GTK_POLICY_AUTOMATIC
				         : GTK_POLICY_NEVER);
    }
    return value;
}


void gglk_prefs_init(void)
{
    g_value_register_transform_func(G_TYPE_STRING, GDK_TYPE_COLOR,
				    value_transform_string_gdkcolor);
    g_value_register_transform_func(GDK_TYPE_COLOR, G_TYPE_STRING,
				    value_transform_gdkcolor_string);
    g_value_register_transform_func(G_TYPE_INT, GDK_TYPE_COLOR,
				    value_transform_int_gdkcolor);
    g_value_register_transform_func(GDK_TYPE_COLOR, G_TYPE_INT,
				    value_transform_gdkcolor_int);

    g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ENUM,
				    value_transform_string_enum);

    g_bindable_gconf_set_gconf_default(gconf_client_get_default(),
				       "/apps/gtkglk");

    gglk_gestalt_bind_gconf();
    gglk_styles_bind_gconf(NULL,
			   &gglk_individual_styles, &gglk_important_styles,
			   gglk_trigger_styles);

    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-statusbar"),
	g_bindable_new_from_gobject(G_OBJECT(lookup_widget(gglk_win,
							   "statusbar")),
				    "visible"),
	g_bindable_new_from_func_gvalue(G_TYPE_BOOLEAN, gglk_trigger_arrange,
					"statusbar", NULL, NULL),
	NULL);
    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-menu"),
	g_bindable_new_from_gobject(G_OBJECT(lookup_widget(gglk_win,
							   "menubar")),
				    "visible"),
	g_bindable_new_from_func_gvalue(G_TYPE_BOOLEAN, gglk_trigger_arrange,
					"menu", NULL, NULL),
	NULL);
    g_bindable_bind(
	g_bindable_new_from_gconf("interface/show-scrollbars"),
	g_bindable_new_from_func_boolean(gglk_usescrollbars, NULL, NULL),
	NULL);
}
