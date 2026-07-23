#include "ui.h"

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    // Кольцо с уменьшенным радиусом (80 вместо 120)
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 10, 0xFF000000);
    
    // Внутренний стик – теперь смещается на 100% радиуса (раньше было 0.6f)
    int sx = joy->centerX + (int)(joy->dirX * joy->radius);
    int sy = joy->centerY + (int)(joy->dirY * joy->radius);
    graphics_draw_circle(rb, sx, sy, 25, 0xFF000000); // размер кружка уменьшен до 25
}
