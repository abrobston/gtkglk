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

#ifndef GGLK_H
#define GGLK_H

#include "../config.h"

#ifdef HAVE_VISIBILITY_ATTRIBUTE
#define PRIVATE __attribute__ ((visibility ("internal")))
#else
#define PRIVATE
#endif

#include <gtk/gtk.h>
#include "glk.h"
#include "gi_dispa.h"
#include "sglk.h"
#include "gglk_text.h"
#include "gui.h"


extern GtkWidget *gglk_win PRIVATE;
extern GtkWidget *gglk_root PRIVATE;
extern glui32 gglk_age PRIVATE;
extern gboolean gglk_frame PRIVATE;


#define stylehint_Underline  (stylehint_NUMHINTS)
#define stylehint_Variant    (stylehint_NUMHINTS+1)
#define GGLK_NUMHINTS (stylehint_NUMHINTS+2)


#define style_TextGrid   (style_NUMSTYLES)
#define style_Hyperlinks (style_NUMSTYLES+1)
#define style_AllStyles  (style_NUMSTYLES+2)
#define style_SGlk       (style_NUMSTYLES+3)
#define GGLK_NUMSTYLES   (style_NUMSTYLES+4)



#define GGLK_MAGIC_FREE 0x4652
#define GGLK_MAGIC_WIN  0x4777
#define GGLK_MAGIC_STR  0x4773
#define GGLK_MAGIC_FREF 0x4766
#define GGLK_MAGIC_SCHN 0x4763



gboolean gglk_isvalid(const void *obj,
		      glui32 goodmagic, const glui32 *realmagic,
		      const char *function) PRIVATE;
gboolean gglk_isvalidtypes(const void *obj,
			   glui32 goodmagic, const glui32 *realmagic,
			   const char *function, const glui32 *realtype,
			   glui32 goodtype1, glui32 goodtype2) PRIVATE;

#define gglk_validmagic(obj, goodmagic, die) \
        if(!gglk_isvalid(obj, goodmagic, &obj->magic, __func__)) die

#define gglk_validwin(obj, die) gglk_validmagic(obj, GGLK_MAGIC_WIN, die)
#define gglk_validstr(obj, die) gglk_validmagic(obj, GGLK_MAGIC_STR, die)
#define gglk_validfref(obj, die) gglk_validmagic(obj, GGLK_MAGIC_FREF, die)
#define gglk_validschn(obj, die) gglk_validmagic(obj, GGLK_MAGIC_SCHN, die)

#define gglk_validmagictypes(obj, goodmagic, die, goodtype1, goodtype2) \
       if(!gglk_isvalidtypes(obj, goodmagic, &obj->magic, __func__, \
                             &obj->type, goodtype1, goodtype2)) die

#define GGLK_WIN_TYPE(obj, die, goodtype) \
       gglk_validmagictypes(obj, GGLK_MAGIC_WIN, die, goodtype, 0xffffffff)
#define GGLK_WIN_TYPES(obj, die, goodtype1, goodtype2) \
       gglk_validmagictypes(obj, GGLK_MAGIC_WIN, die, goodtype1, goodtype2)
#define GGLK_STR_TYPE(obj, die, goodtype) \
       gglk_validmagictypes(obj, GGLK_MAGIC_STR, die, goodtype, 0xffffffff)
#define GGLK_STR_TYPES(obj, die, goodtype1, goodtype2) \
       gglk_validmagictypes(obj, GGLK_MAGIC_STR, die, goodtype1, goodtype2)


  
#define MAKE_ROCK(type, name, valid) \
    glui32 glk_##name##_get_rock(type n) \
    { \
        valid(n, return 0); \
        return n->rock; \
    } \
    gidispatch_rock_t gglk_##name##_get_disprock(type n) \
    { \
        valid(n, return dispa_bad); \
        return n->disprock; \
    } \
    void gglk_##name##_set_disprock(type n, gidispatch_rock_t r) \
    { \
        valid(n, return); \
        n->disprock = r; \
    }
