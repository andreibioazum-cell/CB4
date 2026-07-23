#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <math.h>
#include <stdlib.h>
#include "graphics.h"
#include "ui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct engine {
    struct android_app* app;
    Joystick joy;
    float px, py;
    int width, height;
    uint32_t* tex_pixels;
    int tex_width, tex_height;
    int tex_ready;
    float current_angle;
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        e->width = ANativeWindow_getWidth(app->window);
        e->height = ANativeWindow_getHeight(app->window);
        e->px = e->width / 2; 
        e->py = e->height / 2;
        // Джойстик ниже и левее
        e->joy.centerX = 150; 
        e->joy.centerY = e->height - 150;
        e->joy.radius = 80;

        // Загрузка текстуры
        AAssetManager* mgr = app->activity->assetManager;
        AAsset* asset = AAssetManager_open(mgr, "cube.png", AASSET_MODE_BUFFER);
        if (asset) {
            size_t size = AAsset_getLength(asset);
            unsigned char* data = (unsigned char*)malloc(size);
            AAsset_read(asset, data, size);
            AAsset_close(asset);
            int w, h, n;
            unsigned char* img = stbi_load_from_memory(data, size, &w, &h, &n, 4);
            free(data);
            if (img) {
                e->tex_width = w;
                e->tex_height = h;
                e->tex_pixels = (uint32_t*)malloc(w * h * sizeof(uint32_t));
                for (int i = 0; i < w * h; ++i) {
                    uint8_t r = img[i*4], g = img[i*4+1], b = img[i*4+2], a = img[i*4+3];
                    e->tex_pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
                }
                stbi_image_free(img);
                e->tex_ready = 1;
            }
        }
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event);
        if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            e->joy.dirX = e->joy.dirY = 0.0f;
            e->joy.touchOffX = e->joy.touchOffY = 0.0f;
            return 1;
        }
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        float dx = x - e->joy.centerX;
        float dy = y - e->joy.centerY;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 10.0f) {
            e->joy.dirX = dx / len;
            e->joy.dirY = dy / len;
            if (len > e->joy.radius) {
                e->joy.touchOffX = e->joy.dirX * e->joy.radius;
                e->joy.touchOffY = e->joy.dirY * e->joy.radius;
            } else {
                e->joy.touchOffX = dx;
                e->joy.touchOffY = dy;
            }
        } else {
            e->joy.dirX = e->joy.dirY = 0.0f;
            e->joy.touchOffX = e->joy.touchOffY = 0.0f;
        }
        return 1;
    }
    return 0;
}

void android_main(struct android_app* app) {
    struct engine e = {0};
    e.current_angle = 0.0f;
    app->userData = &e;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (1) {
        int ident;
        struct android_poll_source* source;
        while ((ident = ALooper_pollOnce(0, NULL, NULL, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                if (e.tex_pixels) free(e.tex_pixels);
                return;
            }
        }

        if (app->window) {
            e.px += e.joy.dirX * 10.0f;
            e.py += e.joy.dirY * 10.0f;

            float scale = 1.5f;
            // Ограничиваем позицию, чтобы текстура полностью помещалась
            if (e.tex_ready) {
                // Вычисляем реальный размер текстуры после масштабирования
                float scaledW = e.tex_width * scale;
                float scaledH = e.tex_height * scale;
                // Половина размера для ограничения (отступ 5 пикселей для безопасности)
                float halfW = scaledW / 2.0f + 5.0f;
                float halfH = scaledH / 2.0f + 5.0f;
                
                if (e.px < halfW) e.px = halfW;
                if (e.px > e.width - halfW) e.px = e.width - halfW;
                if (e.py < halfH) e.py = halfH;
                if (e.py > e.height - halfH) e.py = e.height - halfH;
            } else {
                float half = 40.0f;
                if (e.px < half) e.px = half;
                if (e.px > e.width - half) e.px = e.width - half;
                if (e.py < half) e.py = half;
                if (e.py > e.height - half) e.py = e.height - half;
            }

            if (e.joy.dirX != 0.0f || e.joy.dirY != 0.0f)
                e.current_angle = atan2f(e.joy.dirX, -e.joy.dirY);

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                graphics_clear(&rb, 0xFFCCCCCC);
                if (e.tex_ready) {
                    graphics_draw_texture_ex(&rb, (int)e.px, (int)e.py,
                                             e.tex_pixels, e.tex_width, e.tex_height,
                                             e.current_angle, scale);
                } else {
                    graphics_draw_rect(&rb, (int)e.px, (int)e.py, 80, 0xFFEE7722);
                }
                ui_draw_joystick(&rb, &e.joy);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
