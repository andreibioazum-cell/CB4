#include "ui.h"
#include <math.h>
#include <android/input.h>
#include "font.h"

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
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist < 1.0f) {
        joy->dirX = joy->dirY = joy->touchOffX = joy->touchOffY = 0.0f;
        return;
    }
    float inv = 1.0f / dist;
    joy->dirX = dx * inv;
    joy->dirY = dy * inv;
    float clamped = (dist > joy->radius) ? joy->radius : dist;
    joy->touchOffX = joy->dirX * clamped;
    joy->touchOffY = joy->dirY * clamped;
}

void ui_draw_button(RenderBuffer* rb, Button* btn, const char* text) {
    // Рисуем фон (круг)
    graphics_draw_circle(rb, btn->x, btn->y, btn->radius, 0xFFFF0000); // красный
    // Обводка (чёрная)
    graphics_draw_ring(rb, btn->x, btn->y, btn->radius, 3, 0xFF000000);
    // Текст с обводкой
    draw_text_outlined(rb, btn->x - 30, btn->y - 12, text, 0xFFFFFFFF, 0xFF000000);
}

int ui_handle_button(Button* btn, float x, float y, int action) {
    float dx = x - btn->x, dy = y - btn->y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (action == AMOTION_EVENT_ACTION_DOWN && dist < btn->radius) {
        btn->pressed = 1;
        return 1;
    }
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        if (btn->pressed && dist < btn->radius) {
            btn->pressed = 0;
            return 1; // клик
        }
        btn->pressed = 0;
    }
    return 0;
}

void draw_text_outlined(RenderBuffer* rb, int x, int y, const char* text, uint32_t color, uint32_t outline) {
    // Рисуем обводку (смещения на 1 пиксель)
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            font_draw_text(NULL, rb, x + dx, y + dy, text, outline);
        }
    }
    // Основной текст
    font_draw_text(NULL, rb, x, y, text, color);
}
