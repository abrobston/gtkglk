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

#include "gglk_colorpicker.h"
#include <gtk/gtkhbox.h>
#include <stdio.h>


enum {
    PROP_0,
    PROP_COLOR
};

static void gglk_colorpicker_class_init(GglkColorpickerClass *klass);
static void gglk_colorpicker_init(GglkColorpicker *cp);
static void gglk_colorpicker_destroy(GtkObject *object,
				     gpointer unused_data);
static gboolean gglk_colorpicker_mnemonic_activate(GtkWidget *widget,
						   gboolean   group_cycling);
static void gglk_colorpicker_set_property(GObject            *object,
					  guint               param_id,
					  const GValue       *value,
					  GParamSpec         *pspec);
static void gglk_colorpicker_get_property(GObject            *object,
					  guint               param_id,
					  GValue             *value,
					  GParamSpec         *pspec);


GType gglk_colorpicker_get_type(void)
{
    static GType gg_type = 0;

    if(!gg_type) {
	static const GTypeInfo gg_info = {
	    sizeof(GglkColorpickerClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) gglk_colorpicker_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(GglkColorpicker),
	    0,
	    (GInstanceInitFunc) gglk_colorpicker_init,
	    NULL
	};

	gg_type = g_type_register_static(GTK_TYPE_HBOX, "GglkColorpicker",
					 &gg_info, 0);
    }

    return gg_type;
}


static void gglk_colorpicker_class_init(GglkColorpickerClass *klass)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
    GObjectClass *gobject_class = (GObjectClass *) klass;

    gobject_class->get_property = gglk_colorpicker_get_property;
    gobject_class->set_property = gglk_colorpicker_set_property;

    widget_class->mnemonic_activate = gglk_colorpicker_mnemonic_activate;

    g_object_class_install_property(
	gobject_class, PROP_COLOR,
	g_param_spec_boxed("color", NULL, 
			   "Color that has been picked",
			   GDK_TYPE_COLOR,
			   (G_PARAM_READABLE | G_PARAM_WRITABLE)));
}



static void gglk_colorpicker_set_color(GglkColorpicker *cp, GdkColor *color)
{
    GtkWidget *label;

    if(!cp->enabled && !color)
	return;
    if(cp->enabled && color &&
       cp->color.red == color->red &&
       cp->color.green == color->green &&
       cp->color.blue == color->blue)
	return;

    if(color) {
	cp->color = *color;
	cp->enabled = TRUE;
    } else {
	cp->enabled = FALSE;
    }

    g_object_notify(G_OBJECT(cp), "color");

    label = gtk_bin_get_child(GTK_BIN(cp->check));

    if(cp->enabled) {
	char buf[30];
	sprintf(buf, "<tt>#%02X%02X%02X</tt>",
		cp->color.red >> 8, cp->color.green >> 8, cp->color.blue >> 8);
	gtk_label_set_markup(GTK_LABEL(label), buf);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->check), TRUE);
	gtk_widget_modify_bg(GTK_WIDGET(cp->drawing),
			     GTK_STATE_NORMAL, &cp->color);
    } else {
	gtk_label_set_markup(GTK_LABEL(label), "<tt>#------</tt>");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->check), FALSE);
#if GTK_CHECK_VERSION(2,2,0)
	gtk_widget_modify_bg(GTK_WIDGET(cp->drawing), GTK_STATE_NORMAL, NULL);
#else
	{
	    GtkRcStyle *rc = gtk_widget_get_modifier_style(
		GTK_WIDGET(cp->drawing));
	    rc->color_flags[GTK_STATE_NORMAL] &= ~GTK_RC_BG;
	    gtk_widget_modify_style(GTK_WIDGET(cp->drawing), rc);
	}
#endif
    }
}



