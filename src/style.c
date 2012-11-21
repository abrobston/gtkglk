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



#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gglk.h"
#include <gdk/gdk.h>
#include <glib.h>
#include <math.h>

/* Conveniently, the Gtk text buffer system uses program-defined
   tags for styles, so we can map Glk styles to a tag table.

   Unconveniently, it looks like we will have to make a tag table
   for each buffer as we don't want changes to the tags in one
   buffer to affect another. */



/* This exists to query the default color of a textview */
static GtkWidget *gglk_style_textview;

static glsi32 gglk_gdkcolor_to_glk(const GdkColor *color)
{
    glsi32 result;
    
    result = ((color->red >> 8) << 16) |
	((color->green >> 8) << 8) |
	(color->blue >> 8);

    return result;
}

static void set_text_color(GtkTextTag *tag, gboolean fore, glsi32 val)
{
    const char *prop    = fore ? "foreground" : "background";
    const char *propset = fore ? "foreground-set" : "background-set";
    
    if(val == -1) {
	g_object_set(G_OBJECT(tag), propset, FALSE, NULL);
    } else {
	char color[20];
	sprintf(color, "#%02X%02X%02X",
		((val & 0xff0000) >> 16),
		((val & 0x00ff00) >> 8),
		(val & 0x0000ff));
	g_object_set(G_OBJECT(tag), prop, color, NULL);
    }
}

static inline void gglk_set_colors(GtkTextTag *tag, glsi32 fore, glsi32 back,
				   glsi32 reverse)
{
    if(reverse) {
	glsi32 t;
	
	if(fore == -1)
	    fore = gglk_gdkcolor_to_glk(
		&gglk_style_textview->style->text[GTK_STATE_NORMAL]);
	
	if(back == -1)
	    back = gglk_gdkcolor_to_glk(
		&gglk_style_textview->style->base[GTK_STATE_NORMAL]);

	t = fore;
	fore = back;
	back = t;
    }

    set_text_color(tag, TRUE, fore);
    set_text_color(tag, FALSE, back);
}




static PangoFontFamily **family_list;
static int num_families;



