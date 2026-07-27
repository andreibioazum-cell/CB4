#ifndef UI_H
#define UI_H
#include "graphics.h"

typedef struct {
    int centerX, centerY;
    int radius;
    float dirX, dirY;
    float touchOffX, touchOffY;
} Joystick;

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);
void ui_handle_joystick_touch(Joystick* joy, float x, float y, int action);

#endif
