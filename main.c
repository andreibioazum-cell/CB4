#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <math.h>
#include <stdlib.h>
#include "graphics.h"
#include "ui.h"

// Используем stb_image только для распаковки из памяти
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct engine {
    struct android_app* app;
    Joystick joy;
    Image player;
    float x, y;
    int active;
};

// Полностью новая функция загрузки
void safe_load_asset(struct engine* e) {
    AAssetManager* am = e->app->activity->assetManager;
    AAsset* asset = AAssetManager_open(am, "cube_ordinary.png", AASSET_MODE_BUFFER);
    if (!asset) return;

    size_t size = AAsset_getLength(asset);
    unsigned char* fileData = (unsigned char*)malloc(size);
    if (fileData) {
        AAsset_read(asset, fileData, size);
        
        int w, h, n;
        // Принудительно запрашиваем 4 канала (RGBA)
        unsigned char* decoded = stbi_load_from_memory(fileData, (int)size, &w, &h, &n, 4);
        
        if (decoded) {
            e->player.pixels = (uint32_t*)decoded;
            e->player.width = w;
            e->player.height = h;
        }
        free(fileData);
    }
    AAsset_close(asset);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
                e->x = ANativeWindow_getWidth(app->window) / 2.0f;
                e->y = ANativeWindow_getHeight(app->window) / 2.0f;
                e->joy.cx = 200; e->joy.cy = ANativeWindow_getHeight(app->window) - 200; e->joy.r = 120;
                safe_load_asset(e);
                e->active = 1;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            e->active = 0;
            break;
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* ev) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_MOTION) {
        float mx = AMotionEvent_getX(ev, 0), my = AMotionEvent_getY(ev, 0);
        float dx = mx - e->joy.cx, dy = my - e->joy.cy, l = sqrtf(dx*dx + dy*dy);
        if (l > 10.0f) { e->joy.dx = dx/l; e->joy.dy = dy/l; }
        else { e->joy.dx = 0; e->joy.dy = 0; }
        return 1;
    }
    return 0;
}

void android_main(struct android_app* app) {
    struct engine e = {0};
    app->userData = &e;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (1) {
        int id; struct android_poll_source* s;
        while ((id = ALooper_pollOnce(0, NULL, NULL, (void**)&s)) >= 0) {
            if (s) s->process(app, s);
            if (app->destroyRequested) return;
        }

        if (e.active && app->window) {
            e.x += e.joy.dx * 8.0f; e.y += e.joy.dy * 8.0f;

            ANativeWindow_Buffer b;
            if (ANativeWindow_lock(app->window, &b, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)b.bits, b.width, b.height, b.stride };
                graphics_clear(&rb, 0xFFCCCCCC);
                
                // Рисуем игрока только если картинка загрузилась
                if (e.player.pixels) {
                    graphics_draw_image(&rb, &e.player, (int)e.x, (int)e.y);
                }
                
                ui_draw(&rb, &e.joy);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