/* Applies the glk stylehint to the tag */
void gglk_adjust_tag(GtkTextTag *tag, glui32 hint, glsi32 val)
{
    double x;
    int i = 0;
    const char *s = NULL;
    GObject *tag_object = G_OBJECT(tag);
    
    switch(hint) {
    case stylehint_Indentation:
	g_object_set(tag_object, "left_margin", 5 + val, NULL);
	g_object_set(tag_object, "right_margin", 5 + val, NULL);
	break;
    case stylehint_ParaIndentation:
	g_object_set(tag_object, "indent", val, NULL);
	break;
    case stylehint_Justification:
	switch(val) {
	case stylehint_just_LeftFlush: i = GTK_JUSTIFY_LEFT; break;
	case stylehint_just_LeftRight: i = GTK_JUSTIFY_FILL; break;
	case stylehint_just_Centered:  i = GTK_JUSTIFY_CENTER; break;
	case stylehint_just_RightFlush:i = GTK_JUSTIFY_RIGHT; break;
	case -1: i = -1; break;
	default: g_warning("Unknown justification %" G_GINT32_FORMAT ".", val);
	    i = GTK_JUSTIFY_LEFT;
	}
	if(i == -1)
	    g_object_set(tag_object, "justification-set", FALSE, NULL);
	else
	    g_object_set(tag_object, "justification", i, NULL);
	break;
    case stylehint_Size:
	val = CLAMP(val, -4, 4);
	x = pow(1.2, val / 2.0);
	g_object_set(tag_object, "scale", x, NULL);
	break;

    case stylehint_Weight:
	switch(CLAMP(val, -2, 2)) {
	case -2: i = PANGO_WEIGHT_ULTRALIGHT; break;
	case -1: i = PANGO_WEIGHT_LIGHT; break;
	case  0: i = PANGO_WEIGHT_NORMAL; break;
	case  1: i = PANGO_WEIGHT_BOLD; break;
	case  2: i = PANGO_WEIGHT_ULTRABOLD; break;
	}
	g_object_set(tag_object, "weight", i, NULL);
	break;
    case stylehint_Oblique:
	g_object_set(tag_object, "style",
		     val ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL, NULL);
	break;
    case stylehint_Proportional:
	if(val >= 0 && val < num_families)
	    g_object_set(tag_object, "family",
			 pango_font_family_get_name(family_list[val]), NULL);
	else
	    g_object_set(tag_object, "family-set", FALSE, NULL);
	break;
    case stylehint_TextColor:
    case stylehint_BackColor:
    case stylehint_ReverseColor:

	switch(hint) {
	case stylehint_TextColor:    s = "TextColor"; break;
	case stylehint_BackColor:    s = "BackColor"; break;
	case stylehint_ReverseColor: s = "ReverseColor"; break;
	}

	g_object_set_data(tag_object, s, GINT_TO_POINTER(val));

	gglk_set_colors(tag,
			GPOINTER_TO_INT(g_object_get_data(tag_object,
							  "TextColor")),
			GPOINTER_TO_INT(g_object_get_data(tag_object,
							  "BackColor")),
			GPOINTER_TO_INT(g_object_get_data(tag_object,
							  "ReverseColor")));
	break;

    case stylehint_Direction:
	g_object_set(tag_object,
		     "direction", val ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR,
		     NULL);
	break;
    case stylehint_Underline:
	switch(val) {
	case 0: i = PANGO_UNDERLINE_NONE; break;
	case 1: i = PANGO_UNDERLINE_SINGLE; break;
	case 2: i = PANGO_UNDERLINE_DOUBLE; break;
	}
	g_object_set(tag_object, "underline", i, NULL);
	break;
	
    case stylehint_Variant:
	g_object_set(tag_object, "variant",
		     val ? PANGO_VARIANT_SMALL_CAPS : PANGO_VARIANT_NORMAL,
		     NULL);
	break;

    default:
	break;
    }
}




/* Array of stylehints to be applied by default */

