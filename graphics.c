#include "graphics.h"
#include <arm_neon.h>
#include <math.h>

void graphics_clear(RenderBuffer* rb, uint32_t color) {
    uint32x4_t v = vdupq_n_u32(color);
    int i = 0, total = rb->stride * rb->height;
    for (; i <= total - 4; i += 4) vst1q_u32(&rb->pixels[i], v);
    for (; i < total; i++) rb->pixels[i] = color;
}

void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color, int ring) {
    int r2 = r * r, rin2 = (r-4)*(r-4);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x, d2 = x*x + y*y;
            if (sx >= 0 && sx < rb->width && d2 <= r2 && (!ring || d2 >= rin2))
                rb->pixels[sy * rb->stride + sx] = color;
        }
    }
}

void graphics_draw_texture(RenderBuffer* rb, int cx, int cy, uint32_t* tex, int tw, int th, float angle, float scale) {
    if (!tex) return;
    float c = cosf(-angle), s = sinf(-angle);
    int hsw = (int)(tw * scale / 2), hsh = (int)(th * scale / 2);
    for (int y = -hsh; y < hsh; y++) {
        for (int x = -hsw; x < hsw; x++) {
            int sx = cx + x, sy = cy + y;
            if (sx < 0 || sx >= rb->width || sy < 0 || sy >= rb->height) continue;
            float tx = (x * c - y * s) / scale + tw/2.0f;
            float ty = (x * s + y * c) / scale + th/2.0f;
            if (tx >= 0 && tx < tw && ty >= 0 && ty < th) {
                uint32_t pix = tex[(int)ty * tw + (int)tx];
                if (pix >> 24) rb->pixels[sy * rb->stride + sx] = pix;
            }
        }
    }
}