#define MAKE_ITER(type, name) \
    static GList *name##_list = NULL; \
    type glk_##name##_iterate(type n, glui32 *rockptr) \
    { \
        GList *item; type res;\
        if(n == NULL) item = g_list_first(name##_list); \
        else          item = g_list_next(n->node); \
        if(item == NULL) return NULL; \
        res = item->data; \
        if(rockptr) *rockptr = glk_##name##_get_rock(res); \
        return res; \
    } \
    static inline GList *gglk_##name##_add(type n) \
    { \
        name##_list = g_list_prepend(name##_list, n); \
        gglk_##name##_set_disprock(n, gglk_dispa_newobj(n, gidisp_Class_##name)); \
        return g_list_first(name##_list); \
    } \
    static inline void gglk_##name##_del(type n) \
    { \
        gglk_dispa_endobj(n, gidisp_Class_##name); \
        name##_list = g_list_delete_link(name##_list, n->node); \
    }
gidispatch_rock_t gglk_window_get_disprock(winid_t win) PRIVATE;
gidispatch_rock_t gglk_stream_get_disprock(strid_t str) PRIVATE;
gidispatch_rock_t gglk_fileref_get_disprock(frefid_t fref) PRIVATE;
gidispatch_rock_t gglk_schannel_get_disprock(schanid_t chan) PRIVATE;
void gglk_window_set_disprock(winid_t win, gidispatch_rock_t r) PRIVATE;
void gglk_stream_set_disprock(strid_t str, gidispatch_rock_t r) PRIVATE;
void gglk_fileref_set_disprock(frefid_t fref, gidispatch_rock_t r) PRIVATE;
void gglk_schannel_set_disprock(schanid_t chan, gidispatch_rock_t r) PRIVATE;


extern int gglk_statusbar_context_sglk PRIVATE;
extern int gglk_statusbar_context_exit PRIVATE;
extern int gglk_statusbar_context_more PRIVATE;
void gglk_status_set_mesg(const char *message, int context) PRIVATE;


/* gestalt.c */
#define gestalt_NUMGESTALTS 16
extern glui32 gglk_gestalt_version PRIVATE;
extern gboolean gglk_gestalt_possible[gestalt_NUMGESTALTS] PRIVATE;
extern gboolean gglk_gestalt_support[gestalt_NUMGESTALTS] PRIVATE;
extern gboolean gglk_gestalt_claim[gestalt_NUMGESTALTS] PRIVATE;
void gglk_init_gestalt(void) PRIVATE;
gboolean gglk_gestalt_validate_event(glui32 evtype) PRIVATE;
void gglk_set_gest_support(GtkWidget *widget, gpointer user_data) PRIVATE;
void gglk_set_gest_claim(GtkWidget *widget, gpointer user_data) PRIVATE;



/* window.c */
gboolean gglk_win_contains(winid_t a, winid_t b) PRIVATE;
void gglk_set_focus(void) PRIVATE;
void gglk_auto_scroll(void) PRIVATE;
void gglk_update_scrollmark_win(winid_t win) PRIVATE;
void gglk_win_set_scroll_policy(GtkPolicyType p) PRIVATE;
GtkWidget *gglk_get_line_input_view(void) PRIVATE;
GtkWidget *gglk_get_selection_view(void) PRIVATE;
void gglk_window_resize_all_in(winid_t win) PRIVATE;
GtkTextView *gglk_window_get_view(winid_t win) PRIVATE;
gboolean gglk_win_validate_event(glui32 evtype, winid_t win) PRIVATE;
winid_t glk_window_open_with_styles(winid_t split, glui32 method, glui32 size,
				    glui32 wintype, 
				    struct glk_stylehint_struct *stylehints,
				    glui32 rock) PRIVATE;
void gglk_window_set_background_color(winid_t win) PRIVATE;


/* stream.c */
strid_t gglk_strid_dummy(void) PRIVATE;
strid_t gglk_strid_from_gglk_text(GglkText *gbuffer,
				  winid_t win, glui32 wintype) PRIVATE;
strid_t gglk_strid_from_strid(strid_t src, glui32 start, glui32 length) PRIVATE;
strid_t gglk_strid_from_filename(const char *name,
				 glui32 usage, glui32 fmode, glui32 rock,
				 glui32 orig_fmode) PRIVATE;
int gglk_fix_editable_region(strid_t str, char *buf, glui32 maxlen) PRIVATE;
void gglk_stream_set_echo(strid_t str, strid_t echo) PRIVATE;
void gglk_stream_textgrid_size(strid_t str, glui32 xsize, glui32 ysize) PRIVATE;
strid_t gglk_stream_get_echo(strid_t str) PRIVATE;
void gglk_str_update_edit(strid_t str, const char *buf,
			  int maxlen, int curlen, int curpos) PRIVATE;
gboolean gglk_text_mouse(GtkTextTag *texttag,
			 GObject *unused,
			 GdkEvent *event, 
			 GtkTextIter *iter,
			 gpointer user_data) PRIVATE;



/* fileref.c */
void gglk_init_fref(void) PRIVATE;
gboolean gglk_valid_fmode(glui32 fmode) PRIVATE;


/* style.c */
struct glk_stylehint_struct {
    GtkTextTagTable *table;
    glsi32 hints[GGLK_NUMSTYLES][GGLK_NUMHINTS];
    glui32 wintype;

    GList *node;
    gint ref_count;
};
struct glk_stylehint_important_struct {
    gboolean important[GGLK_NUMSTYLES][GGLK_NUMHINTS];
};


extern struct glk_stylehint_struct gglk_individual_styles PRIVATE;
extern struct glk_stylehint_important_struct gglk_important_styles PRIVATE;

void gglk_init_styles(void) PRIVATE;
void gglk_set_tags(GtkTextView *view,
		   struct glk_stylehint_struct *stylehints) PRIVATE;
void gglk_adjust_tag(GtkTextTag *tag, glui32 hint, glsi32 val) PRIVATE;
gboolean gglk_font_transform(GValue *value, GValue *tvalue,
			     gboolean forwards, gpointer unused_data) PRIVATE;

glui32 gglk_style_distinguish(GtkTextTagTable *table, GtkTextView *view,
			      glui32 styl1, glui32 styl2) PRIVATE;
glui32 gglk_style_measure(GtkTextTagTable *table,
			  glui32 styl, glui32 hint, glsi32 *result) PRIVATE;
void gglk_update_tag_table(struct glk_stylehint_struct *stylehints,
			   int style, int hint) PRIVATE;
void gglk_set_background_color(GtkTextView *view,
			       struct glk_stylehint_struct *stylehints) PRIVATE;


/* misc.c */
gboolean gglk_trigger_event(glui32 evtype, winid_t win, glui32 val1, glui32 val2) PRIVATE;
glui32 gglk_keycode_gdk_to_glk(guint k) PRIVATE G_GNUC_CONST;
guint gglk_keycode_glk_to_gdk(glui32 k) PRIVATE G_GNUC_CONST;
int gglk_printinfo(void) PRIVATE;


/* blorb.c */
void gglk_free_pic_cache(void) PRIVATE;
GdkPixbuf *gglk_load_pic(glui32 num) PRIVATE;
strid_t glk_blorb_get_str(glui32 usage, glui32 resnum,
			  glui32 *chunknum, glui32 *chunktype) PRIVATE;
void *gglk_map_chunk(glui32 usage, glui32 resnum,
		     glui32 *size, glui32 *chunknum, glui32 *chunktype) PRIVATE;
void gglk_unmap_chunk(glui32 chunknum) PRIVATE;
void gglk_free_blorb(void) PRIVATE;


/* global.c */
void gglk_stylehint_init(void) PRIVATE;
void gglk_stylehint_ref(struct glk_stylehint_struct *stylehints) PRIVATE;
void gglk_stylehint_unref(struct glk_stylehint_struct *stylehints) PRIVATE;
void gglk_stylehints_update(int style, int hint) PRIVATE;


/* gglk_prefs.c */
#define GLK_TYPE_JUSTIFICATION_TYPE (glk_justification_type_get_type())
GType glk_justification_type_get_type(void) G_GNUC_CONST PRIVATE;
void gglk_prefs_init(void) PRIVATE;

void gglk_styles_bind_gconf(GConfChangeSet *changeset,
			    struct glk_stylehint_struct *base,
			    struct glk_stylehint_important_struct *base_important,
			    void (*trigger)(GValue *, gboolean, gpointer *)) PRIVATE;


/* dispatch.c */
extern const gidispatch_rock_t dispa_bad PRIVATE;

gidispatch_rock_t gglk_dispa_newobj(void *obj, glui32 objclass) PRIVATE;
void gglk_dispa_endobj(void *obj, glui32 objclass) PRIVATE;
gidispatch_rock_t gglk_dispa_newmem(void *array, glui32 len,
				    const char *typecode) PRIVATE;
void gglk_dispa_endmem(void *array, glui32 len, const char *typecode,
		       gidispatch_rock_t objrock) PRIVATE;
gboolean gglk_stillvalid(void *obj, glui32 objclass) PRIVATE;



/* names.c */
const char *gglk_magic_to_typename(glui32 magic) G_GNUC_CONST PRIVATE;
const char *gglk_subtype_names(glui32 magic, glui32 type) G_GNUC_CONST PRIVATE;
const char *gglk_gestalt_get_name(glui32 g) G_GNUC_CONST PRIVATE;
const char *gglk_get_tag(glui32 style) G_GNUC_CONST PRIVATE;
const char *gglk_get_hint(glui32 hint) G_GNUC_CONST PRIVATE;


/* Silence some gcc warnings */
#define unused          unused          G_GNUC_UNUSED
#define unused_key      unused_key      G_GNUC_UNUSED
#define unused_data     unused_data     G_GNUC_UNUSED
#define unused_value    unused_value    G_GNUC_UNUSED
#define unused_widget   unused_widget   G_GNUC_UNUSED
#define unused_mark     unused_mark     G_GNUC_UNUSED
#define unused_iter     unused_iter     G_GNUC_UNUSED
#define unused_menuitem unused_menuitem G_GNUC_UNUSED

#endif
