#include "graphics.h"
#include <string.h>

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    if (!rb->pixels) return;
    for (int y = 0; y < rb->height; y++) {
        uint32_t* line = rb->pixels + (y * rb->stride);
        for (int x = 0; x < rb->width; x++) {
            line[x] = color;
        }
    }
}

void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y) {
    if (!rb->pixels || !img || !img->pixels) return;

    int x0 = x - img->width / 2;
    int y0 = y - img->height / 2;

    for (int iy = 0; iy < img->height; iy++) {
        int sy = y0 + iy;
        if (sy < 0 || sy >= rb->height) continue;

        uint32_t* dst_line = rb->pixels + (sy * rb->stride);
        uint32_t* src_line = img->pixels + (iy * img->width);

        for (int ix = 0; ix < img->width; ix++) {
            int sx = x0 + ix;
            if (sx < 0 || sx >= rb->width) continue;

            uint32_t p = src_line[ix];
            uint32_t a = (p >> 24) & 0xFF;

            if (a == 255) {
                dst_line[sx] = p;
            } else if (a > 0) {
                uint32_t d = dst_line[sx];
                uint32_t r = (((p & 0xFF) * a) + ((d & 0xFF) * (255 - a))) >> 8;
                uint32_t g = ((((p >> 8) & 0xFF) * a) + (((d >> 8) & 0xFF) * (255 - a))) >> 8;
                uint32_t b = ((((p >> 16) & 0xFF) * a) + (((d >> 16) & 0xFF) * (255 - a))) >> 8;
                dst_line[sx] = 0xFF000000 | (b << 16) | (g << 8) | r;
            }
        }
    }
}

void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color) {
    if (!rb->pixels) return;
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        uint32_t* line = rb->pixels + (sy * rb->stride);
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            if (x*x + y*y <= r2) line[sx] = color;
        }
    }
}

void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int t, uint32_t color) {
    if (!rb->pixels) return;
    int r2 = r * r;
    int r_in2 = (r - t) * (r - t);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        uint32_t* line = rb->pixels + (sy * rb->stride);
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            int d2 = x*x + y*y;
            if (d2 <= r2 && d2 >= r_in2) line[sx] = color;
        }
    }
}
