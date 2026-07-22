#include "raylib.h"
#include "raymath.h"

Vector2 playerPos = { 400, 400 };
Vector2 joyCenter = { 150, 450 };
Vector2 moveDir = { 0, 0 };
Camera2D camera = { 0 };

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
    camera.target = Vector2Lerp(camera.target, playerPos, 0.1f);
}

void DrawGame(void) {
    BeginDrawing();
    ClearBackground((Color){ 10, 10, 15, 255 });
    
    BeginMode2D(camera);
        for (int i = -1000; i <= 1000; i += 100) {
            DrawLine(i, -1000, i, 1000, DARKGRAY);
            DrawLine(-1000, i, 1000, i, DARKGRAY);
        }
        DrawCircleV(playerPos, 40, SKYBLUE);
    EndMode2D();
    
    DrawCircleV(joyCenter, 80, (Color){ 255, 255, 255, 60 });
    Vector2 stickPos = Vector2Add(joyCenter, Vector2Scale(moveDir, 50));
    DrawCircleV(stickPos, 35, WHITE);
    
    EndDrawing();
}

int main(void) {
    InitWindow(0, 0, "Geometrium");
    camera.target = playerPos;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.zoom = 1.0f;
    joyCenter.y = GetScreenHeight() - 150;

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }
    CloseWindow();
    return 0;
}
