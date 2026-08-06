#include "graphics.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void graphics_clear(RenderBuffer* b, uint32_t c) {
    if (!b || !b->pixels || b->width <= 0 || b->height <= 0) return;
    int t = b->stride * b->height;
    for (int i = 0; i < t; i++) {
        b->pixels[i] = c;
    }
}

void graphics_draw_rect(RenderBuffer* b, int x, int y, int s, uint32_t c) {
    if (!b || !b->pixels || s <= 0) return;
    int half = s / 2;
    int x1 = x - half;
    int x2 = x + half;
    int y1 = y - half;
    int y2 = y + half;

    if (x1 < 0) x1 = 0;
    if (x2 > b->width) x2 = b->width;
    if (y1 < 0) y1 = 0;
    if (y2 > b->height) y2 = b->height;

    if (x1 >= x2 || y1 >= y2) return;

    for (int i = y1; i < y2; i++) {
        uint32_t* l = b->pixels + i * b->stride;
        for (int j = x1; j < x2; j++) {
            l[j] = c;
        }
    }
}

void graphics_draw_rect_exact(RenderBuffer* b, int x, int y, int w, int h, uint32_t c) {
    if (!b || !b->pixels || w <= 0 || h <= 0) return;
    int x1 = x;
    int x2 = x + w;
    int y1 = y;
    int y2 = y + h;

    if (x1 < 0) x1 = 0;
    if (x2 > b->width) x2 = b->width;
    if (y1 < 0) y1 = 0;
    if (y2 > b->height) y2 = b->height;

    if (x1 >= x2 || y1 >= y2) return;

    for (int i = y1; i < y2; i++) {
        uint32_t* l = b->pixels + i * b->stride;
        for (int j = x1; j < x2; j++) {
            l[j] = c;
        }
    }
}

void graphics_draw_rect_lines(RenderBuffer* b, int x, int y, int w, int h, int thickness, uint32_t c) {
    if (thickness <= 0 || w <= 0 || h <= 0) return;
    graphics_draw_rect_exact(b, x, y, w, thickness, c);
    graphics_draw_rect_exact(b, x, y + h - thickness, w, thickness, c);
    graphics_draw_rect_exact(b, x, y + thickness, thickness, h - 2 * thickness, c);
    graphics_draw_rect_exact(b, x + w - thickness, y + thickness, thickness, h - 2 * thickness, c);
}

void graphics_draw_circle(RenderBuffer* b, int cx, int cy, int r, uint32_t c) {
    if (!b || !b->pixels || r <= 0) return;
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= b->height) continue;
        int y2 = y * y;
        int rem = r2 - y2;
        if (rem < 0) continue;
        int dx = (int)sqrtf((float)rem);
        int x1 = cx - dx;
        int x2 = cx + dx + 1;
        if (x1 < 0) x1 = 0;
        if (x2 > b->width) x2 = b->width;
        uint32_t* l = b->pixels + sy * b->stride;
        for (int j = x1; j < x2; j++) {
            l[j] = c;
        }
    }
}

void graphics_draw_ring(RenderBuffer* b, int cx, int cy, int r, int t, uint32_t c) {
    if (!b || !b->pixels || r <= 0 || t <= 0) return;
    int ro = r * r;
    int inner_r = (r > t) ? (r - t) : 0;
    int ri = inner_r * inner_r;
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= b->height) continue;
        uint32_t* l = b->pixels + sy * b->stride;
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= b->width) continue;
            int d = x * x + y2;
            if (d <= ro && d >= ri) {
                l[sx] = c;
            }
        }
    }
}

