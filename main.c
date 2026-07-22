#include "raylib.h"
#include "raymath.h"
#include <math.h>

#define JOY_RADIUS 80.0f
#define STICK_RADIUS 40.0f
#define WORLD_SIZE 2000.0f
#define PLAYER_SPEED 400.0f

int main(void) {
    // Инициализация
    InitWindow(0, 0, "Geometrium 2D"); // На Android 0,0 — это полный экран
    
    // Игрок и камера
    Vector2 playerPos = { 0, 0 };
    Camera2D camera = { 0 };
    camera.target = playerPos;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Состояние джойстика
    Vector2 joyCenter = { 150, (float)GetScreenHeight() - 150 };
    Vector2 moveDir = { 0, 0 };
    int movePointerId = -1;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- ЛОГИКА ВВОДА (ДЖОЙСТИК) ---
        int touchCount = GetTouchPointCount();
        moveDir = (Vector2){ 0, 0 };
        bool touchingJoy = false;

        for (int i = 0; i < touchCount; i++) {
            Vector2 touchPos = GetTouchPosition(i);
            
            // Если нажали в зоне джойстика
            if (CheckCollisionPointCircle(touchPos, joyCenter, JOY_RADIUS * 2.0f)) {
                float dist = Vector2Distance(touchPos, joyCenter);
                if (dist > 10.0f) {
                    moveDir.x = (touchPos.x - joyCenter.x) / dist;
                    moveDir.y = (touchPos.y - joyCenter.y) / dist;
                }
                touchingJoy = true;
            }
        }

        // --- ОБНОВЛЕНИЕ ---
        float dt = GetFrameTime();
        playerPos.x += moveDir.x * PLAYER_SPEED * dt;
        playerPos.y += moveDir.y * PLAYER_SPEED * dt;

        // Ограничение мира
        playerPos.x = Clamp(playerPos.x, -WORLD_SIZE/2, WORLD_SIZE/2);
        playerPos.y = Clamp(playerPos.y, -WORLD_SIZE/2, WORLD_SIZE/2);

        // Камера плавно следует за игроком
        camera.target = Vector2Lerp(camera.target, playerPos, 0.1f);
        camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };

        // --- ОТРИСОВКА ---
        BeginDrawing();
            ClearBackground((Color){ 20, 20, 30, 255 });

            BeginMode2D(camera);
                // Рисуем сетку мира
                for(int i = -WORLD_SIZE/2; i <= WORLD_SIZE/2; i += 100) {
                    DrawLine(i, -WORLD_SIZE/2, i, WORLD_SIZE/2, DARKGRAY);
                    DrawLine(-WORLD_SIZE/2, i, WORLD_SIZE/2, i, DARKGRAY);
                }

                // Игрок
                DrawRectangleV((Vector2){playerPos.x - 25, playerPos.y - 25}, (Vector2){50, 50}, SKYBLUE);
                DrawRectangleLines(playerPos.x - 25, playerPos.y - 25, 50, 50, WHITE);
            EndMode2D();

            // UI - Джойстик (рисуем поверх всего)
            DrawCircleV(joyCenter, JOY_RADIUS, (Color){ 200, 200, 200, 100 });
            DrawCircleLines(joyCenter.x, joyCenter.y, JOY_RADIUS, WHITE);
            
            Vector2 stickPos = { 
                joyCenter.x + moveDir.x * (JOY_RADIUS * 0.8f), 
                joyCenter.y + moveDir.y * (JOY_RADIUS * 0.8f) 
            };
            DrawCircleV(stickPos, STICK_RADIUS, (Color){ 255, 255, 255, 180 });

            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
