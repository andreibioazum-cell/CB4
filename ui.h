#ifndef UI_H
#define UI_H

#include "graphics.h"
#include <android/input.h>

// Состояния игры
typedef enum {
    STATE_LOBBY,
    STATE_PLAYING
} GameState;

// Структура джойстика
typedef struct {
    int centerX, centerY;
    int radius;
    float dirX, dirY;
    float touchOffX, touchOffY;
} Joystick;

// Структура кнопки
typedef struct {
    int x, y;
    int w, h;
    int hover;
} Button;

// Функции UI
void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);
void ui_draw_button(RenderBuffer* rb, Button* btn, const char* text);
int ui_is_point_in_button(Button* btn, float x, float y);

#endif
