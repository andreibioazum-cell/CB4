#include "ui.h"
#include <math.h>
#include <android/input.h>

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 4, 0xFF000000);
    int sx = joy->centerX + (int)joy->touchOffX;
    int sy = joy->centerY + (int)joy->touchOffY;
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}

void ui_handle_joystick(Joystick* joy, float x, float y, int action) {
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        joy->dirX = joy->dirY = joy->touchOffX = joy->touchOffY = 0.0f;
        return;
    }
    float dx = x - joy->centerX, dy = y - joy->centerY;
    float dist = hypotf(dx, dy);
    if (dist > joy->radius + 30.0f) {
        joy->dirX = joy->dirY = joy->touchOffX = joy->touchOffY = 0.0f;  // сброс при слишком далёком касании
        return;
    }
    if (dist < 15.0f) {
        joy->dirX = joy->dirY = joy->touchOffX = joy->touchOffY = 0.0f;
        return;
    }
    float clamped = (dist > joy->radius) ? joy->radius : dist;
    joy->dirX = dx / dist; joy->dirY = dy / dist;
    joy->touchOffX = joy->dirX * clamped;
    joy->touchOffY = joy->dirY * clamped;
}
