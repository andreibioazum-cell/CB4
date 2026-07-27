#ifndef UI_H
#define UI_H
#include "graphics.h"

typedef struct {
    int centerX, centerY, radius;
    float dirX, dirY, touchOffX, touchOffY;
} Joystick;

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);
void ui_handle_joystick(Joystick* joy, float x, float y, int action);

#endif
