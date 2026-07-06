include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# ───────────────────────────────
# Build Test Runner Executable
# ───────────────────────────────
add_executable(unit_tests
    test_opcodes.cpp
    ../src/Intel8080.cpp
    ../src/Bus.cpp
    ../src/Memory.cpp
    ../src/Opcodes_ALU.cpp
    ../src/Opcodes_Move.cpp
    ../src/Opcodes_System.cpp
    ../src/Opcode_Table.cpp
)

# Explicitly force the header search path directory lookups!
target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${catch2_SOURCE_DIR}/src/catch2/..
)

# Link directly against the un-namespaced targets
target_link_libraries(unit_tests PRIVATE
    Catch2
    Catch2WithMain
)

include(Catch)
catch_discover_tests(unit_tests)
