#ifndef GAME_H
#define GAME_H

#include <android/asset_manager.h>
#include <sys/time.h>
#include "graphics.h"
#include "ui.h"
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, angle, last_angle, scale;
    uint32_t* texture;
    int tex_width, tex_height;
    int tex_ready;
} Player;

typedef struct {
    float x, y, vx, vy;
    int active;
    int radius;
} Bullet;

typedef struct {
    Player player;
    Joystick joy;
    Button attackBtn;
    Bullet bullet;
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

#ifdef __cplusplus
}
#endif

#endif /* GAME_H */
