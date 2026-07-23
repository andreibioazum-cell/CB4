#include "ui.h"

void ui_draw(RenderBuffer* rb, Joystick* joy) {
    // 1. Рисуем черное кольцо (основание джойстика)
    // Параметры: буфер, центр X, центр Y, радиус, толщина линии, цвет (непрозрачный черный)
    graphics_draw_ring(rb, joy->cx, joy->cy, joy->r, 10, 0xFF000000);
    
    // 2. Вычисляем позицию стика (внутреннего круга)
    // Умножаем на 0.6, чтобы стик не выходил за границы кольца
    int stick_x = joy->cx + (int)(joy->dx * (float)joy->r * 0.6f);
    int stick_y = joy->cy + (int)(joy->dy * (float)joy->r * 0.6f);
    
    // 3. Рисуем черный внутренний кружочек (стик)
    // Параметры: буфер, X стика, Y стика, радиус стика, цвет (непрозрачный черный)
    graphics_draw_circle(rb, stick_x, stick_y, 35, 0xFF000000);
}
