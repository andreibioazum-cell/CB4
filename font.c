#include "font.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Font {
    stbtt_fontinfo info;
    float scale;
    unsigned char* ttf_buffer;
    int data_size;
};

int font_init(Font** out_font, const unsigned char* ttf_data, int data_size, float pixel_height) {
    Font* f = (Font*)calloc(1, sizeof(Font));
    if (!f) return 0;
    f->ttf_buffer = (unsigned char*)malloc(data_size);
    if (!f->ttf_buffer) { free(f); return 0; }
    memcpy(f->ttf_buffer, ttf_data, data_size);
    f->data_size = data_size;
    if (!stbtt_InitFont(&f->info, f->ttf_buffer, 0)) {
        free(f->ttf_buffer);
        free(f);
        return 0;
    }
    f->scale = stbtt_ScaleForPixelHeight(&f->info, pixel_height);
    *out_font = f;
    return 1;
}

void font_set_size(Font* f, float pixel_height) {
    if (!f) return;
    f->scale = stbtt_ScaleForPixelHeight(&f->info, pixel_height);
}

void font_draw_text(Font* f, RenderBuffer* rb, int x, int y, const char* text, uint32_t color) {
    if (!f || !rb || !text) return;
    int cx = x;
    for (const char* p = text; *p; ++p) {
        int glyph_index = stbtt_FindGlyphIndex(&f->info, *p);
        if (glyph_index == 0) { cx += 8; continue; }
        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(&f->info, glyph_index, f->scale, f->scale, &x0, &y0, &x1, &y1);
        int width = x1 - x0;
        int height = y1 - y0;
        if (width > 0 && height > 0) {
            unsigned char* bitmap = (unsigned char*)malloc(width * height);
            if (bitmap) {
                stbtt_MakeGlyphBitmap(&f->info, bitmap, width, height, width, f->scale, f->scale, glyph_index);
                for (int row = 0; row < height; ++row) {
                    int screen_y = y + y0 + row;
                    if (screen_y < 0 || screen_y >= rb->height) continue;
                    uint32_t* line = rb->pixels + screen_y * rb->stride;
                    for (int col = 0; col < width; ++col) {
                        unsigned char alpha = bitmap[row * width + col];
                        if (alpha) {
                            int screen_x = cx + x0 + col;
                            if (screen_x >= 0 && screen_x < rb->width) {
                                line[screen_x] = color;
                            }
                        }
                    }
                }
                free(bitmap);
            }
        }
        int advance;
        stbtt_GetGlyphHMetrics(&f->info, glyph_index, &advance, NULL);
        cx += (int)(advance * f->scale);
    }
}

void font_free(Font* f) {
    if (f) {
        free(f->ttf_buffer);
        free(f);
    }
}
