#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PI 3.14159265f
#define JOY_RADIUS 70.0f          // Меньше
#define JOY_X_OFFSET 120.0f
#define JOY_Y_OFFSET 120.0f
#define STICK_RADIUS 50.0f        // Больше
#define CIRCLE_SIDES 32
#define PLAYER_SIZE 40.0f
#define SPEED 300.0f
#define WORLD_SIZE 1000.0f        // Размер мира

struct engine {
    struct android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width, height;
    
    GLuint program;
    GLint uResLoc, uColorLoc;
    
    float playerX, playerY;
    float moveDirX, moveDirY;
    int32_t movePointerId;
    
    // Камера
    float camX, camY;
    
    // Кэшированные данные для круга
    float unitCircle[(CIRCLE_SIDES + 1) * 2];
    bool circleGenerated;
    
    // Второй объект
    float objX, objY;
    float objSpeedX, objSpeedY;
};

// Шейдеры
static const char* vertexShader =
    "attribute vec2 pos;"
    "uniform vec2 res;"
    "uniform vec2 cam;"
    "void main() {"
    "  vec2 p = (pos - cam) / res * 2.0;"
    "  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);"
    "}";

static const char* fragmentShader =
    "precision mediump float;"
    "uniform vec4 color;"
    "void main() { gl_FragColor = color; }";

// Время
static double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Генерация единичного круга
static void generate_unit_circle(struct engine* e) {
    if (!e->circleGenerated) {
        for (int i = 0; i <= CIRCLE_SIDES; i++) {
            float a = (float)i / CIRCLE_SIDES * 2.0f * PI;
            e->unitCircle[i*2] = cosf(a);
            e->unitCircle[i*2+1] = sinf(a);
        }
        e->circleGenerated = true;
    }
}

// Отрисовка круга
static void draw_circle(float cx, float cy, float r, float cr, float cg, float cb, float ca, 
                        struct engine* e, bool ring, float thickness) {
    generate_unit_circle(e);
    
    int vertexCount = ring ? (CIRCLE_SIDES + 1) * 2 : (CIRCLE_SIDES + 2);
    float verts[vertexCount * 2];
    int idx = 0;
    
    if (!ring) {
        verts[idx++] = cx;
        verts[idx++] = cy;
        for (int i = 0; i <= CIRCLE_SIDES; i++) {
            verts[idx++] = cx + e->unitCircle[i*2] * r;
            verts[idx++] = cy + e->unitCircle[i*2+1] * r;
        }
    } else {
        for (int i = 0; i <= CIRCLE_SIDES; i++) {
            verts[idx++] = cx + e->unitCircle[i*2] * r;
            verts[idx++] = cy + e->unitCircle[i*2+1] * r;
            verts[idx++] = cx + e->unitCircle[i*2] * (r - thickness);
            verts[idx++] = cy + e->unitCircle[i*2+1] * (r - thickness);
        }
    }
    
    glUseProgram(e->program);
    glUniform2f(e->uResLoc, (float)e->width, (float)e->height);
    glUniform2f(glGetUniformLocation(e->program, "cam"), e->camX, e->camY);
    glUniform4f(e->uColorLoc, cr, cg, cb, ca);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(ring ? GL_TRIANGLE_STRIP : GL_TRIANGLE_FAN, 0, idx / 2);
}

