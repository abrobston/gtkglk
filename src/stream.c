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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netinet/in.h>         /* For ntohl */
#include "gglk.h"

#include "gglk_text.h"
#include "gglk_textbuffer.h"

typedef enum { strDummy, strTextBuffer, strTextGrid, strFile, strMem } strType;

struct glk_stream_struct {
    glui32 magic, rock, age, type;
    GList *node;
    gidispatch_rock_t disprock;

    glui32 fmode, usage;
    strid_t echo;
    glui32 readcount, writecount;

    gboolean is_dup; /* True if we shouldn't close the file, because someone
			else has owns it. */
    glsi32 length;   /* Total length of stream, or -1 for no maximum */
    glsi32 index;    /* Current offset into stream */
    glsi32 start;    /* Start of stream from beginning of file */
	    
    winid_t win;
    glui32 wintype;
    GglkText *gbuffer;
    
    FILE *file;

    char *mem;
    gidispatch_rock_t mem_rock;
};


#define gidisp_Class_stream gidisp_Class_Stream
MAKE_ROCK(strid_t, stream, gglk_validstr)
MAKE_ITER(strid_t, stream)



static void gglk_put_buffer_stream_ucs4(strid_t str, const glui32 *buf, glui32 len, gboolean eightbit);

#include "gui_stream.c"


static strid_t gglk_create_str(glui32 type, glui32 fmode, glui32 rock) 
{
    strid_t str = g_malloc(sizeof(struct glk_stream_struct));
    str->magic  = GGLK_MAGIC_STR;
    str->rock   = rock;
    str->age    = gglk_age++;
    str->type   = type;

    str->fmode  = fmode;
    str->usage  = fileusage_Data | fileusage_BinaryMode;
    str->echo   = NULL;    
    str->readcount = str->writecount = 0;

    str->win = 0;
    str->wintype = 0;
    str->gbuffer = NULL;
    str->file = NULL;
    str->mem = NULL;
    str->start = str->index = 0;
    str->is_dup = FALSE;
    str->length = -1;
    str->mem_rock = dispa_bad;
    
    str->node = gglk_stream_add(str);

    return str;
}


/* Graphics windows have dummy streams */
strid_t gglk_strid_dummy(void)
{
    return gglk_create_str(strDummy, filemode_Write, 0);
}


/* Creates a new strid_t from a section of an old strid_t.
   Not fully implemented.
   Used to create a stream from a blorb chunk, which is wrapped by SDL stuff...
     see sound_sdl.c and  blorb.c */
strid_t gglk_strid_from_strid(strid_t src, glui32 start, glui32 length)
{
    strid_t str = gglk_create_str(src->type, src->fmode, src->rock);
    str->is_dup = TRUE;
    
    switch(src->type) {
    case strDummy:
    case strTextBuffer:
    case strTextGrid:
	break;

    case strFile:
	str->file = src->file;
	str->length = length;
	str->start = start;
	str->index = 0;
	break;

    case strMem:
	str->mem = src->mem + start;
	str->length = length;
	str->index = 0;
	break;
    }

    return str;
}



strid_t gglk_strid_from_gglk_text(GglkText *gbuffer,
				  winid_t win, glui32 wintype)
{
    strType type = (wintype==wintype_TextBuffer) ? strTextBuffer : strTextGrid;
    strid_t str = gglk_create_str(type, filemode_Write, 0);

    str->win = win; str->wintype = wintype;
    str->gbuffer = gbuffer;
    g_object_set_data(G_OBJECT(str->gbuffer), "strid", str);
    glk_set_style_stream(str, style_Normal);
    return str;
}



strid_t gglk_strid_from_filename(const char *name,
				 glui32 usage, glui32 fmode, glui32 rock,
				 glui32 orig_fmode)
{
    strid_t str = gglk_create_str(strFile, fmode, rock);
    char cmode[6] = "";

    str->usage = usage;

    switch(fmode) {
    case filemode_Write:       g_strlcpy(cmode, "w", 5); break;
    case filemode_Read:        g_strlcpy(cmode, "r", 5); break;
    case filemode_ReadWrite:   g_strlcpy(cmode, "r+", 5); break;
    case filemode_WriteAppend: g_strlcpy(cmode, "a", 5);
    }
    if(!(usage & fileusage_TextMode)) g_strlcat(cmode, "b", 5);

    /* If they opened a file in write mode but didn't specifically get
       permission to do so, complain if the file already exists */
    if(orig_fmode == filemode_Read && fmode != orig_fmode) {
	struct stat buf;
	if(stat(name, &buf) != -1) {
	    GtkWidget *dialog;
	    GtkResponseType response;
	    dialog = gtk_message_dialog_new(GTK_WINDOW(gglk_win),
					    GTK_DIALOG_MODAL,
					    GTK_MESSAGE_WARNING,
					    GTK_BUTTONS_YES_NO,
					 "File %s already exists.  Overwrite?",
					    name);
	    response = gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    if(response != GTK_RESPONSE_YES) {
		glk_stream_close(str, NULL);
		return NULL;
	    }
	}
    }    

    str->file = fopen(name, cmode);

    if(!str->file) {
	glk_stream_close(str, NULL);
	return NULL;
    }
    return str;
}

