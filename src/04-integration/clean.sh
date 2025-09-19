#!/bin/bash

# Clean script for Process Monitor Integration

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Cleaning Process Monitor Integration..."

# Stop any running daemons
pkill -f integration_daemon 2>/dev/null || true

# Remove build artifacts
rm -rf "$SCRIPT_DIR/build"
rm -f "$SCRIPT_DIR"/*.o
rm -f "$SCRIPT_DIR"/*.log
rm -f "$SCRIPT_DIR"/core.*

# Remove test files
rm -f "$SCRIPT_DIR"/test_*.conf
rm -f /tmp/test_daemon.pid
rm -f /tmp/test_monitor_fifo
rm -f /tmp/monitor_fifo

# Remove common build artifacts
rm -f "$SCRIPT_DIR/../commom"/*.o

echo "Clean completed"