// Отрисовка прямоугольника (с учетом камеры)
static void draw_rect(float x, float y, float w, float h, float cr, float cg, float cb, float ca, struct engine* e) {
    float hw = w * 0.5f, hh = h * 0.5f;
    float verts[] = {
        x-hw, y-hh, x+hw, y-hh, x+hw, y+hh,
        x-hw, y-hh, x+hw, y+hh, x-hw, y+hh
    };
    
    glUseProgram(e->program);
    glUniform2f(e->uResLoc, (float)e->width, (float)e->height);
    glUniform2f(glGetUniformLocation(e->program, "cam"), e->camX, e->camY);
    glUniform4f(e->uColorLoc, cr, cg, cb, ca);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Обработка ввода
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    int action = AMotionEvent_getAction(event);
    int code = action & AMOTION_EVENT_ACTION_MASK;
    int pi = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int id = AMotionEvent_getPointerId(event, pi);

    if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        float x = AMotionEvent_getX(event, pi);
        float y = AMotionEvent_getY(event, pi);
        float jx = JOY_X_OFFSET;
        float jy = e->height - JOY_Y_OFFSET;
        float dx = x - jx, dy = y - jy;
        float distSq = dx*dx + dy*dy;
        float radiusSq = (JOY_RADIUS * 2.5f) * (JOY_RADIUS * 2.5f); // Большая зона захвата
        
        if (distSq < radiusSq) {
            e->movePointerId = id;
            // Обновляем направление (только направление, без учета расстояния до центра)
            float dist = sqrtf(distSq);
            if (dist > 5.0f) {
                // Нормализуем вектор, игнорируя расстояние до центра
                e->moveDirX = dx / dist;
                e->moveDirY = dy / dist;
            }
            return 1;
        }
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_MOVE) {
        for (int i = 0; i < AMotionEvent_getPointerCount(event); i++) {
            if (AMotionEvent_getPointerId(event, i) == e->movePointerId) {
                float x = AMotionEvent_getX(event, i);
                float y = AMotionEvent_getY(event, i);
                float dx = x - JOY_X_OFFSET;
                float dy = y - (e->height - JOY_Y_OFFSET);
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist > 5.0f) {
                    // Только направление, скорость постоянная
                    e->moveDirX = dx / dist;
                    e->moveDirY = dy / dist;
                } else {
                    e->moveDirX = 0;
                    e->moveDirY = 0;
                }
                return 1;
            }
        }
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || 
        code == AMOTION_EVENT_ACTION_CANCEL) {
        if (id == e->movePointerId) {
            e->movePointerId = -1;
            e->moveDirX = 0;
            e->moveDirY = 0;
            return 1;
        }
        return 1;
    }

    return 0;
}

// Команды приложения
static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            if (!app->window) break;
            
            e->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(e->display, NULL, NULL);
            
            EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_NONE
            };
            EGLConfig config;
            EGLint numConfigs;
            eglChooseConfig(e->display, attribs, &config, 1, &numConfigs);
            
            e->surface = eglCreateWindowSurface(e->display, config, app->window, NULL);
            
            EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            e->context = eglCreateContext(e->display, config, EGL_NO_CONTEXT, ctxAttribs);
            eglMakeCurrent(e->display, e->surface, e->surface, e->context);
            
            // Компиляция шейдеров
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vs, 1, &vertexShader, NULL);
            glCompileShader(vs);
            
            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs, 1, &fragmentShader, NULL);
            glCompileShader(fs);
            
            e->program = glCreateProgram();
            glAttachShader(e->program, vs);
            glAttachShader(e->program, fs);
            glBindAttribLocation(e->program, 0, "pos");
            glLinkProgram(e->program);
            
            glDeleteShader(vs);
            glDeleteShader(fs);
            
            // Кэшируем локации
            e->uResLoc = glGetUniformLocation(e->program, "res");
            e->uColorLoc = glGetUniformLocation(e->program, "color");
            
            // Получаем размеры
            eglQuerySurface(e->display, e->surface, EGL_WIDTH, &e->width);
            eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &e->height);
            
            // Инициализация игрока
            e->playerX = 0;
            e->playerY = 0;
            e->camX = 0;
            e->camY = 0;
            e->movePointerId = -1;
            e->circleGenerated = false;
            
            // Второй объект
            e->objX = 300;
            e->objY = 200;
            e->objSpeedX = 50;
            e->objSpeedY = 30;
            break;
        }
        
        case APP_CMD_TERM_WINDOW: {
            eglMakeCurrent(e->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (e->context) eglDestroyContext(e->display, e->context);
            if (e->surface) eglDestroySurface(e->display, e->surface);
            eglTerminate(e->display);
            e->display = EGL_NO_DISPLAY;
            e->surface = EGL_NO_SURFACE;
            e->context = EGL_NO_CONTEXT;
            break;
        }
    }
}