struct glk_stylehint_struct gglk_individual_styles = {
    NULL,
    {
/* Normal */
	{ 0, 0, -1 /* default just */,    0, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* Emphasized */
	{ 0, 0, -1 /* default just */,    0, 0, 1, -1, -1, -1, 0, 0, 0, 0 },
/* Preformatted */
	{ 0, 0, -1 /* default just */,    0, 0, 0,  0, -1, -1, 0, 0, 0, 0 },
/* Header */
	{ 0, 0, -1 /* default just */,    2, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* Subheader */
	{ 0, 0, -1 /* default just */,    0, 1, 0, -1, -1, -1, 0, 0, 0, 0 },
/* Alert */
	{ 0, 0, -1 /* default just */,    0, 0, 0, -1, -1, -1, 0, 0, 1, 0 },
/* Note */
	{ 0, 0, -1 /* default just */,   -1, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* BlockQuote */
	{50, 0, -1 /* default just */,    0, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* Input */
	{ 0, 0, -1 /* default just */,    0, 1, 0,  0, -1, -1, 0, 0, 0, 0 },
/* User1 */
	{ 0, 0, -1 /* default just */,    0, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* User2 */
	{ 0, 0, -1 /* default just */,    0, 0, 0, -1, -1, -1, 0, 0, 0, 0 },
/* TextGrid */
	{ 0, 0, -1 /* default just */,    0, 0, 0,  0, -1, -1, 0, 0, 0, 0 },
/* Hyperlink */
	{ 0, 0, -1 /* default just */,    0, 0, 0, -1,0xff,-1, 0, 0, 1, 0 },
/* All Styles */
	{ 0, 0, stylehint_just_LeftFlush, 0, 0, 0,  1, -1, -1, 0, 0, 0, 0 }
    },
    wintype_AllTypes,
    NULL,
    1
};

struct glk_stylehint_important_struct gglk_important_styles;


static glsi32 gglk_stylehint_combine(glui32 hint, glsi32 val1, glsi32 val2)
{
    switch(hint) {
    case stylehint_Indentation:
    case stylehint_ParaIndentation:
    case stylehint_Size:
	return val1 + val2;
	break;

    case stylehint_Justification:
    case stylehint_Proportional:
    case stylehint_TextColor:
    case stylehint_BackColor:
	if(val2 == -1)
	    return val1;
	return val2;

    case stylehint_Weight:
    default:
	return val1 ^ val2;
	break;
    }
}

static const char *gglk_default_font;

static int gglk_family_list_search_by_name(const void *a, const void *b)
{
    PangoFontFamily **fb = (PangoFontFamily **) b;
    const char *ca = *((const char **) a);
    const char *cb = pango_font_family_get_name(*fb);

    /* stylehint_Proportional is used to pick a family for the font.  In
       order to do this properly, we need to get a list of all the
       available families and make 0 be monospace and 1 be the default
       font. */

    static const char *first[4] = { "Monospace", NULL, "Serif", "Sans" };
    int i;
    int comparison = g_ascii_strcasecmp(ca, cb);

    if(comparison == 0)
	return 0;

    first[1] = gglk_default_font;

    for(i = 0; i < 4; i++) {
	if(g_ascii_strcasecmp(ca, first[i]) == 0)
	    return -1;
	if(g_ascii_strcasecmp(cb, first[i]) == 0)
	    return 1;
    }

    return comparison;
}
static int gglk_family_list_sort_by_name(const void *a, const void *b)
{
    PangoFontFamily **fa = (PangoFontFamily **) a;
    const char *ca = pango_font_family_get_name(*fa);
    return gglk_family_list_search_by_name(&ca, b);
}


gboolean gglk_font_transform(GValue *value, GValue *tvalue,
			     gboolean forwards, gpointer unused_data)
{
    const char *family_name = g_value_get_string(value);
    int tint = g_value_get_int(tvalue);
    const char *tfamily_name = NULL;

    if(!family_list)
	return FALSE;
    if(tint >= 0 && tint < num_families)
	tfamily_name = pango_font_family_get_name(family_list[tint]);

    if(family_name && tfamily_name && !strcmp(family_name, tfamily_name))
	return FALSE;

    if(forwards) {
	PangoFontFamily **result = NULL;
	if(family_name && *family_name) {
	    result = bsearch(&family_name, family_list, num_families,
			     sizeof(*family_list),
			     gglk_family_list_search_by_name);
	}
	g_value_set_int(tvalue, result ? result - family_list : -1);
    } else {
	g_value_set_string(value, tfamily_name ? tfamily_name : "");
    }
    return TRUE;
}


void gglk_init_styles(void)
{
    gglk_stylehint_init();
    
    gglk_important_styles.important[style_TextGrid][stylehint_Proportional] = TRUE;
    gglk_important_styles.important[style_TextGrid][stylehint_Size] = TRUE;
    gglk_important_styles.important[style_Hyperlinks][stylehint_Underline] = TRUE;
    gglk_important_styles.important[style_Hyperlinks][stylehint_TextColor] = TRUE;
    
    if(!family_list){
	PangoContext *context = gtk_widget_get_pango_context(gglk_win);
	PangoFontDescription *desc=pango_context_get_font_description(context);
	gglk_default_font = pango_font_description_get_family(desc);
	
	pango_context_list_families(context, &family_list, &num_families);

	qsort(family_list, num_families, sizeof(*family_list),
	      gglk_family_list_sort_by_name);
    }

    gglk_style_textview = gtk_text_view_new();
    gtk_widget_ensure_style(gglk_style_textview);
    
}


static glsi32 gglk_get_stylehint_val(struct glk_stylehint_struct *stylehints,
				     struct glk_stylehint_struct *base,
				     struct glk_stylehint_important_struct *base_important,
				     glui32 style, glui32 hint)
{
    gboolean important;
    glsi32 val;

    val = base->hints[style_AllStyles][hint];
    important = base_important->important[style_AllStyles][hint];

    if(style != style_AllStyles &&
       (!important || base_important->important[style][hint])) {
	val = gglk_stylehint_combine(hint, val,
				     base->hints[style][hint]);
	important = base_important->important[style][hint];
    }

    if(stylehints && stylehints->wintype == wintype_TextGrid &&
       (!important || base_important->important[style_TextGrid][hint])) {

	if(hint == stylehint_Size) {
	    val = base->hints[style_TextGrid][hint];
	} else {
	    val = gglk_stylehint_combine(hint, val,
					 base->hints[style_TextGrid][hint]);
	}
	
	important = base_important->important[style_TextGrid][hint];
    }

    if(stylehints && !important) {
	/* FIXME- combinations should work differently here?? */
	val = gglk_stylehint_combine(hint, val,
				     stylehints->hints[style][hint]);
    }
    return val;
}


void gglk_update_tag_table(struct glk_stylehint_struct *stylehints,
			   int style, int hint)
{
    if(style == style_AllStyles) {
	for(style = 0; style < style_AllStyles; style++)
	    gglk_update_tag_table(stylehints, style, hint);
    } else {
	GtkTextTag *tag = gtk_text_tag_table_lookup(stylehints->table,
						gglk_get_tag(style));
	glsi32 val;

	val = gglk_get_stylehint_val(stylehints, &gglk_individual_styles,
				     &gglk_important_styles,
				     style, hint);
	gglk_adjust_tag(tag, hint, val);
    }
}


void gglk_set_background_color(GtkTextView *view,
			       struct glk_stylehint_struct *stylehints)
{
    /* Set background color of view */
    glsi32 val = gglk_get_stylehint_val(stylehints, &gglk_individual_styles,
					&gglk_important_styles,
					style_Normal,
					stylehint_BackColor);
    
    if(val == -1) {
#if GTK_CHECK_VERSION(2,2,0)
	gtk_widget_modify_base(GTK_WIDGET(view), GTK_STATE_NORMAL, NULL);
#else
	{
	    GtkRcStyle *rc = gtk_widget_get_modifier_style(
		GTK_WIDGET(view));
	    rc->color_flags[GTK_STATE_NORMAL] &= ~GTK_RC_BASE;
	    gtk_widget_modify_style(GTK_WIDGET(view), rc);
	}
#endif
    } else {
	GdkColor color;
    
	color.red   = ((val & 0xff0000) >> 16) * 257;
	color.green = ((val & 0xff00) >> 8) * 257;
	color.blue  = (val & 0xff) * 257;

	gdk_colormap_alloc_color(gtk_widget_get_colormap(GTK_WIDGET(view)),
				 &color,
				 FALSE, TRUE);

	gtk_widget_modify_base(GTK_WIDGET(view), GTK_STATE_NORMAL, &color);
    }
}


void gglk_set_tags(GtkTextView *view, struct glk_stylehint_struct *stylehints)
{
    GtkTextBuffer *buffer;
    int h, s;
    gboolean make_new_table = FALSE;

    if(!stylehints->table) {
	stylehints->table = gtk_text_tag_table_new();
	make_new_table = TRUE;
    }

    buffer = gtk_text_buffer_new(stylehints->table);
    gtk_text_view_set_buffer(view, buffer);

    if(!make_new_table)
	return;
    for(s = 0; s < GGLK_NUMSTYLES; s++) {
	GtkTextTag *tag = gtk_text_tag_new(gglk_get_tag(s));
	gtk_text_tag_table_add(stylehints->table, tag);
	g_signal_connect(tag, "event", G_CALLBACK(gglk_text_mouse),
			 GINT_TO_POINTER(0));

	for(h = 0; h < GGLK_NUMHINTS; h++)
	    gglk_update_tag_table(stylehints, s, h);
    }

    gtk_text_buffer_create_tag(buffer, "editable", "editable", TRUE, NULL);



    gglk_set_background_color(view, stylehints);
}




static inline glui32 gglk_style_measure_tag(GtkTextTag *tag,
				     glui32 hint, glsi32 *result)
{
    PangoWeight weight;
    PangoStyle  style;
    GtkJustification justification;
    GtkTextDirection direction;
    const char *family;
    gint val;
    double x;
    GObject *tag_object = G_OBJECT(tag);

    switch(hint) {
    case stylehint_Indentation:
	g_object_get(tag_object, "left_margin", &val, NULL);
	*result = val - 5; return 1;
    case stylehint_ParaIndentation:
	g_object_get(tag_object, "indent", &val, NULL);
	*result = val; return 1;
    case stylehint_Justification:
	g_object_get(tag_object, "justification", &justification, NULL);
	switch(justification) {
	case GTK_JUSTIFY_LEFT:  *result = stylehint_just_LeftFlush; return 1;
	case GTK_JUSTIFY_RIGHT: *result = stylehint_just_RightFlush; return 1;
	case GTK_JUSTIFY_CENTER:*result = stylehint_just_Centered; return 1;
	case GTK_JUSTIFY_FILL:  *result = stylehint_just_LeftRight; return 1;
	}
	return 0;
    case stylehint_Size:
	g_object_get(tag_object, "scale", &x, NULL);
	*result = log(val) / log(1.2) * 2;
	return 1;
    case stylehint_Weight:
	g_object_get(tag_object, "weight", &weight, NULL);
	if(weight <= PANGO_WEIGHT_ULTRALIGHT)      { *result = -2; }
	else if(weight <= PANGO_WEIGHT_LIGHT)      { *result = -1; }
	else if(weight <= PANGO_WEIGHT_NORMAL)     { *result =  0; }
	else if(weight <= PANGO_WEIGHT_BOLD)       { *result =  1; }
	else if(weight <= PANGO_WEIGHT_ULTRABOLD)  { *result =  2; }
	else return 0;
	return 1;
    case stylehint_Oblique:
	g_object_get(tag_object, "style", &style, NULL);
	switch(style) {
	case PANGO_STYLE_NORMAL: *result = 0; return 1;
	case PANGO_STYLE_OBLIQUE: *result = 1; return 1;
	case PANGO_STYLE_ITALIC: *result = 1; return 1;
	}
	return 0;
    case stylehint_Proportional:
	g_object_get(tag_object, "family", &family, NULL);
	if(family)
	    *result = strcmp(family, "monospace") != 0;
	else
	    *result = 1;
	return 1;
    case stylehint_TextColor:
	*result = GPOINTER_TO_INT(g_object_get_data(tag_object,
						    "TextColor"));
	if(*result == -1)
	    *result = gglk_gdkcolor_to_glk(
		&gglk_style_textview->style->text[GTK_STATE_NORMAL]);

	return 1;
    case stylehint_BackColor:
	*result = GPOINTER_TO_INT(g_object_get_data(tag_object,
						    "BackColor"));
	if(*result == -1)
	    *result = gglk_gdkcolor_to_glk(
		&gglk_style_textview->style->base[GTK_STATE_NORMAL]);
	return 1;
    case stylehint_ReverseColor:
	*result = GPOINTER_TO_INT(g_object_get_data(tag_object,
						    "ReverseColor"));
	return 1;
    case stylehint_Direction:
	g_object_get(tag_object, "direction", &direction, NULL);
	if(direction == GTK_TEXT_DIR_RTL)
	    *result = 1;
	else
	    *result = 0;
	return 1;
    default:
	return 0;
    }
}



glui32 gglk_style_measure(GtkTextTagTable *table,
			  glui32 styl, glui32 hint, glsi32 *result)
{
    GtkTextTag *tag;

    tag = gtk_text_tag_table_lookup(table, gglk_get_tag(styl));

    return gglk_style_measure_tag(tag, hint, result);
}

#include "gui_style.c"
