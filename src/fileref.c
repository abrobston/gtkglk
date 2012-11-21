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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "gglk.h"
#include "glkstart.h"



struct glk_fileref_struct {
    glui32 magic, rock, age, type;
    GList *node;
    gidispatch_rock_t disprock;

    glui32 usage;
    glui32 orig_fmode;

    char *filename;
};


#define gidisp_Class_fileref gidisp_Class_Fileref
MAKE_ROCK(frefid_t, fileref, gglk_validfref)
MAKE_ITER(frefid_t, fileref)

static char *working_dir;

/* default filenames for each usage */
static char *usage_name[4];

#include "gui_fileref.c"

void gglk_init_fref(void)
{
    working_dir = g_strdup(".");
    usage_name[fileusage_Data]        = g_strdup("file.dat");
    usage_name[fileusage_SavedGame]   = g_strdup("game.sav");
    usage_name[fileusage_Transcript]  = g_strdup("script.txt");
    usage_name[fileusage_InputRecord] = g_strdup("commands.rec");
}


static frefid_t gglk_create_fref(glui32 usage, glui32 rock, const char *name,
				 glui32 fmode)
{
    frefid_t fref   = g_malloc(sizeof(struct glk_fileref_struct));
    fref->magic     = GGLK_MAGIC_FREF;
    fref->rock      = rock;
    fref->age       = gglk_age++;
    fref->type      = 0;
    fref->usage     = usage;
    fref->orig_fmode= fmode;

    fref->filename = g_strdup(name);

    fref->node      = gglk_fileref_add(fref);

    return fref;
}

gboolean gglk_valid_fmode(glui32 fmode)
{
    switch(fmode) {
    case filemode_Write: return TRUE;
    case filemode_Read:  return TRUE;
    case filemode_ReadWrite: return TRUE;
    case filemode_WriteAppend: return TRUE;
    }
    g_warning("%s: bad fmode %d", __func__, fmode);
    return FALSE;
}


static char *string_from_usage(glui32 usage, glui32 mode)
{
    static const char modes[]=
	"Writing" "\0" "Reading" "\0" "Modifying" "\0" "Appending" ;
    static const char uses[] =
	"Data" "\0" "Saved Game" "\0" "Transcript" "\0" "Input Record" ;
    static const short int modes_offs[] = { 7, 0, 8, 16, 7, 26 };
    static const short int uses_offs[] = { 0, 5, 16, 27 };
    
    const char *mode_string = "";
    const char *use_string;
    
    if(mode < 6)
	mode_string = modes + modes_offs[mode];
    if(!*mode_string)
	mode_string = "**UNKNOWN MODE**";
    
    if((usage & fileusage_TypeMask) < 4)
	use_string = uses + uses_offs[usage & fileusage_TypeMask];
    else
	use_string = "Unknown Type";

    return g_strconcat(use_string, " for ", mode_string, NULL);
}


frefid_t glk_fileref_create_temp(glui32 usage, glui32 rock)
{
    char *filename = NULL;
    /* FIXME */
    return gglk_create_fref(usage, rock, filename, filemode_ReadWrite);
}


frefid_t glk_fileref_create_by_name(glui32 usage, char *name, glui32 rock)
{
    if(*name == 0) {
	return NULL;
    } else {
	frefid_t f;
	char *s = g_strconcat(working_dir, "/", name, NULL);
	char *p;
	/* Filename may contain only letters, numbers, and hyphens */
	for(p = s + strlen(working_dir) + 1; *p; p++)
	    if(!g_ascii_isalnum(*p))
		*p = '-';

	f = gglk_create_fref(usage, rock, s, filemode_Read);
	g_free(s);
	return f;
    }
}



frefid_t glk_fileref_create_by_prompt(glui32 usage, glui32 fmode, glui32 rock)
{
    frefid_t fref = NULL;
    char *title;
    GtkWidget *dialog;
    glui32 usage_type;
    if(!gglk_valid_fmode(fmode))
	return NULL;

    usage_type = usage & fileusage_TypeMask;
    if(usage_type > fileusage_InputRecord)
	return NULL;
    
    title  = string_from_usage(usage, fmode);
    dialog = gtk_file_selection_new(title);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog),
				    usage_name[usage_type]);
    
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	const char *osname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialog));
	g_free(usage_name[usage_type]);
	usage_name[usage_type] = g_strdup(osname);
	
	fref = gglk_create_fref(usage, rock, osname, fmode);
	/*char *utf8name = g_filename_to_utf8(osname, -1, NULL, NULL, NULL);
	  char *filename = g_convert(utf8name, -1, "UTF8", */
    }
    gtk_widget_destroy(dialog);
    g_free(title);
    return fref;
}


frefid_t glk_fileref_create_from_fileref(glui32 usage, frefid_t fref, glui32 rock)
{
    frefid_t f;
    gglk_validfref(fref, return NULL);

    f = gglk_create_fref(usage, rock, NULL, fref->orig_fmode);
    f->filename = g_strdup(fref->filename);
    
    return f;
}



void glk_fileref_destroy(frefid_t fref)
{
    gglk_validfref(fref, return);

    gglk_fileref_del(fref);
    fref->magic = GGLK_MAGIC_FREE;

    g_free(fref->filename);
    g_free(fref);
}


void glk_fileref_delete_file(frefid_t fref)
{
    /* I have no idea why a Glk game would need to delete files, but ah well */
    gglk_validfref(fref, return);

    /* If they don't have permission to write to the file, ask permission
       before deleting it */
    if(fref->orig_fmode == filemode_Read) {
	GtkWidget *dialog;
	GtkResponseType response;
	dialog = gtk_message_dialog_new(GTK_WINDOW(gglk_win),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_YES_NO,
					"Delete File %s ?",
					fref->filename);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(response != GTK_RESPONSE_YES)
	    return;
    }
    
    unlink(fref->filename);
}


glui32 glk_fileref_does_file_exist(frefid_t fref)
{
    struct stat s;
    gglk_validfref(fref, return 0);

    if(!fref->filename)
	return 0;
    if(stat(fref->filename, &s) == -1)
	return 0;
    return S_ISREG(s.st_mode) != 0;
}


strid_t glk_stream_open_file(frefid_t fref, glui32 fmode, glui32 rock)
{
    gglk_validfref(fref, return NULL);
    if(!gglk_valid_fmode(fmode))
	return NULL;

    return gglk_strid_from_filename(fref->filename, fref->usage,
				    fmode, rock, fref->orig_fmode);
}


void glkunix_set_base_file(char *filename)
{
    int i;
    char *s, *p, *q;
    g_free(working_dir);
    s = g_strdup(filename);
    if((p = strrchr(s, '/')) != 0) {
	*p++ = '\0';
	working_dir = g_strdup(s);
    } else {
	p = s;
	working_dir = g_strdup(".");
    }

    if((q = strrchr(p, '.')) != 0)
	*q = '\0';

    for(i = 0; i < 4; i++) {
	static const char extensions[4][4] = { "dat", "sav", "txt", "rec" };
	
	g_free(usage_name[i]);

	usage_name[i] = g_strconcat(working_dir, "/", p, ".", extensions[i],
				    NULL);
    }
    
    g_free(s);
}
