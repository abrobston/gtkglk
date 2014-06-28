#include "glk.h"
#include "sglk.h"
#include <string.h>

/* Trivial functions which should be identical on every unix-like Glk */

/* TODO: investigate static-linking these into the application
         (should slightly improve performance, decrease size)  */


int main(int argc, char *argv[]) __attribute__ ((weak));

int main(int argc, char *argv[])
{
    glkunix_startup_t data;
    
    if(!sglk_init(argc, argv, &data))
	return 1;
    
    glkunix_startup_code(&data);
    glk_main();
    glk_exit();

    return 0;
}

void glk_tick(void) { ; }

unsigned char glk_char_to_lower(unsigned char ch)
{
    if((ch >= 'A' && ch <= 'Z') || (ch >= 0xc0 && ch <= 0xde && ch != 0xd7))
	return ch + 32;
    return ch;
}
unsigned char glk_char_to_upper(unsigned char ch)
{
    if((ch >= 'a' && ch <= 'z') || (ch >= 0xe0 && ch <= 0xfe && ch != 0xf7))
	return ch - 32;
    return ch;
}


/* Glk functions which use the global "default stream" */
static strid_t cur_str;
void glk_stream_set_current(strid_t str) { cur_str = str; }
strid_t glk_stream_get_current(void) { return cur_str; }
void glk_set_window(winid_t win)
{
    glk_stream_set_current(win ? glk_window_get_stream(win) : NULL);
}
void glk_put_char(unsigned char ch) { glk_put_char_stream(cur_str, ch); }
void glk_put_string(char *s)        { glk_put_string_stream(cur_str, s); }
void glk_put_buffer(char *buf, glui32 len) { glk_put_buffer_stream(cur_str, buf, len); }
void glk_put_char_ucs4(glui32 ch)   { glk_put_char_stream_ucs4(cur_str, ch); }
void glk_put_string_ucs4(glui32 *s) { glk_put_string_stream_ucs4(cur_str, s); }
void glk_put_buffer_ucs4(glui32 *buf, glui32 len) { glk_put_buffer_stream_ucs4(cur_str, buf, len); }
void glk_set_style(glui32 styl)     { glk_set_style_stream(cur_str, styl); }
void glk_set_hyperlink(glui32 linkval) { glk_set_hyperlink_stream(cur_str, linkval); }


/* Translate character-oriented functions to buffer-oriented ones */
glsi32 glk_get_char_stream(strid_t str)
{
    char c;
    if(glk_get_buffer_stream(str, &c, 1) == 1)
	return (unsigned char) c;
    return -1;
}

glsi32 glk_get_char_stream_ucs4(strid_t str)
{
    glui32 c;
    if(glk_get_buffer_stream_ucs4(str, &c, 1) == 1)
	return c;
    return -1;
}

void glk_put_char_stream(strid_t str, unsigned char ch)
{
    glk_put_buffer_stream(str, &ch, 1);
}

void glk_put_char_stream_ucs4(strid_t str, glui32 ch)
{
    glk_put_buffer_stream_ucs4(str, &ch, 1);
}

void glk_put_string_stream(strid_t str, char *s)
{
    glk_put_buffer_stream(str, s, strlen(s));
}

void glk_put_string_stream_ucs4(strid_t str, glui32 *s)
{
    int len;
    for(len = 0; s[len]; len++)
	continue;
    glk_put_buffer_stream_ucs4(str, s, len);
}

glui32 glk_get_line_stream(strid_t str, char *buf, glui32 len)
{
    glui32 readlen;

    if(len <= 0)
	return 0;
    
    len--;
    for(readlen = 0; readlen < len && buf[readlen] != '\n'; readlen++) {
	if(glk_get_buffer_stream(str, buf + readlen, 1) != 1)
	    break;
    }
    buf[readlen] = '\0';
    return readlen;
}

glui32 glk_get_line_stream_ucs4(strid_t str, glui32 *buf, glui32 len)
{
    glui32 readlen;

    if(len <= 0)
	return 0;
    
    len--;
    for(readlen = 0; readlen < len && buf[readlen] != '\n'; readlen++) {
	if(glk_get_buffer_stream_ucs4(str, buf + readlen, 1) != 1)
	    break;
    }
    buf[readlen] = 0;
    return readlen;
}


glui32 glk_schannel_play(schanid_t chan, glui32 snd)
{
    return glk_schannel_play_ext(chan, snd, 1, 0);
}

glui32 glk_gestalt(glui32 sel, glui32 val)
{
    return glk_gestalt_ext(sel, val, NULL, 0);
}

