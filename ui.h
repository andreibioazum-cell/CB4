#ifndef UI_H
#define UI_H
#include "graphics.h"

typedef struct {
    int centerX, centerY;
    int radius;
    float dirX, dirY;       // нормализованное направление (для движения)
    float touchOffX, touchOffY; // смещение стика от центра (пиксели)
} Joystick;

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);
#endif
