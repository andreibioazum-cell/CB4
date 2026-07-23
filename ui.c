#include "ui.h"

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    // Черное кольцо (непрозрачное)
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 10, 0xFF000000);
    
    // Черный внутренний кружочек (стик)
    int sx = joy->centerX + (int)(joy->dirX * (joy->radius * 0.6f));
    int sy = joy->centerY + (int)(joy->dirY * (joy->radius * 0.6f));
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}
