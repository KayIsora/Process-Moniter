## tương tác với các file procfs do module tạo ra, ví dụ đọc/ghi với /proc/sysmonitor, /proc/sysstats, /proc/sysio
#!/bin/bash
echo "Testing procfs interface..."
cat /proc/sysmonitor
echo "0" | sudo tee /proc/sysmonitor
cat /proc/sysmonitor
cat /proc/sysstats
cat /proc/sysio
echo "1" | sudo tee /proc/sysmonitor
cat /proc/sysmonitor
cat /proc/sysstats
cat /proc/sysio
