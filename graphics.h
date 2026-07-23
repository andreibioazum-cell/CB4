#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

typedef struct {
    uint32_t* pixels;
    int w, h;
} Image;

typedef struct {
    uint32_t* pixels;
    int width, height, stride;
} RenderBuffer;

void graphics_clear(RenderBuffer* rb, uint32_t color);
void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color);
void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int t, uint32_t color);
void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y);

// Новая функция загрузки TGA
void graphics_load_tga(Image* img, unsigned char* data);

#endif
