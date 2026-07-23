#include "raylib.h"

int main(void)
{
    InitWindow(0, 0, "Geometrium");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("RAYLIB ANDROID AUTO", 60, 60, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
