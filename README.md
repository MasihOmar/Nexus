<p align="center">
  <img src="https://img.shields.io/badge/language-C-blue?style=for-the-badge&logo=c&logoColor=white" />
  <img src="https://img.shields.io/badge/platform-Linux-orange?style=for-the-badge&logo=linux&logoColor=white" />
  <img src="https://img.shields.io/badge/shell-Bash-green?style=for-the-badge&logo=gnu-bash&logoColor=white" />
  <img src="https://img.shields.io/badge/AI-Ollama-purple?style=for-the-badge" />
  <img src="https://img.shields.io/badge/license-MIT-brightgreen?style=for-the-badge" />
</p>

# Nexus OS

A lightweight, Linux-based operating system built from scratch in C. Nexus OS features a custom terminal UI, an integrated AI assistant, process management with isolated memory, a round-robin scheduler, inter-process communication, a virtual filesystem, and a built-in web server — all packaged into a bootable ISO.

---

## ✨ Features

| Category                  | Description                                                                              |
| ------------------------- | ---------------------------------------------------------------------------------------- |
| **Terminal UI**           | Custom ncurses-style TUI with login screen, dashboard, and interactive menus             |
| **Expert Shell**          | Full command shell with history, tab completion, and built-in commands                   |
| **AI Assistant**          | Ollama-powered natural language command helper — describe what you want, get the command |
| **Process Manager**       | Create, monitor, block, unblock, and terminate processes with state tracking             |
| **Memory Isolation**      | Per-process isolated memory spaces with read/write protection and memory dumps           |
| **Round-Robin Scheduler** | Preemptive task scheduler with configurable time slices and priority levels              |
| **IPC (Mailbox)**         | Inter-process communication via a message-passing mailbox system                         |
| **Virtual Filesystem**    | Simple flat filesystem with create, read, list, and delete operations                    |
| **Web Server**            | Lightweight built-in HTTP server for serving static content                              |
| **System Monitor**        | Real-time hardware info, memory usage, disk stats, and process overview                  |
| **Log Analyzer**          | Security-focused log analysis and network traffic monitoring scripts                     |
| **Bootable ISO**          | Full ISO builder — boots in QEMU or VirtualBox out of the box                            |

---

## 🏗️ Architecture

```
nexus/
├── src/                    # Core C source
│   ├── main.c              # Entry point & signal handling
│   ├── tui.c               # Terminal UI rendering & login
│   ├── shell.c             # Interactive shell & command parsing
│   ├── ai.c                # AI assistant (Ollama integration)
│   ├── process.c           # Process lifecycle management
│   ├── memory.c            # Isolated per-process memory
│   ├── scheduler.c         # Round-robin scheduler
│   ├── ipc.c               # Inter-process communication
│   ├── fs.c                # Virtual filesystem
│   ├── hardware.c          # System info & hardware abstraction
│   ├── utils.c             # Shared utilities
│   └── globals.h           # Types, constants & prototypes
├── scripts/
│   ├── doctor.sh           # System diagnostics
│   ├── webserver.sh        # HTTP server
│   ├── log_analyzer.sh     # Log analysis
│   └── analyze_traffic.sh  # Network traffic monitor
├── distro/
│   ├── build_iso.sh        # ISO builder
│   ├── init                # Init script (PID 1)
│   └── udhcp.script        # DHCP config
└── Makefile                # Build system
```

---

## � Quick Start

### Prerequisites

- Linux (Ubuntu/Debian recommended)
- GCC with static linking support
- `genisoimage` or `xorriso` for ISO builds
- [Ollama](https://ollama.ai) _(optional, for AI features)_

```bash
# Install build dependencies
sudo apt update && sudo apt install build-essential genisoimage wget
```

### Build

```bash
# Compile the binary
make

# Build a bootable ISO (downloads kernel, busybox, and optionally Ollama)
make iso
```

### Run

```bash
# Boot in QEMU
qemu-system-x86_64 -cdrom distro/nexos.iso -m 2G -enable-kvm

# Or mount in VirtualBox:
# Create VM → Linux → Other Linux (64-bit) → 2GB+ RAM → Mount ISO → Boot
```

### Clean

```bash
make clean       # Remove build artifacts
make distclean   # Full clean including ISO
```

---

## 🤖 AI Assistant

Nexus OS ships with an optional AI assistant powered by [Ollama](https://ollama.ai). In the expert shell, simply describe what you want in natural language:

```
nexos> ai "show me disk usage by directory"
🤖 Suggested command: du -sh /* 2>/dev/null | sort -rh | head -20
Execute? [y/n]:
```

Commands are validated against a whitelist and checked for dangerous patterns before execution.

---

## 🔧 Technical Details

- **Language**: C (compiled with GCC, statically linked for portability)
- **Process model**: Up to 10 concurrent simulated processes with 4KB isolated memory each
- **Scheduler**: Round-robin with configurable time quantum and priority levels (0–9)
- **IPC**: Mailbox-based message passing (10 messages per process, 256 bytes each)
- **Filesystem**: Flat virtual FS supporting up to 50 files (1KB each), persisted to disk
- **Default RAM**: 64MB simulated RAM with configurable limits

---

## � License

MIT License — see [LICENSE](LICENSE) for details.
