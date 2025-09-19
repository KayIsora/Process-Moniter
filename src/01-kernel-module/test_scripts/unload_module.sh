#!/bin/bash

echo "=== Remove proc_monitor module ==="
echo

echo "-- Check current module..."
lsmod | grep proc_monitor
echo

echo "-- Start removing module..."
sudo rmmod proc_monitor
echo

echo "-- Check module list again:"
lsmod | grep proc_monitor
echo

echo "-- See last log from kernel:"
sudo dmesg | tail -n 5
echo

echo "-- Module removal done!"