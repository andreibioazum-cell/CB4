#include "font.h"
#include <stdlib.h>
#include <string.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Font { stbtt_fontinfo info; float scale; unsigned char* buf; };

int font_init(Font** f, const unsigned char* data, int sz, float height) {
    Font* n = (Font*)calloc(1, sizeof(Font));
    if (!n) return 0;
    n->buf = (unsigned char*)malloc(sz);
    if (!n->buf) { free(n); return 0; }
    memcpy(n->buf, data, sz);
    if (!stbtt_InitFont(&n->info, n->buf, 0)) { free(n->buf); free(n); return 0; }
    n->scale = stbtt_ScaleForPixelHeight(&n->info, height);
    *f = n;
    return 1;
}

void font_set_size(Font* f, float height) { if (f) f->scale = stbtt_ScaleForPixelHeight(&f->info, height); }

void font_draw_text(Font* f, RenderBuffer* rb, int x, int y, const char* txt, uint32_t col) {
    if (!f || !rb || !txt) return;
    int cx = x;
    for (const char* p = txt; *p; ++p) {
        int gi = stbtt_FindGlyphIndex(&f->info, *p);
        if (!gi) { cx += 8; continue; }
        int x0,y0,x1,y1;
        stbtt_GetGlyphBitmapBox(&f->info, gi, f->scale, f->scale, &x0,&y0,&x1,&y1);
        int w = x1-x0, h = y1-y0;
        if (w>0 && h>0) {
            unsigned char* bm = (unsigned char*)malloc(w*h);
            if (bm) {
                stbtt_MakeGlyphBitmap(&f->info, bm, w, h, w, f->scale, f->scale, gi);
                for (int row=0; row<h; ++row) {
                    int sy = y + y0 + row;
                    if (sy<0 || sy>=rb->height) continue;
                    uint32_t* line = rb->pixels + sy * rb->stride;
                    for (int col=0; col<w; ++col) {
                        unsigned char a = bm[row*w + col];
                        if (a) {
                            int sx = cx + x0 + col;
                            if (sx>=0 && sx<rb->width) line[sx] = col;
                        }
                    }
                }
                free(bm);
            }
        }
        int adv; stbtt_GetGlyphHMetrics(&f->info, gi, &adv, 0);
        cx += (int)(adv * f->scale);
    }
}

void font_free(Font* f) { if (f) { free(f->buf); free(f); } }
