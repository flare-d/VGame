# Makefile for the physics platformer (SFML 3).
# Sources live in src/, tunable constants in params/, build artifacts in build/.

CXX ?= g++

SRC_DIR   := src
BUILD_DIR := build
TARGET    := game

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Project root is on the include path so headers can be referenced as
# "params/WorldParams.h" from anywhere in src/.
CPPFLAGS += -I. -I$(SRC_DIR)
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

# SFML 3 flags resolved through pkg-config (falls back to plain -l flags).
SFML_CFLAGS := $(shell pkg-config --cflags sfml-graphics sfml-window sfml-system 2>/dev/null)
SFML_LIBS   := $(shell pkg-config --libs   sfml-graphics sfml-window sfml-system 2>/dev/null)
ifeq ($(strip $(SFML_LIBS)),)
SFML_LIBS := -lsfml-graphics -lsfml-window -lsfml-system
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SFML_LIBS)

# -MMD -MP emits a .d file per object with exact header dependencies,
# so editing any header (including the ones in params/) rebuilds only what matters.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SFML_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
