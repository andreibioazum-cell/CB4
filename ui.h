#ifndef UI_H
#define UI_H

#include "graphics.h"

typedef struct {
    int centerX, centerY;
    int radius;
    float dirX, dirY;
} Joystick;

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);

#endif