strid_t glkunix_stream_open_pathname(char *pathname, glui32 textmode, 
				     glui32 rock)
{
    /* Intentionally doesn't check to make sure we're still in startup because:
       1) I haven't implemented startup stuff yet
       2) This is so much nicer than standard glk file ops.
    */
    return gglk_strid_from_filename(pathname, textmode, filemode_Read, rock, 
				    filemode_Read);
}


strid_t glk_stream_open_memory(char *buf, glui32 buflen, glui32 fmode,
			       glui32 rock)
{
    strid_t str = gglk_create_str(strMem, fmode, rock);
    if(!gglk_valid_fmode(fmode))
	return NULL;

    if(buf == NULL || buflen == 0) {
	buf = NULL;
	buflen = 0;
    }    
    str->mem = buf;
    str->length = buflen;
    str->index = 0;
    str->mem_rock = gglk_dispa_newmem(str->mem, str->length, "&+#!Cn");
    return str;
}



void glk_stream_close(strid_t str, stream_result_t *result)
{
    strid_t estr;

    gglk_validstr(str, return);

    if(result) {
	result->readcount = str->readcount;
	result->writecount = str->writecount;
    }

    if(glk_stream_get_current() == str)
	glk_stream_set_current(NULL);

    if(str->file && !str->is_dup)
	fclose(str->file);

    if(str->mem && !str->is_dup)
	gglk_dispa_endmem(str->mem, str->length, "&+#!Cn", str->mem_rock);

    gglk_stream_del(str);
    str->magic = GGLK_MAGIC_FREE;

    estr = NULL;
    while((estr = glk_stream_iterate(estr, NULL)) != NULL) {
	if(estr->echo == str)
	    gglk_stream_set_echo(estr, NULL);
    }
    

    g_free(str);
}



void glk_stream_set_position(strid_t str, glsi32 pos, glui32 seekmode)
{
    GGLK_STR_TYPES(str, return, strFile, strMem);
    switch(seekmode) {
    case seekmode_Start:   str->index = pos; break;
    case seekmode_Current: str->index += pos; break;
    case seekmode_End:
	if(str->length == -1) {
	    fseek(str->file, pos, SEEK_END);
	    str->index = ftell(str->file) - str->start;
	} else {
	    str->index = str->length + pos;
	}
	break;
    default: g_warning("%s: bad seekmode", __func__); return;
    }
    if(str->index < 0)
	str->index = 0;
    if(str->length != -1 && str->index > str->length)
	str->index = str->length;
}


glui32 glk_stream_get_position(strid_t str)
{
    GGLK_STR_TYPES(str, return 0, strFile, strMem);
    return str->index;
}


void gglk_stream_set_echo(strid_t str, strid_t echo)
{
    strid_t estr;
    gglk_validstr(str, return);
    if(echo != NULL) {
	gglk_validstr(echo, return);
    }
    
    for(estr=echo; estr; estr=estr->echo) {
	if(estr == str) {
	    g_warning("trying to create an echo loop");
	    return;
	}
    }

    str->echo = echo;
}

strid_t gglk_stream_get_echo(strid_t str)
{
    gglk_validstr(str, return NULL);
    return str->echo;
}

static gboolean gglk_stream_check_read(strid_t str, const char *func)
{
    if(!(str->fmode & filemode_Read)) {
	g_warning("%s: read permission denied", func);
	return FALSE;
    }
    return TRUE;
}
static gboolean gglk_stream_check_write(strid_t str, const char *func)
{
    if(!(str->fmode & filemode_Write)) {
	g_warning("%s: write permission denied", func);
	return FALSE;
    }
    return TRUE;
}


