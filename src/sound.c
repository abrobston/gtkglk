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

#include "sound_dummy.h"

static gboolean sound_initialized = FALSE;

struct glk_schannel_struct {
    glui32 magic, rock, age, type;
    GList *node;
    gidispatch_rock_t disprock;

    glui32 snd, repeats, notify, volume;
    void *player_data;
};

#define gidisp_Class_schannel gidisp_Class_Schannel
MAKE_ROCK(schanid_t, schannel, gglk_validschn)
MAKE_ITER(schanid_t, schannel)


schanid_t glk_schannel_create(glui32 rock)
{
    schanid_t chan;
    void *player_data;

    if(!gglk_gestalt_claim[gestalt_Sound]) return NULL;

    if(gglk_gestalt_support[gestalt_Sound] && !sound_initialized) {
	if(!gglk_sound_init())
	    return NULL;
	sound_initialized = TRUE;
    }

    if(gglk_gestalt_support[gestalt_Sound]) {
	player_data = gglk_sound_open_channel();
	if(!player_data)
	    return NULL;
    } else {
	player_data = NULL;
    }

    chan         = g_malloc(sizeof(struct glk_schannel_struct));
    chan->player_data = player_data;
    chan->magic  = GGLK_MAGIC_SCHN;
    chan->rock   = rock;
    chan->age    = gglk_age++;
    chan->type   = 0;
 
    chan->node   = gglk_schannel_add(chan);

    chan->snd = chan->repeats = chan->notify = 0;
    chan->volume = 0x10000;

    return chan;
}

void glk_schannel_destroy(schanid_t chan)
{
    gglk_validschn(chan, return);


    glk_schannel_stop(chan);

    gglk_sound_close_channel(chan->player_data);

    gglk_schannel_del(chan);
    chan->magic = GGLK_MAGIC_FREE;

    g_free(chan);
}


glui32 glk_schannel_play_ext(schanid_t chan, glui32 snd, glui32 repeats,
			     glui32 notify)
{
    if(!gglk_gestalt_claim[gestalt_Sound]) return 0;
    if(!gglk_gestalt_support[gestalt_Sound]) return 1;

    gglk_validschn(chan, return 0);
    glk_schannel_stop(chan);
    if(repeats == 0)
	return 1;
    chan->snd = snd;
    chan->repeats = repeats;
    chan->notify = notify;

    if(!gglk_sound_play(chan->player_data, snd, repeats, notify))
	return 0;

    return 1;
}

void glk_schannel_stop(schanid_t chan)
{
    gglk_validschn(chan, return);
    chan->snd = chan->repeats = chan->notify = 0;
    gglk_sound_stop(chan->player_data);
}

void glk_schannel_set_volume(schanid_t chan, glui32 vol)
{
    gglk_validschn(chan, return);
    chan->volume = vol;
    gglk_sound_volume(chan->player_data, vol);
}

void glk_sound_load_hint(glui32 snd, glui32 flag)
{
    /* Should we be doing something here? */
}
