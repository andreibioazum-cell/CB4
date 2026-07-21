#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PI 3.14159265f
#define JOY_RADIUS 80.0f
#define JOY_X_OFFSET 130.0f
#define JOY_Y_OFFSET 130.0f
#define STICK_RADIUS 32.0f

struct engine {
    struct android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width, height;
    GLuint program;
    
    float playerX, playerY;
    float moveDirX, moveDirZ;
    float lastTouchX, lastTouchY;
    int movePointerId, lookPointerId;
    bool isMoving;
};

// ========== ШЕЙДЕРЫ ==========
static const char* vertexShaderSource =
    "attribute vec2 pos;"
    "uniform vec2 resolution;"
    "uniform vec2 translate;"
    "uniform float scale;"
    "void main() {"
    "  vec2 p = (pos * scale + translate) / resolution * 2.0 - 1.0;"
    "  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);"
    "}";

static const char* fragmentShaderSource =
    "precision mediump float;"
    "uniform vec4 color;"
    "void main() {"
    "  gl_FragColor = color;"
    "}";

// ========== ОТРИСОВКА КРУГА ==========
static void draw_circle(float cx, float cy, float r, 
                         float r_color, float g_color, float b_color, float a_color,
                         struct engine* eng) {
    int segs = 32;
    float verts[(32+1)*2];
    verts[0] = cx; verts[1] = cy;
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2.0f * PI;
        verts[(i+1)*2+0] = cx + cosf(a) * r;
        verts[(i+1)*2+1] = cy + sinf(a) * r;
    }
    
    glUseProgram(eng->program);
    glUniform2f(glGetUniformLocation(eng->program, "resolution"), eng->width, eng->height);
    glUniform2f(glGetUniformLocation(eng->program, "translate"), 0, 0);
    glUniform1f(glGetUniformLocation(eng->program, "scale"), 1.0f);
    glUniform4f(glGetUniformLocation(eng->program, "color"), r_color, g_color, b_color, a_color);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segs + 2);
}

// ========== ОТРИСОВКА КВАДРАТА ==========
static void draw_rect(float x, float y, float w, float h,
                       float r_color, float g_color, float b_color, float a_color,
                       struct engine* eng) {
    float verts[] = {
        x - w/2, y - h/2,
        x + w/2, y - h/2,
        x + w/2, y + h/2,
        x - w/2, y - h/2,
        x + w/2, y + h/2,
        x - w/2, y + h/2
    };
    
    glUseProgram(eng->program);
    glUniform2f(glGetUniformLocation(eng->program, "resolution"), eng->width, eng->height);
    glUniform2f(glGetUniformLocation(eng->program, "translate"), 0, 0);
    glUniform1f(glGetUniformLocation(eng->program, "scale"), 1.0f);
    glUniform4f(glGetUniformLocation(eng->program, "color"), r_color, g_color, b_color, a_color);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// ========== ОТРИСОВКА КОЛЬЦА ==========
static void draw_ring(float cx, float cy, float r, float thick,
                       float r_color, float g_color, float b_color, float a_color,
                       struct engine* eng) {
    int segs = 32;
    float verts[(32+1)*4];
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2.0f * PI;
        float c = cosf(a), s = sinf(a);
        verts[i*4+0] = cx + c * r;
        verts[i*4+1] = cy + s * r;
        verts[i*4+2] = cx + c * (r - thick);
        verts[i*4+3] = cy + s * (r - thick);
    }
    
    glUseProgram(eng->program);
    glUniform2f(glGetUniformLocation(eng->program, "resolution"), eng->width, eng->height);
    glUniform2f(glGetUniformLocation(eng->program, "translate"), 0, 0);
    glUniform1f(glGetUniformLocation(eng->program, "scale"), 1.0f);
    glUniform4f(glGetUniformLocation(eng->program, "color"), r_color, g_color, b_color, a_color);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (segs+1)*2);
}

