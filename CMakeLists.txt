cmake_minimum_required(VERSION 3.15)
project(intel-8080-emulator-cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch SDL2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.3
)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# 2. Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.6
)
FetchContent_MakeAvailable(imgui)

# 3. Locate ImGui backends directory from FetchContent paths
FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_SOURCE_DIR)
set(IMGUI_BACKENDS_DIR "${IMGUI_SOURCE_DIR}/backends")

# 4. Collect core emulator code
file(GLOB CORE_FILES CONFIGURE_DEPENDS "*.cpp" "*.h")

# 5. Add the explicit ImGui backend files required for an SDL2 + SDL_Renderer setup
set(IMGUI_BACKEND_FILES
    "${IMGUI_SOURCE_DIR}/imgui.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_draw.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_tables.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_widgets.cpp"
    "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.cpp"
    "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer2.cpp"
)

add_executable(intel-8080-emulator-cpp ${CORE_FILES} ${IMGUI_BACKEND_FILES})

# 6. Link libraries and set up include paths
target_link_libraries(intel-8080-emulator-cpp PRIVATE SDL2::SDL2)
target_include_directories(intel-8080-emulator-cpp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_BACKENDS_DIR}
)

target_compile_definitions(intel-8080-emulator-cpp PRIVATE ROM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/roms")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")