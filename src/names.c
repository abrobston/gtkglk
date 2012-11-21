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

const char *gglk_magic_to_typename(glui32 magic)
{
    switch(magic) {
    case GGLK_MAGIC_WIN: return "winid_t";
    case GGLK_MAGIC_STR: return "strid_t";
    case GGLK_MAGIC_FREF:return "frefid_t";
    case GGLK_MAGIC_SCHN:return "schanid_t";
    default: return "monkey_t";
    }
}

const char *gglk_subtype_names(glui32 magic, glui32 type)
{
    static const char window_types[] =
	"None" "\0" "Pair" "\0" "Blank" "\0" "TextBuffer" "\0"
	"TextGrid" "\0" "Graphics" ;
    static const char stream_types[] =
	"Dummy" "\0" "TextBuffer" "\0" "TextGrid" "\0" "File" "\0" "Memory" ;
    static const short int window_offs[] = { 0, 5, 10, 16, 27, 36 };
    static const short int stream_offs[] = { 0, 6, 17, 26, 31 };

    switch(magic) {
    case GGLK_MAGIC_WIN: if(type < 6) return window_types + window_offs[type];
    case GGLK_MAGIC_STR: if(type < 5) return stream_types + stream_offs[type];
    }
    return gglk_magic_to_typename(magic);
}


const char *gglk_gestalt_get_name(glui32 g)
{
    static const char names[] =
	"Version" "\0"              "CharInput" "\0"
	"LineInput" "\0"            "CharOutput" "\0"
	"MouseInput" "\0"           "Timer" "\0"
	"Graphics" "\0"             "DrawImage" "\0"
	"Sound" "\0"                "SoundVolume" "\0"
	"SoundNotify" "\0"          "Hyperlinks" "\0"
	"HyperlinkInput" "\0"       "SoundMusic" "\0"
	"GraphicsTransparency" "\0" "Unicode" ;

    static const short int offs[] = {
	0, 8, 18, 28, 39, 50, 56, 65, 75, 81, 93, 105, 116, 131, 142, 163
    };
    
    if(g <= gestalt_Unicode)
	return names + offs[g];
    return NULL;
}

const char *gglk_get_tag(glui32 style)
{
    static const char names[] =
	"Normal" "\0"       "Emphasized" "\0"
	"Preformatted" "\0" "Header" "\0"
	"Subheader" "\0"    "Alert" "\0"
	"Note" "\0"         "BlockQuote" "\0"
	"Input" "\0"        "User1" "\0"
	"User2" "\0"        "TextGrid" "\0"
	"Hyperlinks" "\0"   "All Styles" "\0"
	"sglk" ;
    static const short int offs[] = {
	0, 7, 18, 31, 38, 48, 54, 59, 70, 76, 82, 88, 97, 108, 111
    };

    if(style < GGLK_NUMSTYLES)
	return names + offs[style];
    return NULL;
}

const char *gglk_get_hint(glui32 hint)
{
    static const char names[] =
	"Indentation" "\0"   "ParaIndentation" "\0"
	"Justification" "\0" "Size" "\0"
	"Weight" "\0"        "Oblique" "\0"
	"Proportional" "\0"  "TextColor" "\0"
	"BackColor" "\0"     "ReverseColor" "\0"
	"Direction" "\0"     "Underline" "\0"
	"Variant" ;
    static const short int offs[] = {
	0, 12, 28, 42, 47, 54, 62, 75, 85, 95, 108, 118, 128
    };
    
    if(hint < stylehint_NUMHINTS + 2)
	return names + offs[hint];
    return NULL;
}
