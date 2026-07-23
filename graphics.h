#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

typedef struct {
    uint32_t* pixels;
    int width;
    int height;
} Image;

typedef struct {
    uint32_t* pixels;
    int width;
    int height;
    int stride;
} RenderBuffer;

void graphics_clear(RenderBuffer* rb, uint32_t color);
void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color);
void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int thickness, uint32_t color);

// Новая функция: отрисовка картинки с учетом прозрачности (Alpha Blending)
void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y);

#endif
