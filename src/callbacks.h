#include <gtk/gtk.h>
#include "gglk.h"

GtkWidget*
gglk_colorpicker_new_glade (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2) PRIVATE;

void
do_glk_objects                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
do_styles                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
do_gestalt                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
do_preferences                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
do_about                               (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_objects                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_styles                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_gestalt                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_preferences                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_about                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_cut                               (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_copy                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_paste                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
menu_delete                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_showmenu_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data) PRIVATE;

void
on_showstatusbar_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data) PRIVATE;

void
on_usescrollbars_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data) PRIVATE;

void
on_clear_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_undo_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_restore_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_restart_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_game_menu_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
on_edit_menu_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data) PRIVATE;

void
styles_ok_clicked                      (GtkWidget       *window,
                                        gpointer         user_data) PRIVATE;
