#include "ui.h"

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 4, 0xFF000000);
    int sx = joy->centerX + (int)joy->touchOffX;
    int sy = joy->centerY + (int)joy->touchOffY;
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}