static glui32 gglk_get_buffer_stream_ucs4(strid_t str, glui32 len,
						 glui32 * restrict buf32,
						 unsigned char * restrict buf8)
{
    if(str->length != -1) {
	if(buf8) {
	    if(str->index + len >= (glui32) str->length)
		len = str->length - str->index;
	} else {
	    glui32 bytelen = len * 4;
	    if(str->index + bytelen >= (glui32) str->length) {
		bytelen = str->length - str->index;
		len = bytelen / 4;
	    }
	}
    }
    str->readcount += len;

    switch(str->type) {
    case strFile:
	fseek(str->file, str->start + str->index, SEEK_SET);
	if(str->usage & fileusage_TextMode) {
	    /* FIXME: use UTF-8 */
	    if(buf8)
		len = fread(buf8, 1, len, str->file);
	    else
		; /* FIXME */
	} else if(buf8) {
	    len = fread(buf8, 1, len, str->file);
	} else {
	    glui32 i;
	    len = fread(buf32, 4, len, str->file);
	    for(i = 0; i < len; i++)
		buf32[i] = ntohl(buf32[i]);
	}
	str->index = ftell(str->file) - str->start;
	break;
    case strMem:
	if(buf8) {
	    memmove(buf8, str->mem + str->index, len);
	    str->index += len;
	} else {
	    glui32 i;
	    memmove(buf32, str->mem + str->index, len * 4);
	    for(i = 0; i < len; i++)
		buf32[i] = ntohl(buf32[i]);
	    str->index += len * 4;
	}
	break;
    }

    return len;
}
glui32 glk_get_buffer_stream(strid_t str, char *buf, glui32 len)
{
    GGLK_STR_TYPES(str, return 0, strFile, strMem);
    if(!gglk_stream_check_read(str, __func__)) return 0;
    return gglk_get_buffer_stream_ucs4(str, len, NULL, buf);
}

glui32 glk_get_buffer_stream_ucs4(strid_t str, glui32 *buf, glui32 len)
{
    GGLK_STR_TYPES(str, return 0, strFile, strMem);
    if(!gglk_stream_check_read(str, __func__)) return 0;
    return gglk_get_buffer_stream_ucs4(str, len, buf, NULL);
}


void glk_put_buffer_stream(strid_t str, char *buf, glui32 len)
{
    glui32 *unistr;
    glui32 i;

    gglk_validstr(str, return);
    if(!gglk_stream_check_write(str, __func__)) return;

    /* Convert the buffer to an array of glui32s.

       **TODO**: If profiling shows that a significant amount of time
       is spent here, consider improving this to not do the conversion
       if writing to mem/file. */

    unistr = g_malloc(sizeof(*unistr) * len);
    for(i = 0; i < len; i++)
	unistr[i] = (unsigned char) buf[i];

    gglk_put_buffer_stream_ucs4(str, unistr, len, TRUE);
    g_free(unistr);
}



void glk_put_buffer_stream_ucs4(strid_t str, glui32 *buf, glui32 len)
{
    gglk_validstr(str, return);
    if(!gglk_stream_check_write(str, __func__)) return;

    gglk_put_buffer_stream_ucs4(str, buf, len, FALSE);
}



/* assumes gglk_validstr and gglk_stream_check_write have already been done.
   eightbit is TRUE if the output should be done one byte per character -
      need to keep track of eightbit, because glk_put_buffer() should write
      one byte to binary streams, and glk_put_bufer_ucs4() should write four */
static void gglk_put_buffer_stream_ucs4(strid_t str,
					const glui32 * restrict buf,
					glui32 len, gboolean eightbit)
{
    glui32 i, j;
    char *utfstr;

    /* Update the writecount by the number of characters *attempted* to
       be written, not the actual number written */
    str->writecount += len;

    if(str->echo)
	gglk_put_buffer_stream_ucs4(str->echo, buf, len, eightbit);

    if(str->length != -1) {
	if(eightbit) {
	    if(str->index + len >= (glui32) str->length)
		len = str->length - str->index;
	} else {
	    glui32 bytelen = len * 4;
	    if(str->index + bytelen >= (glui32) str->length) {
		bytelen = str->length - str->index;
		len = bytelen / 4;
	    }
	}
    }

    switch(str->type) {
    case strFile:
	fseek(str->file, str->start + str->index, SEEK_SET);
	if(str->usage & fileusage_TextMode) {  /* for text, UTF-8 */
	    utfstr = g_ucs4_to_utf8(buf, len, NULL, NULL, NULL);
	    fwrite(utfstr, 1, strlen(utfstr), str->file);
	    g_free(utfstr);
	} else if(eightbit) {    /* for 8-bit binary, One byte per character */
	    for(i = 0; i < len; i++)
		fputc(buf[i] & 0xff, str->file);
	} else {                 /* for 32-bit binary, UTF-32BE */
	    for(i = 0; i < len; i++) {
		fputc((buf[i] & 0xff000000) >> 24, str->file);
		fputc((buf[i] & 0x00ff0000) >> 16, str->file);
		fputc((buf[i] & 0x0000ff00) >>  8, str->file);
		fputc((buf[i] & 0x000000ff) >>  0, str->file);
	    }
	}
	str->index = ftell(str->file) - str->start;
	break;
    case strMem:
	j = str->index;
	if(eightbit) {
	    for(i = 0; i < len; i++)
		str->mem[j++] = buf[i] & 0xff;
	} else {
	    for(i = 0; i < len; i++) {
		str->mem[j++] = (buf[i] & 0xff000000) >> 24;
		str->mem[j++] = (buf[i] & 0x00ff0000) >> 16;
		str->mem[j++] = (buf[i] & 0x0000ff00) >>  8;
		str->mem[j++] = (buf[i] & 0x000000ff) >>  0;
	    }
	}
	str->index = j;
	break;

    case strTextGrid:
    case strTextBuffer:
	gglk_text_put_buffer(str->gbuffer, buf, len);
	str->index += len;
    }
        
}


