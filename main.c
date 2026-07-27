#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "graphics.h"
#include "ui.h"

struct engine {
    struct android_app* app;
    Game game;
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);
            ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
            game_init(&e->game, w, h, app->activity->assetManager);
            break;
        }
        case APP_CMD_TERM_WINDOW:
            game_free(&e->game);
            break;
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event);
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        ui_handle_joystick_touch(&e->game.joy, x, y, action);
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
                game_free(&e.game);
                return;
            }
        }

        if (app->window) {
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);
            game_update(&e.game, w, h);

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, NULL) == 0) {
                RenderBuffer rb = { (uint32_t*)winBuf.bits, winBuf.width, winBuf.height, winBuf.stride };
                game_draw(&e.game, &rb);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
