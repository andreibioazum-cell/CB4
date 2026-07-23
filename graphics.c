#include "graphics.h"
#include <arm_neon.h>
#include <string.h>

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    uint32x4_t v_color = vdupq_n_u32(color);
    int total = rb->stride * rb->height;
    int i = 0;
    for (; i <= total - 4; i += 4) vst1q_u32(&rb->pixels[i], v_color);
    for (; i < total; i++) rb->pixels[i] = color;
}

// Отрисовка картинки с Alpha-каналом
void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y) {
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
            uint8_t a = (src >> 24) & 0xFF; // Прозрачность

            if (a == 255) {
                dst_line[sx] = src; // Непрозрачный пиксель
            } else if (a > 0) {
                // Смешивание цветов (Alpha Blending)
                uint32_t dst = dst_line[sx];
                uint32_t r = ((((src >> 0) & 0xFF) * a) + (((dst >> 0) & 0xFF) * (255 - a))) >> 8;
                uint32_t g = ((((src >> 8) & 0xFF) * a) + (((dst >> 8) & 0xFF) * (255 - a))) >> 8;
                uint32_t b = ((((src >> 16) & 0xFF) * a) + (((dst >> 16) & 0xFF) * (255 - a))) >> 8;
                dst_line[sx] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
    }
}

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
