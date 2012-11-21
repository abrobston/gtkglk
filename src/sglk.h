#include "glkstart.h"

int sglk_init(int argc, char **argv, glkunix_startup_t *data);
void sglk_window_set_title(const char *title);
void sglk_status_set_mesg(const char *message);
void sglk_icon_set(glui32 image);
void *sglk_get_image(winid_t win, glsi32 x, glsi32 y,
		     glui32 width, glui32 height);
void *sglk_get_blorbimage(glui32 image);
void sglk_delete_image(void *b);
glui32 sglk_image_draw(winid_t win, void *b, glsi32 val1, glsi32 val2);
glui32 sglk_image_draw_scaled(winid_t win, void *b, 
			      glsi32 val1, glsi32 val2,
			      glui32 width, glui32 height);

void sglk_set_style(strid_t str, glui32 fg, glui32 bg, glui32 attr);
