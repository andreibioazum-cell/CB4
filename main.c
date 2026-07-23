#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arm_neon.h> // "Секретное оружие" для разгона CPU

#define JOY_RADIUS 110
#define STICK_RADIUS 45
#define PLAYER_SIZE 64

struct engine {
    struct android_app* app;
    int width, height;
    float px, py, dx, dy;
};

// --- МАКСИМАЛЬНЫЙ РАЗГОН: NEON СЕКЦИЯ ---

// Быстрая очистка экрана (заливка цветом)
// Обрабатывает 4 пикселя (128 бит) за одну операцию
static void fast_clear(uint32_t* dest, uint32_t color, int pixel_count) {
    uint32x4_t v_color = vdupq_n_u32(color);
    int i = 0;
    // Основной цикл: 4 пикселя за раз
    for (; i <= pixel_count - 4; i += 4) {
        vst1q_u32(&dest[i], v_color);
    }
    // Дорисовываем остаток
    for (; i < pixel_count; i++) {
        dest[i] = color;
    }
}

// --- ОПТИМИЗИРОВАННОЕ РИСОВАНИЕ ---

// Быстрый прямоугольник (игрок)
static void draw_rect(ANativeWindow_Buffer* buf, int x, int y, int size, uint32_t color) {
    int x1 = x - size/2, x2 = x + size/2;
    int y1 = y - size/2, y2 = y + size/2;
    
    // Ограничение по границам экрана
    if (x1 < 0) x1 = 0; if (x2 > buf->width) x2 = buf->width;
    if (y1 < 0) y1 = 0; if (y2 > buf->height) y2 = buf->height;

    for (int i = y1; i < y2; i++) {
        uint32_t* line = (uint32_t*)buf->bits + (i * buf->stride);
        for (int j = x1; j < x2; j++) {
            line[j] = color;
        }
    }
}

// Быстрый круг (джойстик) с предвычисленным радиусом
static void draw_circle(ANativeWindow_Buffer* buf, int cx, int cy, int r, uint32_t color) {
    int r2 = r * r;
    int y_start = cy - r, y_end = cy + r;
    int x_start = cx - r, x_end = cx + r;

    for (int y = y_start; y < y_end; y++) {
        if (y < 0 || y >= buf->height) continue;
        uint32_t* line = (uint32_t*)buf->bits + (y * buf->stride);
        int dy = y - cy;
        int dy2 = dy * dy;
        
        for (int x = x_start; x < x_end; x++) {
            if (x < 0 || x >= buf->width) continue;
            int dx = x - cx;
            if (dx*dx + dy2 <= r2) {
                line[x] = color;
            }
        }
    }
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW) {
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, WINDOW_FORMAT_RGBA_8888);
        e->width = ANativeWindow_getWidth(app->window);
        e->height = ANativeWindow_getHeight(app->window);
        e->px = e->width / 2.0f;
        e->py = e->height / 2.0f;
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE) {
            float jx = 200, jy = e->height - 200;
            float vx = x - jx, vy = y - jy;
            float len = sqrtf(vx*vx + vy*vy);
            if (len > 5.0f) {
                e->dx = vx / len; e->dy = vy / len;
            }
        } else {
            e->dx = 0; e->dy = 0;
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

        if (app->window == NULL) continue;

        // Физика (8 пикселей за кадр)
        e.px += e.dx * 8.0f;
        e.py += e.dy * 8.0f;

        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(app->window, &buffer, NULL) < 0) continue;

        // 1. Очистка экрана через NEON (Темно-синий)
        fast_clear((uint32_t*)buffer.bits, 0xFF221111, buffer.stride * buffer.height);

        // 2. Рисуем джойстик (прозрачно-белый)
        draw_circle(&buffer, 200, e.height - 200, JOY_RADIUS, 0x44FFFFFF);
        
        // 3. Рисуем стик
        int sx = 200 + (int)(e.dx * 60);
        int sy = (e.height - 200) + (int)(e.dy * 60);
        draw_circle(&buffer, sx, sy, STICK_RADIUS, 0xFFFFFFFF);

        // 4. Рисуем игрока (зеленый)
        draw_rect(&buffer, (int)e.px, (int)e.py, PLAYER_SIZE, 0xFF00FF00);

        ANativeWindow_unlockAndPost(app->window);
    }
}
