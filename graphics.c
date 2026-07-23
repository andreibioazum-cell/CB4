#include "graphics.h"
#include <arm_neon.h>
#include <string.h>

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    uint32x4_t v_color = vdupq_n_u32(color);
    int total_pixels = rb->stride * rb->height;
    int i = 0;
    for (; i <= total_pixels - 4; i += 4) {
        vst1q_u32(&rb->pixels[i], v_color);
    }
    for (; i < total_pixels; i++) rb->pixels[i] = color;
}

void graphics_draw_rect(RenderBuffer* rb, int x, int y, int size, uint32_t color) {
    int x1 = x - size/2, x2 = x + size/2;
    int y1 = y - size/2, y2 = y + size/2;
    if (x1 < 0) x1 = 0; if (x2 > rb->width) x2 = rb->width;
    if (y1 < 0) y1 = 0; if (y2 > rb->height) y2 = rb->height;

    for (int i = y1; i < y2; i++) {
        uint32_t* line = rb->pixels + (i * rb->stride);
        for (int j = x1; j < x2; j++) line[j] = color;
    }
}

void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color) {
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        int screen_y = cy + y;
        if (screen_y < 0 || screen_y >= rb->height) continue;
        uint32_t* line = rb->pixels + (screen_y * rb->stride);
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int screen_x = cx + x;
            if (screen_x < 0 || screen_x >= rb->width) continue;
            if (x * x + y2 <= r2) line[screen_x] = color;
        }
    }
}

void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int thickness, uint32_t color) {
    int r_out2 = r * r;
    int r_in2 = (r - thickness) * (r - thickness);
    for (int y = -r; y <= r; y++) {
        int screen_y = cy + y;
        if (screen_y < 0 || screen_y >= rb->height) continue;
        uint32_t* line = rb->pixels + (screen_y * rb->stride);
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int screen_x = cx + x;
            if (screen_x < 0 || screen_x >= rb->width) continue;
            int dist2 = x * x + y2;
            if (dist2 <= r_out2 && dist2 >= r_in2) line[screen_x] = color;
        }
    }
}

void graphics_draw_texture(RenderBuffer* rb, int cx, int cy, 
                           uint32_t* tex, int tw, int th) {
    int left = cx - tw/2;
    int top  = cy - th/2;
    int right = left + tw;
    int bottom = top + th;
    
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > rb->width) right = rb->width;
    if (bottom > rb->height) bottom = rb->height;
    if (left >= right || top >= bottom) return;

    int src_x_start = left - (cx - tw/2);
    int src_y_start = top - (cy - th/2);
    
    for (int y = top; y < bottom; ++y) {
        uint32_t* out = rb->pixels + y * rb->stride;
        uint32_t* src = tex + (src_y_start + (y - top)) * tw + src_x_start;
        for (int x = left; x < right; ++x) {
            uint32_t pix = *src++;
            if ((pix & 0xFF000000) != 0) { // Рисуем только непрозрачные пиксели
                out[x] = pix;
            }
        }
    }
}
