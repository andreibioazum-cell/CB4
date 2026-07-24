#include "graphics.h"
#include <arm_neon.h>
#include <string.h>
#include <math.h>

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

void graphics_draw_texture_ex(RenderBuffer* rb, int cx, int cy,
                              uint32_t* tex, int tw, int th,
                              float angle, float scale) {
    if (!tex || tw <= 0 || th <= 0) return;
    int sw = (int)(tw * scale);
    int sh = (int)(th * scale);
    if (sw <= 0 || sh <= 0) return;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    int left = cx - sw/2, top = cy - sh/2;
    int right = cx + sw/2, bottom = cy + sh/2;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > rb->width) right = rb->width;
    if (bottom > rb->height) bottom = rb->height;
    if (left >= right || top >= bottom) return;

    for (int y = top; y < bottom; ++y) {
        uint32_t* out = rb->pixels + y * rb->stride;
        for (int x = left; x < right; ++x) {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            float src_x = dx * cos_a + dy * sin_a;
            float src_y = -dx * sin_a + dy * cos_a;
            float tx = src_x / scale + tw / 2.0f;
            float ty = src_y / scale + th / 2.0f;
            int ix = (int)(tx + 0.5f);
            int iy = (int)(ty + 0.5f);
            if (ix >= 0 && ix < tw && iy >= 0 && iy < th) {
                uint32_t pix = tex[iy * tw + ix];
                if ((pix & 0xFF000000) != 0) out[x] = pix;
            }
        }
    }
}
