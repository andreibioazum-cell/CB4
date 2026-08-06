#include "game.h"
#include "graphics.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <android/asset_manager.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define BULLET_SPEED 16.0f
#define PLAYER_SCALE 3.0f
#define PLAYER_MAX_HP 100
#define SHOOT_COOLDOWN_BASE 0.18f

static float randf(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

static uint32_t* load_tex(AAssetManager* m, const char* f, int* w, int* h) {
    if (!m || !f) return NULL;
    AAsset* a = AAssetManager_open(m, f, AASSET_MODE_BUFFER);
    if (!a) return NULL;
    size_t sz = AAsset_getLength(a);
    unsigned char* d = (unsigned char*)malloc(sz);
    if (!d) {
        AAsset_close(a);
        return NULL;
    }
    AAsset_read(a, d, sz);
    AAsset_close(a);

    int n;
    unsigned char* img = stbi_load_from_memory(d, sz, w, h, &n, 4);
    free(d);
    if (!img) return NULL;

    uint32_t* p = (uint32_t*)malloc((*w) * (*h) * sizeof(uint32_t));
    if (!p) {
        stbi_image_free(img);
        return NULL;
    }
    for (int i = 0; i < (*w) * (*h); i++) {
        uint8_t r = img[i * 4];
        uint8_t g = img[i * 4 + 1];
        uint8_t b = img[i * 4 + 2];
        uint8_t a = img[i * 4 + 3];
        p[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
    stbi_image_free(img);
    return p;
}

void game_spawn_bullet(Game* g, float x, float y, float vx, float vy, int is_enemy, int damage, uint32_t color) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g->bullets[i].active) {
            g->bullets[i].x = x;
            g->bullets[i].y = y;
            g->bullets[i].vx = vx;
            g->bullets[i].vy = vy;
            g->bullets[i].damage = damage;
            g->bullets[i].is_enemy = is_enemy;
            g->bullets[i].color = color;
            g->bullets[i].radius = is_enemy ? 8 : 6;
            g->bullets[i].active = 1;
            break;
        }
    }
}

void game_spawn_particles(Game* g, float x, float y, uint32_t color, int count, float speed) {
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!g->particles[j].active) {
                float angle = randf(0.0f, 6.283185f);
                float spd = randf(speed * 0.3f, speed);
                g->particles[j].x = x;
                g->particles[j].y = y;
                g->particles[j].vx = cosf(angle) * spd;
                g->particles[j].vy = sinf(angle) * spd;
                g->particles[j].max_life = randf(0.3f, 0.7f);
                g->particles[j].life = g->particles[j].max_life;
                g->particles[j].radius = (int)randf(2.0f, 6.0f);
                g->particles[j].color = color;
                g->particles[j].active = 1;
                break;
            }
        }
    }
}

void game_spawn_pickup(Game* g, float x, float y, PickupType type) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!g->pickups[i].active) {
            g->pickups[i].x = x;
            g->pickups[i].y = y;
            g->pickups[i].type = type;
            g->pickups[i].radius = 16;
            g->pickups[i].pulse = 0.0f;
            g->pickups[i].active = 1;
            break;
        }
    }
}

void game_spawn_enemy(Game* g, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) {
            float ex = 0.0f, ey = 0.0f;
            int side = rand() % 4;
            if (side == 0) {
                ex = randf(0, (float)g->screen_w);
                ey = -40.0f;
            } else if (side == 1) {
                ex = randf(0, (float)g->screen_w);
                ey = (float)g->screen_h + 40.0f;
            } else if (side == 2) {
                ex = -40.0f;
                ey = randf(0, (float)g->screen_h);
            } else {
                ex = (float)g->screen_w + 40.0f;
                ey = randf(0, (float)g->screen_h);
            }

            g->enemies[i].x = ex;
            g->enemies[i].y = ey;
            g->enemies[i].type = type;
            g->enemies[i].active = 1;
            g->enemies[i].hit_flash = 0.0f;

            if (type == ENEMY_TYPE_FAST) {
                g->enemies[i].max_hp = 30;
                g->enemies[i].hp = 30;
                g->enemies[i].speed = randf(4.5f, 6.0f);
                g->enemies[i].radius = 16;
                g->enemies[i].color = 0xFFFF5533; // Bright Orange-Red
            } else if (type == ENEMY_TYPE_TANK) {
                g->enemies[i].max_hp = 180;
                g->enemies[i].hp = 180;
                g->enemies[i].speed = randf(1.2f, 1.8f);
                g->enemies[i].radius = 34;
                g->enemies[i].color = 0xFF882299; // Purple
            } else {
                g->enemies[i].max_hp = 60;
                g->enemies[i].hp = 60;
                g->enemies[i].speed = randf(2.5f, 3.5f);
                g->enemies[i].radius = 22;
                g->enemies[i].color = 0xFFCC2222; // Red
            }
            g->enemies_alive++;
            break;
        }
    }
}

