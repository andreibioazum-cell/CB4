#ifndef UI_H
#define UI_H
#include "graphics.h"

typedef struct {
    int centerX, centerY, radius;
    float dirX, dirY, touchOffX, touchOffY;
} Joystick;

typedef struct {
    int x, y, radius;
    int pressed;
} Button;

void ui_draw_joystick(RenderBuffer* rb, Joystick* joy);
void ui_handle_joystick(Joystick* joy, float x, float y, int action);
void ui_draw_button(RenderBuffer* rb, Button* btn, const char* text);
int ui_handle_button(Button* btn, float x, float y, int action);
void draw_text_outlined(RenderBuffer* rb, int x, int y, const char* text, uint32_t color, uint32_t outline);

#endif
