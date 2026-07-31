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
    float dx = x - (float)joy->centerX, dy = y - (float)joy->centerY;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist > (float)(joy->radius + 30)) return; // игнорируем, НЕ сбрасываем

    if (dist < 0.001f) {
        joy->dirX = joy->dirY = joy->touchOffX = joy->touchOffY = 0.0f;
        return;
    }
    float clamped = (dist > (float)joy->radius) ? (float)joy->radius : dist;
    joy->dirX = dx / dist;
    joy->dirY = dy / dist;
    joy->touchOffX = joy->dirX * clamped;
    joy->touchOffY = joy->dirY * clamped;
}