static void start_next_wave(Game* g) {
    g->wave++;
    g->enemies_to_spawn = 6 + g->wave * 4;
    g->spawn_timer = 0.5f;
    g->wave_delay_timer = 0.0f;
}

void game_restart(Game* g) {
    g->state = GAME_STATE_PLAYING;
    g->player.x = g->screen_w / 2.0f;
    g->player.y = g->screen_h / 2.0f;
    g->player.hp = PLAYER_MAX_HP;
    g->player.max_hp = PLAYER_MAX_HP;
    g->player.invulnerable_timer = 1.0f;
    g->player.shoot_cooldown = 0.0f;
    g->player.weapon_level = 1;
    g->score = 0;
    g->wave = 0;
    g->enemies_alive = 0;
    g->screen_shake = 0.0f;

    memset(g->bullets, 0, sizeof(g->bullets));
    memset(g->enemies, 0, sizeof(g->enemies));
    memset(g->particles, 0, sizeof(g->particles));
    memset(g->pickups, 0, sizeof(g->pickups));

    start_next_wave(g);
}

int game_init(Game* g, int w, int h, AAssetManager* m) {
    if (!g) return 0;
    memset(g, 0, sizeof(Game));
    g->screen_w = w;
    g->screen_h = h;
    g->asset_mgr = m;

    g->player.scale = PLAYER_SCALE;
    g->joy.centerX = 150;
    g->joy.centerY = h - 150;
    g->joy.radius = 85;

    g->attackBtn.x = w - 130;
    g->attackBtn.y = h - 130;
    g->attackBtn.radius = 65;

    g->restartBtn.x = w / 2;
    g->restartBtn.y = h / 2 + 80;
    g->restartBtn.radius = 55;

    if (m) {
        g->player.texture = load_tex(m, "cube.png", &g->player.tex_width, &g->player.tex_height);
        g->player.tex_ready = (g->player.texture != NULL);

        g->fontSize = h / 28;
        if (g->fontSize < 14) g->fontSize = 14;
        if (g->fontSize > 42) g->fontSize = 42;

        AAsset* fa = AAssetManager_open(m, "Roboto-Regular.ttf", AASSET_MODE_BUFFER);
        if (fa) {
            size_t sz = AAsset_getLength(fa);
            unsigned char* fd = (unsigned char*)malloc(sz);
            if (fd) {
                AAsset_read(fa, fd, sz);
                AAsset_close(fa);
                font_init(&g->font, fd, sz, (float)g->fontSize);
                free(fd);
            } else {
                AAsset_close(fa);
            }
        }
    }

    gettimeofday(&g->lastTime, NULL);
    game_restart(g);
    return 1;
}

