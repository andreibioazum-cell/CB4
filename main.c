#include "raylib.h"
#include "raymath.h"
#include <android_native_app_glue.h>

// Состояние игрока
Vector2 playerPos = { 0, 0 };
Vector2 joyCenter = { 150, 450 };
Vector2 moveDir = { 0, 0 };

void UpdateGame(void) {
    float dt = GetFrameTime();
    int touchCount = GetTouchPointCount();
    moveDir = (Vector2){ 0, 0 };

    for (int i = 0; i < touchCount; i++) {
        Vector2 touchPos = GetTouchPosition(i);
        if (CheckCollisionPointCircle(touchPos, joyCenter, 150.0f)) {
            float dist = Vector2Distance(touchPos, joyCenter);
            if (dist > 5.0f) {
                moveDir.x = (touchPos.x - joyCenter.x) / dist;
                moveDir.y = (touchPos.y - joyCenter.y) / dist;
            }
        }
    }
    playerPos.x += moveDir.x * 400.0f * dt;
    playerPos.y += moveDir.y * 400.0f * dt;
}

void DrawGame(void) {
    BeginDrawing();
    ClearBackground((Color){ 10, 10, 15, 255 });
    
    // Рисуем мир
    DrawCircleV(playerPos, 30, SKYBLUE);
    
    // Рисуем UI
    DrawCircleV(joyCenter, 80, (Color){ 255, 255, 255, 60 });
    Vector2 stickPos = Vector2Add(joyCenter, Vector2Scale(moveDir, 50));
    DrawCircleV(stickPos, 35, WHITE);
    
    DrawFPS(10, 10);
    EndDrawing();
}

// Главная функция для Android
void android_main(struct android_app* state) {
    // Важно для Raylib на Android
    InitWindow(0, 0, "Geometrium");
    
    joyCenter.y = GetScreenHeight() - 150;
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
    CloseWindow();
}
