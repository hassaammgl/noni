CXX = g++

CXXFLAGS_BASE = -Wall -Wextra -std=c++23 -g -MMD -MP -Iincludes
CXXFLAGS = $(CXXFLAGS_BASE) -O2
LDFLAGS = -lncursesw -pthread -lutil -ltree-sitter -ldl

TARGET = build/bin/app

SRC := $(wildcard *.cpp) $(shell find src -type f -name '*.cpp')
OBJ := $(patsubst %.cpp,build/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug:
	$(MAKE) all CXXFLAGS="$(CXXFLAGS_BASE) -O0"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(OBJ:.o=.d)

.PHONY: all run clean debug
