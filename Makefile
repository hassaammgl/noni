# Root Makefile — drives the NEW editor rewrite in new/ (pure vim-style C++23).
# The legacy code under old/ and the legacy root build/ dir are never touched.
#
#   make          configure + build (new/build)
#   make run      build + run ./new/build/noni (ARGS="-foo" to pass args)
#   make test     build + ctest
#   make clean    delete new/build
#   make help

NEW_DIR   := new
BUILD_DIR := $(NEW_DIR)/build
BIN       := $(BUILD_DIR)/noni
TESTS     := $(BUILD_DIR)/tests/noni_tests

ARGS ?=

.PHONY: all configure build run test clean help

all: build

configure:
	@cmake -S $(NEW_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

build: configure
	@cmake --build $(BUILD_DIR) -j4

run: build
	@./$(BIN) $(ARGS)

test: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

sanitize: run_tests_asan

run_tests_asan:
	@ASAN_OPTIONS=detect_leaks=1 $(TESTS)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "make        = configure + build"
	@echo "make run    = build + run $(BIN) [ARGS=...]"
	@echo "make test   = build + ctest"
	@echo "make sanitize = run test binary under ASan+UBSan"
	@echo "make clean  = rm -rf $(BUILD_DIR)"
