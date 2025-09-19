# Process Monitor Phase 3 - Communication Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread

# Directories
CLIENT_DIR = client
DAEMON_DIR = daemon
TEST_DIR = test
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
CLIENT_SOURCES = $(CLIENT_DIR)/client_main.c $(CLIENT_DIR)/command_parser.c $(CLIENT_DIR)/socket_client.c
DAEMON_SOURCES = $(DAEMON_DIR)/daemon_main.c $(DAEMON_DIR)/socket_server.c $(DAEMON_DIR)/command_handler.c
TEST_SOURCES = $(TEST_DIR)/test_socket_basic.c

# Object files (đặt trong build/obj/)
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.c,$(OBJ_DIR)/$(CLIENT_DIR)/%.o,$(CLIENT_SOURCES))
DAEMON_OBJS = $(patsubst $(DAEMON_DIR)/%.c,$(OBJ_DIR)/$(DAEMON_DIR)/%.o,$(DAEMON_SOURCES))
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/$(TEST_DIR)/%.o,$(TEST_SOURCES))

# Executable targets (đặt trực tiếp trong build/)
CLIENT_EXE = $(BUILD_DIR)/client
DAEMON_EXE = $(BUILD_DIR)/daemon
TEST_EXE = $(BUILD_DIR)/test_socket_basic

# Default target
all: directories $(CLIENT_EXE) $(DAEMON_EXE) $(TEST_EXE)
	@echo "All targets built successfully"

# Create build directories
directories:
	@mkdir -p $(OBJ_DIR)/$(CLIENT_DIR)
	@mkdir -p $(OBJ_DIR)/$(DAEMON_DIR)
	@mkdir -p $(OBJ_DIR)/$(TEST_DIR)

# Client executable
$(CLIENT_EXE): $(CLIENT_OBJS)
	@echo "Linking client executable..."
	$(CC) $(CLIENT_OBJS) $(LDFLAGS) -o $@
	@echo "Client built: $@"

# Daemon executable
$(DAEMON_EXE): $(DAEMON_OBJS)
	@echo "Linking daemon executable..."
	$(CC) $(DAEMON_OBJS) $(LDFLAGS) -o $@
	@echo "Daemon built: $@"

# Test executable
$(TEST_EXE): $(TEST_OBJS)
	@echo "Linking test executable..."
	$(CC) $(TEST_OBJS) $(LDFLAGS) -o $@
	@echo "Test built: $@"

# Client object files
$(OBJ_DIR)/$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Daemon object files
$(OBJ_DIR)/$(DAEMON_DIR)/%.o: $(DAEMON_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Test object files
$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Install executables to project root
install: all
	@echo "Installing executables..."
	@if [ -f $(CLIENT_EXE) ]; then cp $(CLIENT_EXE) ./client_exe && echo "Installed: ./client_exe"; fi
	@if [ -f $(DAEMON_EXE) ]; then cp $(DAEMON_EXE) ./daemon_exe && echo "Installed: ./daemon_exe"; fi
	@if [ -f $(TEST_EXE) ]; then cp $(TEST_EXE) ./test_socket_basic && echo "Installed: ./test_socket_basic"; fi
	@echo "Installation complete"

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@rm -f client_exe daemon_exe test_socket_basic
	@echo "Clean complete"

# Test targets
test: $(TEST_EXE)
	@echo "Running basic socket test..."
	$(TEST_EXE)

test-full: install
	@echo "Running full test suite..."
	@echo "Note: Start daemon first with: ./daemon_exe -f &"

# Development helpers
debug: CFLAGS += -g -DDEBUG
debug: all

release: CFLAGS += -O2 -DNDEBUG
release: all

# Individual targets
client: directories $(CLIENT_EXE)
daemon: directories $(DAEMON_EXE)
test_socket_basic: directories $(TEST_EXE)

# Show current structure
show-structure:
	@echo "Build structure:"
	@echo "  Object files: $(OBJ_DIR)/{client,daemon,test}/"
	@echo "  Executables: $(BUILD_DIR)/{client,daemon,test_socket_basic}"
	@echo "  Install to: {client_exe,daemon_exe,test_socket_basic}"

# Help
help:
	@echo "Usage:"
	@echo "  make all           - Build all executables"
	@echo "  make client        - Build client only"
	@echo "  make daemon        - Build daemon only"
	@echo "  make test_socket_basic - Build test only"
	@echo "  make install       - Install executables to project root"
	@echo "  make clean         - Remove build files and installed executables"
	@echo "  make test          - Run basic socket test"
	@echo "  make show-structure - Show build directory structure"
	@echo "  make help          - Show this help"

# Phony targets
.PHONY: all directories clean install test test-full debug release help client daemon test_socket_basic show-structure

# Dependencies
$(OBJ_DIR)/$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/client.h
$(OBJ_DIR)/$(DAEMON_DIR)/%.o: $(DAEMON_DIR)/daemon.h
