# Root Makefile — drives the NEW editor rewrite in new/ (pure vim-style C++23).
# The legacy code under old/ and the legacy root build/ dir are never touched.
#
#   make          configure + build (new/build)
#   make run      build + run ./new/build/noni (ARGS="-foo" to pass args)
#   make test     build + ctest
#   make sanitize run the test binary under ASan+UBSan
#   make clean    delete new/build
#   make help

NEW_DIR   := new
BUILD_DIR := $(NEW_DIR)/build
BIN       := $(BUILD_DIR)/noni
TESTS     := $(BUILD_DIR)/tests/noni_tests

ARGS ?=

.PHONY: all configure build run test sanitize clean help

all: build

# quiet-then-loud: cmake chatter is hidden while it succeeds; on failure the
# full output is dumped so errors/warnings are never swallowed.
configure:
	@out=$$(cmake -S $(NEW_DIR) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug \
	        -Wno-deprecated 2>&1); status=$$?; \
	if [ $$status -ne 0 ]; then printf '%s\n' "$$out" >&2; exit $$status; fi

build: configure
	@out=$$(cmake --build $(BUILD_DIR) -j4 2>&1); status=$$?; \
	printf '%s\n' "$$out" | grep -E ':[[:space:]]*(error|warning):' >&2 || true; \
	if [ $$status -ne 0 ]; then printf '%s\n' "$$out" >&2; exit $$status; fi; \
	echo "build: ok"

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
	@echo "make        = configure + build (quiet; errors surface fully)"
	@echo "make run    = build + run $(BIN) [ARGS=...]"
	@echo "make test   = build + ctest"
	@echo "make sanitize = run test binary under ASan+UBSan"
	@echo "make clean  = rm -rf $(BUILD_DIR)"
