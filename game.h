#ifndef GAME_H
#define GAME_H

#include <android/asset_manager.h>
#include "graphics.h"
#include "ui.h"
#include "font.h"

typedef struct {
    float x, y;
    float angle;          // текущий угол
    float last_angle;     // запоминаем последнее направление
    float scale;
    uint32_t* texture;
    int tex_width, tex_height;
    int tex_ready;
} Player;

typedef struct {
    Player player;
    Joystick joy;
    Font* font;
    int screen_w, screen_h;
    int fontSize;
    int frameCount;
    float fps;
    struct timeval lastTime;
} Game;

int game_init(Game* g, int w, int h, AAssetManager* mgr);
void game_update(Game* g, int w, int h);
void game_draw(Game* g, RenderBuffer* rb);
void game_free(Game* g);
void game_handle_touch(Game* g, float x, float y, int action);

#endif
