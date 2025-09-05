# Process-Moniter
# Project Overview
Main objective: 
Build a resource monitoring system (CPU, RAM, I/O…) on an embedded Linux board, allowing remote clients to send START/STOP commands and receive statistical data via socket.
Components:
Kernel module (driver)
Procfs interface (/proc/sysmonitor, /proc/sysstats)
User-space daemon reads procfs and sends data
Embedded client runs on another device or PC, communicates via TCP/UDP
