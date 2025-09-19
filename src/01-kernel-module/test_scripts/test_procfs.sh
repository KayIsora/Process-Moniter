#!/bin/bash

echo "=== Test proc_monitor module ==="
echo "Note: Run this script with root or sudo to ensure read/write permissions."
echo

echo "-- Remove old module if any..."
sudo rmmod proc_monitor 2>/dev/null

echo "-- Load new module..."
sudo insmod ../proc_monitor.ko
echo

echo "-- Final dmesg output:"
sudo dmesg | tail -n 5
echo

echo "-- Read initial monitor status:"
sudo cat /proc/sysmonitor
echo

echo "-- Turn off monitor mode (write 0):"
echo "0" | sudo tee /proc/sysmonitor 2>/dev/null
sudo cat /proc/sysmonitor
echo

echo "-- Read system statistics:"
sudo cat /proc/sysstats
echo

echo "-- Read IO data:"
sudo cat /proc/sysio
echo

echo "-- Enable monitor mode (write 1):"
echo "1" | sudo tee /proc/sysmonitor 2>/dev/null
sudo cat /proc/sysmonitor
echo

echo "-- Repeat reading statistics to verify values:"
sudo cat /proc/sysstats
sudo cat /proc/sysio
echo

echo "-- Test completed!"