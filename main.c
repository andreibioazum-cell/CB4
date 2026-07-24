#include <android_native_app_glue.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float x, y, z; uint32_t c; } Vertex;
typedef struct { float x, y; } Vec2;

float ang = 0;

// Вспомогательная функция для отрисовки горизонтальной линии с градиентом
void draw_scanline(ANativeWindow_Buffer* b, int y, int x1, int x2, uint32_t color) {
    if (y < 0 || y >= b->height) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x1 < 0) x1 = 0;
    if (x2 >= b->width) x2 = b->width - 1;
    
    uint32_t* row = (uint32_t*)b->bits + (y * b->stride);
    for (int x = x1; x <= x2; x++) {
        row[x] = color;
    }
}

// Отрисовка заполненного треугольника (простой алгоритм сканирования)
void fill_tri(ANativeWindow_Buffer* b, Vec2 v0, Vec2 v1, Vec2 v2, uint32_t color) {
    // Сортировка вершин по Y
    if (v0.y > v1.y) { Vec2 t = v0; v0 = v1; v1 = t; }
    if (v0.y > v2.y) { Vec2 t = v0; v0 = v2; v2 = t; }
    if (v1.y > v2.y) { Vec2 t = v1; v1 = v2; v2 = t; }

    int total_h = (int)(v2.y - v0.y);
    if (total_h == 0) return;

    for (int i = 0; i < total_h; i++) {
        bool second_half = i > v1.y - v0.y || v1.y == v0.y;
        int segment_h = second_half ? (int)(v2.y - v1.y) : (int)(v1.y - v0.y);
        if (segment_h == 0) continue;

        float alpha = (float)i / total_h;
        float beta  = (float)(i - (second_half ? v1.y - v0.y : 0)) / segment_h;
        
        int ax = (int)(v0.x + (v2.x - v0.x) * alpha);
        int bx = second_half ? (int)(v1.x + (v2.x - v1.x) * beta) : (int)(v0.x + (v1.x - v0.x) * beta);
        
        // Меняем яркость цвета в зависимости от Y (простой градиент)
        uint32_t g_color = color | (uint8_t)(alpha * 255); 
        draw_scanline(b, (int)(v0.y + i), ax, bx, g_color);
    }
}

void render(struct android_app* app) {
    if (!app->window) return;

    ANativeWindow_Buffer b;
    if (ANativeWindow_lock(app->window, &b, NULL) < 0) return;

    // Очистка экрана (темно-серый)
    uint32_t* p = (uint32_t*)b.bits;
    for(int i=0; i < b.height * b.stride; i++) p[i] = 0xFF111111;

    // Вершины одного квадрата
    Vertex v[4] = {
        {-1, -1, 0, 0xFFFF0000}, // Красный
        { 1, -1, 0, 0xFF00FF00}, // Зеленый
        { 1,  1, 0, 0xFF0000FF}, // Синий
        {-1,  1, 0, 0xFFFFFFFF}  // Белый
    };

    ang += 0.05f;
    float s = sinf(ang), c = cosf(ang);
    Vec2 p[4];

    // Трансформация и проекция
    for(int i=0; i<4; i++) {
        // Вращение по Y и X
        float x = v[i].x * c - v[i].z * s;
        float z = v[i].x * s + v[i].z * c;
        float y = v[i].y * c - z * s;
        z = v[i].y * s + z * c;

        float f = 500.0f / (z + 4.0f);
        p[i].x = x * f + b.width / 2.0f;
        p[i].y = y * f + b.height / 2.0f;
    }

    // Рисуем два треугольника, составляющих квадрат
    // Используем разные цвета для градиента (синий и фиолетовый)
    fill_tri(&b, p[0], p[1], p[2], 0xFF0055FF);
    fill_tri(&b, p[0], p[2], p[3], 0xFF8800CC);

    ANativeWindow_unlockAndPost(app->window);
}

void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            app->userData = (void*)1;
            break;
        case APP_CMD_TERM_WINDOW:
            app->userData = NULL;
            break;
    }
}

void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    while(1) {
        int id, events;
        struct android_poll_source* source;
        // Таймаут 0, если окно готово, чтобы рисовать постоянно, иначе -1 (ждем событий)
        while((id = ALooper_pollOnce(app->userData ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            if(source) source->process(app, source);
            if(app->destroyRequested) return;
        }
        if(app->userData) render(app);
    }
}
