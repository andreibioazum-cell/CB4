#include <android_native_app_glue.h>
#include <math.h>  // Добавлено для cosf/sinf
#include "game.h"
#include "ui.h"

#define BULLET_SPEED 15.0f  // Объявлена константа

struct engine { struct android_app* app; Game game; };

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        int w = ANativeWindow_getWidth(app->window);
        int h = ANativeWindow_getHeight(app->window);
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        game_init(&e->game, w, h, app->activity->assetManager);
    } else if (cmd == APP_CMD_TERM_WINDOW)
        game_free(&e->game);
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        struct engine* e = (struct engine*)app->userData;
        int action = AMotionEvent_getAction(event);
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        handle_joystick(&e->game.joy, x, y, action);
        if (handle_button(&e->game.attackBtn, x, y, action)) {
            if (!e->game.bullet.active) {
                float angle = e->game.player.angle;
                float cos_a = cosf(angle);
                float sin_a = sinf(angle);
                e->game.bullet.x = e->game.player.x + 46.0f * cos_a;
                e->game.bullet.y = e->game.player.y + 46.0f * sin_a;
                e->game.bullet.vx = BULLET_SPEED * cos_a;
                e->game.bullet.vy = BULLET_SPEED * sin_a;
                e->game.bullet.active = 1;
                e->game.bullet.radius = 10;
            }
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
        while ((ident = ALooper_pollOnce(0, 0, 0, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) { game_free(&e.game); return; }
        }
        if (app->window) {
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);
            game_update(&e.game, w, h);
            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, 0) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                game_draw(&e.game, &rb);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
