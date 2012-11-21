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
#include "sound_dummy.h"
#include <stdio.h>


glui32 gglk_gestalt_version;
gboolean gglk_gestalt_possible[gestalt_NUMGESTALTS];
gboolean gglk_gestalt_support[gestalt_NUMGESTALTS];
gboolean gglk_gestalt_claim[gestalt_NUMGESTALTS];




gboolean gglk_gestalt_validate_event(glui32 evtype)
{
    switch(evtype) {
    case evtype_Timer:
	return gglk_gestalt_support[gestalt_Timer];
    case evtype_MouseInput:
	return gglk_gestalt_support[gestalt_MouseInput];
    case evtype_SoundNotify:
	return gglk_gestalt_support[gestalt_SoundNotify];
    case evtype_Hyperlink:
	return gglk_gestalt_support[gestalt_HyperlinkInput];
    }
    return TRUE;
}


void gglk_init_gestalt(void)
{
    int i;
    gglk_gestalt_version = 0x0601;
    for(i = 1; i < gestalt_NUMGESTALTS; i++)
	gglk_gestalt_possible[i] = TRUE;

    gglk_gestalt_possible[gestalt_Sound] =
	gglk_gestalt_possible[gestalt_SoundVolume] =
	gglk_gestalt_possible[gestalt_SoundMusic] =
	gglk_gestalt_possible[gestalt_SoundNotify] = gglk_sound_available;

    for(i = 0; i < gestalt_NUMGESTALTS; i++)
	gglk_gestalt_claim[i]=gglk_gestalt_support[i]=gglk_gestalt_possible[i];
}


static gboolean gglk_gestalt_table_query(glui32 gestalt)
{
    if(!gglk_gestalt_claim[gestalt])
	return FALSE;
    /* Pretend is claim=true and support=false */
    if(!gglk_gestalt_support[gestalt])
	return TRUE;
    /* Tell the truth */
    return gglk_gestalt_possible[gestalt];
}


glui32 glk_gestalt_ext(glui32 sel, glui32 val, glui32 *arr, glui32 arrlen)
{
    glui32 foo;
    if(arrlen == 0 || arr == NULL) {
	arr = &foo;
	arrlen = 1;
    }
    
    switch(sel) {
    case gestalt_Version:
	return gglk_gestalt_version;
	
    case gestalt_CharInput: 
    {
	GdkKeymapKey *keys = NULL;
	gint n_keys;
	return gdk_keymap_get_entries_for_keyval(NULL,
						 gglk_keycode_glk_to_gdk(val),
						 &keys, &n_keys);
    }
    case gestalt_LineInput:
	return g_unichar_isdefined(val);

    case gestalt_CharOutput:
	if(val <= 255) {
	    if(val < ' ' || (val > '~' && val < 160)) {
		*arr = 0;
		return gestalt_CharOutput_CannotPrint;
	    } else {
		*arr = 1;
		return gestalt_CharOutput_ExactPrint;
	    }
	} else {
	    if(!g_unichar_isdefined(val)) {
		*arr = 0;
		return gestalt_CharOutput_CannotPrint;
	    }
	    if(g_unichar_iswide(val)) {
		*arr = 2;
		return gestalt_CharOutput_ApproxPrint;
	    }
	    
	}

    case gestalt_MouseInput:
	if(!gglk_gestalt_table_query(gestalt_MouseInput))
	    return FALSE;
	return (val == wintype_Graphics) || (val == wintype_TextGrid);

    case gestalt_DrawImage:
	if(!gglk_gestalt_table_query(gestalt_DrawImage))
	    return FALSE;
	return (val == wintype_Graphics) || (val == wintype_TextBuffer);

    case gestalt_HyperlinkInput:
	if(!gglk_gestalt_table_query(gestalt_HyperlinkInput))
	    return FALSE;
	return (val == wintype_TextGrid) || (val == wintype_TextBuffer);
	
    default:
	if(sel <= gestalt_GraphicsTransparency)
	    return gglk_gestalt_table_query(sel);
	return 0;
    }
}

