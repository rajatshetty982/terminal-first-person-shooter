# a common build template for all my small projects
CXX      := g++
BUILD_DIR := build
SRC_DIR   := .

TARGET := $(BUILD_DIR)/my_program

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Normalise build mode to lowercase
BUILD ?= debug
BUILD_LOWER := $(shell echo $(BUILD) | tr '[:upper:]' '[:lower:]')

# Conditional flags based on build mode
ifeq ($(BUILD_LOWER),release)
    CXXFLAGS := -O3 -Wall -Wextra -std=c++20 -MMD -MP
else ifeq ($(BUILD_LOWER),debug)
    CXXFLAGS := -ggdb3 -O0 -Wall -Wextra -std=c++20 -MMD -MP
else
    $(error Invalid build mode "$(BUILD)". Use "debug" or "release")
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)

.PHONY: all run clean
