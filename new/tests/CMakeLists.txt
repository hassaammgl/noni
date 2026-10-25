# P0.3 tests — doctest via FetchContent (approved dep, AGENTS.md).
include(FetchContent)

fetchcontent_declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
    SYSTEM)
fetchcontent_makeavailable(doctest)

add_executable(noni_tests
    smoke_test.cpp
    text_test.cpp)

target_link_libraries(noni_tests PRIVATE noni_core noni_ext noni_terminal
                                         noni_syntax noni_lsp noni_app noni_ui
                                         doctest::doctest)

include(CTest)
add_test(NAME smoke COMMAND noni_tests)
