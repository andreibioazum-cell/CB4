#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <stdlib.h>
#include <math.h>
#include "graphics.h"
#include "ui.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct engine {
    struct android_app* app;
    Joystick joy;
    float px, py, angle;
    int w, h, tw, th;
    uint32_t* tex;
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        e->w = ANativeWindow_getWidth(app->window);
        e->h = ANativeWindow_getHeight(app->window);
        e->px = e->w/2.0f; e->py = e->h/2.0f;
        e->joy = (Joystick){150, e->h - 150, 80, 0, 0, 0, 0};
        AAsset* a = AAssetManager_open(app->activity->assetManager, "cube.png", AASSET_MODE_BUFFER);
        if (a) {
            int n;
            unsigned char* img = stbi_load_from_memory(AAsset_getBuffer(a), AAsset_getLength(a), &e->tw, &e->th, &n, 4);
            e->tex = malloc(e->tw * e->th * 4);
            for (int i = 0; i < e->tw * e->th; i++) {
                uint8_t* p = &img[i*4];
                e->tex[i] = (p[3]<<24)|(p[0]<<16)|(p[1]<<8)|p[2];
            }
            stbi_image_free(img); AAsset_close(a);
        }
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* ev) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_MOTION) {
        int act = AMotionEvent_getAction(ev);
        if (act == AMOTION_EVENT_ACTION_UP) { e->joy.dirX = e->joy.dirY = e->joy.tx = e->joy.ty = 0; return 1; }
        float dx = AMotionEvent_getX(ev, 0) - e->joy.cx, dy = AMotionEvent_getY(ev, 0) - e->joy.cy;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 10.0f) {
            e->joy.dirX = dx/len; e->joy.dirY = dy/len;
            e->angle = atan2f(dy, dx) + 1.57f; // Вращение (90 градусов в радианах)
            e->joy.tx = e->joy.dirX * (len > e->joy.r ? e->joy.r : len);
            e->joy.ty = e->joy.dirY * (len > e->joy.r ? e->joy.r : len);
        }
        return 1;
    }
    return 0;
}

void android_main(struct android_app* app) {
    struct engine e = {0};
    app->userData = &e; app->onAppCmd = handle_cmd; app->onInputEvent = handle_input;
    while (1) {
        int id; struct android_poll_source* src;
        while ((id = ALooper_pollOnce(0, NULL, NULL, (void**)&src)) >= 0) {
            if (src) src->process(app, src);
            if (app->destroyRequested) return;
        }
        if (app->window) {
            e.px += e.joy.dirX * 8.0f; e.py += e.joy.dirY * 8.0f;
            ANativeWindow_Buffer wb;
            if (ANativeWindow_lock(app->window, &wb, NULL) == 0) {
                RenderBuffer rb = {(uint32_t*)wb.bits, wb.width, wb.height, wb.stride};
                graphics_clear(&rb, 0xFFCCCCCC);
                graphics_draw_texture(&rb, (int)e.px, (int)e.py, e.tex, e.tw, e.th, e.angle, 1.5f);
                ui_draw_joystick(&rb, &e.joy);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
