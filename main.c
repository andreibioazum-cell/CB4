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
    GameState state;
    // Игрок
    float px, py;
    float angle;
    uint32_t* tex_pixels;
    int tex_width, tex_height;
    int tex_ready;
    // Джойстик
    Joystick joy;
    // Кнопка
    Button playBtn;
    int screen_w, screen_h;
};

static void game_init(struct engine* e, int w, int h) {
    e->screen_w = w;
    e->screen_h = h;
    e->state = STATE_LOBBY;
    
    // Игрок
    e->px = w / 2.0f;
    e->py = h / 2.0f;
    e->angle = 0.0f;
    e->tex_pixels = NULL;
    e->tex_ready = 0;
    
    // Джойстик
    e->joy.centerX = 150;
    e->joy.centerY = h - 150;
    e->joy.radius = 80;
    e->joy.dirX = e->joy.dirY = 0.0f;
    e->joy.touchOffX = e->joy.touchOffY = 0.0f;
    
    // Кнопка Play
    e->playBtn.w = 200;
    e->playBtn.h = 80;
    e->playBtn.x = (w - e->playBtn.w) / 2;
    e->playBtn.y = (h - e->playBtn.h) / 2 + 50;
    e->playBtn.hover = 0;
}

static void game_load_texture(struct engine* e, void* asset_manager) {
    AAssetManager* mgr = (AAssetManager*)asset_manager;
    AAsset* asset = AAssetManager_open(mgr, "cube.png", AASSET_MODE_BUFFER);
    if (!asset) return;
    
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

static void game_update(struct engine* e) {
    if (e->state != STATE_PLAYING) return;
    
    float speed = 6.0f;
    e->px += e->joy.dirX * speed;
    e->py += e->joy.dirY * speed;
    
    if (e->joy.dirX != 0.0f || e->joy.dirY != 0.0f) {
        e->angle = atan2f(e->joy.dirX, -e->joy.dirY);
    }
    
    float scale = 2.0f;
    if (e->tex_ready) {
        float halfW = (e->tex_width * scale) / 2.0f;
        float halfH = (e->tex_height * scale) / 2.0f;
        float maxRadius = sqrtf(halfW * halfW + halfH * halfH);
        
        if (e->px < maxRadius) e->px = maxRadius;
        if (e->px > e->screen_w - maxRadius) e->px = e->screen_w - maxRadius;
        if (e->py < maxRadius) e->py = maxRadius;
        if (e->py > e->screen_h - maxRadius) e->py = e->screen_h - maxRadius;
    }
}

static void game_draw(struct engine* e, RenderBuffer* rb) {
    if (e->state == STATE_LOBBY) {
        graphics_clear(rb, 0xFF1a1a2e);
        ui_draw_button(rb, &e->playBtn, "PLAY");
    } else {
        graphics_clear(rb, 0xFFCCCCCC);
        if (e->tex_ready) {
            float scale = 2.0f;
            graphics_draw_texture_ex(rb, (int)e->px, (int)e->py,
                                     e->tex_pixels, e->tex_width, e->tex_height,
                                     e->angle, scale);
        } else {
            graphics_draw_rect(rb, (int)e->px, (int)e->py, 80, 0xFFEE7722);
        }
        ui_draw_joystick(rb, &e->joy);
    }
}

static void game_free(struct engine* e) {
    if (e->tex_pixels) {
        free(e->tex_pixels);
        e->tex_pixels = NULL;
    }
}

static void game_handle_touch(struct engine* e, float x, float y, int action) {
    if (e->state == STATE_LOBBY) {
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            if (ui_is_point_in_button(&e->playBtn, x, y)) {
                e->playBtn.hover = 1;
            }
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            if (e->playBtn.hover && ui_is_point_in_button(&e->playBtn, x, y)) {
                e->state = STATE_PLAYING;
            }
            e->playBtn.hover = 0;
        }
        return;
    }
    
    // Джойстик
    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        e->joy.dirX = e->joy.dirY = 0.0f;
        e->joy.touchOffX = e->joy.touchOffY = 0.0f;
        return;
    }
    
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
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        int w = ANativeWindow_getWidth(app->window);
        int h = ANativeWindow_getHeight(app->window);
        game_init(e, w, h);
        game_load_texture(e, app->activity->assetManager);
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        int action = AMotionEvent_getAction(event);
        game_handle_touch(e, x, y, action);
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
                game_free(&e);
                return;
            }
        }

        if (app->window) {
            game_update(&e);
            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                game_draw(&e, &rb);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
