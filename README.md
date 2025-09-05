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
b. Daemon User-Space (VTS – Block 3501 & 3502)
- Monitors entries in procfs and manages monitoring threads
- Handles socket communication with remote clients
- Uses FIFO for internal data exchange (Recv data ↔ Get Coords)
- Creates threads for each “room” (Room1, Room2 … RoomN) to run multiple independent monitoring sessions
- Logs all monitoring events and results
c. CLI Client (Remote)
- Runs on another device or PC
- Connects to the daemon via TCP/UDP socket
- Sends control commands (START, STOP, EDIT, CREATE, DELETE, SHOW)
- Displays monitoring results
d. LOC GEN (Block 3502)
- Generates location/context information for monitoring data
- Acts as an intermediary between CLI commands and room/thread management