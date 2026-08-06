#!/usr/bin/env bash
set -e

echo "=== Building CB4 Graphics (Zig) and Game Logic (C) ==="

# 1. Check Zig installation
if ! command -v zig &> /dev/null; then
    echo "Zig is not found in PATH. Please install Zig 0.12.0 or 0.13.0."
    exit 1
fi

# 2. Check Android NDK
if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "ANDROID_NDK_ROOT is not set. Please set ANDROID_NDK_ROOT to your NDK path."
    exit 1
fi

TOOLCHAIN="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin"
GLUE="$ANDROID_NDK_ROOT/sources/android/native_app_glue"

# 3. Download stb dependencies if not present
if [ ! -f stb_image.h ]; then
    echo "Downloading stb_image.h..."
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
fi

if [ ! -f stb_truetype.h ]; then
    echo "Downloading stb_truetype.h..."
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
fi

# 4. Compile Zig Graphics Module
echo "Compiling graphics.zig for aarch64..."
zig build-obj graphics.zig -target aarch64-linux-android -O ReleaseFast -fPIC -femit-bin=graphics_64.o

echo "Compiling graphics.zig for armv7a..."
zig build-obj graphics.zig -target arm-linux-androideabi -O ReleaseFast -fPIC -femit-bin=graphics_32.o

# 5. Compile C scripts and link with Zig
C_SRCS="main.c game.c font.c ui.c $GLUE/android_native_app_glue.c"
FLAGS="-O3 -s -fPIC -shared -I. -I$GLUE -landroid -llog -lm"

echo "Linking libmain_64.so (ARM64)..."
"$TOOLCHAIN/aarch64-linux-android21-clang" $C_SRCS graphics_64.o -o libmain_64.so $FLAGS

echo "Linking libmain_32.so (ARMv7)..."
"$TOOLCHAIN/armv7a-linux-androideabi21-clang" $C_SRCS graphics_32.o -o libmain_32.so $FLAGS -mfpu=neon

echo "=== Build succeeded! ==="
