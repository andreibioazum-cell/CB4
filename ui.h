#ifndef UI_H
#define UI_H

#include "graphics.h"

// Структура джойстика
typedef struct {
    int cx;    // Центр X основания
    int cy;    // Центр Y основания
    int r;     // Радиус кольца
    float dx;  // Смещение стика по X (от -1.0 до 1.0)
    float dy;  // Смещение стика по Y (от -1.0 до 1.0)
} Joystick;

// Функция отрисовки интерфейса
void ui_draw(RenderBuffer* rb, Joystick* joy);

#endif
