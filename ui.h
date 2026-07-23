#ifndef UI_H
#define UI_H

#include "graphics.h"

typedef struct {
    int cx;    // Центр X
    int cy;    // Центр Y
    int r;     // Радиус
    float dx;  // Направление X (от -1.0 до 1.0)
    float dy;  // Направление Y (от -1.0 до 1.0)
} Joystick;

void ui_draw(RenderBuffer* rb, Joystick* joy);

#endif
