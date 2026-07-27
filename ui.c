#include "ui.h"
#include <math.h>
#include <android/input.h>

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy) {
    graphics_draw_ring(rb, joy->centerX, joy->centerY, joy->radius, 4, 0xFF000000);
    int sx = joy->centerX + (int)joy->touchOffX;
    int sy = joy->centerY + (int)joy->touchOffY;
    graphics_draw_circle(rb, sx, sy, 35, 0xFF000000);
}

void ui_handle_joystick_touch(Joystick* joy, float x, float y, int action) {
    float dx = x - joy->centerX;
    float dy = y - joy->centerY;
    float dist = sqrtf(dx*dx + dy*dy);

    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        joy->dirX = joy->dirY = 0.0f;
        joy->touchOffX = joy->touchOffY = 0.0f;
        return;
    }

    // Игнорируем касания вне зоны + отступ
    if (dist > joy->radius + 30.0f)
        return;

    const float deadZone = 15.0f;
    if (dist > deadZone) {
        // Нормализованное направление
        joy->dirX = dx / dist;
        joy->dirY = dy / dist;
        // Ограничиваем смещение радиусом
        float clampedDist = (dist > joy->radius) ? joy->radius : dist;
        joy->touchOffX = joy->dirX * clampedDist;
        joy->touchOffY = joy->dirY * clampedDist;
    } else {
        // В мёртвой зоне – сбрасываем
        joy->dirX = joy->dirY = 0.0f;
        joy->touchOffX = joy->touchOffY = 0.0f;
    }
}
