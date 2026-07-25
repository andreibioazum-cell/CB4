#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            // Освобождаем старую текстуру, если была
            if (e->tex_pixels) {
                free(e->tex_pixels);
                e->tex_pixels = NULL;
                e->tex_ready = 0;
            }
            ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
            e->width = ANativeWindow_getWidth(app->window);
            e->height = ANativeWindow_getHeight(app->window);
            e->px = e->width / 2.0f;
            e->py = e->height / 2.0f;
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
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            // Освобождаем текстуру при уничтожении окна
            if (e->tex_pixels) {
                free(e->tex_pixels);
                e->tex_pixels = NULL;
                e->tex_ready = 0;
            }
            e->width = e->height = 0;
            break;
        }
        case APP_CMD_DESTROY: {
            // Полное освобождение – уже будет сделано в main при выходе
            break;
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
        const float deadZone = 20.0f; // увеличенная мёртвая зона
        if (len > deadZone) {
            // Нормализуем направление
            float invLen = 1.0f / len;
            e->joy.dirX = dx * invLen;
            e->joy.dirY = dy * invLen;
            // Ограничиваем смещение радиусом
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
            // Обновляем актуальные размеры окна (на случай изменения)
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);
            if (w != e.width || h != e.height) {
                e.width = w;
                e.height = h;
                e.joy.centerY = h - 150; // подстраиваем джойстик
            }

            // Движение
            e.px += e.joy.dirX * 10.0f;
            e.py += e.joy.dirY * 10.0f;

            // Ограничение позиции с учётом поворота (максимальный радиус)
            float scale = 1.5f;
            float maxExtent = 0.0f;
            if (e.tex_ready) {
                float halfW = e.tex_width * scale * 0.5f;
                float halfH = e.tex_height * scale * 0.5f;
                maxExtent = sqrtf(halfW*halfW + halfH*halfH);
            } else {
                maxExtent = 40.0f; // половина стороны квадрата
            }
            if (e.px < maxExtent) e.px = maxExtent;
            if (e.px > e.width - maxExtent) e.px = e.width - maxExtent;
            if (e.py < maxExtent) e.py = maxExtent;
            if (e.py > e.height - maxExtent) e.py = e.height - maxExtent;

            // Вычисление угла поворота (в направлении движения)
            float angle = 0.0f;
            float dirLen = sqrtf(e.joy.dirX*e.joy.dirX + e.joy.dirY*e.joy.dirY);
            if (dirLen > 0.001f) {
                // atan2(dirY, dirX) даёт угол от оси X; текстура смотрит вверх, поэтому вычитаем PI/2
                angle = atan2f(e.joy.dirY, e.joy.dirX) - M_PI_2;
            }

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                graphics_clear(&rb, 0xFFCCCCCC);
                if (e.tex_ready) {
                    graphics_draw_texture_ex(&rb, (int)e.px, (int)e.py,
                                             e.tex_pixels, e.tex_width, e.tex_height,
                                             angle, scale);
                } else {
                    graphics_draw_rect(&rb, (int)e.px, (int)e.py, 80, 0xFFEE7722);
                }
                ui_draw_joystick(&rb, &e.joy);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
