#include "ui.h"

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    // Толщина кольца – 4 пикселя
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 4, 0xFF000000);
    // Стик – радиус 35 (было 40)
    int sx = joy->centerX + (int)joy->touchOffX;
    int sy = joy->centerY + (int)joy->touchOffY;
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}
