#include "raylib.h"

int main(void)
{
    InitWindow(0, 0, "Geometrium");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("RAYMOB WORKS", 80, 80, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
