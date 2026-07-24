#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <stdint.h>

typedef struct { uint32_t* pixels; int width, height, stride; } RenderBuffer;

void graphics_clear(RenderBuffer* rb, uint32_t color);
void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color, int ring);
void graphics_draw_texture(RenderBuffer* rb, int cx, int cy, uint32_t* tex, int tw, int th, float angle, float scale);

#endif
