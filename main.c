#include "raylib.h"
#include "raymath.h"

// Состояние игрока
Vector2 playerPos = { 400, 400 };
Vector2 joyCenter = { 150, 450 };
Vector2 moveDir = { 0, 0 };

// Объявляем камеру
Camera2D camera = { 0 };

void UpdateGame(void) {
    float dt = GetFrameTime();
    int touchCount = GetTouchPointCount();
    moveDir = (Vector2){ 0, 0 };

    // Логика джойстика
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

    // Движение игрока
    playerPos.x += moveDir.x * 400.0f * dt;
    playerPos.y += moveDir.y * 400.0f * dt;

    // Камера плавно следует за игроком
    camera.target = Vector2Lerp(camera.target, playerPos, 0.1f);
}

void DrawGame(void) {
    BeginDrawing();
    ClearBackground((Color){ 10, 10, 15, 255 });
    
    // Отрисовка мира с использованием камеры
    BeginMode2D(camera);
        // Рисуем простую сетку, чтобы видеть движение
        for (int i = -2000; i <= 2000; i += 100) {
            DrawLine(i, -2000, i, 2000, DARKGRAY);
            DrawLine(-2000, i, 2000, i, DARKGRAY);
        }
        // Игрок
        DrawCircleV(playerPos, 40, SKYBLUE);
    EndMode2D();
    
    // Отрисовка UI (джойстик) поверх камеры
    DrawCircleV(joyCenter, 80, (Color){ 255, 255, 255, 60 });
    Vector2 stickPos = Vector2Add(joyCenter, Vector2Scale(moveDir, 50));
    DrawCircleV(stickPos, 35, WHITE);
    
    DrawFPS(10, 10);
    EndDrawing();
}

int main(void) {
    // Инициализация окна
    InitWindow(0, 0, "Geometrium");
    
    // Настройка камеры
    camera.target = playerPos;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Позиция джойстика внизу экрана
    joyCenter.y = GetScreenHeight() - 150;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    CloseWindow();
    return 0;
}