// ========== ОБРАБОТКА ВВОДА ==========
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* eng = (struct engine*)app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    int action = AMotionEvent_getAction(event);
    int code = action & AMOTION_EVENT_ACTION_MASK;
    int pCount = AMotionEvent_getPointerCount(event);

    if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        int pi = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        float x = AMotionEvent_getX(event, pi);
        float y = AMotionEvent_getY(event, pi);
        int id = AMotionEvent_getPointerId(event, pi);

        // Проверяем попадание в джойстик
        float jx = JOY_X_OFFSET;
        float jy = eng->height - JOY_Y_OFFSET;
        float dx = x - jx;
        float dy = y - jy;
        if (sqrtf(dx*dx + dy*dy) < JOY_RADIUS * 1.5f) {
            eng->isMoving = true;
            eng->movePointerId = id;
            return 1;
        }

        // Запоминаем для взгляда (на самом деле здесь не используется)
        eng->lastTouchX = x;
        eng->lastTouchY = y;
        eng->lookPointerId = id;
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_MOVE) {
        for (int i = 0; i < pCount; i++) {
            float x = AMotionEvent_getX(event, i);
            float y = AMotionEvent_getY(event, i);
            int id = AMotionEvent_getPointerId(event, i);

            if (id == eng->movePointerId) {
                float dx = x - JOY_X_OFFSET;
                float dy = y - (eng->height - JOY_Y_OFFSET);
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > 5.0f) {
                    float clampDist = dist > JOY_RADIUS ? JOY_RADIUS : dist;
                    eng->moveDirX = (dx / dist) * (clampDist / JOY_RADIUS);
                    eng->moveDirZ = (dy / dist) * (clampDist / JOY_RADIUS);
                } else {
                    eng->moveDirX = 0;
                    eng->moveDirZ = 0;
                }
            }
        }
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP) {
        int pi = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int id = AMotionEvent_getPointerId(event, pi);

        if (id == eng->movePointerId) {
            eng->isMoving = false;
            eng->moveDirX = 0;
            eng->moveDirZ = 0;
            eng->movePointerId = -1;
        } else if (id == eng->lookPointerId) {
            eng->lookPointerId = -1;
        }
        return 1;
    }

    return 0;
}

// ========== ИНИЦИАЛИЗАЦИЯ ШЕЙДЕРОВ ==========
static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        __android_log_print(ANDROID_LOG_ERROR, "Game2D", "Shader compile error: %s", log);
    }
    return shader;
}

static GLuint create_program() {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ========== КОМАНДЫ APP ==========
static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* eng = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            eng->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(eng->display, NULL, NULL);
            
            EGLConfig config;
            EGLint numConfigs;
            EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_NONE
            };
            eglChooseConfig(eng->display, attribs, &config, 1, &numConfigs);
            
            eng->surface = eglCreateWindowSurface(eng->display, config, app->window, NULL);
            EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            eng->context = eglCreateContext(eng->display, config, EGL_NO_CONTEXT, contextAttribs);
            eglMakeCurrent(eng->display, eng->surface, eng->surface, eng->context);
            
            eng->program = create_program();
            
            eglQuerySurface(eng->display, eng->surface, EGL_WIDTH, &eng->width);
            eglQuerySurface(eng->display, eng->surface, EGL_HEIGHT, &eng->height);
            
            eng->playerX = eng->width / 2.0f;
            eng->playerY = eng->height / 2.0f;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            eglMakeCurrent(eng->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (eng->context) eglDestroyContext(eng->display, eng->context);
            if (eng->surface) eglDestroySurface(eng->display, eng->surface);
            eglTerminate(eng->display);
            eng->display = EGL_NO_DISPLAY;
            eng->surface = EGL_NO_SURFACE;
            eng->context = EGL_NO_CONTEXT;
            break;
        }
    }
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ==========
void android_main(struct android_app* app) {
    app_dummy();
    
    struct engine* eng = malloc(sizeof(struct engine));
    memset(eng, 0, sizeof(struct engine));
    eng->app = app;
    eng->movePointerId = -1;
    eng->lookPointerId = -1;
    eng->playerX = 100;
    eng->playerY = 100;
    
    app->userData = eng;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
    
    while (1) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollAll(0, NULL, &events, (void**)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }
            if (app->destroyRequested) {
                free(eng);
                return;
            }
        }
        
        if (eng->display == EGL_NO_DISPLAY) continue;
        
        // Обновление позиции игрока
        if (eng->isMoving) {
            float speed = 300.0f;
            eng->playerX += eng->moveDirX * speed * 0.016f;
            eng->playerY += eng->moveDirZ * speed * 0.016f;
            
            // Ограничение по границам экрана
            if (eng->playerX < 20) eng->playerX = 20;
            if (eng->playerX > eng->width - 20) eng->playerX = eng->width - 20;
            if (eng->playerY < 20) eng->playerY = 20;
            if (eng->playerY > eng->height - 20) eng->playerY = eng->height - 20;
        }
        
        // Отрисовка
        glViewport(0, 0, eng->width, eng->height);
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Рисуем джойстик
        float jx = JOY_X_OFFSET;
        float jy = eng->height - JOY_Y_OFFSET;
        
        // Обводка джойстика
        draw_ring(jx, jy, JOY_RADIUS, 3.0f, 1.0f, 1.0f, 1.0f, 0.5f, eng);
        
        // Стик
        float sx = jx + eng->moveDirX * JOY_RADIUS * 0.7f;
        float sy = jy + eng->moveDirZ * JOY_RADIUS * 0.7f;
        draw_circle(sx, sy, STICK_RADIUS, 1.0f, 1.0f, 1.0f, 0.4f, eng);
        
        // Рисуем игрока (красный квадрат)
        draw_rect(eng->playerX, eng->playerY, 30.0f, 30.0f, 1.0f, 0.2f, 0.2f, 1.0f, eng);
        
        // Информация о направлении
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Dir: (%.2f, %.2f)", eng->moveDirX, eng->moveDirZ);
        
        eglSwapBuffers(eng->display, eng->surface);
    }
}
