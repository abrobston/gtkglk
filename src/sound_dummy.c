#include "gglk.h"
#include "sound_dummy.h"

const gboolean gglk_sound_available = FALSE;

gboolean gglk_sound_init(void)
{
    return FALSE;
}

void *gglk_sound_open_channel(void)
{
    return NULL;
}

void gglk_sound_close_channel(void *unused_data)
{
    return;
}

gboolean gglk_sound_play(void *unused_data, glui32 unused_snd,
			 glui32 unused_repeats, glui32 unused_notify)
{
    return FALSE;
}


void gglk_sound_stop(void *unused_data)
{
    return;
}
		     
void gglk_sound_volume(void *unused_data, glui32 unused_vol)
{
    return;
}
