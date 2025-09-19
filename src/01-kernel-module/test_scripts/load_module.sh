## nạp kernel module vào hệ thống
#!/bin/bash
sudo rmmod proc_monitor 2>/dev/null  # Gỡ module cũ nếu có
sudo insmod ../proc_monitor.ko       # Nạp module
sudo dmesg | tail -n 5               # Đọc log kernel với quyền root
lsmod | grep proc_monitor            # Kiểm tra module đã nạp
echo "Module loaded"
sleep 2
