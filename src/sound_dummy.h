extern const gboolean gglk_sound_available PRIVATE;
gboolean gglk_sound_init(void) PRIVATE;
void *gglk_sound_open_channel(void) PRIVATE;
void gglk_sound_close_channel(void *data) PRIVATE;
gboolean gglk_sound_play(void *data, glui32 snd, glui32 repeats, glui32 notify) PRIVATE;
void gglk_sound_stop(void *data) PRIVATE;
void gglk_sound_volume(void *data, glui32 vol) PRIVATE;
