#ifndef GAME_H
#define GAME_H

#include "ui.h"

typedef struct {
    float x, y;
    float angle;
    uint32_t* texture;
    int tex_width, tex_height;
    int ready;
} Player;

typedef struct {
    GameState state;
    Player player;
    Joystick joy;
    Button playBtn;
    int screen_w, screen_h;
} Game;

void game_init(Game* g, int w, int h);
void game_load_texture(Game* g, void* asset_manager);
void game_update(Game* g);
void game_draw(Game* g, RenderBuffer* rb);
void game_free(Game* g);
void game_handle_touch(Game* g, float x, float y, int action);

#endif