static void player_shoot(Game* g) {
    float angle = g->player.angle;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    float muzzle_dist = 44.0f;
    float sx = g->player.x + muzzle_dist * cos_a;
    float sy = g->player.y + muzzle_dist * sin_a;

    if (g->player.weapon_level == 1) {
        // Single shot
        game_spawn_bullet(g, sx, sy, BULLET_SPEED * cos_a, BULLET_SPEED * sin_a, 0, 35, 0xFFFFFF00);
    } else if (g->player.weapon_level == 2) {
        // Double shot
        float perp_x = -sin_a * 12.0f;
        float perp_y = cos_a * 12.0f;
        game_spawn_bullet(g, sx + perp_x, sy + perp_y, BULLET_SPEED * cos_a, BULLET_SPEED * sin_a, 0, 30, 0xFF33FFFF);
        game_spawn_bullet(g, sx - perp_x, sy - perp_y, BULLET_SPEED * cos_a, BULLET_SPEED * sin_a, 0, 30, 0xFF33FFFF);
    } else {
        // Triple spread shot
        float spread1 = angle - 0.18f;
        float spread2 = angle + 0.18f;
        game_spawn_bullet(g, sx, sy, BULLET_SPEED * cos_a, BULLET_SPEED * sin_a, 0, 32, 0xFFFF33CC);
        game_spawn_bullet(g, sx, sy, BULLET_SPEED * cosf(spread1), BULLET_SPEED * sinf(spread1), 0, 28, 0xFFFF33CC);
        game_spawn_bullet(g, sx, sy, BULLET_SPEED * cosf(spread2), BULLET_SPEED * sinf(spread2), 0, 28, 0xFFFF33CC);
    }

    // Muzzle particles
    game_spawn_particles(g, sx, sy, 0xFFFFFFAA, 3, 3.0f);
}

