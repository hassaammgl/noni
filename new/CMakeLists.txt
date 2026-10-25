cmake_minimum_required(VERSION 3.28)

project(noni
    VERSION 0.1.0
    DESCRIPTION "Noni — vim-style ncursesw terminal editor"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(NONI_SANITIZERS "Enable ASan+UBSan in Debug builds" ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NONI_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

# --- layers (ARCHITECTURE §1): core <- ext/terminal/syntax/lsp <- app <- ui <- noni
# External deps are scoped to the layer that owns them, so a wrong include
# (e.g. <ncurses.h> in core/) fails to compile.

find_package(PkgConfig REQUIRED)
pkg_check_modules(ncursesw IMPORTED_TARGET ncursesw)
pkg_check_modules(tree_sitter QUIET IMPORTED_TARGET tree-sitter)

set(NONI_COMMON
    -fno-exceptions
    -fvisibility=hidden
)
add_compile_options(${NONI_COMMON})

add_library(noni_core STATIC src/core/version.cpp src/core/text.cpp
                             src/core/buffer.cpp src/core/undo.cpp
                             src/core/buffer_search.cpp
                             src/core/editor_state.cpp)
target_include_directories(noni_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(noni_ext STATIC src/ext/ext.cpp)
target_link_libraries(noni_ext PUBLIC noni_core)

add_library(noni_terminal STATIC src/terminal/terminal.cpp)
target_link_libraries(noni_terminal PUBLIC noni_core)

add_library(noni_syntax STATIC src/syntax/syntax.cpp)
target_link_libraries(noni_syntax PUBLIC noni_core)
if(TARGET PkgConfig::tree_sitter)
    target_link_libraries(noni_syntax PUBLIC PkgConfig::tree_sitter)
endif()

add_library(noni_lsp STATIC src/lsp/lsp.cpp)
target_link_libraries(noni_lsp PUBLIC noni_core)

add_library(noni_app STATIC src/app/app.cpp src/app/reducer.cpp)
target_link_libraries(noni_app PUBLIC noni_core noni_ext noni_terminal noni_syntax noni_lsp)

add_library(noni_ui STATIC src/ui/ui.cpp src/ui/screen.cpp src/ui/input.cpp
                            src/ui/view.cpp)
target_link_libraries(noni_ui PUBLIC noni_app)
if(TARGET PkgConfig::ncursesw)
    # ncurses headers/flags propagate only ui-ward: app/ and below never see them.
    target_link_libraries(noni_ui PUBLIC PkgConfig::ncursesw)
    target_compile_definitions(noni_ui PUBLIC NCURSES_WIDECHAR)
endif()

add_executable(noni src/main.cpp)
target_link_libraries(noni PRIVATE noni_ui)

enable_testing()

# P0.2 layer lint (tests/layer_lint.sh): ncurses only in ui/, tree-sitter only
# in syntax/. Compile-time include scoping is impossible for system-wide
# headers on this toolchain, so the gate is this lint + the link-direction
# rules above. ncursesw targets stay out of core/app/exe links by construction
# (exe gets them transitively through ui only).
add_test(NAME layer_lint
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/layer_lint.sh)
set_tests_properties(layer_lint PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tests)
