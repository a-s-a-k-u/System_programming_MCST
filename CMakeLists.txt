cmake_minimum_required(VERSION 3.20)
project(System_programming LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Явно указываем CMake, где искать Bison и Flex
set(BISON_EXECUTABLE "C:/msys64/usr/bin/bison.exe" CACHE FILEPATH "Path to bison")
set(FLEX_EXECUTABLE  "C:/msys64/usr/bin/flex.exe"  CACHE FILEPATH "Path to flex")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2 # стабильная версия
)
FetchContent_MakeAvailable(Catch2)
# =========================================================================
# ЛАБА 1: ГРАФЫ (System_programming)
# =========================================================================
add_executable(System_programming
        src/main.cpp
        tools/graph.hpp
        tools/tree.hpp
)
# Убрали frontend_main.cpp и AST.h отсюда! У них свои main().
target_include_directories(System_programming PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# =========================================================================
# ЛАБА 2: ФРОНТЕНД (frontend)
# =========================================================================
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)


# Обратите внимание на пути: frontend/parser.y и frontend/lexer.l
bison_target(MyParser frontend/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp COMPILE_FLAGS "-d")
flex_target(MyLexer frontend/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)
add_flex_bison_dependency(MyLexer MyParser)

# Создаём ОТДЕЛЬНУЮ программу для фронтенда
add_executable(frontend
        frontend_main.cpp
        ${BISON_MyParser_OUTPUTS}
        ${FLEX_MyLexer_OUTPUTS}
)

# Чтобы фронтенд видел AST.h и сгенерированный parser.tab.hpp
target_include_directories(frontend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/frontend
        ${CMAKE_CURRENT_BINARY_DIR}
)

# =========================================================================
# ТЕСТЫ
# =========================================================================
add_executable(test_graph tests/test_graph.cpp)
target_link_libraries(test_graph PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_graph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

add_executable(test_tree tests/test_tree.cpp)
target_link_libraries(test_tree PRIVATE Catch2::Catch2WithMain)
target_include_directories(test_tree PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tools)

enable_testing()
include(Catch)
catch_discover_tests(test_graph)
catch_discover_tests(test_tree)