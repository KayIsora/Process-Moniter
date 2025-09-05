# Process-Monitor
# I. Overview
The system is designed to monitor hardware/software resources (CPU, memory, I/O, network) on an embedded Linux board. A remote CLI client can send commands such as START, STOP, EDIT, SHOW, DELETE... via a socket, and the system will respond with real-time monitoring data.
# II. Objectives
- Collect and record system resource statistics at a configurable interval
- Provide an interface to enable/disable monitoring through procfs (proc file system)
- Support remote control (CLI client) to start/stop monitoring tasks
- Allow multiple monitoring "rooms" to run in parallel for different resources
- Store logs for debugging and later analysis
# III. System Components
a. Kernel Driver (My Driver)
- Collects data and system statistics (CPU usage, memory usage, disk I/O, network throughput)
- Exposes data and control via procfs (/proc/sysmonitor, /proc/sysstats, /proc/sysio)
- Supports enabling/disabling monitoring through write commands (1 = enable, 0 = disable)