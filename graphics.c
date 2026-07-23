#include "graphics.h"
#include <arm_neon.h>
#include <string.h>

// Быстрая очистка экрана через NEON
void graphics_clear(RenderBuffer* rb, uint32_t color) {
    uint32_t* dest = rb->pixels;
    int count = rb->stride * rb->height;
    uint32x4_t v_color = vdupq_n_u32(color);
    int i = 0;
    for (; i <= count - 4; i += 4) {
        vst1q_u32(&dest[i], v_color);
    }
    for (; i < count; i++) dest[i] = color;
}

// Быстрая отрисовка картинки с прозрачностью
void graphics_draw_image(RenderBuffer* rb, Image* img, int x, int y) {
    if (!rb->pixels || !img || !img->pixels) return;
    int x0 = x - img->width / 2, y0 = y - img->height / 2;
    
    for (int iy = 0; iy < img->height; iy++) {
        int sy = y0 + iy;
        if (sy < 0 || sy >= rb->height) continue;
        
        uint32_t* dst = rb->pixels + (sy * rb->stride);
        uint32_t* src = img->pixels + (iy * img->width);
        
        for (int ix = 0; ix < img->width; ix++) {
            int sx = x0 + ix;
            if (sx < 0 || sx >= rb->width) continue;
            
            uint32_t p = src[ix];
            uint32_t a = (p >> 24) & 0xFF;
            
            if (a == 255) dst[sx] = p;
            else if (a > 0) {
                uint32_t d = dst[sx];
                // Оптимизированный "фокус" смешивания без деления
                uint32_t rb_bits = ((p & 0xFF00FF) * a + (d & 0xFF00FF) * (255 - a)) >> 8;
                uint32_t g_bits  = ((p & 0x00FF00) * a + (d & 0x00FF00) * (255 - a)) >> 8;
                dst[sx] = 0xFF000000 | (rb_bits & 0xFF00FF) | (g_bits & 0x00FF00);
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
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            if (x * x + y2 <= r2) line[sx] = color;
        }
    }
}

void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int t, uint32_t color) {
    int r_out2 = r * r, r_in2 = (r - t) * (r - t);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        uint32_t* line = rb->pixels + (sy * rb->stride);
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= rb->width) continue;
            int d2 = x * x + y2;
            if (d2 <= r_out2 && d2 >= r_in2) line[sx] = color;
        }
    }
}
