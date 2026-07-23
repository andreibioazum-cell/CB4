#include "raylib.h"

int main(void) {
    // На Android 0,0 означает нативный размер экрана
    InitWindow(0, 0, "Geometrium");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("IT WORKS!", 100, 100, 40, WHITE);
            DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 50, RED);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
