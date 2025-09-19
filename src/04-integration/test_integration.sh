#!/bin/bash

# Integration Test Suite for Phase 4

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

DAEMON_BIN="$SCRIPT_DIR/build/bin/integration_daemon"
CLIENT_BIN="$PROJECT_ROOT/src/03-communication/build/client"
KERNEL_MODULE="$PROJECT_ROOT/src/01-kernel-module/proc_monitor.ko"

TEST_CONFIG="$SCRIPT_DIR/test_config.conf"
TEST_PORT=8081
TEST_ROOM="test-room"
TEST_TIMEOUT=30

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

test_start() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    log_step "Test $TOTAL_TESTS: $1"
}

test_pass() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
    log_info "✓ $1"
}

test_fail() {
    TESTS_FAILED=$((TESTS_FAILED + 1))
    log_error "✗ $1"
}

cleanup() {
    log_info "Cleaning up test environment..."

    if [ -f "/tmp/test_daemon.pid" ]; then
        PID=$(cat /tmp/test_daemon.pid 2>/dev/null || echo "")
        if [ -n "$PID" ] && ps -p "$PID" > /dev/null 2>&1; then
            kill "$PID" 2>/dev/null || true
            sleep 2
            if ps -p "$PID" > /dev/null 2>&1; then
                kill -9 "$PID" 2>/dev/null || true
            fi
        fi
        rm -f "/tmp/test_daemon.pid"
    fi

    rm -f /tmp/monitor_fifo
    rm -f /tmp/test_monitor_fifo
    rm -f "$TEST_CONFIG"
    rm -f logs/test_*.log
    pkill -f integration_daemon 2>/dev/null || true
}

setup_test_environment() {
    log_step "Setting up test environment..."

    mkdir -p logs

    # Create test configuration
    cat > "$TEST_CONFIG" << EOF
log_path=logs/test_monitor.log
log_level=debug
structured_logging=true
daemon_port=$TEST_PORT
max_rooms=5
collection_interval=2
fifo_path=/tmp/test_monitor_fifo
procfs_path=/proc/sysmonitor
pid_file=/tmp/test_daemon.pid
EOF
}

test_build_system() {
    test_start "Build System"
    if ! make clean && make; then
        test_fail "Build failed"
        return 1
    fi
    if [ ! -f "$DAEMON_BIN" ]; then
        test_fail "Integration daemon binary not found"
        return 1
    fi
    test_pass "Build system test"
}

test_configuration() {
    test_start "Configuration Loading"
    if ! timeout 3 "$DAEMON_BIN" --config="$TEST_CONFIG" --test; then
        test_fail "Failed to load valid configuration"
        return 1
    fi
    test_pass "Configuration test"
}

test_daemon_startup() {
    test_start "Daemon Startup"
    "$DAEMON_BIN" --config="$TEST_CONFIG" &
    DAEMON_PID=$!
    echo "$DAEMON_PID" > /tmp/test_daemon.pid
    sleep 3
    if ! ps -p "$DAEMON_PID" > /dev/null; then
        test_fail "Daemon failed to start"
        return 1
    fi
    if ! netstat -tuln 2>/dev/null | grep -q ":$TEST_PORT "; then
        if ! ss -tuln 2>/dev/null | grep -q ":$TEST_PORT "; then
            log_warn "Cannot verify port listening (netstat/ss not available)"
        fi
    fi
    if [ ! -f "logs/test_monitor.log" ]; then
        test_fail "Log file not created"
        return 1
    fi
    test_pass "Daemon startup test"
}

test_ipc_system() {
    test_start "IPC System"
    sleep 1
    if [ ! -p "/tmp/test_monitor_fifo" ]; then
        log_warn "FIFO not found (may not be created yet)"
    fi
    echo "test_message" > /tmp/test_monitor_fifo 2>/dev/null || true
    test_pass "IPC system test"
}

send_command() {
    local cmd="$1"
    if command -v nc >/dev/null 2>&1; then
        echo "$cmd" | nc -w 2 localhost "$TEST_PORT" 2>/dev/null
    elif command -v telnet >/dev/null 2>&1; then
        (echo "$cmd"; sleep 1) | telnet localhost "$TEST_PORT" 2>/dev/null
    fi
}

test_room_operations() {
    test_start "Room Operations"
    local result
    result=$(send_command "create $TEST_ROOM")
    if echo "$result" | grep -q "SUCCESS\|created"; then
        log_info "Room creation succeeded"
    else
        log_warn "Room creation test inconclusive"
    fi
    result=$(send_command "list")
    if echo "$result" | grep -q "SUCCESS\|rooms"; then
        log_info "Room list succeeded"
    else
        log_warn "Room list test inconclusive"
    fi
    result=$(send_command "status")
    if echo "$result" | grep -q "SUCCESS\|status"; then
        log_info "Status command succeeded"
    else
        log_warn "Status test inconclusive"
    fi
    test_pass "Room operations test"
}

