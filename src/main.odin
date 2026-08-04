package main

import "core:c"
import "base:runtime"

APP_CMD_INIT_WINDOW :: 1
APP_CMD_TERM_WINDOW :: 2
APP_CMD_WINDOW_RESIZED :: 3
APP_CMD_DESTROY :: 15

AINPUT_EVENT_TYPE_MOTION :: 2
AMOTION_EVENT_ACTION_MASK :: 0xFF
AMOTION_EVENT_ACTION_DOWN :: 0
AMOTION_EVENT_ACTION_UP :: 1
AMOTION_EVENT_ACTION_MOVE :: 2
AMOTION_EVENT_ACTION_CANCEL :: 3

handle_app_command :: proc "c" (app: ^Android_App, command: c.int32_t) {
    // C callbacks do not receive Odin's implicit context parameter.
    context = runtime.default_context()

    if app == nil || app.userData == nil {
        return
    }

    game := cast(^Game)app.userData

    switch int(command) {
    case APP_CMD_INIT_WINDOW:
        if app.window != nil {
            width := int(ANativeWindow_getWidth(app.window))
            height := int(ANativeWindow_getHeight(app.window))

            ANativeWindow_setBuffersGeometry(
                app.window,
                c.int32_t(width),
                c.int32_t(height),
                WINDOW_FORMAT_RGBA_8888,
            )

            if !game.initialized {
                game_init(game, width, height)
            } else {
                game_resize(game, width, height)
            }
        }

    case APP_CMD_TERM_WINDOW:
        game.joystick.active = false
        game.attack.pressed = false

    case APP_CMD_WINDOW_RESIZED:
        if app.window != nil {
            game_resize(
                game,
                int(ANativeWindow_getWidth(app.window)),
                int(ANativeWindow_getHeight(app.window)),
            )
        }

    case APP_CMD_DESTROY:
        game.joystick.active = false
        game.attack.pressed = false
    }
}

handle_input_event :: proc "c" (
    app: ^Android_App,
    event: ^AInputEvent,
) -> c.int32_t {
    // C callbacks do not receive Odin's implicit context parameter.
    context = runtime.default_context()

    if app == nil || app.userData == nil || event == nil {
        return 0
    }

    if AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION {
        return 0
    }

    game := cast(^Game)app.userData

    raw_action := int(AMotionEvent_getAction(event))
    action := raw_action & AMOTION_EVENT_ACTION_MASK

    x := AMotionEvent_getX(event, 0)
    y := AMotionEvent_getY(event, 0)

    switch action {
    case AMOTION_EVENT_ACTION_DOWN:
        if ui_button_press(&game.attack, x, y) {
            game_fire(game)
        } else {
            ui_joystick_begin(&game.joystick, x, y)
        }

    case AMOTION_EVENT_ACTION_MOVE:
        ui_joystick_move(&game.joystick, x, y)

        if game.attack.pressed &&
           !ui_button_contains(&game.attack, x, y) {
            game.attack.pressed = false
        }

    case AMOTION_EVENT_ACTION_UP:
        ui_button_release(&game.attack, x, y)
        ui_joystick_end(&game.joystick)

    case AMOTION_EVENT_ACTION_CANCEL:
        game.attack.pressed = false
        ui_joystick_end(&game.joystick)
    }

    return 1
}

// Exported with the exact name expected by android_native_app_glue.c.
@(export)
android_main :: proc "c" (app: ^Android_App) {
    // Initialize Odin runtime context when entering from C.
    context = runtime.default_context()

    game: Game

    app.userData = &game
    app.onAppCmd = handle_app_command
    app.onInputEvent = handle_input_event

    for {
        source: ^Android_Poll_Source

        ident := ALooper_pollOnce(
            0,
            nil,
            nil,
            cast(^rawptr)&source,
        )

        for ident >= 0 {
            if source != nil {
                source.process(app, source)
            }

            if app.destroyRequested != 0 {
                return
            }

            ident = ALooper_pollOnce(
                0,
                nil,
                nil,
                cast(^rawptr)&source,
            )
        }

        if app.destroyRequested != 0 {
            return
        }

        if app.window == nil {
            continue
        }

        width := int(ANativeWindow_getWidth(app.window))
        height := int(ANativeWindow_getHeight(app.window))

        if !game.initialized {
            game_init(&game, width, height)
        } else if width != game.width || height != game.height {
            game_resize(&game, width, height)
        }

        game_update(&game)

        native_buffer: ANativeWindow_Buffer

        if ANativeWindow_lock(
            app.window,
            &native_buffer,
            nil,
        ) == 0 {
            render_buffer := Render_Buffer{
                pixels = cast([^]u32)native_buffer.bits,
                width = int(native_buffer.width),
                height = int(native_buffer.height),
                stride = int(native_buffer.stride),
            }

            game_draw(&game, &render_buffer)

            ANativeWindow_unlockAndPost(app.window)
        }
    }
}
