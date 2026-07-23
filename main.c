#include "raylib.h"
#include <android/log.h>

// Макрос для вывода логов в Logcat (можно смотреть через adb logcat)
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "GEOMETRIUM", __VA_ARGS__)

int main(void) {
    LOG("Инициализация окна...");
    InitWindow(0, 0, "Geometrium 2D");
    
    SetTargetFPS(60);
    LOG("Игра запущена!");

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("GEOMETRIUM RUNNING", 50, 100, 40, RAYWHITE);
            DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 60, RED);
        EndDrawing();
    }

    LOG("Закрытие окна...");
    CloseWindow();
    return 0;
}
