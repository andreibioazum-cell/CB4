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

#define MAX_BULLETS 64
#define MAX_ENEMIES 32
#define MAX_PARTICLES 128
#define MAX_PICKUPS 16

typedef enum {
    GAME_STATE_PLAYING,
    GAME_STATE_GAMEOVER
} GameState;

typedef enum {
    ENEMY_TYPE_NORMAL,
    ENEMY_TYPE_FAST,
    ENEMY_TYPE_TANK
} EnemyType;

typedef enum {
    PICKUP_TYPE_HEALTH,
    PICKUP_TYPE_WEAPON,
    PICKUP_TYPE_SCORE
} PickupType;

typedef struct {
    float x;
    float y;
    float angle;
    float last_angle;
    float scale;
    int hp;
    int max_hp;
    float invulnerable_timer;
    float shoot_cooldown;
    int weapon_level;
    uint32_t* texture;
    int tex_width;
    int tex_height;
    int tex_ready;
} Player;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int active;
    int radius;
    int damage;
    int is_enemy;
    uint32_t color;
} Bullet;

typedef struct {
    float x;
    float y;
    int active;
    int hp;
    int max_hp;
    float speed;
    int radius;
    EnemyType type;
    uint32_t color;
    float hit_flash;
} Enemy;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    int radius;
    uint32_t color;
    int active;
} Particle;

typedef struct {
    float x;
    float y;
    PickupType type;
    int active;
    float pulse;
    int radius;
} Pickup;

typedef struct {
    Player player;
    Joystick joy;
    Button attackBtn;
    Button restartBtn;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Particle particles[MAX_PARTICLES];
    Pickup pickups[MAX_PICKUPS];
    Font* font;
    GameState state;
    int screen_w;
    int screen_h;
    int fontSize;
    int frameCount;
    float fps;
    struct timeval lastTime;
    
    // Wave, Score, and Effects
    int score;
    int high_score;
    int wave;
    int enemies_to_spawn;
    int enemies_alive;
    float spawn_timer;
    float wave_delay_timer;
    float screen_shake;
    AAssetManager* asset_mgr;
} Game;

int game_init(Game* g, int w, int h, AAssetManager* mgr);
void game_restart(Game* g);
void game_update(Game* g, int w, int h);
void game_draw(Game* g, RenderBuffer* rb);
void game_free(Game* g);

void game_spawn_bullet(Game* g, float x, float y, float vx, float vy, int is_enemy, int damage, uint32_t color);
void game_spawn_particles(Game* g, float x, float y, uint32_t color, int count, float speed);
void game_spawn_enemy(Game* g, EnemyType type);
void game_spawn_pickup(Game* g, float x, float y, PickupType type);

#ifdef __cplusplus
}
#endif

#endif /* GAME_H */
