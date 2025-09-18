#!/bin/bash

# Test script for Process Monitor Phase 3 - Communication
# Tests client-daemon communication through socket

set -e  # Exit on any error

# Colors for output
RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
NC='\\033[0m' # No Color

# Configuration
CLIENT_BIN="../client/client"
DAEMON_BIN="../daemon/daemon"
DAEMON_PID=""
TEST_LOG="test_results.log"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "PASS")
            echo -e "${GREEN}[PASS]${NC} $message"
            echo "[PASS] $message" >> $TEST_LOG
            ;;
        "FAIL")
            echo -e "${RED}[FAIL]${NC} $message"
            echo "[FAIL] $message" >> $TEST_LOG
            ;;
        "INFO")
            echo -e "${YELLOW}[INFO]${NC} $message"
            echo "[INFO] $message" >> $TEST_LOG
            ;;
    esac
}

# Function to start daemon
start_daemon() {
    print_status "INFO" "Starting daemon in background..."
    $DAEMON_BIN -f &
    DAEMON_PID=$!
    sleep 2  # Give daemon time to start
    
    # Check if daemon is running
    if kill -0 $DAEMON_PID 2>/dev/null; then
        print_status "PASS" "Daemon started successfully (PID: $DAEMON_PID)"
        return 0
    else
        print_status "FAIL" "Failed to start daemon"
        return 1
    fi
}

# Function to stop daemon
stop_daemon() {
    if [ ! -z "$DAEMON_PID" ]; then
        print_status "INFO" "Stopping daemon (PID: $DAEMON_PID)..."
        kill $DAEMON_PID 2>/dev/null || true
        wait $DAEMON_PID 2>/dev/null || true
        DAEMON_PID=""
        print_status "INFO" "Daemon stopped"
    fi
}

# Function to test command
test_command() {
    local test_name="$1"
    local command="$2"
    local expected_result="$3"  # "success" or "failure"
    
    print_status "INFO" "Testing: $test_name"
    print_status "INFO" "Command: $command"
    
    # Run command and capture output
    local output
    local exit_code
    output=$($command 2>&1) || exit_code=$?
    
    if [ "$expected_result" = "success" ]; then
        if [ $exit_code -eq 0 ]; then
            print_status "PASS" "$test_name"
            echo "Output: $output"
        else
            print_status "FAIL" "$test_name (exit code: $exit_code)"
            echo "Output: $output"
            return 1
        fi
    else
        if [ $exit_code -ne 0 ]; then
            print_status "PASS" "$test_name (expected failure)"
            echo "Output: $output"
        else
            print_status "FAIL" "$test_name (should have failed)"
            echo "Output: $output"
            return 1
        fi
    fi
    
    echo "---"
    return 0
}

# Function to test socket connectivity
test_socket_connectivity() {
    print_status "INFO" "Testing socket connectivity..."
    
    # Use netstat to check if port 8080 is listening
    if netstat -tlnp 2>/dev/null | grep -q ":8080.*LISTEN"; then
        print_status "PASS" "Daemon is listening on port 8080"
    else
        print_status "FAIL" "Daemon is not listening on port 8080"
        return 1
    fi
    
    # Test basic TCP connection using telnet-like approach
    if command -v nc >/dev/null 2>&1; then
        print_status "INFO" "Testing TCP connection with netcat..."
        echo "SHOW" | nc -w 5 127.0.0.1 8080 >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            print_status "PASS" "TCP connection test successful"
        else
            print_status "FAIL" "TCP connection test failed"
            return 1
        fi
    else
        print_status "INFO" "Netcat not available, skipping direct TCP test"
    fi
    
    return 0
}

# Cleanup function
cleanup() {
    print_status "INFO" "Cleaning up..."
    stop_daemon
    exit 0
}

# Set up signal handlers
trap cleanup EXIT INT TERM

# Main test execution
main() {
    print_status "INFO" "Starting Process Monitor Phase 3 Communication Tests"
    echo "Test started at $(date)" > $TEST_LOG
    
    # Check if binaries exist
    if [ ! -x "$CLIENT_BIN" ]; then
        print_status "FAIL" "Client binary not found or not executable: $CLIENT_BIN"
        exit 1
    fi
    
    if [ ! -x "$DAEMON_BIN" ]; then
        print_status "FAIL" "Daemon binary not found or not executable: $DAEMON_BIN"
        exit 1
    fi
    
    # Start daemon
    if ! start_daemon; then
        print_status "FAIL" "Cannot proceed without daemon"
        exit 1
    fi
    
    # Test socket connectivity
    test_socket_connectivity
    
    # Phase 3A: Client CLI Tests
    print_status "INFO" "=== Phase 3A: Client CLI Tests ==="
    
    test_command "Help command" "$CLIENT_BIN help" "success"
    test_command "Create cpu-room" "$CLIENT_BIN create cpu-room 1000" "success"
    test_command "Start cpu-room" "$CLIENT_BIN start cpu-room" "success"
    test_command "Show cpu-room" "$CLIENT_BIN show cpu-room" "success"
    test_command "Stop cpu-room" "$CLIENT_BIN stop cpu-room" "success"
    
    # Phase 3B: Error handling tests
    print_status "INFO" "=== Phase 3B: Error Handling Tests ==="
    
    test_command "Invalid command" "$CLIENT_BIN invalid-command" "failure"
    test_command "Create without size" "$CLIENT_BIN create test-room" "failure"
    test_command "Start non-existent room" "$CLIENT_BIN start non-existent" "success"  # Should return error from daemon
    test_command "Create duplicate room" "$CLIENT_BIN create cpu-room 500" "success"  # Should return error from daemon
    
    # Phase 3C: Multiple operations test
    print_status "INFO" "=== Phase 3C: Multiple Operations Tests ==="
    
    test_command "Create memory-room" "$CLIENT_BIN create memory-room 2000" "success"
    test_command "Create disk-room" "$CLIENT_BIN create disk-room 1500" "success"
    test_command "List all rooms" "$CLIENT_BIN show" "success"
    test_command "Start memory-room" "$CLIENT_BIN start memory-room" "success"
    test_command "Start disk-room" "$CLIENT_BIN start disk-room" "success"
    test_command "Show memory-room status" "$CLIENT_BIN show memory-room" "success"
    test_command "Show disk-room status" "$CLIENT_BIN show disk-room" "success"
    
    # Test summary
    print_status "INFO" "=== Test Summary ==="
    local total_tests=$(grep -c "\\[PASS\\]\\|\\[FAIL\\]" $TEST_LOG | head -1)
    local passed_tests=$(grep -c "\\[PASS\\]" $TEST_LOG)
    local failed_tests=$(grep -c "\\[FAIL\\]" $TEST_LOG)
    
    print_status "INFO" "Total tests: $total_tests"
    print_status "INFO" "Passed: $passed_tests"
    print_status "INFO" "Failed: $failed_tests"
    
    if [ $failed_tests -eq 0 ]; then
        print_status "PASS" "All tests completed successfully!"
        exit 0
    else
        print_status "FAIL" "Some tests failed. Check $TEST_LOG for details."
        exit 1
    fi
}

# Run main function
main "$@"
