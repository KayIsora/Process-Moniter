# Makefile for Process Monitor Integration Daemon
# Phase 4: Integration

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -g -O2 -D_GNU_SOURCE
INCLUDES = -I../commom -I../03-communication/daemon -I../02-loc-gen -I.
LDFLAGS = -pthread -lm

# Debug/Release flags
DEBUG_FLAGS = -DDEBUG -g3 -O0
RELEASE_FLAGS = -DNDEBUG -O2

# Directories
SRC_DIR = .
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
SOURCES = main_daemon.c ipc_handler.c logger.c
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)

# Common source files
COMMON_DIR = ../commom
COMMON_SOURCES = $(COMMON_DIR)/protocol.c $(COMMON_DIR)/utils.c
COMMON_OBJECTS = $(COMMON_SOURCES:%.c=$(OBJ_DIR)/%.o)

# Target binary
TARGET = $(BIN_DIR)/integration_daemon

# Default target
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p ../../logs
	@mkdir -p ../../config

# Build main target
$(TARGET): $(OBJECTS) $(COMMON_OBJECTS)
	@echo "Linking integration daemon..."
	$(CC) $(OBJECTS) $(COMMON_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build completed: $@"

# Compile integration source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile common source files  
$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.c
	@echo "Compiling common $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean directories $(TARGET)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: clean directories $(TARGET)

# Install target
install: $(TARGET)
	@echo "Installing integration daemon..."
	sudo mkdir -p /usr/local/bin
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@if [ -f "../../config/monitor.conf" ]; then \
		sudo mkdir -p /etc/monitor; \
		sudo cp ../../config/monitor.conf /etc/monitor/; \
	fi

# Test target
test: $(TARGET)
	@echo "Running integration tests..."
	@if [ -f "test_integration.sh" ]; then \
		chmod +x test_integration.sh && ./test_integration.sh; \
	else \
		echo "Test script not found"; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f *.log core

.PHONY: all debug release install test clean directories
