#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PI 3.14159265f
#define JOY_RADIUS 100.0f
#define JOY_X_OFFSET 140.0f
#define JOY_Y_OFFSET 140.0f
#define STICK_RADIUS 40.0f

struct engine {
    struct android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width, height;
    GLuint program;
    float playerX, playerY;
    float moveDirX, moveDirZ;
    int movePointerId;
};

// Шейдеры
static const char* vertexShader =
    "attribute vec2 pos;"
    "uniform vec2 res;"
    "void main() {"
    "  vec2 p = pos / res * 2.0 - 1.0;"
    "  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);"
    "}";

static const char* fragmentShader =
    "precision mediump float;"
    "uniform vec4 color;"
    "void main() { gl_FragColor = color; }";

// Отрисовка круга
static void draw_circle(float cx, float cy, float r, float cr, float cg, float cb, float ca, struct engine* e) {
    float verts[66*2];
    verts[0] = cx; verts[1] = cy;
    for (int i = 0; i <= 32; i++) {
        float a = (float)i / 32 * 2 * PI;
        verts[(i+1)*2] = cx + cosf(a) * r;
        verts[(i+1)*2+1] = cy + sinf(a) * r;
    }
    glUseProgram(e->program);
    glUniform2f(glGetUniformLocation(e->program, "res"), e->width, e->height);
    glUniform4f(glGetUniformLocation(e->program, "color"), cr, cg, cb, ca);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 34);
}

// Отрисовка прямоугольника
static void draw_rect(float x, float y, float w, float h, float cr, float cg, float cb, float ca, struct engine* e) {
    float verts[] = {
        x-w/2, y-h/2, x+w/2, y-h/2, x+w/2, y+h/2,
        x-w/2, y-h/2, x+w/2, y+h/2, x-w/2, y+h/2
    };
    glUseProgram(e->program);
    glUniform2f(glGetUniformLocation(e->program, "res"), e->width, e->height);
    glUniform4f(glGetUniformLocation(e->program, "color"), cr, cg, cb, ca);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Отрисовка кольца
static void draw_ring(float cx, float cy, float r, float thick, float cr, float cg, float cb, float ca, struct engine* e) {
    float verts[33*4];
    for (int i = 0; i <= 32; i++) {
        float a = (float)i / 32 * 2 * PI;
        float c = cosf(a), s = sinf(a);
        verts[i*4] = cx + c * r;
        verts[i*4+1] = cy + s * r;
        verts[i*4+2] = cx + c * (r - thick);
        verts[i*4+3] = cy + s * (r - thick);
    }
    glUseProgram(e->program);
    glUniform2f(glGetUniformLocation(e->program, "res"), e->width, e->height);
    glUniform4f(glGetUniformLocation(e->program, "color"), cr, cg, cb, ca);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 66);
}

// Обработка ввода
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* e = (struct engine*)app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    int action = AMotionEvent_getAction(event);
    int code = action & AMOTION_EVENT_ACTION_MASK;
    int pi = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int id = AMotionEvent_getPointerId(event, pi);
    float x = AMotionEvent_getX(event, pi);
    float y = AMotionEvent_getY(event, pi);

    if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        float jx = JOY_X_OFFSET;
        float jy = e->height - JOY_Y_OFFSET;
        float dx = x - jx, dy = y - jy;
        if (sqrtf(dx*dx + dy*dy) < JOY_RADIUS * 2.0f) {
            e->movePointerId = id;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > 5.0f) {
                float cd = dist > JOY_RADIUS ? JOY_RADIUS : dist;
                e->moveDirX = (dx / dist) * (cd / JOY_RADIUS);
                e->moveDirZ = (dy / dist) * (cd / JOY_RADIUS);
            }
            return 1;
        }
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_MOVE) {
        if (id == e->movePointerId) {
            float dx = x - JOY_X_OFFSET;
            float dy = y - (e->height - JOY_Y_OFFSET);
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > 5.0f) {
                float cd = dist > JOY_RADIUS ? JOY_RADIUS : dist;
                e->moveDirX = (dx / dist) * (cd / JOY_RADIUS);
                e->moveDirZ = (dy / dist) * (cd / JOY_RADIUS);
            } else {
                e->moveDirX = e->moveDirZ = 0;
            }
            return 1;
        }
        return 1;
    }

    if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (id == e->movePointerId) {
            e->movePointerId = -1;
            e->moveDirX = e->moveDirZ = 0;
            return 1;
        }
        return 1;
    }

    return 0;
}

// Создание шейдера
static GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

// Команды приложения
static void handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* e = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            e->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(e->display, NULL, NULL);
            
            EGLConfig config;
            EGLint numConfigs;
            EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                EGL_NONE
            };
            eglChooseConfig(e->display, attribs, &config, 1, &numConfigs);
            
            e->surface = eglCreateWindowSurface(e->display, config, app->window, NULL);
            EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            e->context = eglCreateContext(e->display, config, EGL_NO_CONTEXT, ctxAttribs);
            eglMakeCurrent(e->display, e->surface, e->surface, e->context);
            
            GLuint vs = compile_shader(GL_VERTEX_SHADER, vertexShader);
            GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragmentShader);
            e->program = glCreateProgram();
            glAttachShader(e->program, vs);
            glAttachShader(e->program, fs);
            glBindAttribLocation(e->program, 0, "pos");
            glLinkProgram(e->program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            
            eglQuerySurface(e->display, e->surface, EGL_WIDTH, &e->width);
            eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &e->height);
            e->playerX = e->width / 2.0f;
            e->playerY = e->height / 2.0f;
            e->movePointerId = -1;
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
    struct engine* e = calloc(1, sizeof(struct engine));
    e->app = app;
    e->movePointerId = -1;
    app->userData = e;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;
    
    while (1) {
        struct android_poll_source* source;
        int ident;
        while ((ident = ALooper_pollOnce(0, NULL, NULL, (void**)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) { free(e); return; }
        }
        
        if (e->display == EGL_NO_DISPLAY) continue;
        
        // Движение
        if (e->movePointerId != -1 && (e->moveDirX != 0 || e->moveDirZ != 0)) {
            float speed = 300.0f;
            e->playerX += e->moveDirX * speed * 0.016f;
            e->playerY += e->moveDirZ * speed * 0.016f;
            if (e->playerX < 20) e->playerX = 20;
            if (e->playerX > e->width - 20) e->playerX = e->width - 20;
            if (e->playerY < 20) e->playerY = 20;
            if (e->playerY > e->height - 20) e->playerY = e->height - 20;
        }
        
        // Отрисовка
        glViewport(0, 0, e->width, e->height);
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        float jx = JOY_X_OFFSET;
        float jy = e->height - JOY_Y_OFFSET;
        
        draw_ring(jx, jy, JOY_RADIUS, 3.0f, 0.0f, 0.0f, 0.0f, 0.6f, e);
        float sx = jx + e->moveDirX * JOY_RADIUS * 0.7f;
        float sy = jy + e->moveDirZ * JOY_RADIUS * 0.7f;
        draw_circle(sx, sy, STICK_RADIUS, 0.0f, 0.0f, 0.0f, 0.7f, e);
        draw_rect(e->playerX, e->playerY, 30.0f, 30.0f, 1.0f, 0.2f, 0.2f, 1.0f, e);
        
        eglSwapBuffers(e->display, e->surface);
    }
}
