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
#include <string.h>



/* global Application Stylehints */
static struct glk_stylehint_struct *text_styles[2] = { NULL, NULL };
static GList *stylehint_list = NULL;

void glk_stylehint_clear(glui32 wintype, glui32 styl, glui32 hint)
{
    switch(hint) {
    case stylehint_Justification:
    case stylehint_Proportional:
    case stylehint_TextColor:
    case stylehint_BackColor:
	glk_stylehint_set(wintype, styl, hint, -1);
	break;

    default:
	glk_stylehint_set(wintype, styl, hint, 0);
	break;
    }
}


static struct glk_stylehint_struct *gglk_stylehint_new(void)
{
    struct glk_stylehint_struct *n;
    n = g_malloc(sizeof(*n));
    n->table = NULL;
    n->ref_count = 1;
    stylehint_list = g_list_prepend(stylehint_list, n);
    n->node = g_list_first(stylehint_list);
    return n;
}

void gglk_stylehint_ref(struct glk_stylehint_struct *stylehints)
{
    stylehints->ref_count++;
}

void gglk_stylehint_unref(struct glk_stylehint_struct *stylehints)
{
    stylehints->ref_count--;
    if(stylehints->ref_count == 0) {
	if(stylehints->table)
	    g_object_unref(G_OBJECT(stylehints->table));
	stylehint_list = g_list_delete_link(stylehint_list, stylehints->node);
	g_free(stylehints);
    }
}

void gglk_stylehint_init(void)
{
    int i, h;

    for(i = 0; i < 2; i++)
	text_styles[i] = gglk_stylehint_new();

    text_styles[0]->wintype = wintype_TextBuffer;
    text_styles[1]->wintype = wintype_TextGrid;

    for(i = 0; i < GGLK_NUMSTYLES; i++) {
	for(h = 0; h < GGLK_NUMHINTS; h++) {
	    glk_stylehint_clear(wintype_TextBuffer, i, h);
	    glk_stylehint_clear(wintype_TextGrid, i, h);
	}
    }
}

static void gglk_stylehint_change(int index,
				  glui32 styl, glui32 hint, glsi32 val)
{
    if(text_styles[index]->hints[styl][hint] == val)
	return;

    if(text_styles[index]->table) {
	struct glk_stylehint_struct *new_style = gglk_stylehint_new();
	memcpy(&new_style->hints, &text_styles[index]->hints,
	       sizeof(new_style->hints));
	new_style->wintype = text_styles[index]->wintype;

	gglk_stylehint_unref(text_styles[index]);
	text_styles[index] = new_style;
    }

    text_styles[index]->hints[styl][hint] = val;
}


static inline int wintype_to_index(glui32 wintype)
{
    return wintype == wintype_TextGrid;
}


void glk_stylehint_set(glui32 wintype, glui32 styl, glui32 hint, glsi32 val)
{
    if(styl < GGLK_NUMSTYLES && hint < GGLK_NUMHINTS) {
	if(wintype == wintype_AllTypes) {
	    gglk_stylehint_change(0, styl, hint, val);
	    gglk_stylehint_change(1, styl, hint, val);
	} else {
	    int index = wintype_to_index(wintype);
	    gglk_stylehint_change(index, styl, hint, val);
	}
    }
}


winid_t glk_window_open(winid_t split, glui32 method, glui32 size,
			glui32 wintype, glui32 rock)
{
    return glk_window_open_with_styles(split, method, size, wintype,
				       text_styles[wintype_to_index(wintype)],
				       rock);
}

void gglk_stylehints_update(int style, int hint)
{
    GList *s;
    winid_t w;
    
    for(s = stylehint_list; s; s = g_list_next(s)) {
	struct glk_stylehint_struct *h = s->data;
	if(h->table)
	    gglk_update_tag_table(h, style, hint);
    }
    w = NULL;
    while((w = glk_window_iterate(w, NULL)) != NULL) {
	gglk_window_set_background_color(w);
    }
}