void glk_set_style_stream(strid_t str, glui32 styl)
{
    gglk_validstr(str, return);
    if(str->echo)
	glk_set_style_stream(str->echo, styl);
    if(str->gbuffer && styl < style_NUMSTYLES) {
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(str->gbuffer));
	gglk_text_set_style(str->gbuffer, gtk_text_tag_table_lookup(
				gtk_text_buffer_get_tag_table(buffer),
				gglk_get_tag(styl)));
    }
}


/* FIXME: this doesn't follow the description of sglk_set_style in
   DorianX's dosglk.  Check to see what nitfol needs before fixing
   this up. */
void sglk_set_style(strid_t str, glui32 fg, glui32 bg, glui32 attr)
{
    GtkTextTagTable *table;
    GtkTextTag *sglk_tag;
    GtkTextView *view;
    GtkTextBuffer *buffer;
    char *sglk_tag_name;
    gglk_validstr(str, return);
    
    if(str->echo)
	sglk_set_style(str->echo, fg, bg, attr);

    if(fg == 0xffffffff && bg == 0xffffffff && attr == 0) {
	glk_set_style_stream(str, style_Normal);
	return;
    }
    if(!str->gbuffer)
	return;
    
    view = GTK_TEXT_VIEW(str->gbuffer);
    buffer = gtk_text_view_get_buffer(view);
    table = gtk_text_buffer_get_tag_table(buffer);

    /* If we created new tags every time someone called this function,
       we would soon have thousands of them, mostly duplicates.  So
       put a description of the tag in the name of the tag, and see
       if the same description has been used before... */

    sglk_tag_name = g_strdup_printf("sglk#%06x:#%06x,%d",
				    (unsigned int) fg, (unsigned int) bg,
				    (unsigned int) attr);
    sglk_tag = gtk_text_tag_table_lookup(table, sglk_tag_name);
    if(!sglk_tag) {
	sglk_tag = gtk_text_tag_new(sglk_tag_name);

	/* You might look at this and ask "Why didn't he just use
	   glk_stylehint_set?" and that would be a reasonable question
	   given that I'm only using standand Glk stylehints.  But with
	   this, 1) I don't have to know which styles I'll be using before
	   I open a window, and 2) I'm not limited by the number of styles
	   Glk provides.  This is necessary for nitfol to emulate the
	   Z-machine's screen model in a reasonable way. */

	if(fg != 0xffffffff)
	    gglk_adjust_tag(sglk_tag, stylehint_TextColor, fg);
	if(bg != 0xffffffff)
	    gglk_adjust_tag(sglk_tag, stylehint_BackColor, bg);
	if(attr & 1)  /* BOLD */
	    gglk_adjust_tag(sglk_tag, stylehint_Weight, 1);
	if(attr & 2)  /* ITALIC */
	    gglk_adjust_tag(sglk_tag, stylehint_Oblique, 1);
	if(attr & 4)  /* FIXED */
	    gglk_adjust_tag(sglk_tag, stylehint_Proportional, 0);
	if(attr & 8)  /* REVERSE */
	    gglk_adjust_tag(sglk_tag, stylehint_ReverseColor, 1);
	if(attr & 16) /* DIM */
	    gglk_adjust_tag(sglk_tag, stylehint_Weight, -1);

	gtk_text_tag_table_add(table, sglk_tag);
    }
    g_free(sglk_tag_name);

    gglk_text_set_style(str->gbuffer, sglk_tag);
}



void glk_set_hyperlink_stream(strid_t str, glui32 linkval)
{
    /* Plan: keep a mark at where the hyperlink starts, and when the
       hyperlink ends, apply the tag to the region. */

    gglk_validstr(str, return);

    if(!gglk_gestalt_support[gestalt_Hyperlinks])
	return;

    if(str->gbuffer)
	gglk_text_set_hyperlink(str->gbuffer, linkval);
}
