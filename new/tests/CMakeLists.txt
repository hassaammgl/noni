# P0.3 tests — doctest via FetchContent (approved dep, AGENTS.md).
include(FetchContent)

fetchcontent_declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
    SYSTEM)
fetchcontent_makeavailable(doctest)

add_executable(noni_tests
    smoke_test.cpp
    text_test.cpp
    buffer_test.cpp
    undo_test.cpp
    buffer_search_test.cpp)

target_link_libraries(noni_tests PRIVATE noni_core noni_ext noni_terminal
                                         noni_syntax noni_lsp noni_app noni_ui
                                         doctest::doctest)

include(CTest)
# P1.8 phase check: tests carry P1-prefixed names so `ctest -R P1` is the
# phase ACCEPT gate; each entry filters one doctest TEST_CASE family.
add_test(NAME P1.smoke COMMAND noni_tests -tc=smoke:*)
add_test(NAME P1.text COMMAND noni_tests -tc=text:*)
add_test(NAME P1.buffer COMMAND noni_tests -tc=buffer:*)
add_test(NAME P1.undo COMMAND noni_tests -tc=undo:*)
add_test(NAME P1.search COMMAND noni_tests -tc=search:*)
