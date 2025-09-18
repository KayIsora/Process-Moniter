## nạp kernel module vào hệ thống
#!/bin/bash
sudo insmod ../proc_monitor.ko
dmesg | tail -n 5
echo "Module loaded"
sleep 2
