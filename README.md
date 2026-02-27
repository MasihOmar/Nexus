# NexOS / MiniOS Project

A lightweight Linux-based mini operating system with TUI, AI assistant integration, web server, and system monitoring tools.

## 📁 Project Structure

```
upload_ready/
├── src/           # C source files
│   ├── main.c     # Entry point
│   ├── tui.c      # Terminal UI
│   ├── shell.c    # Expert shell
│   ├── ai.c       # AI command helper (Ollama)
│   ├── hardware.c # Hardware info
│   ├── memory.c   # Memory management
│   ├── process.c  # Process management
│   ├── scheduler.c # Task scheduler
│   ├── ipc.c      # Inter-process communication
│   ├── fs.c       # Filesystem operations
│   ├── utils.c    # Utility functions
│   └── globals.h  # Global definitions
├── scripts/       # Bash scripts
│   ├── doctor.sh       # System diagnostics
│   ├── webserver.sh    # Built-in web server
│   ├── log_analyzer.sh # Log analysis
│   └── analyze_traffic.sh # Traffic monitoring
├── distro/        # ISO build files
│   ├── build_iso.sh    # Main ISO builder
│   ├── init            # Init script for boot
│   └── udhcp.script    # DHCP configuration
└── Makefile       # Build automation
```

## 🛠️ Build Requirements

- **Linux system** (Ubuntu/Debian recommended)
- **GCC compiler** with static linking support
- **glibc-static** or **musl-libc** for static builds
- **xorriso** or **genisoimage** for ISO creation
- **Optional**: Ollama for AI features

### Install dependencies (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install build-essential genisoimage wget
```

## 🚀 Building the Project

### 1. Build the binary only:
```bash
make
```

### 2. Build the complete ISO:
```bash
make iso
```
This will download required components (kernel, busybox, ollama) and create `distro/nexos.iso`

### 3. Clean build artifacts:
```bash
make clean      # Remove build files
make distclean  # Remove everything including ISO
```

## 🖥️ Running

### In QEMU:
```bash
qemu-system-x86_64 -cdrom distro/nexos.iso -m 2G -enable-kvm
```

### In VirtualBox:
1. Create new VM → Linux → Other Linux (64-bit)
2. RAM: 2GB+, CPU: 2+ cores
3. Mount `nexos.iso` as optical drive
4. Boot from optical

## ⚙️ Features

- **TUI Interface**: ncurses-like terminal interface
- **Expert Shell**: Command shell with AI assistance
- **AI Helper**: Ollama-powered command suggestions
- **Web Server**: Built-in HTTP server
- **System Monitor**: Hardware/memory/process monitoring
- **Log Analysis**: Security and traffic analysis tools

## 📝 Notes

- AI features require Ollama and the GGUF model (downloaded during ISO build)
- The ISO is ~900MB-1GB with full AI support
- Without AI, the base system is much smaller

## 📜 License

Educational project - MIT License