void game_update(Game* g, int w, int h) {
    if (!g) return;
    g->screen_w = w;
    g->screen_h = h;
    g->joy.centerY = h - 150;
    g->attackBtn.x = w - 130;
    g->attackBtn.y = h - 130;
    g->restartBtn.x = w / 2;
    g->restartBtn.y = h / 2 + 80;

    int ns = h / 28;
    if (ns < 14) ns = 14;
    if (ns > 42) ns = 42;
    if (ns != g->fontSize && g->font) {
        g->fontSize = ns;
        font_set_size(g->font, (float)ns);
    }

    // Calculate delta time
    struct timeval now;
    gettimeofday(&now, NULL);
    float dt = (now.tv_sec - g->lastTime.tv_sec) + (now.tv_usec - g->lastTime.tv_usec) / 1000000.0f;
    g->lastTime = now;
    if (dt > 0.1f) dt = 0.1f; // Clamp delta time

    // FPS calculation
    g->frameCount++;
    static float fps_timer = 0.0f;
    fps_timer += dt;
    if (fps_timer >= 0.5f) {
        g->fps = (float)g->frameCount / fps_timer;
        g->frameCount = 0;
        fps_timer = 0.0f;
    }

    // Screen shake decay
    if (g->screen_shake > 0.0f) {
        g->screen_shake -= dt * 15.0f;
        if (g->screen_shake < 0.0f) g->screen_shake = 0.0f;
    }

    // If game over, only handle restart logic
    if (g->state == GAME_STATE_GAMEOVER) {
        return;
    }

    // Update Player invulnerability
    if (g->player.invulnerable_timer > 0.0f) {
        g->player.invulnerable_timer -= dt;
    }

    // Player movement
    float move_speed = 8.5f;
    g->player.x += g->joy.dirX * move_speed;
    g->player.y += g->joy.dirY * move_speed;

    float player_bounds = 36.0f;
    if (g->player.x < player_bounds) g->player.x = player_bounds;
    if (g->player.x > (float)w - player_bounds) g->player.x = (float)w - player_bounds;
    if (g->player.y < player_bounds) g->player.y = player_bounds;
    if (g->player.y > (float)h - player_bounds) g->player.y = (float)h - player_bounds;

    // Player rotation
    float joy_len = hypotf(g->joy.dirX, g->joy.dirY);
    if (joy_len > 0.05f) {
        g->player.angle = atan2f(g->joy.dirY, g->joy.dirX);
        g->player.last_angle = g->player.angle;
    } else {
        g->player.angle = g->player.last_angle;
    }

    // Auto-fire while attack button is pressed
    if (g->player.shoot_cooldown > 0.0f) {
        g->player.shoot_cooldown -= dt;
    }
    if (g->attackBtn.pressed && g->player.shoot_cooldown <= 0.0f) {
        player_shoot(g);
        float cd = SHOOT_COOLDOWN_BASE - (float)(g->player.weapon_level - 1) * 0.03f;
        if (cd < 0.1f) cd = 0.1f;
        g->player.shoot_cooldown = cd;
    }

    // Spawn Enemy Waves
    if (g->enemies_to_spawn > 0) {
        g->spawn_timer -= dt;
        if (g->spawn_timer <= 0.0f) {
            EnemyType t = ENEMY_TYPE_NORMAL;
            int r = rand() % 100;
            if (r < 25 && g->wave >= 2) {
                t = ENEMY_TYPE_FAST;
            } else if (r < 45 && g->wave >= 3) {
                t = ENEMY_TYPE_TANK;
            }
            game_spawn_enemy(g, t);
            g->enemies_to_spawn--;
            g->spawn_timer = randf(0.4f, 1.2f);
        }
    } else if (g->enemies_alive == 0) {
        g->wave_delay_timer += dt;
        if (g->wave_delay_timer >= 2.0f) {
            start_next_wave(g);
        }
    }

    // Update Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g->bullets[i].active) continue;
        g->bullets[i].x += g->bullets[i].vx;
        g->bullets[i].y += g->bullets[i].vy;

        // Screen boundary check
        if (g->bullets[i].x < -50.0f || g->bullets[i].x > (float)w + 50.0f ||
            g->bullets[i].y < -50.0f || g->bullets[i].y > (float)h + 50.0f) {
            g->bullets[i].active = 0;
            continue;
        }

        // Check bullet collision with enemies
        if (!g->bullets[i].is_enemy) {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!g->enemies[e].active) continue;
                float edx = g->bullets[i].x - g->enemies[e].x;
                float edy = g->bullets[i].y - g->enemies[e].y;
                float edist2 = edx * edx + edy * edy;
                float hit_r = (float)(g->enemies[e].radius + g->bullets[i].radius);

                if (edist2 < hit_r * hit_r) {
                    g->enemies[e].hp -= g->bullets[i].damage;
                    g->enemies[e].hit_flash = 0.12f;
                    g->bullets[i].active = 0;

                    // Spark particles
                    game_spawn_particles(g, g->bullets[i].x, g->bullets[i].y, 0xFFFFAA33, 4, 4.0f);

                    // Enemy death
                    if (g->enemies[e].hp <= 0) {
                        g->enemies[e].active = 0;
                        g->enemies_alive--;
                        int add_score = (g->enemies[e].type == ENEMY_TYPE_TANK) ? 50 :
                                        (g->enemies[e].type == ENEMY_TYPE_FAST) ? 30 : 20;
                        g->score += add_score;
                        if (g->score > g->high_score) g->high_score = g->score;

                        // Big explosion
                        game_spawn_particles(g, g->enemies[e].x, g->enemies[e].y, g->enemies[e].color, 16, 7.0f);
                        game_spawn_particles(g, g->enemies[e].x, g->enemies[e].y, 0xFFFFAA00, 8, 5.0f);

                        // Random pickup drop
                        int drop_roll = rand() % 100;
                        if (drop_roll < 15) {
                            game_spawn_pickup(g, g->enemies[e].x, g->enemies[e].y, PICKUP_TYPE_HEALTH);
                        } else if (drop_roll < 25) {
                            game_spawn_pickup(g, g->enemies[e].x, g->enemies[e].y, PICKUP_TYPE_WEAPON);
                        } else if (drop_roll < 45) {
                            game_spawn_pickup(g, g->enemies[e].x, g->enemies[e].y, PICKUP_TYPE_SCORE);
                        }
                    }
                    break;
                }
            }
        }
    }

    // Update Enemies (AI)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) continue;

        if (g->enemies[i].hit_flash > 0.0f) {
            g->enemies[i].hit_flash -= dt;
        }

        // Move towards player
        float dx = g->player.x - g->enemies[i].x;
        float dy = g->player.y - g->enemies[i].y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            g->enemies[i].x += (dx / dist) * g->enemies[i].speed;
            g->enemies[i].y += (dy / dist) * g->enemies[i].speed;
        }

        // Collision with player
        float coll_r = (float)(g->enemies[i].radius + 24);
        if (dist < coll_r) {
            if (g->player.invulnerable_timer <= 0.0f) {
                int dmg = (g->enemies[i].type == ENEMY_TYPE_TANK) ? 35 : 20;
                g->player.hp -= dmg;
                g->player.invulnerable_timer = 0.8f;
                g->screen_shake = 12.0f;
                game_spawn_particles(g, g->player.x, g->player.y, 0xFFFF2222, 12, 6.0f);

                if (g->player.hp <= 0) {
                    g->player.hp = 0;
                    g->state = GAME_STATE_GAMEOVER;
                    game_spawn_particles(g, g->player.x, g->player.y, 0xFFFFFF00, 30, 9.0f);
                    game_spawn_particles(g, g->player.x, g->player.y, 0xFFFF3333, 25, 7.0f);
                }
            }
        }
    }

    // Update Pickups
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!g->pickups[i].active) continue;
        g->pickups[i].pulse += dt * 4.0f;

        float pdx = g->player.x - g->pickups[i].x;
        float pdy = g->player.y - g->pickups[i].y;
        float pdist = sqrtf(pdx * pdx + pdy * pdy);

        if (pdist < (float)(g->pickups[i].radius + 28)) {
            g->pickups[i].active = 0;
            if (g->pickups[i].type == PICKUP_TYPE_HEALTH) {
                g->player.hp += 30;
                if (g->player.hp > g->player.max_hp) g->player.hp = g->player.max_hp;
                game_spawn_particles(g, g->player.x, g->player.y, 0xFF33FF33, 10, 4.0f);
            } else if (g->pickups[i].type == PICKUP_TYPE_WEAPON) {
                if (g->player.weapon_level < 3) g->player.weapon_level++;
                game_spawn_particles(g, g->player.x, g->player.y, 0xFF33FFFF, 14, 5.0f);
            } else {
                g->score += 100;
                if (g->score > g->high_score) g->high_score = g->score;
                game_spawn_particles(g, g->player.x, g->player.y, 0xFFFFFF33, 8, 4.0f);
            }
        }
    }

    // Update Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g->particles[i].active) continue;
        g->particles[i].x += g->particles[i].vx;
        g->particles[i].y += g->particles[i].vy;
        g->particles[i].vx *= 0.94f;
        g->particles[i].vy *= 0.94f;
        g->particles[i].life -= dt;
        if (g->particles[i].life <= 0.0f) {
            g->particles[i].active = 0;
        }
    }
}