void graphics_draw_line(RenderBuffer* b, int x0, int y0, int x1, int y1, uint32_t c) {
    if (!b || !b->pixels) return;
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if (x0 >= 0 && x0 < b->width && y0 >= 0 && y0 < b->height) {
            b->pixels[y0 * b->stride + x0] = c;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void graphics_draw_texture(RenderBuffer* b, int x, int y, const uint32_t* t, int tw, int th) {
    if (!b || !b->pixels || !t || tw <= 0 || th <= 0) return;
    int x1 = x;
    int x2 = x + tw;
    int y1 = y;
    int y2 = y + th;

    if (x1 < 0) x1 = 0;
    if (x2 > b->width) x2 = b->width;
    if (y1 < 0) y1 = 0;
    if (y2 > b->height) y2 = b->height;

    if (x1 >= x2 || y1 >= y2) return;

    for (int row = y1; row < y2; row++) {
        const uint32_t* src_row = t + (row - y) * tw;
        uint32_t* dst_row = b->pixels + row * b->stride;
        for (int col = x1; col < x2; col++) {
            uint32_t p = src_row[col - x];
            uint32_t a = (p >> 24) & 0xFF;
            if (a == 255) {
                dst_row[col] = p;
            } else if (a > 0) {
                uint32_t dst = dst_row[col];
                uint32_t inv_a = 255 - a;
                uint32_t r = (((p >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * inv_a) / 255;
                uint32_t g = (((p >> 8) & 0xFF) * a + ((dst >> 8) & 0xFF) * inv_a) / 255;
                uint32_t bl = ((p & 0xFF) * a + (dst & 0xFF) * inv_a) / 255;
                dst_row[col] = 0xFF000000 | (r << 16) | (g << 8) | bl;
            }
        }
    }
}

void graphics_draw_texture_ex(RenderBuffer* b, int cx, int cy, const uint32_t* t, int tw, int th, float a, float s) {
    if (!b || !b->pixels || !t || tw <= 0 || th <= 0 || s <= 0.0f) return;
    int sw = (int)(tw * s);
    int sh = (int)(th * s);
    if (sw <= 0 || sh <= 0) return;

    float ca = cosf(a);
    float sa = sinf(a);
    int l = cx - sw / 2;
    int tp = cy - sh / 2;
    int r = cx + sw / 2;
    int bt = cy + sh / 2;

    if (l < 0) l = 0;
    if (tp < 0) tp = 0;
    if (r > b->width) r = b->width;
    if (bt > b->height) bt = b->height;
    if (l >= r || tp >= bt) return;

    float htw = tw / 2.0f;
    float hth = th / 2.0f;
    float inv = 1.0f / s;

    for (int y = tp; y < bt; y++) {
        uint32_t* o = b->pixels + y * b->stride;
        for (int x = l; x < r; x++) {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            float sx = dx * ca + dy * sa;
            float sy = -dx * sa + dy * ca;
            float tx = sx * inv + htw;
            float ty = sy * inv + hth;
            int ix = (int)(tx + 0.5f);
            int iy = (int)(ty + 0.5f);
            if (ix >= 0 && ix < tw && iy >= 0 && iy < th) {
                uint32_t p = t[iy * tw + ix];
                uint32_t alpha = (p >> 24) & 0xFF;
                if (alpha == 255) {
                    o[x] = p;
                } else if (alpha > 0) {
                    uint32_t dst = o[x];
                    uint32_t inv_a = 255 - alpha;
                    uint32_t red = (((p >> 16) & 0xFF) * alpha + ((dst >> 16) & 0xFF) * inv_a) / 255;
                    uint32_t grn = (((p >> 8) & 0xFF) * alpha + ((dst >> 8) & 0xFF) * inv_a) / 255;
                    uint32_t blu = ((p & 0xFF) * alpha + (dst & 0xFF) * inv_a) / 255;
                    o[x] = 0xFF000000 | (red << 16) | (grn << 8) | blu;
                }
            }
        }
    }
}

void graphics_draw_health_bar(RenderBuffer* rb, int x, int y, int w, int h,
                              int cur_hp, int max_hp,
                              uint32_t fg_col, uint32_t bg_col, uint32_t border_col) {
    if (w <= 4 || h <= 4 || max_hp <= 0) return;

    graphics_draw_rect_exact(rb, x, y, w, h, bg_col);

    int clamped_hp = cur_hp;
    if (clamped_hp < 0) clamped_hp = 0;
    if (clamped_hp > max_hp) clamped_hp = max_hp;

    int inner_w = w - 4;
    int fill_w = (inner_w * clamped_hp) / max_hp;
    if (fill_w > 0) {
        graphics_draw_rect_exact(rb, x + 2, y + 2, fill_w, h - 4, fg_col);
    }

    graphics_draw_rect_lines(rb, x, y, w, h, 2, border_col);
}

void graphics_draw_particle(RenderBuffer* rb, int cx, int cy, int radius, uint32_t color, float alpha) {
    if (!rb || !rb->pixels || radius <= 0 || alpha <= 0.0f) return;
    if (alpha > 1.0f) alpha = 1.0f;
    uint32_t u_alpha = (uint32_t)(alpha * 255.0f);
    if (u_alpha == 0) return;

    int r2 = radius * radius;
    uint32_t sr = (color >> 16) & 0xFF;
    uint32_t sg = (color >> 8) & 0xFF;
    uint32_t sb = color & 0xFF;
    uint32_t inv_a = 255 - u_alpha;

    for (int y = -radius; y <= radius; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= rb->height) continue;
        int y2 = y * y;
        int rem = r2 - y2;
        if (rem < 0) continue;
        int dx = (int)sqrtf((float)rem);
        int x1 = cx - dx;
        int x2 = cx + dx + 1;
        if (x1 < 0) x1 = 0;
        if (x2 > rb->width) x2 = rb->width;

        uint32_t* line = rb->pixels + sy * rb->stride;
        for (int x = x1; x < x2; x++) {
            uint32_t dst = line[x];
            uint32_t dr = (dst >> 16) & 0xFF;
            uint32_t dg = (dst >> 8) & 0xFF;
            uint32_t db = dst & 0xFF;

            uint32_t out_r = (sr * u_alpha + dr * inv_a) / 255;
            uint32_t out_g = (sg * u_alpha + dg * inv_a) / 255;
            uint32_t out_b = (sb * u_alpha + db * inv_a) / 255;

            line[x] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
        }
    }
}
