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

/* SDL_mixer implementation of sound for GtkGlk. */

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <unistd.h>

#include "gglk.h"
#include "gi_blorb.h"
#include "sound_dummy.h"

const gboolean gglk_sound_available = TRUE;


struct glk_schannel_sdl {
    int channel;

    glui32 snd;
    glui32 notify;

    glui32 chunknum;


    Mix_Chunk *chunk;
    Mix_Music *music;
};

static struct glk_schannel_sdl *chans[MIX_CHANNELS];

static void gglk_free_stuff(struct glk_schannel_sdl *chan)
{
    if(chan->music) {
	Mix_FreeMusic(chan->music);
	chan->music = 0;
    }
    if(chan->chunk) {
	Mix_FreeChunk(chan->chunk);
	chan->chunk = 0;
    }
}


static void channel_notify(int channel)
{
    struct glk_schannel_sdl *chan = chans[channel];

    if(chan->notify && glk_gestalt(gestalt_SoundNotify, 0)) {
	gglk_trigger_event(evtype_SoundNotify, NULL, chan->snd, chan->notify);
    }
    gglk_free_stuff(chan);
}



static void music_notify(void)
{
    int i;
    for(i = 0; i < MIX_CHANNELS; i++) {
	if(chans[i] && chans[i]->music) {
	    char buf[50];
	    channel_notify(i);
	    sprintf(buf, "music channel %d finished\n", i);
	    sglk_status_set_mesg(buf);
	    break;
	}
    }
}


static int gglk_SDL_seek(struct SDL_RWops *context, int offset, int whence)
{
    strid_t str = context->hidden.unknown.data1;
    switch(whence) {
    case SEEK_SET: glk_stream_set_position(str,offset,seekmode_Start); break;
    case SEEK_CUR: glk_stream_set_position(str,offset,seekmode_Current); break;
    case SEEK_END: glk_stream_set_position(str, offset, seekmode_End); break;
    }
    return glk_stream_get_position(str);
}

static int gglk_SDL_read(struct SDL_RWops *context, void *ptr, int size, int maxnum)
{
    strid_t str = context->hidden.unknown.data1;
    return glk_get_buffer_stream(str, ptr, size * maxnum) / size;
}
		
static int gglk_SDL_write(struct SDL_RWops *context, const void *ptr, int size, int num)
{
    strid_t str = context->hidden.unknown.data1;
    glk_put_buffer_stream(str, (char *) ptr, size * num);
    return num;
}

static int gglk_SDL_close(struct SDL_RWops *context)
{
    strid_t str = context->hidden.unknown.data1;
    glk_stream_close(str, NULL);
    return 0;
}

static SDL_RWops *gglk_SDL_RWFromStr(strid_t str)
{
    SDL_RWops *rw;
    
    rw = g_malloc(sizeof(*rw));
 
    rw->seek = gglk_SDL_seek;
    rw->read = gglk_SDL_read;
    rw->write = gglk_SDL_write;
    rw->close = gglk_SDL_close;
    rw->hidden.unknown.data1 = str;

    return rw;
}


gboolean gglk_sound_init(void)
{
    g_message("Initializing SDL");
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
	return FALSE;
    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY,
		     MIX_DEFAULT_FORMAT, 2, 4096) < 0)
	return FALSE;

    Mix_ChannelFinished(channel_notify);
    Mix_HookMusicFinished(music_notify);
    return TRUE;
}

void *gglk_sound_open_channel(void)
{
    struct glk_schannel_sdl *chan = g_malloc(sizeof(*chan));
    int i;
    
    /* Find an unused channel */
    for(i = 0; i < MIX_CHANNELS; i++)
	if(!chans[i])
	    break;
    if(i == MIX_CHANNELS)
	return NULL;
    chans[i] = chan;
    chan->channel = i;
    chan->chunk = NULL;
    chan->music = NULL;
    chan->chunknum = 0;
    return chan;
}

void gglk_sound_close_channel(void *data)
{
    struct glk_schannel_sdl *chan = data;
    chans[chan->channel] = NULL;
    g_free(data);
}


gboolean gglk_sound_play(void *data, glui32 snd, glui32 repeats, glui32 notify)
{
    glui32 chunknum, chunktype;
    struct glk_schannel_sdl *chan = data;
    
    int reps = (repeats == 0xffffffff) ? -1 : (int) repeats - 1;
    strid_t str;

    str = glk_blorb_get_str(giblorb_ID_Snd, snd, &chunknum, &chunktype);
    if(!str) return 0;
    
    chan->chunknum = chunknum;
    chan->snd = snd;
    chan->notify = notify;
    

    if(chunktype == giblorb_make_id('F', 'O', 'R', 'M')) {
	SDL_RWops *s = gglk_SDL_RWFromStr(str);
	chan->chunk = Mix_LoadWAV_RW(s, 1);
    } else {
	char *name;
	int fd;
	void *p;
	int size;
	GError *error = NULL;
	fd = g_file_open_tmp("XXXXXX", &name, &error);

	glk_stream_set_position(str, 0, seekmode_End);
	size = glk_stream_get_position(str);
	glk_stream_set_position(str, 0, seekmode_Start);

	p = g_malloc(size);
	glk_get_buffer_stream(str, p, size);
	write(fd, p, size);
	close(fd);
	g_free(p);

	chan->music = Mix_LoadMUS(name);
	unlink(name);
	g_free(name);

	glk_stream_close(str, NULL);
    }
	
    if(chan->music)
	if(Mix_PlayMusic(chan->music, reps) < 0)
	    return FALSE;
    if(chan->chunk)
	if(Mix_PlayChannel(chan->channel, chan->chunk, reps) < 0)
	    return FALSE;
    return TRUE;
}

void gglk_sound_stop(void *data)
{
    struct glk_schannel_sdl *chan = data;

    chan->notify = 0;

    if(chan->music) {
	Mix_HaltMusic();
    } else {
	Mix_HaltChannel(chan->channel);
    }

    gglk_free_stuff(chan);
}

void gglk_sound_volume(void *data, glui32 vol)
{
    struct glk_schannel_sdl *chan = data;

    if(chan->music)
	Mix_VolumeMusic(vol * MIX_MAX_VOLUME / 0x10000);
    else
	Mix_Volume(chan->channel, vol * MIX_MAX_VOLUME / 0x10000);
}
