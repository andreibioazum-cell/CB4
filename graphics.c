#include "graphics.h"
#include <math.h>
#include <string.h>

void clear(RenderBuffer* b, uint32_t c) {
    int t = b->stride * b->height;
    for (int i = 0; i < t; i++) b->pixels[i] = c;
}

void draw_rect(RenderBuffer* b, int x, int y, int s, uint32_t c) {
    int x1 = x - s/2, x2 = x + s/2;
    int y1 = y - s/2, y2 = y + s/2;
    if (x1 < 0) x1 = 0; if (x2 > b->width) x2 = b->width;
    if (y1 < 0) y1 = 0; if (y2 > b->height) y2 = b->height;
    for (int i = y1; i < y2; i++) {
        uint32_t* l = b->pixels + i * b->stride;
        for (int j = x1; j < x2; j++) l[j] = c;
    }
}

void draw_circle(RenderBuffer* b, int cx, int cy, int r, uint32_t c) {
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= b->height) continue;
        uint32_t* l = b->pixels + sy * b->stride;
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= b->width) continue;
            if (x*x + y2 <= r2) l[sx] = c;
        }
    }
}

void draw_ring(RenderBuffer* b, int cx, int cy, int r, int t, uint32_t c) {
    int ro = r * r, ri = (r - t) * (r - t);
    for (int y = -r; y <= r; y++) {
        int sy = cy + y;
        if (sy < 0 || sy >= b->height) continue;
        uint32_t* l = b->pixels + sy * b->stride;
        int y2 = y * y;
        for (int x = -r; x <= r; x++) {
            int sx = cx + x;
            if (sx < 0 || sx >= b->width) continue;
            int d = x*x + y2;
            if (d <= ro && d >= ri) l[sx] = c;
        }
    }
}

void draw_tex(RenderBuffer* b, int cx, int cy, uint32_t* t, int tw, int th, float a, float s) {
    if (!t || tw <= 0 || th <= 0) return;
    int sw = tw * s, sh = th * s;
    if (sw <= 0 || sh <= 0) return;
    float ca = cosf(a), sa = sinf(a);
    int l = cx - sw/2, tp = cy - sh/2, r = cx + sw/2, bt = cy + sh/2;
    if (l < 0) l = 0; if (tp < 0) tp = 0;
    if (r > b->width) r = b->width; if (bt > b->height) bt = b->height;
    if (l >= r || tp >= bt) return;
    float htw = tw / 2.0f, hth = th / 2.0f, inv = 1.0f / s;
    for (int y = tp; y < bt; y++) {
        uint32_t* o = b->pixels + y * b->stride;
        for (int x = l; x < r; x++) {
            float dx = x - cx, dy = y - cy;
            float sx = dx * ca + dy * sa;
            float sy = -dx * sa + dy * ca;
            float tx = sx * inv + htw, ty = sy * inv + hth;
            int ix = tx + 0.5f, iy = ty + 0.5f;
            if (ix >= 0 && ix < tw && iy >= 0 && iy < th) {
                uint32_t p = t[iy * tw + ix];
                if (p & 0xFF000000) o[x] = p;
            }
        }
    }
}
