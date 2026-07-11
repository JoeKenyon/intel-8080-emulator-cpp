cmake_minimum_required(VERSION 3.15)
project(intel-8080-emulator-cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ───────────────────────────────
# SDL2 via FetchContent
# ───────────────────────────────
include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.0
)

FetchContent_MakeAvailable(SDL2)

# ───────────────────────────────
# Core Emulator Executable
# ───────────────────────────────
add_executable(intel-8080-emulator-cpp
src/CPU.cpp src/CPU.h src/Display.cpp src/Display.h src/Emulator.cpp src/Emulator.h src/MachineBus.h src/MachineConfig.h src/main.cpp src/SpaceInvaders.h
)

target_include_directories(intel-8080-emulator-cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ───────────────────────────────
# Link Platform Libraries
# ───────────────────────────────
target_link_libraries(intel-8080-emulator-cpp PRIVATE SDL2::SDL2)

# ───────────────────────────────
# ROM Asset Asset Copying
# ───────────────────────────────
target_compile_definitions(intel-8080-emulator-cpp PRIVATE
    ROM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/roms"
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
