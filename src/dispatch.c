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


const gidispatch_rock_t dispa_bad;

static gidispatch_rock_t (*dispa_objreg)(void *obj, glui32 objclass);
static void (*dispa_objunreg)(void *obj, glui32 objclass,
			   gidispatch_rock_t objrock);

static gidispatch_rock_t (*dispa_memreg)(void *array, glui32 len,
					 char *typecode);

static void (*dispa_memunreg)(void *array, glui32 len, char *typecode,
			      gidispatch_rock_t objrock);


gidispatch_rock_t gglk_dispa_newobj(void *obj, glui32 objclass)
{
    if(dispa_objreg)
	return dispa_objreg(obj, objclass);
    return dispa_bad;
}

void gglk_dispa_endobj(void *obj, glui32 objclass)
{
    if(dispa_objunreg)
	dispa_objunreg(obj, objclass, gidispatch_get_objrock(obj, objclass));
}

gidispatch_rock_t gglk_dispa_newmem(void *array, glui32 len,
				    const char *typecode)
{
    if(dispa_memreg)
	return dispa_memreg(array, len, (char *) typecode);
    return dispa_bad;
}


void gglk_dispa_endmem(void *array, glui32 len, const char *typecode,
		       gidispatch_rock_t objrock)
{
    if(dispa_memunreg)
	dispa_memunreg(array, len, (char *) typecode, objrock);
}
    



void gidispatch_set_object_registry(gidispatch_rock_t (*reg)(void *obj,
							     glui32 objclass),
				    void (*unreg)(void *obj, glui32 objclass,
						  gidispatch_rock_t objrock))
{
    winid_t win = NULL; strid_t str = NULL;
    frefid_t fref = NULL; schanid_t chan = NULL;
    
    dispa_objreg = reg;
    dispa_objunreg = unreg;

    /* register all existing objects */
    while((win = glk_window_iterate(win, NULL)) != NULL)
	gglk_window_set_disprock(win, gglk_dispa_newobj(win, gidisp_Class_Window));
    while((str = glk_stream_iterate(str, NULL)) != NULL)
	gglk_stream_set_disprock(str, gglk_dispa_newobj(str, gidisp_Class_Stream));
    while((fref = glk_fileref_iterate(fref, NULL)) != NULL)
	gglk_fileref_set_disprock(fref, gglk_dispa_newobj(fref, gidisp_Class_Fileref));
    while((chan = glk_schannel_iterate(chan, NULL)) != NULL)
	gglk_schannel_set_disprock(chan, gglk_dispa_newobj(chan, gidisp_Class_Schannel));
}


gidispatch_rock_t gidispatch_get_objrock(void *obj, glui32 objclass)
{
    gidispatch_rock_t disprock;
    
    switch(objclass) {
    case gidisp_Class_Window:
	disprock = gglk_window_get_disprock(obj);
	break;
    case gidisp_Class_Stream:
	disprock = gglk_stream_get_disprock(obj);
	break;
    case gidisp_Class_Fileref:
	disprock = gglk_fileref_get_disprock(obj);
	break;
    case gidisp_Class_Schannel:
	disprock = gglk_schannel_get_disprock(obj);
	break;
    default:
	g_warning("%s: bad objclass %" G_GUINT32_FORMAT, __func__, objclass);
	return dispa_bad;
    }

    return disprock;
}


/* The gglk_valid* functions can complain quickly about a lot of things,
   but sometimes they're wrong or segfault.  This function is intended
   to always be right and never crash */
gboolean gglk_stillvalid(void *obj, glui32 objclass)
{
    winid_t win = NULL; strid_t str = NULL;
    frefid_t fref = NULL; schanid_t chan = NULL;
    
    switch(objclass){
    case gidisp_Class_Window:
	while((win = glk_window_iterate(win, NULL)) != NULL)
	    if(win == obj)
		return TRUE;
	return FALSE;
    case gidisp_Class_Stream:
	while((str = glk_stream_iterate(str, NULL)) != NULL)
	    if(str == obj)
		return TRUE;
	return FALSE;
    case gidisp_Class_Fileref:
	while((fref = glk_fileref_iterate(fref, NULL)) != NULL)
	    if(fref == obj)
		return TRUE;
	return FALSE;
    case gidisp_Class_Schannel:
	while((chan = glk_schannel_iterate(chan, NULL)) != NULL)
	    if(chan == obj)
		return TRUE;
	return FALSE;
    }
    return FALSE;
}


void gidispatch_set_retained_registry(gidispatch_rock_t (*reg)(void *array,
							       glui32 len,
							       char *typecode),
				      void (*unreg)(void *array, glui32 len,
						    char *typecode,
						    gidispatch_rock_t objrock))
{
    dispa_memreg = reg;
    dispa_memunreg = unreg;
}


