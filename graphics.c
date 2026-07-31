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

#define OFF 0.0f

static uint32_t* load_tex(AAssetManager* m, const char* f, int* w, int* h) {
    AAsset* a = AAssetManager_open(m, f, AASSET_MODE_BUFFER);
    if (!a) return 0;
    size_t sz = AAsset_getLength(a);
    unsigned char* d = (unsigned char*)malloc(sz);
    AAsset_read(a, d, sz); AAsset_close(a);
    int n; unsigned char* img = stbi_load_from_memory(d, sz, w, h, &n, 4);
    free(d);
    if (!img) return 0;
    uint32_t* p = (uint32_t*)malloc((*w)*(*h)*4);
    for (int i=0; i<(*w)*(*h); ++i) {
        uint8_t r=img[i*4], g=img[i*4+1], b=img[i*4+2], a=img[i*4+3];
        p[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
    stbi_image_free(img);
    return p;
}

int game_init(Game* g, int w, int h, AAssetManager* m) {
    memset(g, 0, sizeof(Game));
    g->screen_w=w; g->screen_h=h;
    g->player.x=w/2.0f; g->player.y=h/2.0f; g->player.scale=1.5f;
    g->joy.centerX=150; g->joy.centerY=h-150; g->joy.radius=80;
    g->player.texture = load_tex(m, "cube.png", &g->player.tex_width, &g->player.tex_height);
    g->player.tex_ready = (g->player.texture != 0);
    g->fontSize = h/30; if(g->fontSize<12) g->fontSize=12; if(g->fontSize>48) g->fontSize=48;
    AAsset* fa = AAssetManager_open(m, "Roboto-Regular.ttf", AASSET_MODE_BUFFER);
    if (fa) {
        size_t sz = AAsset_getLength(fa);
        unsigned char* fd = (unsigned char*)malloc(sz);
        AAsset_read(fa, fd, sz); AAsset_close(fa);
        font_init(&g->font, fd, sz, (float)g->fontSize);
        free(fd);
    }
    gettimeofday(&g->lastTime, 0);
    return 1;
}

void game_update(Game* g, int w, int h) {
    g->screen_w=w; g->screen_h=h; g->joy.centerY=h-150;
    int ns = h/30; if(ns<12) ns=12; if(ns>48) ns=48;
    if (ns != g->fontSize && g->font) { g->fontSize=ns; font_set_size(g->font, (float)ns); }
    g->player.x += g->joy.dirX * 10.0f;
    g->player.y += g->joy.dirY * 10.0f;
    float sc = g->player.scale;
    float maxE = g->player.tex_ready ? hypotf(g->player.tex_width*sc*0.5f, g->player.tex_height*sc*0.5f) : 40.0f;
    if (g->player.x < maxE) g->player.x = maxE;
    if (g->player.x > w - maxE) g->player.x = w - maxE;
    if (g->player.y < maxE) g->player.y = maxE;
    if (g->player.y > h - maxE) g->player.y = h - maxE;
    float len = hypotf(g->joy.dirX, g->joy.dirY);
    if (len > 0.001f) {
        g->player.angle = atan2f(g->joy.dirX, -g->joy.dirY) + OFF;
        g->player.last_angle = g->player.angle;
    } else g->player.angle = g->player.last_angle;
    g->frameCount++;
    struct timeval now;
    gettimeofday(&now, 0);
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
    int fps_int = (int)(g->fps + 0.5f);
    snprintf(fps, sizeof(fps), "FPS: %d", fps_int);
    font_draw_text(g->font, rb, rb->width-120, 40, fps, 0xFF000000);
}

void game_free(Game* g) {
    if (g->player.texture) { free(g->player.texture); g->player.texture = 0; }
    if (g->font) { font_free(g->font); g->font = 0; }
}
