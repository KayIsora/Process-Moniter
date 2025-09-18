# Process Monitor Phase 3 - Communication Makefile
# Builds client and daemon executables with proper linking

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread

# Directories
CLIENT_DIR = client
DAEMON_DIR = daemon
TEST_DIR = test
BUILD_DIR = build

# Client files
CLIENT_SOURCES = $(CLIENT_DIR)/client_main.c $(CLIENT_DIR)/command_parser.c $(CLIENT_DIR)/socket_client.c
CLIENT_OBJECTS = $(CLIENT_SOURCES:%.c=$(BUILD_DIR)/%.o)
CLIENT_TARGET = $(BUILD_DIR)/client

# Daemon files  
DAEMON_SOURCES = $(DAEMON_DIR)/daemon_main.c $(DAEMON_DIR)/socket_server.c $(DAEMON_DIR)/command_handler.c
DAEMON_OBJECTS = $(DAEMON_SOURCES:%.c=$(BUILD_DIR)/%.o)
DAEMON_TARGET = $(BUILD_DIR)/daemon

# Test files
TEST_SOURCES = $(TEST_DIR)/test_socket_basic.c
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
TEST_TARGET = $(BUILD_DIR)/test_socket_basic

# Default target
all: directories $(CLIENT_TARGET) $(DAEMON_TARGET) $(TEST_TARGET)

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR)/$(CLIENT_DIR)
	@mkdir -p $(BUILD_DIR)/$(DAEMON_DIR)
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)

# Client executable
$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	@echo "Linking client executable..."
	$(CC) $(CLIENT_OBJECTS) $(LDFLAGS) -o $@
	@echo "Client built successfully: $@"

# Daemon executable
$(DAEMON_TARGET): $(DAEMON_OBJECTS)
	@echo "Linking daemon executable..."
	$(CC) $(DAEMON_OBJECTS) $(LDFLAGS) -o $@
	@echo "Daemon built successfully: $@"

# Test executable
$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "Linking test executable..."
	$(CC) $(TEST_OBJECTS) $(LDFLAGS) -o $@
	@echo "Test built successfully: $@"

# Client object files
$(BUILD_DIR)/$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c $(CLIENT_DIR)/client.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Daemon object files
$(BUILD_DIR)/$(DAEMON_DIR)/%.o: $(DAEMON_DIR)/%.c $(DAEMON_DIR)/daemon.h
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Test object files
$(BUILD_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Install targets (copy to project root for easy access)
install: all
	@echo "Installing executables..."
	cp $(CLIENT_TARGET) ./client
	cp $(DAEMON_TARGET) ./daemon
	cp $(TEST_TARGET) ./test_socket_basic
	@echo "Installation complete"

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	rm -f client daemon test_socket_basic
	@echo "Clean complete"

# Test targets
test: all
	@echo "Running basic socket tests..."
	$(TEST_TARGET)

test-full: all install
	@echo "Running full test suite..."
	cd $(TEST_DIR) && chmod +x test_commands.sh && ./test_commands.sh

# Development helpers
debug: CFLAGS += -g -DDEBUG
debug: all

release: CFLAGS += -O2 -DNDEBUG
release: all

# Check for required tools
check-deps:
	@command -v $(CC) >/dev/null 2>&1 || { echo "gcc not found"; exit 1; }
	@command -v netstat >/dev/null 2>&1 || echo "Warning: netstat not found (optional for testing)"
	@command -v nc >/dev/null 2>&1 || echo "Warning: netcat not found (optional for testing)"
	@echo "Dependencies check passed"

# Help target
help:
	@echo "Process Monitor Phase 3 - Communication Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all         - Build client, daemon and test executables"
	@echo "  client      - Build only client executable"
	@echo "  daemon      - Build only daemon executable"
	@echo "  test        - Build test executable and run basic tests"
	@echo "  test-full   - Run comprehensive test suite"
	@echo "  install     - Install executables to project root"
	@echo "  clean       - Remove all build files"
	@echo "  debug       - Build with debug symbols"
	@echo "  release     - Build optimized release version"
	@echo "  check-deps  - Check for required build dependencies"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build everything"
	@echo "  make client             # Build only client"
	@echo "  make install            # Build and install"
	@echo "  make test-full          # Full test suite"
	@echo "  make clean && make      # Clean rebuild"

# Individual targets
client: directories $(CLIENT_TARGET)
daemon: directories $(DAEMON_TARGET)

# Phony targets
.PHONY: all directories clean install test test-full debug release check-deps help client daemon

# Make sure intermediate files are not deleted
.SECONDARY: $(CLIENT_OBJECTS) $(DAEMON_OBJECTS) $(TEST_OBJECTS)
