#include "graphics.h"
#include <string.h>

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    // Безопасная очистка: идем по строкам
    for (int y = 0; y < rb->height; y++) {
        uint32_t* line = rb->pixels + (y * rb->stride);
        for (int x = 0; x < rb->width; x++) {
            line[x] = color;
        }
    }
}

void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y) {
    if (!img || !img->pixels) return;

    int start_x = x - img->width / 2;
    int start_y = y - img->height / 2;

    for (int iy = 0; iy < img->height; iy++) {
        int sy = start_y + iy;
        if (sy < 0 || sy >= rb->height) continue;

        uint32_t* dst_line = rb->pixels + (sy * rb->stride);
        uint32_t* src_line = img->pixels + (iy * img->width);

        for (int ix = 0; ix < img->width; ix++) {
            int sx = start_x + ix;
            if (sx < 0 || sx >= rb->width) continue;

            uint32_t src = src_line[ix];
            uint8_t a = (src >> 24) & 0xFF; 

            if (a == 255) {
                dst_line[sx] = src;
            } else if (a > 0) {
                uint32_t dst = dst_line[sx];
                // Оптимизированный Alpha Blending
                uint32_t rb_bits = (src & 0xFF00FF) * a + (dst & 0xFF00FF) * (255 - a);
                uint32_t g_bits  = (src & 0x00FF00) * a + (dst & 0x00FF00) * (255 - a);
                dst_line[sx] = ((rb_bits >> 8) & 0xFF00FF) | ((g_bits >> 8) & 0x00FF00) | 0xFF000000;
            }
        }
    }
}

// Функции кругов оставляем без изменений (они безопасны)
void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color) {
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

void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int thickness, uint32_t color) {
    int r_out2 = r * r;
    int r_in2 = (r - thickness) * (r - thickness);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        uint32_t* line = rb->pixels + (sy * rb->stride);
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            int d2 = x*x + y*y;
            if (d2 <= r_out2 && d2 >= r_in2) line[sx] = color;
        }
    }
}
