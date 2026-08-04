package main

import "core:c"

// The Android NDK exposes these as opaque C handles.  Keeping them opaque is
// enough for a software renderer: only pointers to the handles cross the FFI.
ANativeActivity :: struct {}
AConfiguration  :: struct {}
ALooper         :: struct {}
AInputQueue     :: struct {}
ANativeWindow   :: struct {}
AInputEvent     :: struct {}

ARect :: struct {
    left:   c.int32_t,
    top:    c.int32_t,
    right:  c.int32_t,
    bottom: c.int32_t,
}

App_Command_Proc :: #type proc "c" (app: ^Android_App, command: c.int32_t)
Input_Event_Proc :: #type proc "c" (app: ^Android_App, event: ^AInputEvent) -> c.int32_t

// The first part of this structure is public in android_native_app_glue.h.
// The private pthread fields are intentionally not needed by Odin, so the
// binding stops after destroyRequested.  The offsets of all fields above are
// identical to the NDK structure on both supported ABIs.
Android_App :: struct {
    userData:       rawptr,
    onAppCmd:       App_Command_Proc,
    onInputEvent:   Input_Event_Proc,
    activity:       ^ANativeActivity,
    config:         ^AConfiguration,
    savedState:     rawptr,
    savedStateSize: c.size_t,
    looper:         ^ALooper,
    inputQueue:     ^AInputQueue,
    window:         ^ANativeWindow,
    contentRect:    ARect,
    activityState:  c.int,
    destroyRequested: c.int,
}

Android_Poll_Source :: struct {
    id:      c.int32_t,
    app:     ^Android_App,
    process: #type proc "c" (app: ^Android_App, source: ^Android_Poll_Source),
}

ANativeWindow_Buffer :: struct {
    width:    c.int32_t,
    height:   c.int32_t,
    stride:   c.int32_t,
    format:   c.int32_t,
    bits:     rawptr,
    reserved: [6]u32,
}

foreign import android "system:android"
foreign import libc "system:c"

@(default_calling_convention = "c")
foreign android {
    ALooper_pollOnce :: proc(
        timeout_millis: c.int,
        out_fd:         ^c.int,
        out_events:     ^c.int,
        out_data:       ^rawptr,
    ) -> c.int ---

    ANativeWindow_getWidth :: proc(window: ^ANativeWindow) -> c.int32_t ---
    ANativeWindow_getHeight :: proc(window: ^ANativeWindow) -> c.int32_t ---
    ANativeWindow_setBuffersGeometry :: proc(
        window: ^ANativeWindow,
        width: c.int32_t,
        height: c.int32_t,
        format: c.int32_t,
    ) -> c.int32_t ---
    ANativeWindow_lock :: proc(
        window: ^ANativeWindow,
        out_buffer: ^ANativeWindow_Buffer,
        in_out_dirty_bounds: ^ARect,
    ) -> c.int32_t ---
    ANativeWindow_unlockAndPost :: proc(window: ^ANativeWindow) -> c.int32_t ---

    AInputEvent_getType :: proc(event: ^AInputEvent) -> c.int32_t ---
    AMotionEvent_getAction :: proc(event: ^AInputEvent) -> c.int32_t ---
    AMotionEvent_getX :: proc(event: ^AInputEvent, pointer_index: c.size_t) -> f32 ---
    AMotionEvent_getY :: proc(event: ^AInputEvent, pointer_index: c.size_t) -> f32 ---
}

when ODIN_ARCH == .arm32 {
    // time_t and long are 32-bit in Android's LP32 ABI.
    Clock_Timespec :: struct {
        seconds:     i32,
        nanoseconds: i32,
    }
} else {
    Clock_Timespec :: struct {
        seconds:     i64,
        nanoseconds: i64,
    }
}

@(default_calling_convention = "c")
foreign libc {
    clock_gettime :: proc(clock_id: c.int, time: ^Clock_Timespec) -> c.int ---
}

// WINDOW_FORMAT_RGBA_8888 from <android/window.h> / native_window.h.
WINDOW_FORMAT_RGBA_8888 :: 1
CLOCK_MONOTONIC :: 1
