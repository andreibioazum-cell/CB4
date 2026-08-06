#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t* pixels;
    int width;
    int height;
    int stride;
} RenderBuffer;

/* Zig Graphics Core Engine Functions */
void graphics_clear(RenderBuffer* rb, uint32_t color);
void graphics_draw_rect(RenderBuffer* rb, int x, int y, int size, uint32_t color);
void graphics_draw_rect_exact(RenderBuffer* rb, int x, int y, int w, int h, uint32_t color);
void graphics_draw_rect_lines(RenderBuffer* rb, int x, int y, int w, int h, int thickness, uint32_t color);
void graphics_draw_circle(RenderBuffer* rb, int cx, int cy, int r, uint32_t color);
void graphics_draw_ring(RenderBuffer* rb, int cx, int cy, int r, int thickness, uint32_t color);
void graphics_draw_line(RenderBuffer* rb, int x0, int y0, int x1, int y1, uint32_t color);
void graphics_draw_texture(RenderBuffer* rb, int x, int y, const uint32_t* tex, int tw, int th);
void graphics_draw_texture_ex(RenderBuffer* rb, int cx, int cy,
                              const uint32_t* tex, int tw, int th,
                              float angle, float scale);
void graphics_draw_health_bar(RenderBuffer* rb, int x, int y, int w, int h,
                              int cur_hp, int max_hp,
                              uint32_t fg_col, uint32_t bg_col, uint32_t border_col);
void graphics_draw_particle(RenderBuffer* rb, int cx, int cy, int radius, uint32_t color, float alpha);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHICS_H */
