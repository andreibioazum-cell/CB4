#include <android_native_app_glue.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y; } Vec2;

float a = 0;

void line(ANativeWindow_Buffer* b, int x0, int y0, int x1, int y1, uint32_t c) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    while(1) {
        if(x0>=0 && x0<b->width && y0>=0 && y0<b->height)
            ((uint32_t*)b->bits)[y0*b->stride + x0] = c;
        if(x0==x1 && y0==y1) break;
        e2 = 2*err;
        if(e2>=dy) { err+=dy; x0+=sx; }
        if(e2<=dx) { err+=dx; y0+=sy; }
    }
}

void render(struct android_app* app) {
    ANativeWindow_Buffer b;
    if (ANativeWindow_lock(app->window, &b, NULL) < 0) return;
    uint32_t* p = (uint32_t*)b.bits;
    for(int i=0; i<b.height*b.stride; i++) p[i] = 0xFF000000;

    Vec3 v[8] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    int ed[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    
    a += 0.03f;
    float s = sinf(a), c = cosf(a);

    for(int i=0; i<12; i++) {
        Vec3 r[2];
        Vec2 p2d[2];
        for(int j=0; j<2; j++) {
            Vec3 t = v[ed[i][j]];
            float x = t.x*c - t.z*s;
            float z = t.x*s + t.z*c;
            float y = t.y*c - z*s;
            z = t.y*s + z*c;
            float f = 600.0f / (z + 5.0f);
            p2d[j].x = x*f + b.width/2.0f;
            p2d[j].y = y*f + b.height/2.0f;
        }
        line(&b, p2d[0].x, p2d[0].y, p2d[1].x, p2d[1].y, 0xFF00FF00);
    }
    ANativeWindow_unlockAndPost(app->window);
}

void handle_cmd(struct android_app* app, int32_t cmd) {
    if (cmd == APP_CMD_INIT_WINDOW) app->userData = (void*)1;
    if (cmd == APP_CMD_TERM_WINDOW) app->userData = NULL;
}

void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    while(1) {
        int id, events;
        struct android_poll_source* source;
        while((id = ALooper_pollAll(app->userData ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            if(source) source->process(app, source);
            if(app->destroyRequested) return;
        }
        if(app->userData) render(app);
    }
}
