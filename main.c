#include <android_native_app_glue.h>
#include <math.h>
#include "game.h"
#include "ui.h"

struct engine {
    struct android_app* app;
    Game game;
};

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        int w = ANativeWindow_getWidth(app->window);
        int h = ANativeWindow_getHeight(app->window);
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        game_init(&e->game, w, h, app->activity->assetManager);
    } else if (cmd == APP_CMD_TERM_WINDOW) {
        game_free(&e->game);
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        struct engine* e = (struct engine*)app->userData;
        int action = AMotionEvent_getAction(event);
        int action_masked = action & 0xFF; // AMOTION_EVENT_ACTION_MASK
        size_t pointer_count = AMotionEvent_getPointerCount(event);

        if (e->game.state == GAME_STATE_GAMEOVER) {
            for (size_t i = 0; i < pointer_count; i++) {
                float x = AMotionEvent_getX(event, i);
                float y = AMotionEvent_getY(event, i);
                if (ui_handle_button(&e->game.restartBtn, x, y, action_masked)) {
                    game_restart(&e->game);
                    return 1;
                }
            }
            return 1;
        }

        // Multi-touch handling for simultaneous Joystick and Attack Button
        for (size_t i = 0; i < pointer_count; i++) {
            float x = AMotionEvent_getX(event, i);
            float y = AMotionEvent_getY(event, i);

            // Left half of screen or proximity to joystick
            if (x < (float)e->game.screen_w * 0.55f) {
                ui_handle_joystick(&e->game.joy, x, y, action_masked);
            }

            // Right half of screen: attack button
            if (x > (float)e->game.screen_w * 0.45f) {
                ui_handle_button(&e->game.attackBtn, x, y, action_masked);
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
            if (app->destroyRequested) {
                game_free(&e->game);
                return;
            }
        }

        if (app->window) {
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);

            game_update(&e->game, w, h);

            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(app->window, &winBuf, 0) == 0) {
                RenderBuffer rb = {
                    (uint32_t*)winBuf.bits,
                    winBuf.width,
                    winBuf.height,
                    winBuf.stride
                };
                game_draw(&e->game, &rb);
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
