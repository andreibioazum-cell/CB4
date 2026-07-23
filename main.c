#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <math.h>
#include <stdlib.h>
#include "graphics.h"
#include "ui.h"

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct engine {
    struct android_app* app;
    Joystick joy;
    Image img;
    float x, y;
    int init;
};

void load_assets(struct engine* e) {
    AAsset* a = AAssetManager_open(e->app->activity->assetManager, "cube_ordinary.png", AASSET_MODE_BUFFER);
    if (!a) return;
    size_t s = AAsset_getLength(a);
    unsigned char* b = malloc(s);
    AAsset_read(a, b, s);
    AAsset_close(a);
    int w, h, c;
    unsigned char* d = stbi_load_from_memory(b, (int)s, &w, &h, &c, 4);
    free(b);
    if (d) { 
        e->img.pixels = (uint32_t*)d; 
        e->img.width = w; 
        e->img.height = h; 
    }
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW && app->window) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        int w = ANativeWindow_getWidth(app->window);
        int h = ANativeWindow_getHeight(app->window);
        if (!e->init) {
            e->x = w / 2.0f;
            e->y = h / 2.0f;
            e->init = 1;
        }
        e->joy.cx = 200; 
        e->joy.cy = h - 200; 
        e->joy.r = 120;
        load_assets(e);
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* ev) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_MOTION) {
        float mx = AMotionEvent_getX(ev, 0);
        float my = AMotionEvent_getY(ev, 0);
        float dx = mx - (float)e->joy.cx;
        float dy = my - (float)e->joy.cy;
        float l = sqrtf(dx*dx + dy*dy);
        if (l > 10.0f) { 
            e->joy.dx = dx/l; 
            e->joy.dy = dy/l; 
        } else { 
            e->joy.dx = 0; 
            e->joy.dy = 0; 
        }
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
        if (e.init && app->window) {
            e.x += e.joy.dx * 10.0f; 
            e.y += e.joy.dy * 10.0f;
            ANativeWindow_Buffer b;
            if (ANativeWindow_lock(app->window, &b, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)b.bits, b.width, b.height, b.stride };
                graphics_clear(&rb, 0xFFCCCCCC);
                if (e.img.pixels) {
                    graphics_draw_image(&rb, &e.img, (int)e.x, (int)e.y);
                }
                ui_draw(&rb, &e.joy);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
