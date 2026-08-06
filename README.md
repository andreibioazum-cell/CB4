# CB4 - Hardcore 2D Engine (C + Zig)

Гибридный модульный 2D движок для Android, использующий мощь **Zig** для графического рендерера и гибкость **C** для игровой логики и скриптов.

---

## Архитектура проекта

```
CB4/
├── graphics.zig        # Графическое ядро (Zig): растеризация, примитивы, спрайты, альфа-блендинг
├── graphics.h          # C ABI интерфейс к графическому модулю
├── graphics.c          # C эталонная реализация графических примитивов
├── main.c              # Точка входа Android Native Activity, обработка событий и жизненный цикл
├── game.h / game.c     # Игровая логика / скрипты (игрок, физика, стрельба, FPS)
├── ui.h / ui.c         # UI скрипты (виртуальный джойстик, кнопки, обработка тачей)
├── font.h / font.c     # TTF шрифтовой модуль с поддержкой UTF-8 (stb_truetype)
├── AndroidManifest.xml # Конфигурация Android приложения
├── build.sh            # Скрипт сборки (Zig + NDK Clang)
└── .github/workflows/  # CI/CD автоматическая сборка APK
```

---

## Разделение обязанностей

### 1. Zig — Графика и растеризация (`graphics.zig`)
* **Высокопроизводительные примитивы**: `graphics_clear`, `graphics_draw_rect`, `graphics_draw_rect_exact`, `graphics_draw_rect_lines`, `graphics_draw_circle`, `graphics_draw_ring`, `graphics_draw_line`.
* **Быстрый блендинг и трансформация спрайтов**: `graphics_draw_texture_ex` (вращение, масштабирование, корректный альфа-блендинг), `graphics_draw_texture`.
* Прямой C ABI экспорт (`pub export fn`) без накладных расходов.

### 2. C — Игровая логика и скрипты (`game.c`, `ui.c`, `main.c`)
* Управление сущностями (игрок, пули, коллизии с границами экрана).
* Обработка пользовательского ввода и жестов (джойстик, кнопки атаки).
* Цикл отрисовки и обновления состояния игры.
* Рендеринг шрифтов и текста.

---

## Сборка

### Локально через `build.sh`:
```bash
./build.sh
```

### GitHub Actions Workflow:
Для сборки через GitHub Actions добавьте шаг `mlugg/setup-zig@v1` в `.github/workflows/main.yml`:
```yaml
    - name: Setup Zig
      uses: mlugg/setup-zig@v1
      with:
        version: 0.13.0

    - name: Build Graphics (Zig) and Scripts (C)
      run: |
        zig build-obj graphics.zig -target aarch64-linux-android -O ReleaseFast -fPIC -femit-bin=graphics_64.o
        zig build-obj graphics.zig -target arm-linux-androideabi -O ReleaseFast -fPIC -femit-bin=graphics_32.o

        NDK_ROOT=$ANDROID_NDK_ROOT
        TOOLCHAIN="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin"
        GLUE="$NDK_ROOT/sources/android/native_app_glue"
        C_SRCS="main.c game.c font.c ui.c $GLUE/android_native_app_glue.c"
        FLAGS="-O3 -s -fPIC -shared -I. -I$GLUE -landroid -llog -lm"

        "$TOOLCHAIN/aarch64-linux-android21-clang" $C_SRCS graphics_64.o -o libmain_64.so $FLAGS
        "$TOOLCHAIN/armv7a-linux-androideabi21-clang" $C_SRCS graphics_32.o -o libmain_32.so $FLAGS -mfpu=neon
```