// Главная функция
void android_main(struct android_app* app) {
    struct engine e = {0};
    e.app = app;
    e.movePointerId = -1;
    app->userData = &e;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
    
    double lastTime = get_time();
    
    while (1) {
        int ident;
        struct android_poll_source* source;
        
        while ((ident = ALooper_pollOnce(0, NULL, NULL, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;
        }
        
        if (e.display == EGL_NO_DISPLAY) continue;
        
        // Delta time
        double currentTime = get_time();
        float dt = (float)(currentTime - lastTime);
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;
        
        // Движение игрока (скорость постоянная, только направление)
        e.playerX += e.moveDirX * SPEED * dt;
        e.playerY += e.moveDirY * SPEED * dt;
        
        // Границы мира
        float halfSize = PLAYER_SIZE * 0.5f;
        if (e.playerX < -WORLD_SIZE/2 + halfSize) e.playerX = -WORLD_SIZE/2 + halfSize;
        if (e.playerX > WORLD_SIZE/2 - halfSize) e.playerX = WORLD_SIZE/2 - halfSize;
        if (e.playerY < -WORLD_SIZE/2 + halfSize) e.playerY = -WORLD_SIZE/2 + halfSize;
        if (e.playerY > WORLD_SIZE/2 - halfSize) e.playerY = WORLD_SIZE/2 - halfSize;
        
        // Камера следует за игроком
        e.camX = e.playerX - e.width * 0.5f;
        e.camY = e.playerY - e.height * 0.5f;
        
        // Движение второго объекта
        e.objX += e.objSpeedX * dt;
        e.objY += e.objSpeedY * dt;
        
        // Отражение от границ
        if (e.objX < -WORLD_SIZE/2 || e.objX > WORLD_SIZE/2) e.objSpeedX = -e.objSpeedX;
        if (e.objY < -WORLD_SIZE/2 || e.objY > WORLD_SIZE/2) e.objSpeedY = -e.objSpeedY;
        
        // Отрисовка
        glViewport(0, 0, e.width, e.height);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Рисуем сетку мира (для ориентира)
        glUseProgram(e.program);
        glUniform2f(e.uResLoc, (float)e.width, (float)e.height);
        glUniform2f(glGetUniformLocation(e.program, "cam"), e.camX, e.camY);
        
        // Второй объект (зеленый круг)
        draw_circle(e.objX, e.objY, 30, 0.0f, 1.0f, 0.0f, 1.0f, &e, false, 0);
        
        // Игрок (синий квадрат)
        draw_rect(e.playerX, e.playerY, PLAYER_SIZE, PLAYER_SIZE, 
                  0.2f, 0.6f, 1.0f, 1.0f, &e);
        
        // Джойстик (НЕПРОЗРАЧНЫЙ, в экранных координатах)
        float jx = JOY_X_OFFSET;
        float jy = e.height - JOY_Y_OFFSET;
        
        // Сброс камеры для UI
        glUseProgram(e.program);
        glUniform2f(e.uResLoc, (float)e.width, (float)e.height);
        glUniform2f(glGetUniformLocation(e.program, "cam"), 0, 0);
        
        // Фон джойстика (непрозрачный)
        draw_circle(jx, jy, JOY_RADIUS, 0.2f, 0.2f, 0.2f, 1.0f, &e, false, 0);
        // Обводка джойстика
        draw_circle(jx, jy, JOY_RADIUS, 1.0f, 1.0f, 1.0f, 1.0f, &e, true, 3.0f);
        
        // Стик (может выходить за пределы)
        float sx = jx + e.moveDirX * JOY_RADIUS * 1.2f; // 1.2 - выход за пределы
        float sy = jy + e.moveDirY * JOY_RADIUS * 1.2f;
        draw_circle(sx, sy, STICK_RADIUS, 0.8f, 0.8f, 0.8f, 1.0f, &e, false, 0);
        // Обводка стика
        draw_circle(sx, sy, STICK_RADIUS, 1.0f, 1.0f, 1.0f, 1.0f, &e, true, 2.0f);
        
        eglSwapBuffers(e.display, e.surface);
    }
}