void game_draw(Game* g, RenderBuffer* b) {
    if (!g || !b || !b->pixels) return;

    // Screen clear with arena dark-grey color
    graphics_clear(b, 0xFF222428);

    // Subtle arena grid lines
    for (int x = 0; x < b->width; x += 100) {
        graphics_draw_line(b, x, 0, x, b->height, 0xFF2D3036);
    }
    for (int y = 0; y < b->height; y += 100) {
        graphics_draw_line(b, 0, y, b->width, y, 0xFF2D3036);
    }

    // Draw Pickups
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!g->pickups[i].active) continue;
        int px = (int)g->pickups[i].x;
        int py = (int)g->pickups[i].y;
        float s = 1.0f + 0.15f * sinf(g->pickups[i].pulse);
        int r = (int)(g->pickups[i].radius * s);

        if (g->pickups[i].type == PICKUP_TYPE_HEALTH) {
            graphics_draw_circle(b, px, py, r, 0xFF22BB44);
            graphics_draw_rect_exact(b, px - 2, py - 8, 4, 16, 0xFFFFFFFF);
            graphics_draw_rect_exact(b, px - 8, py - 2, 16, 4, 0xFFFFFFFF);
        } else if (g->pickups[i].type == PICKUP_TYPE_WEAPON) {
            graphics_draw_circle(b, px, py, r, 0xFF2288EE);
            graphics_draw_ring(b, px, py, r - 3, 3, 0xFFFFFFFF);
        } else {
            graphics_draw_circle(b, px, py, r, 0xFFEEBB22);
            graphics_draw_ring(b, px, py, r - 3, 2, 0xFFFFFFFF);
        }
    }

    // Draw Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g->particles[i].active) continue;
        float alpha = g->particles[i].life / g->particles[i].max_life;
        graphics_draw_particle(b, (int)g->particles[i].x, (int)g->particles[i].y,
                               g->particles[i].radius, g->particles[i].color, alpha);
    }

    // Draw Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g->bullets[i].active) continue;
        graphics_draw_circle(b, (int)g->bullets[i].x, (int)g->bullets[i].y,
                             g->bullets[i].radius, g->bullets[i].color);
    }

    // Draw Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g->enemies[i].active) continue;
        int ex = (int)g->enemies[i].x;
        int ey = (int)g->enemies[i].y;
        uint32_t col = (g->enemies[i].hit_flash > 0.0f) ? 0xFFFFFFFF : g->enemies[i].color;

        graphics_draw_circle(b, ex, ey, g->enemies[i].radius, col);
        graphics_draw_ring(b, ex, ey, g->enemies[i].radius, 3, 0xFF000000);

        // Mini health bar above damaged enemies
        if (g->enemies[i].hp < g->enemies[i].max_hp) {
            int bw = g->enemies[i].radius * 2;
            graphics_draw_health_bar(b, ex - g->enemies[i].radius, ey - g->enemies[i].radius - 12,
                                     bw, 6, g->enemies[i].hp, g->enemies[i].max_hp,
                                     0xFFEE2222, 0xFF550000, 0xFF000000);
        }
    }

    // Draw Player
    if (g->state == GAME_STATE_PLAYING) {
        int px = (int)g->player.x;
        int py = (int)g->player.y;

        int is_visible = 1;
        if (g->player.invulnerable_timer > 0.0f) {
            int flash = (int)(g->player.invulnerable_timer * 20.0f);
            if (flash % 2 == 0) is_visible = 0;
        }

        if (is_visible) {
            if (g->player.tex_ready) {
                graphics_draw_texture_ex(b, px, py,
                                         g->player.texture, g->player.tex_width, g->player.tex_height,
                                         g->player.angle, g->player.scale);
            } else {
                graphics_draw_rect(b, px, py, 60, 0xFFEE7722);
            }
            // Gun barrel indicator
            float cos_a = cosf(g->player.angle);
            float sin_a = sinf(g->player.angle);
            int gx = px + (int)(36.0f * cos_a);
            int gy = py + (int)(36.0f * sin_a);
            graphics_draw_circle(b, gx, gy, 5, 0xFF333333);
        }
    }

    // UI Layer
    ui_draw_joystick(b, &g->joy);
    ui_draw_button(g->font, b, &g->attackBtn, "Атака");

    // Top HUD
    // Player Health Bar
    graphics_draw_health_bar(b, 20, 20, 200, 24, g->player.hp, g->player.max_hp,
                             0xFF22CC44, 0xFF660000, 0xFF000000);
    char hp_str[32];
    snprintf(hp_str, sizeof(hp_str), "HP: %d/%d", g->player.hp, g->player.max_hp);
    draw_text_outlined(g->font, b, 230, 22, hp_str, 0xFFFFFFFF, 0xFF000000);

    // Wave & Score HUD
    char hud_str[64];
    snprintf(hud_str, sizeof(hud_str), "Волна: %d | Очки: %d", g->wave, g->score);
    draw_text_outlined(g->font, b, b->width / 2 - 120, 22, hud_str, 0xFFFFDD33, 0xFF000000);

    // FPS HUD
    char fps_str[32];
    snprintf(fps_str, sizeof(fps_str), "FPS: %d", (int)(g->fps + 0.5f));
    draw_text_outlined(g->font, b, b->width - 120, 22, fps_str, 0xFFFFFFFF, 0xFF000000);

    // Game Over Overlay
    if (g->state == GAME_STATE_GAMEOVER) {
        // Dark translucent overlay
        graphics_draw_rect_exact(b, 0, 0, b->width, b->height, 0xCC000000);

        draw_text_outlined(g->font, b, b->width / 2 - 110, b->height / 2 - 80,
                           "ИГРА ОКОНЧЕНА", 0xFFFF2222, 0xFF000000);

        char final_score[64];
        snprintf(final_score, sizeof(final_score), "Финальный счёт: %d  (Рекорд: %d)", g->score, g->high_score);
        draw_text_outlined(g->font, b, b->width / 2 - 180, b->height / 2 - 20,
                           final_score, 0xFFFFFFFF, 0xFF000000);

        ui_draw_button(g->font, b, &g->restartBtn, "Заново");
    }
}

void game_free(Game* g) {
    if (!g) return;
    if (g->player.texture) {
        free(g->player.texture);
        g->player.texture = NULL;
    }
    if (g->font) {
        font_free(g->font);
        g->font = NULL;
    }
}
