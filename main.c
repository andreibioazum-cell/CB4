#include "raylib.h"
#include "raymath.h"

int main(void) {
    InitWindow(0, 0, "Geometrium");
    SetTargetFPS(60);

    Vector2 playerPos = { 0, 0 };
    Vector2 joyCenter = { 150, GetScreenHeight() - 150.0f };
    Vector2 moveDir = { 0, 0 };
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int tc = GetTouchPointCount();
        moveDir = (Vector2){ 0, 0 };

        for (int i = 0; i < tc; i++) {
            Vector2 tp = GetTouchPosition(i);
            if (CheckCollisionPointCircle(tp, joyCenter, 150.0f)) {
                float dist = Vector2Distance(tp, joyCenter);
                if (dist > 5.0f) {
                    moveDir.x = (tp.x - joyCenter.x) / dist;
                    moveDir.y = (tp.y - joyCenter.y) / dist;
                }
            }
        }

        playerPos.x += moveDir.x * 400.0f * dt;
        playerPos.y += moveDir.y * 400.0f * dt;
        camera.target = Vector2Lerp(camera.target, playerPos, 0.1f);

        BeginDrawing();
            ClearBackground((Color){10, 10, 20, 255});
            BeginMode2D(camera);
                for (int i = -1000; i <= 1000; i += 100) {
                    DrawLine(i, -1000, i, 1000, DARKGRAY);
                    DrawLine(-1000, i, 1000, i, DARKGRAY);
                }
                DrawCircleV(playerPos, 30, SKYBLUE);
            EndMode2D();
            DrawCircleV(joyCenter, 80, (Color){255,255,255,60});
            Vector2 sp = Vector2Add(joyCenter, Vector2Scale(moveDir, 50));
            DrawCircleV(sp, 35, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
