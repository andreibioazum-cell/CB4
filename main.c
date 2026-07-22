#include "raylib.h"
#include "raymath.h"

// Состояние игрока
Vector2 playerPos = { 400, 400 };
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
    
    // Рисуем мир (простой круг игрока)
    DrawCircleV(playerPos, 40, SKYBLUE);
    
    // Рисуем UI Джойстик
    DrawCircleV(joyCenter, 80, (Color){ 255, 255, 255, 60 });
    Vector2 stickPos = Vector2Add(joyCenter, Vector2Scale(moveDir, 50));
    DrawCircleV(stickPos, 35, WHITE);
    
    DrawFPS(10, 10);
    EndDrawing();
}

// ДЛЯ RAYLIB НА ANDROID ИСПОЛЬЗУЕМ ОБЫЧНЫЙ MAIN
int main(void) {
    InitWindow(0, 0, "Geometrium");
    
    joyCenter.y = GetScreenHeight() - 150;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f }; // Если используешь камеру

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
    CloseWindow();
    return 0;
}
