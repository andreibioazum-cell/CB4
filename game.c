#include "game.h"
#include "graphics.h"
#include "ui.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <android/asset_manager.h>
#include <android/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TEXTURE_OFFSET 0.0f

static uint32_t* load_texture(AAssetManager* mgr, const char* fn, int* w, int* h) {
    AAsset* a = AAssetManager_open(mgr, fn, AASSET_MODE_BUFFER);
    if (!a) return NULL;
    size_t sz = AAsset_getLength(a);
    unsigned char* data = (unsigned char*)malloc(sz);
    AAsset_read(a, data, sz);
    AAsset_close(a);
    int n;
    unsigned char* img = stbi_load_from_memory(data, sz, w, h, &n, 4);
    free(data);
    if (!img) return NULL;
    uint32_t* pix = (uint32_t*)malloc((*w)*(*h)*sizeof(uint32_t));
    for (int i = 0; i < (*w)*(*h); ++i) {
        uint8_t r = img[i*4], g = img[i*4+1], b = img[i*4+2], a = img[i*4+3];
        pix[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
    stbi_image_free(img);
    return pix;
}

int game_init(Game* g, int w, int h, AAssetManager* mgr) {
    memset(g, 0, sizeof(Game));
    g->screen_w = w; g->screen_h = h;
    g->player.x = w/2.0f; g->player.y = h/2.0f;
    g->player.scale = 1.5f;
    g->joy.centerX = 150; g->joy.centerY = h-150; g->joy.radius = 80;

    g->player.texture = load_texture(mgr, "cube.png", &g->player.tex_width, &g->player.tex_height);
    g->player.tex_ready = (g->player.texture != NULL);

    g->fontSize = h/30; if(g->fontSize<12) g->fontSize=12; if(g->fontSize>48) g->fontSize=48;
    AAsset* fa = AAssetManager_open(mgr, "Roboto-Regular.ttf", AASSET_MODE_BUFFER);
    if (fa) {
        size_t sz = AAsset_getLength(fa);
        unsigned char* fdata = (unsigned char*)malloc(sz);
        AAsset_read(fa, fdata, sz);
        AAsset_close(fa);
        if (!font_init(&g->font, fdata, sz, (float)g->fontSize))
            __android_log_print(ANDROID_LOG_WARN, "GAME", "Font init failed");
        free(fdata);
    } else
        __android_log_print(ANDROID_LOG_WARN, "GAME", "Font not found");

    gettimeofday(&g->lastTime, NULL);
    return 1;
}

void game_update(Game* g, int w, int h) {
    g->screen_w = w; g->screen_h = h; g->joy.centerY = h-150;
    int newSz = h/30; if(newSz<12) newSz=12; if(newSz>48) newSz=48;
    if (newSz != g->fontSize && g->font) {
        g->fontSize = newSz;
        font_set_size(g->font, (float)newSz);
    }

    g->player.x += g->joy.dirX * 10.0f;
    g->player.y += g->joy.dirY * 10.0f;

    float scale = g->player.scale;
    float maxExt = g->player.tex_ready ?
        sqrtf(powf(g->player.tex_width*scale*0.5f,2) + powf(g->player.tex_height*scale*0.5f,2)) : 40.0f;
    if (g->player.x < maxExt) g->player.x = maxExt;
    if (g->player.x > w - maxExt) g->player.x = w - maxExt;
    if (g->player.y < maxExt) g->player.y = maxExt;
    if (g->player.y > h - maxExt) g->player.y = h - maxExt;

    float len = sqrtf(g->joy.dirX*g->joy.dirX + g->joy.dirY*g->joy.dirY);
    if (len > 0.001f) {
        g->player.angle = atan2f(g->joy.dirX, -g->joy.dirY) + TEXTURE_OFFSET;
        g->player.last_angle = g->player.angle;
    } else
        g->player.angle = g->player.last_angle;

    g->frameCount++;
    struct timeval now;
    gettimeofday(&now, NULL);
    float dt = (now.tv_sec - g->lastTime.tv_sec) + (now.tv_usec - g->lastTime.tv_usec)/1000000.0f;
    if (dt >= 1.0f) { g->fps = g->frameCount/dt; g->frameCount=0; g->lastTime=now; }
}

void game_draw(Game* g, RenderBuffer* rb) {
    graphics_clear(rb, 0xFFCCCCCC);
    if (g->player.tex_ready)
        graphics_draw_texture_ex(rb, (int)g->player.x, (int)g->player.y,
                                 g->player.texture, g->player.tex_width, g->player.tex_height,
                                 g->player.angle, g->player.scale);
    else
        graphics_draw_rect(rb, (int)g->player.x, (int)g->player.y, 80, 0xFFEE7722);

    ui_draw_joystick(rb, &g->joy);

    char fps[32];
    snprintf(fps, sizeof(fps), "FPS: %.1f", g->fps);
    font_draw_text(g->font, rb, rb->width-120, 20, fps, 0xFFFFFFFF);
}

void game_free(Game* g) {
    if (g->player.texture) { free(g->player.texture); g->player.texture = NULL; }
    if (g->font) { font_free(g->font); g->font = NULL; }
}
