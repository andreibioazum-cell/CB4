#include "raylib.h"
#include "raymath.h"

// Для Android важно, чтобы переменные сохранялись между кадрами
Vector2 playerPos = { 0, 0 };
Vector2 joyCenter = { 150, 0 }; // Y обновим в Init
Vector2 moveDir = { 0, 0 };
Camera2D camera = { 0 };

void UpdateGame(void) {
    float dt = GetFrameTime();
    
    // Ввод через тачскрин
    int touchCount = GetTouchPointCount();
    moveDir = (Vector2){ 0, 0 };
    
    for (int i = 0; i < touchCount; i++) {
        Vector2 touchPos = GetTouchPosition(i);
        if (CheckCollisionPointCircle(touchPos, joyCenter, 160.0f)) {
            float dist = Vector2Distance(touchPos, joyCenter);
            if (dist > 10.0f) {
                moveDir.x = (touchPos.x - joyCenter.x) / dist;
                moveDir.y = (touchPos.y - joyCenter.y) / dist;
            }
        }
    }

    playerPos.x += moveDir.x * 400.0f * dt;
    playerPos.y += moveDir.y * 400.0f * dt;
    camera.target = Vector2Lerp(camera.target, playerPos, 0.1f);
}

void DrawGame(void) {
    BeginDrawing();
        ClearBackground((Color){ 20, 20, 30, 255 });
        BeginMode2D(camera);
            // Сетка
            for(int i = -1000; i <= 1000; i += 100) {
                DrawLine(i, -1000, i, 1000, DARKGRAY);
                DrawLine(-1000, i, 1000, i, DARKGRAY);
            }
            DrawRectangleV((Vector2){playerPos.x - 25, playerPos.y - 25}, (Vector2){50, 50}, SKYBLUE);
        EndMode2D();

        // UI Джойстик
        DrawCircleV(joyCenter, 80, (Color){ 200, 200, 200, 100 });
        Vector2 stickPos = { joyCenter.x + moveDir.x * 60, joyCenter.y + moveDir.y * 60 };
        DrawCircleV(stickPos, 40, WHITE);
    EndDrawing();
}

int main(void) {
    // На Android InitWindow сам подстроит размер под экран
    InitWindow(0, 0, "Geometrium 2D");
    
    joyCenter.y = GetScreenHeight() - 150;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    CloseWindow();
    return 0;
}
