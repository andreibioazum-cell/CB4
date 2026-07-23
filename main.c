#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <math.h>
#include <stdlib.h>
#include "graphics.h"
#include "ui.h"

#define LOG_TAG "HardcoreGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct engine {
    struct android_app* app;
    Joystick joy;
    Image playerImg;
    float px, py;
    int width, height;
    int initialized;
};

void load_player_image(struct engine* e) {
    if (e->playerImg.pixels) return; // Уже загружено

    LOGI("Attempting to load cube_ordinary.png...");
    AAsset* asset = AAssetManager_open(e->app->activity->assetManager, "cube_ordinary.png", AASSET_MODE_BUFFER);
    
    if (!asset) {
        LOGE("Failed to open asset: cube_ordinary.png");
        return;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* buffer = (unsigned char*)malloc(size);
    AAsset_read(asset, buffer, size);
    AAsset_close(asset);

    int w, h, ch;
    unsigned char* data = stbi_load_from_memory(buffer, (int)size, &w, &h, &ch, 4);
    free(buffer);

    if (!data) {
        LOGE("stbi_load_from_memory failed!");
        return;
    }

    e->playerImg.pixels = (uint32_t*)data;
    e->playerImg.width = w;
    e->playerImg.height = h;
    LOGI("Image loaded successfully: %dx%d", w, h);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
                e->width = ANativeWindow_getWidth(app->window);
                e->height = ANativeWindow_getHeight(app->window);
                if (!e->initialized) {
                    e->px = e->width / 2.0f;
                    e->py = e->height / 2.0f;
                    e->initialized = 1;
                }
                e->joy.centerX = 200;
                e->joy.centerY = e->height - 200;
                e->joy.radius = 120;
                load_player_image(e);
            }
            break;
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        float jx = e->joy.centerX;
        float jy = e->joy.centerY;
        float dx = x - jx;
        float dy = y - jy;
        float len = sqrtf(dx*dx + dy*dy);
        
        if (len > 10.0f) {
            e->joy.dirX = dx/len;
            e->joy.dirY = dy/len;
        } else {
            e->joy.dirX = 0; e->joy.dirY = 0;
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
        int ident;
        struct android_poll_source* source;
        while ((ident = ALooper_pollOnce(0, NULL, NULL, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;
        }

        if (app->window && e.initialized) {
            e.px += e.joy.dirX * 10.0f;
            e.py += e.joy.dirY * 10.0f;

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                
                graphics_clear(&rb, 0xFFCCCCCC); 

                if (e.playerImg.pixels) {
                    graphics_draw_image(&rb, &e.playerImg, (int)e.px, (int)e.py);
                }

                ui_draw_joystick(&rb, &e.joy);

                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
