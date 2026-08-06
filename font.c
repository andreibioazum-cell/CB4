#include "font.h"
#include <stdlib.h>
#include <string.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Font {
    stbtt_fontinfo info;
    float scale;
    unsigned char* buf;
};

static uint32_t utf8_next(const char** p) {
    const unsigned char* s = (const unsigned char*)*p;
    if (!*s) return 0;
    uint32_t c = *s++;
    if (c < 0x80) {
        *p = (const char*)s;
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        if ((*s & 0xC0) == 0x80) {
            c = ((c & 0x1F) << 6) | (*s++ & 0x3F);
        }
    } else if ((c & 0xF0) == 0xE0) {
        if ((*s & 0xC0) == 0x80 && (*(s+1) & 0xC0) == 0x80) {
            c = ((c & 0x0F) << 12) | ((*s & 0x3F) << 6) | (*(s+1) & 0x3F);
            s += 2;
        }
    } else if ((c & 0xF8) == 0xF0) {
        if ((*s & 0xC0) == 0x80 && (*(s+1) & 0xC0) == 0x80 && (*(s+2) & 0xC0) == 0x80) {
            c = ((c & 0x07) << 18) | ((*s & 0x3F) << 12) | ((*(s+1) & 0x3F) << 6) | (*(s+2) & 0x3F);
            s += 3;
        }
    }
    *p = (const char*)s;
    return c;
}

int font_init(Font** f, const unsigned char* data, int sz, float height) {
    if (!f || !data || sz <= 0) return 0;
    Font* n = (Font*)calloc(1, sizeof(Font));
    if (!n) return 0;
    n->buf = (unsigned char*)malloc(sz);
    if (!n->buf) { free(n); return 0; }
    memcpy(n->buf, data, sz);
    if (!stbtt_InitFont(&n->info, n->buf, 0)) {
        free(n->buf);
        free(n);
        return 0;
    }
    n->scale = stbtt_ScaleForPixelHeight(&n->info, height);
    *f = n;
    return 1;
}

void font_set_size(Font* f, float height) {
    if (f) f->scale = stbtt_ScaleForPixelHeight(&f->info, height);
}

void font_draw_text(Font* f, RenderBuffer* rb, int x, int y, const char* txt, uint32_t color) {
    if (!f || !rb || !txt || !rb->pixels) return;
    int cx = x;
    const char* p = txt;
    while (*p) {
        uint32_t cp = utf8_next(&p);
        if (!cp) break;
        int gi = stbtt_FindGlyphIndex(&f->info, (int)cp);
        if (!gi) {
            cx += (int)(f->scale * 16.0f);
            continue;
        }
        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(&f->info, gi, f->scale, f->scale, &x0, &y0, &x1, &y1);
        int w = x1 - x0;
        int h = y1 - y0;
        if (w > 0 && h > 0) {
            unsigned char* bm = (unsigned char*)malloc(w * h);
            if (bm) {
                stbtt_MakeGlyphBitmap(&f->info, bm, w, h, w, f->scale, f->scale, gi);
                for (int row = 0; row < h; ++row) {
                    int sy = y + y0 + row;
                    if (sy < 0 || sy >= rb->height) continue;
                    uint32_t* line = rb->pixels + sy * rb->stride;
                    for (int col = 0; col < w; ++col) {
                        unsigned char a = bm[row * w + col];
                        if (a) {
                            int sx = cx + x0 + col;
                            if (sx >= 0 && sx < rb->width) {
                                line[sx] = color;
                            }
                        }
                    }
                }
                free(bm);
            }
        }
        int adv = 0, lsb = 0;
        stbtt_GetGlyphHMetrics(&f->info, gi, &adv, &lsb);
        cx += (int)(adv * f->scale);
    }
}

void font_free(Font* f) {
    if (f) {
        if (f->buf) free(f->buf);
        free(f);
    }
}