static void gglk_colorpicker_response(GtkDialog *dialog, gint arg1,
				      gpointer user_data)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(user_data);

    GtkColorSelection *colorsel;
    GdkColor color;

    if(arg1 == GTK_RESPONSE_OK) {
	colorsel = GTK_COLOR_SELECTION(
	    GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel);

	gtk_color_selection_get_current_color(colorsel, &color);

	gglk_colorpicker_set_color(cp, &color);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void gglk_colorpicker_clicked(GtkWidget *unused_widget,
				     gpointer user_data)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(user_data);

    GtkWidget *dialog;
    GtkColorSelection *colorsel;
    
    dialog = gtk_color_selection_dialog_new(cp->title);
    colorsel=GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel);

    gtk_color_selection_set_current_color(colorsel, &cp->color);

    g_signal_connect(dialog, "response",
		     G_CALLBACK(gglk_colorpicker_response),
		     user_data);

    gtk_widget_show_all(dialog);
    
}


static void gglk_colorpicker_toggled(GtkWidget *unused_widget,
				     gpointer user_data)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(user_data);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cp->check))) {
	if(!cp->enabled)
	    gglk_colorpicker_set_color(cp, &cp->color);
    } else {
	gglk_colorpicker_set_color(cp, NULL);
    }
}


static void gglk_colorpicker_init(GglkColorpicker *cp)
{
    GtkHBox *hbox = GTK_HBOX(cp);
    cp->check   = GTK_CHECK_BUTTON(gtk_check_button_new_with_label(""));
    cp->button  = GTK_BUTTON(gtk_button_new());
    cp->drawing = GTK_DRAWING_AREA(gtk_drawing_area_new());
    cp->title   = NULL;
    
    gtk_container_add(GTK_CONTAINER(cp->button), GTK_WIDGET(cp->drawing));

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(cp->check),
		       FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(cp->button),
		       TRUE, TRUE, 0);

    g_signal_connect(cp, "destroy",
		     G_CALLBACK(gglk_colorpicker_destroy), NULL);
    g_signal_connect(cp->button, "clicked",
		     G_CALLBACK(gglk_colorpicker_clicked), cp);
    g_signal_connect(cp->check, "toggled",
		     G_CALLBACK(gglk_colorpicker_toggled), cp);

    gtk_widget_show(GTK_WIDGET(cp->check));
    gtk_widget_show(GTK_WIDGET(cp->button));
    gtk_widget_show(GTK_WIDGET(cp->drawing));

    cp->enabled  = TRUE;  /* ensure that set_color will make a change */
    gglk_colorpicker_set_color(cp, NULL);
}


GtkWidget *gglk_colorpicker_new(const char *title)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(
	g_object_new(gglk_colorpicker_get_type(), NULL));

    cp->title = g_strdup(title);
    
    return GTK_WIDGET(cp);
}


static void gglk_colorpicker_destroy(GtkObject *object,
				     gpointer unused_data)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(object);
    g_free(cp->title);
}


static void gglk_colorpicker_set_property(GObject            *object,
					  guint               param_id,
					  const GValue       *value,
					  GParamSpec         *pspec)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(object);
    
    switch(param_id){
    case PROP_COLOR:
	gglk_colorpicker_set_color(cp, g_value_get_boxed(value));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}


static void gglk_colorpicker_get_property(GObject            *object,
					  guint               param_id,
					  GValue             *value,
					  GParamSpec         *pspec)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(object);
    
    switch(param_id){
    case PROP_COLOR:
	g_value_set_boxed(value, cp->enabled ? &cp->color : NULL);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	break;
    }
}


static gboolean gglk_colorpicker_mnemonic_activate(GtkWidget *widget,
						   gboolean   group_cycling)
{
    GglkColorpicker *cp = GGLK_COLORPICKER(widget);
    gboolean handled;

    g_signal_emit_by_name(cp->enabled ?
			  GTK_WIDGET(cp->check) : GTK_WIDGET(cp->button),
			  "mnemonic_activate",
			  group_cycling, &handled);

    return handled;
}
