#!/bin/bash

set -e
BUILD_DIR="build"
ROM_PATH="roms/tetris.gb"

cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR"

if [ "$1" = "--debug" ]; then
    lldb -o "run" -o "quit" ./"$BUILD_DIR"/gb-header-parser -- "$ROM_PATH"
else
    ./"$BUILD_DIR"/gb-header-parser "$ROM_PATH"
fi