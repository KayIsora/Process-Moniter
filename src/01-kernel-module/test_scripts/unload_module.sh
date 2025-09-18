## gỡ module khỏi kernel
#!/bin/bash
sudo rmmod proc_monitor
dmesg | tail -n 5
echo "Module unloaded"
sleep 2
echo "Test complete"
