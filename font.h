#ifndef FONT_H
#define FONT_H

#include "graphics.h"

typedef struct Font Font;

int font_init(Font** out_font, const unsigned char* ttf_data, int data_size, float pixel_height);
void font_set_size(Font* f, float pixel_height);
void font_draw_text(Font* f, RenderBuffer* rb, int x, int y, const char* text, uint32_t color);
void font_free(Font* f);

#endif
