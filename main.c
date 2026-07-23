#include "raylib.h"

int main(void)
{
    InitWindow(0, 0, "Geometrium");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("AUTO RAYMOB BUILD", 80, 80, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
