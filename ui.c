#include "ui.h"
#include <math.h>
#include <android/input.h>

void ui_draw_joystick(RenderBuffer* b, Joystick* j) {
    if (!b || !j) return;
    graphics_draw_ring(b, j->centerX, j->centerY, j->radius, 4, 0xFF000000);
    int sx = j->centerX + (int)j->touchOffX;
    int sy = j->centerY + (int)j->touchOffY;
    graphics_draw_circle(b, sx, sy, 35, 0xFF000000);
}

void ui_handle_joystick(Joystick* j, float x, float y, int action) {
    if (!j) return;
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        j->dirX = j->dirY = j->touchOffX = j->touchOffY = 0.0f;
        return;
    }
    float dx = x - (float)j->centerX, dy = y - (float)j->centerY;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist < 1.0f) {
        j->dirX = j->dirY = j->touchOffX = j->touchOffY = 0.0f;
        return;
    }
    float inv = 1.0f / dist;
    j->dirX = dx * inv;
    j->dirY = dy * inv;
    float clamped = (dist > (float)j->radius) ? (float)j->radius : dist;
    j->touchOffX = j->dirX * clamped;
    j->touchOffY = j->dirY * clamped;
}

void ui_draw_button(Font* font, RenderBuffer* b, Button* btn, const char* text) {
    if (!b || !btn) return;
    graphics_draw_circle(b, btn->x, btn->y, btn->radius, btn->pressed ? 0xFFCC0000 : 0xFFFF0000);
    graphics_draw_ring(b, btn->x, btn->y, btn->radius, 3, 0xFF000000);
    if (text) {
        draw_text_outlined(font, b, btn->x - 30, btn->y - 12, text, 0xFFFFFFFF, 0xFF000000);
    }
}

int ui_handle_button(Button* btn, float x, float y, int action) {
    if (!btn) return 0;
    float dx = x - (float)btn->x, dy = y - (float)btn->y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (action == AMOTION_EVENT_ACTION_DOWN && dist < (float)btn->radius) {
        btn->pressed = 1;
        return 1;
    }
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        if (btn->pressed && dist < (float)btn->radius) {
            btn->pressed = 0;
            return 1;
        }
        btn->pressed = 0;
    }
    return 0;
}

void draw_text_outlined(Font* font, RenderBuffer* b, int x, int y, const char* text, uint32_t c, uint32_t o) {
    if (!font || !b || !text) return;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            font_draw_text(font, b, x + dx, y + dy, text, o);
        }
    }
    font_draw_text(font, b, x, y, text, c);
}
