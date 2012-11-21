#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "gglk.h"
#include "gui.h"
#include "gglk_colorpicker.h"


GtkWidget*
gglk_colorpicker_new_glade (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *widget = gglk_colorpicker_new(string1);
    return widget;
}

void menu_objects(GtkMenuItem *menuitem, gpointer user_data)
{ gglk_do_glk_objects(menuitem, user_data); }
void menu_styles(GtkMenuItem *menuitem, gpointer user_data)
{ gglk_do_styles(menuitem, user_data); }
void menu_gestalt(GtkMenuItem *menuitem, gpointer user_data)
{ gglk_do_gestalt(menuitem, user_data); }
void menu_preferences(GtkMenuItem *menuitem, gpointer user_data)
{ gglk_do_preferences(menuitem, user_data); }


void menu_cut(GtkMenuItem *unused_menuitem, gpointer unused_data)
{
    GtkWidget *v = gglk_get_selection_view();
    if(v)
	g_signal_emit_by_name(v, "cut-clipboard");
}


void menu_copy(GtkMenuItem *unused_menuitem, gpointer unused_data)
{
    GtkWidget *v = gglk_get_selection_view();
    if(v)
	g_signal_emit_by_name(v, "copy-clipboard");
}


void menu_paste(GtkMenuItem *unused_menuitem, gpointer unused_data)
{
    GtkWidget *v = gglk_get_line_input_view();
    
    if(v)
	g_signal_emit_by_name(v, "paste-clipboard");
}


void on_open_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{

}

static void do_set_text(const char *text, gboolean accept)
{
    GglkText *tb = GGLK_TEXT(gglk_get_line_input_view());
    gunichar *buf_ucs4;
    
    if(!tb) {
	char *msg;
	if(strcmp(text, "") == 0) {
	    msg = g_strdup("No window to clear");
	} else {
	    msg = g_strdup_printf("No window to accept %s", text);
	}
	sglk_status_set_mesg(msg);
	g_free(msg);
	return;
    }
    
    buf_ucs4 = g_utf8_to_ucs4(text, -1, NULL, NULL, NULL);
    gglk_text_line_input_set(tb, tb->line_maxlen, buf_ucs4);
    g_free(buf_ucs4);

    if(accept)
	gglk_text_line_input_accept(tb);
}


void on_clear_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{ do_set_text("", FALSE); }
void on_undo_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{ do_set_text("undo", TRUE); }
void on_save_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{ do_set_text("save", TRUE); }
void on_restore_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{ do_set_text("restore", TRUE); }
void on_restart_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{ do_set_text("restart", TRUE); }

void on_quit_activate(GtkMenuItem *unused_menuitem, gpointer unused_data)
{
    GglkText *tb = GGLK_TEXT(gglk_get_line_input_view());
    if(tb)
	do_set_text("quit", TRUE);
    else {
	GtkWidget *dialog;
	GtkResponseType response;
	dialog = gtk_message_dialog_new(GTK_WINDOW(gglk_win),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"Are you sure you want to quit?");
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(response == GTK_RESPONSE_YES)
	    glk_exit();
    }
}

void on_game_menu_activate(GtkMenuItem *menuitem, gpointer unused_data)
{
    GtkWidget *tb = gglk_get_line_input_view();
    static const char items[] = "undo" "\0"    "save" "\0"
	                        "restore" "\0" "restart" "\0";
    const char *p;

    for(p = items; *p; p += strlen(p) + 1) {
	GtkWidget *m = lookup_widget(GTK_WIDGET(menuitem), p);

	gtk_widget_set_sensitive(m, tb != NULL);
    }
}


void on_edit_menu_activate(GtkMenuItem *menuitem, gpointer unused_data)
{
    GtkWidget *tb = gglk_get_line_input_view();
    GtkWidget *sel = gglk_get_selection_view();
    gboolean text = FALSE;
    
    if(tb) {
	text = gtk_clipboard_wait_is_text_available(
	    gtk_widget_get_clipboard(tb, GDK_SELECTION_CLIPBOARD));
    }

    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(menuitem), "cut1"),
			     sel != NULL);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(menuitem), "copy1"),
			     sel != NULL);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(menuitem), "paste1"),
			     text);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(menuitem), "clear"),
			     tb != NULL);
}

