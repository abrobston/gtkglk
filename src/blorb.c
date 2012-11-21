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

#include <unistd.h>
#include <stdlib.h>
#include "gglk.h"
#include "gi_blorb.h"



static strid_t blorbfile = NULL;
static giblorb_map_t *blorbmap = NULL;


giblorb_err_t giblorb_set_resource_map(strid_t file)
{
    giblorb_err_t err;
    if(!file) return giblorb_err_NotFound;

    if(blorbmap)
	giblorb_destroy_map(blorbmap);

    if((err = giblorb_create_map(file, &blorbmap)) != giblorb_err_None) {
	blorbmap = NULL;
	return err;
    }
    blorbfile = file;
    return giblorb_err_None;
}


giblorb_map_t *giblorb_get_resource_map(void)
{
    return blorbmap;
}


strid_t glk_blorb_get_str(glui32 usage, glui32 resnum,
			  glui32 * restrict chunknum,
			  glui32 * restrict chunktype)
{
    giblorb_result_t res;
    strid_t str;
    
    if(!blorbmap) return NULL;
    if(giblorb_load_resource(blorbmap, giblorb_method_FilePos, &res,
			     usage, resnum) != giblorb_err_None) {
	return NULL;
    }

    if(chunknum)  *chunknum = res.chunknum;
    if(chunktype) *chunktype = res.chunktype;

    str = gglk_strid_from_strid(blorbfile, res.data.startpos, res.length);
    return str;
}



void *gglk_map_chunk(glui32 usage, glui32 resnum,
		     glui32 * restrict size,
		     glui32 * restrict chunknum,
		     glui32 * restrict chunktype)
{
    giblorb_result_t res;
    if(!blorbmap) return NULL;
    
    if(giblorb_load_resource(blorbmap, giblorb_method_Memory, &res,
			     usage, resnum) != giblorb_err_None) {
	return NULL;
    }

    *size = res.length;
    *chunknum = res.chunknum;
    if(chunktype)
	*chunktype = res.chunktype;
    return res.data.ptr;
}

void gglk_unmap_chunk(glui32 chunknum)
{
    giblorb_unload_chunk(blorbmap, chunknum);
}



static GHashTable *pic_hash = NULL;
    

void gglk_free_pic_cache(void)
{
    if(pic_hash) {
	g_hash_table_destroy(pic_hash);
	pic_hash = NULL;
    }
}


GdkPixbuf *gglk_load_pic(glui32 num)
{
    glui32 size, chunknum;
    void *p;
    GdkPixbufLoader *loader;
    GdkPixbuf *result = NULL;
    GError *error = NULL;

    if(!blorbmap) return NULL;

    if(!pic_hash)
	pic_hash = g_hash_table_new_full(NULL, NULL,
					 NULL, g_object_unref);

    if((result = g_hash_table_lookup(pic_hash, GINT_TO_POINTER(num))) != NULL) {
	g_object_ref(result);
	return result;
    }

    p = gglk_map_chunk(giblorb_ID_Pict, num, &size, &chunknum, NULL);
    if(p) {
	loader = gdk_pixbuf_loader_new();
	
	gdk_pixbuf_loader_write(loader, p, size, &error);
	error = NULL;
	gdk_pixbuf_loader_close(loader, &error);
	result = gdk_pixbuf_loader_get_pixbuf(loader);

	gglk_unmap_chunk(chunknum);
    }

    if(result) {
	g_object_ref(result);
	g_hash_table_insert(pic_hash, GINT_TO_POINTER(num), result);
    }
    
    return result;
}


glui32 glk_image_get_info(glui32 image,
			  glui32 * restrict width,
			  glui32 * restrict height)
{
    GdkPixbuf *pic = gglk_load_pic(image);

    if(!gglk_gestalt_claim[gestalt_Graphics]) return FALSE;

    if(pic) {
	if(width)
	    *width = gdk_pixbuf_get_width(pic);
	if(height)
	    *height = gdk_pixbuf_get_height(pic);
	g_object_unref(pic);
	return TRUE;
    }
    return FALSE;
}


void sglk_icon_set(glui32 image)
{
    GdkPixbuf *pic;
    pic = gglk_load_pic(image);
    if(pic) {
	gtk_window_set_icon(GTK_WINDOW(gglk_win), pic);
	g_object_unref(pic);
    }
}

void gglk_free_blorb(void)
{
    if(blorbmap) {
	giblorb_destroy_map(blorbmap);
	blorbmap = NULL;
    }
}
