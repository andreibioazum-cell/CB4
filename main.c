#include "raylib.h"
#include "raymath.h"

// Состояние игры
typedef struct {
    Vector2 playerPos;
    Vector2 joyCenter;
    Vector2 moveDir;
    Camera2D camera;
} GameState;

static GameState state = {0};

// Инициализация
void GameInit(void) {
    state.playerPos = (Vector2){ 0, 0 };
    state.joyCenter = (Vector2){ 150, 0 };
    state.moveDir = (Vector2){ 0, 0 };
    state.camera.zoom = 1.0f;
    state.joyCenter.y = GetScreenHeight() - 150.0f;
    state.camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    state.camera.target = state.playerPos;
}

// Обновление
void GameUpdate(void) {
    float dt = GetFrameTime();
    int touchCount = GetTouchPointCount();
    state.moveDir = (Vector2){ 0, 0 };

    for (int i = 0; i < touchCount; i++) {
        Vector2 tp = GetTouchPosition(i);
        if (CheckCollisionPointCircle(tp, state.joyCenter, 150.0f)) {
            float dist = Vector2Distance(tp, state.joyCenter);
            if (dist > 5.0f) {
                state.moveDir.x = (tp.x - state.joyCenter.x) / dist;
                state.moveDir.y = (tp.y - state.joyCenter.y) / dist;
            }
        }
    }

    state.playerPos.x += state.moveDir.x * 400.0f * dt;
    state.playerPos.y += state.moveDir.y * 400.0f * dt;
    state.camera.target = Vector2Lerp(state.camera.target, state.playerPos, 0.1f);
}

// Отрисовка
void GameDraw(void) {
    BeginDrawing();
    ClearBackground((Color){ 10, 10, 20, 255 });

    BeginMode2D(state.camera);
        for (int i = -1000; i <= 1000; i += 100) {
            DrawLine(i, -1000, i, 1000, DARKGRAY);
            DrawLine(-1000, i, 1000, i, DARKGRAY);
        }
        DrawCircleV(state.playerPos, 30, SKYBLUE);
    EndMode2D();

    // UI Джойстик
    DrawCircleV(state.joyCenter, 80, (Color){ 255,255,255,60 });
    Vector2 stickPos = Vector2Add(state.joyCenter, Vector2Scale(state.moveDir, 50));
    DrawCircleV(stickPos, 35, WHITE);

    DrawFPS(10, 10);
    EndDrawing();
}

int main(void) {
    InitWindow(0, 0, "Geometrium 2D");
    SetTargetFPS(60);
    GameInit();

    while (!WindowShouldClose()) {
        GameUpdate();
        GameDraw();
    }

    CloseWindow();
    return 0;
}
