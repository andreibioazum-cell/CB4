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
    // Текстура игрока
    uint32_t* tex_pixels;
    int tex_width, tex_height;
    int tex_ready;
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        e->width = ANativeWindow_getWidth(app->window);
        e->height = ANativeWindow_getHeight(app->window);
        e->px = e->width / 2; 
        e->py = e->height / 2;
        e->joy.centerX = 200; 
        e->joy.centerY = e->height - 200;
        e->joy.radius = 120;
        
        // Загрузка текстуры из assets
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
                    uint8_t r = img[i*4];
                    uint8_t g = img[i*4+1];
                    uint8_t b = img[i*4+2];
                    uint8_t a = img[i*4+3];
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
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        float dx = x - e->joy.centerX;
        float dy = y - e->joy.centerY;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 10.0f) {
            e->joy.dirX = dx/len; 
            e->joy.dirY = dy/len;
        } else {
            e->joy.dirX = 0; 
            e->joy.dirY = 0;
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
            if (app->destroyRequested) {
                // Освобождаем память
                if (e.tex_pixels) free(e.tex_pixels);
                return;
            }
        }

        if (app->window) {
            e.px += e.joy.dirX * 10.0f;
            e.py += e.joy.dirY * 10.0f;

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                
                graphics_clear(&rb, 0xFFCCCCCC);
                
                // Рисуем игрока текстурой или прямоугольником
                if (e.tex_ready) {
                    graphics_draw_texture(&rb, (int)e.px, (int)e.py, 
                                         e.tex_pixels, e.tex_width, e.tex_height);
                } else {
                    graphics_draw_rect(&rb, (int)e.px, (int)e.py, 80, 0xFFEE7722);
                }
                
                ui_draw_joystick(&rb, &e.joy);

                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
