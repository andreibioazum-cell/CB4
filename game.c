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

static uint32_t* load_texture(AAssetManager* mgr, const char* filename, int* w, int* h) {
    AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_BUFFER);
    if (!asset) return NULL;
    size_t size = AAsset_getLength(asset);
    unsigned char* data = (unsigned char*)malloc(size);
    AAsset_read(asset, data, size);
    AAsset_close(asset);
    int n;
    unsigned char* img = stbi_load_from_memory(data, size, w, h, &n, 4);
    free(data);
    if (!img) return NULL;
    uint32_t* pixels = (uint32_t*)malloc((*w) * (*h) * sizeof(uint32_t));
    for (int i = 0; i < (*w) * (*h); ++i) {
        uint8_t r = img[i*4], g = img[i*4+1], b = img[i*4+2], a = img[i*4+3];
        pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    stbi_image_free(img);
    return pixels;
}

int game_init(Game* g, int w, int h, AAssetManager* mgr) {
    memset(g, 0, sizeof(Game));
    g->screen_w = w;
    g->screen_h = h;
    g->player.x = w / 2.0f;
    g->player.y = h / 2.0f;
    g->player.scale = 1.5f;
    g->joy.centerX = 150;
    g->joy.centerY = h - 150;
    g->joy.radius = 80;
    g->joy.dirX = g->joy.dirY = 0.0f;
    g->joy.touchOffX = g->joy.touchOffY = 0.0f;

    // Загрузка текстуры игрока
    g->player.texture = load_texture(mgr, "cube.png", &g->player.tex_width, &g->player.tex_height);
    g->player.tex_ready = (g->player.texture != NULL);

    // Загрузка шрифта с авторазмером
    g->fontSize = h / 30;   // например, 1/30 высоты экрана
    if (g->fontSize < 12) g->fontSize = 12;  // минимальный размер
    if (g->fontSize > 48) g->fontSize = 48;  // максимальный

    AAsset* font_asset = AAssetManager_open(mgr, "Roboto-Regular.ttf", AASSET_MODE_BUFFER);
    if (font_asset) {
        size_t size = AAsset_getLength(font_asset);
        unsigned char* font_data = (unsigned char*)malloc(size);
        AAsset_read(font_asset, font_data, size);
        AAsset_close(font_asset);
        font_init(&g->font, font_data, size, (float)g->fontSize);
        free(font_data);
    }

    gettimeofday(&g->lastTime, NULL);
    return 1;
}

void game_update(Game* g, int w, int h) {
    g->screen_w = w;
    g->screen_h = h;
    g->joy.centerY = h - 150;

    // Автообновление размера шрифта при изменении высоты экрана
    int newFontSize = h / 30;
    if (newFontSize < 12) newFontSize = 12;
    if (newFontSize > 48) newFontSize = 48;
    if (newFontSize != g->fontSize && g->font) {
        g->fontSize = newFontSize;
        font_set_size(g->font, (float)newFontSize);
    }

    // Движение
    g->player.x += g->joy.dirX * 10.0f;
    g->player.y += g->joy.dirY * 10.0f;

    float scale = g->player.scale;
    float maxExtent = 0.0f;
    if (g->player.tex_ready) {
        float halfW = g->player.tex_width * scale * 0.5f;
        float halfH = g->player.tex_height * scale * 0.5f;
        maxExtent = sqrtf(halfW*halfW + halfH*halfH);
    } else {
        maxExtent = 40.0f;
    }
    if (g->player.x < maxExtent) g->player.x = maxExtent;
    if (g->player.x > w - maxExtent) g->player.x = w - maxExtent;
    if (g->player.y < maxExtent) g->player.y = maxExtent;
    if (g->player.y > h - maxExtent) g->player.y = h - maxExtent;

    float dirLen = sqrtf(g->joy.dirX*g->joy.dirX + g->joy.dirY*g->joy.dirY);
    if (dirLen > 0.001f) {
        g->player.angle = atan2f(g->joy.dirX, -g->joy.dirY);
    } else {
        g->player.angle = 0.0f;
    }

    // FPS
    g->frameCount++;
    struct timeval now;
    gettimeofday(&now, NULL);
    float dt = (now.tv_sec - g->lastTime.tv_sec) + (now.tv_usec - g->lastTime.tv_usec) / 1000000.0f;
    if (dt >= 1.0f) {
        g->fps = g->frameCount / dt;
        g->frameCount = 0;
        g->lastTime = now;
    }
}

void game_draw(Game* g, RenderBuffer* rb) {
    graphics_clear(rb, 0xFFCCCCCC);

    if (g->player.tex_ready) {
        graphics_draw_texture_ex(rb, (int)g->player.x, (int)g->player.y,
                                 g->player.texture, g->player.tex_width, g->player.tex_height,
                                 g->player.angle, g->player.scale);
    } else {
        graphics_draw_rect(rb, (int)g->player.x, (int)g->player.y, 80, 0xFFEE7722);
    }

    ui_draw_joystick(rb, &g->joy);

    if (g->font) {
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", g->fps);
        // Позиция справа сверху с отступом
        font_draw_text(g->font, rb, rb->width - 160, 20, fpsText, 0xFFFFFFFF);
    }
}

void game_free(Game* g) {
    if (g->player.texture) {
        free(g->player.texture);
        g->player.texture = NULL;
    }
    if (g->font) {
        font_free(g->font);
        g->font = NULL;
    }
}

void game_handle_touch(Game* g, float x, float y, int action) {
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        g->joy.dirX = g->joy.dirY = 0.0f;
        g->joy.touchOffX = g->joy.touchOffY = 0.0f;
        return;
    }
    float dx = x - g->joy.centerX;
    float dy = y - g->joy.centerY;
    float len = sqrtf(dx*dx + dy*dy);
    const float deadZone = 20.0f;
    if (len > deadZone) {
        float invLen = 1.0f / len;
        g->joy.dirX = dx * invLen;
        g->joy.dirY = dy * invLen;
        if (len > g->joy.radius) {
            g->joy.touchOffX = g->joy.dirX * g->joy.radius;
            g->joy.touchOffY = g->joy.dirY * g->joy.radius;
        } else {
            g->joy.touchOffX = dx;
            g->joy.touchOffY = dy;
        }
    } else {
        g->joy.dirX = g->joy.dirY = 0.0f;
        g->joy.touchOffX = g->joy.touchOffY = 0.0f;
    }
}
