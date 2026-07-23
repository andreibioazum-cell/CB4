#include "graphics.h"
#include <stdlib.h>

// Ручной разбор TGA без сторонних библиотек
void graphics_load_tga(Image* img, unsigned char* data) {
    if (!data) return;
    // Заголовок TGA: ширина в байтах 12-13, высота 14-15
    img->w = data[12] | (data[13] << 8);
    img->h = data[14] | (data[15] << 8);
    
    int total_pixels = img->w * img->h;
    img->pixels = (uint32_t*)malloc(total_pixels * 4);
    
    // Пиксели в TGA начинаются с 18-го байта
    unsigned char* raw_pixels = data + 18;
    
    for (int i = 0; i < total_pixels; i++) {
        // TGA хранит цвета как BGRA, конвертируем в RGBA
        unsigned char b = raw_pixels[i*4 + 0];
        unsigned char g = raw_pixels[i*4 + 1];
        unsigned char r = raw_pixels[i*4 + 2];
        unsigned char a = raw_pixels[i*4 + 3];
        img->pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
    }
}

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    if (!rb->pixels) return;
    for (int i = 0; i < rb->height * rb->stride; i++) rb->pixels[i] = color;
}

void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y) {
    if (!rb->pixels || !img || !img->pixels) return;
    int x0 = x - img->w / 2;
    int y0 = y - img->h / 2;
    for (int iy = 0; iy < img->h; iy++) {
        int sy = y0 + iy;
        if (sy < 0 || sy >= rb->height) continue;
        uint32_t* dst = rb->pixels + (sy * rb->stride);
        uint32_t* src = img->pixels + ((img->h - 1 - iy) * img->w); // TGA часто перевернут
        for (int ix = 0; ix < img->w; ix++) {
            int sx = x0 + ix;
            if (sx < 0 || sx >= rb->width) continue;
            uint32_t p = src[ix];
            uint32_t a = p >> 24;
            if (a == 255) dst[sx] = p;
            else if (a > 0) {
                uint32_t d = dst[sx];
                uint32_t r = (((p & 0xFF) * a) + ((d & 0xFF) * (255 - a))) >> 8;
                uint32_t g = ((((p >> 8) & 0xFF) * a) + (((d >> 8) & 0xFF) * (255 - a))) >> 8;
                uint32_t b = ((((p >> 16) & 0xFF) * a) + (((d >> 16) & 0xFF) * (255 - a))) >> 8;
                dst[sx] = 0xFF000000 | (b << 16) | (g << 8) | r;
            }
        }
    }
}

void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color) {
    if (!rb->pixels) return;
    int r2 = r*r;
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            if (x*x + y*y <= r2) rb->pixels[sy * rb->stride + sx] = color;
        }
    }
}

void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int t, uint32_t color) {
    if (!rb->pixels) return;
    int r2 = r*r, r_in2 = (r-t)*(r-t);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            int d = x*x + y*y;
            if (d <= r2 && d >= r_in2) rb->pixels[sy * rb->stride + sx] = color;
        }
    }
}
