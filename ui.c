#include "ui.h"

void ui_draw(RenderBuffer* rb, Joystick* joy) {
    // Черное кольцо джойстика
    graphics_draw_ring(rb, joy->cx, joy->cy, joy->r, 10, 0xFF000000);
    
    // Черный внутренний кружок (стик)
    int sx = joy->cx + (int)(joy->dx * (joy->r * 0.6f));
    int sy = joy->cy + (int)(joy->dy * (joy->r * 0.6f));
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}
