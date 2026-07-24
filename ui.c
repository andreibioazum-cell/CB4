#include "ui.h"

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 4, 0xFF000000);
    int sx = joy->centerX + (int)joy->touchOffX;
    int sy = joy->centerY + (int)joy->touchOffY;
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}

void ui_draw_button(RenderBuffer* rb, Button* btn, const char* text) {
    uint32_t color = btn->hover ? 0xFF66BB6A : 0xFF4CAF50;
    for (int y = btn->y; y < btn->y + btn->h; y++) {
        uint32_t* line = rb->pixels + y * rb->stride;
        for (int x = btn->x; x < btn->x + btn->w; x++) {
            line[x] = color;
        }
    }
    graphics_draw_rect(rb, btn->x + btn->w/2, btn->y + btn->h/2, btn->w, 0xFFFFFFFF);
}

int ui_is_point_in_button(Button* btn, float x, float y) {
    return (x >= btn->x && x <= btn->x + btn->w &&
            y >= btn->y && y <= btn->y + btn->h);
}