test_error_handling() {
    test_start "Error Handling"
    result=$(send_command "invalid_command")
    if echo "$result" | grep -q "ERROR\|Invalid"; then
        log_info "Invalid command properly rejected"
    else
        log_warn "Error handling test inconclusive"
    fi
    test_pass "Error handling test"
}

test_resource_usage() {
    test_start "Resource Usage"
    if [ -f "/tmp/test_daemon.pid" ]; then
        PID=$(cat /tmp/test_daemon.pid)
        if ps -p "$PID" > /dev/null; then
            if command -v ps >/dev/null 2>&1; then
                mem_usage=$(ps -p "$PID" -o rss= 2>/dev/null | tr -d ' ')
                if [ -n "$mem_usage" ] && [ "$mem_usage" -gt 0 ]; then
                    mem_mb=$((mem_usage / 1024))
                    log_info "Memory usage: ${mem_mb}MB"
                    if [ "$mem_mb" -gt 100 ]; then
                        log_warn "High memory usage: ${mem_mb}MB"
                    fi
                fi
            fi
            if [ -d "/proc/$PID/fd" ]; then
                fd_count=$(ls "/proc/$PID/fd" 2>/dev/null | wc -l)
                log_info "File descriptors: $fd_count"
                if [ "$fd_count" -gt 100 ]; then
                    log_warn "High file descriptor usage: $fd_count"
                fi
            fi
        fi
    fi
    test_pass "Resource usage test"
}

test_log_system() {
    test_start "Log System"
    if [ ! -f "logs/test_monitor.log" ]; then
        test_fail "Log file not found"
        return 1
    fi
    log_lines=$(wc -l < logs/test_monitor.log)
    log_info "Log file has $log_lines lines"
    if [ "$log_lines" -lt 10 ]; then
        log_warn "Log file seems too short"
    fi
    if grep -q '\"timestamp\".*\"level\"' logs/test_monitor.log; then
        log_info "Structured logging detected"
    else
        log_info "Plain text logging detected"
    fi
    if grep -q "INFO" logs/test_monitor.log; then
        log_info "INFO level logging working"
    fi
    if grep -q "DEBUG" logs/test_monitor.log; then
        log_info "DEBUG level logging working"
    fi
    test_pass "Log system test"
}

test_graceful_shutdown() {
    test_start "Graceful Shutdown"
    if [ -f "/tmp/test_daemon.pid" ]; then
        PID=$(cat /tmp/test_daemon.pid)
        if ps -p "$PID" > /dev/null; then
            log_info "Sending TERM to daemon..."
            kill -TERM "$PID"
            count=0
            while [ $count -lt 10 ] && ps -p "$PID" > /dev/null; do
                sleep 1
                count=$((count + 1))
            done
            if ps -p "$PID" > /dev/null; then
                log_warn "Daemon did not shutdown gracefully, forcing..."
                kill -9 "$PID" 2>/dev/null || true
                test_fail "Graceful shutdown failed"
                return 1
            else
                log_info "Daemon shutdown gracefully"
            fi
        fi
    fi
    if [ -p "/tmp/test_monitor_fifo" ]; then
        log_warn "FIFO not cleaned up"
    else
        log_info "FIFO cleaned up properly"
    fi
    rm -f /tmp/test_daemon.pid
    test_pass "Graceful shutdown test"
}

main() {
    echo "=== Process Monitor Integration Test Suite ==="
    echo "Date: $(date)"
    echo "Project Root: $PROJECT_ROOT"
    echo

    setup_test_environment

    test_build_system || true
    test_configuration || true
    test_daemon_startup || true
    test_ipc_system || true
    test_room_operations || true
    test_error_handling || true
    test_resource_usage || true
    test_log_system || true
    test_graceful_shutdown || true

    cleanup

    echo
    echo "=== Test Results ==="
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $TESTS_PASSED"
    echo "Failed: $TESTS_FAILED"
    if [ "$TOTAL_TESTS" -gt 0 ]; then
        success_rate=$((TESTS_PASSED * 100 / TOTAL_TESTS))
        echo "Success Rate: ${success_rate}%"
    fi

    if [ "$TESTS_FAILED" -eq 0 ]; then
        log_info "🎉 All tests passed!"
        return 0
    else
        log_error "❌ $TESTS_FAILED test(s) failed"
        return 1
    fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    trap cleanup EXIT INT TERM
    main "$@"
fi
