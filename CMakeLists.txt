cmake_minimum_required(VERSION 3.15)
project(intel-8080-emulator-cpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(intel-8080-emulator-cpp main.cpp)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)