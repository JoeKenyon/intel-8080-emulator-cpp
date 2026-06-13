cmake_minimum_required(VERSION 3.15)
project(GBHeaderParser VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the executable and its source files
add_executable(gb-header-parser main.cpp)