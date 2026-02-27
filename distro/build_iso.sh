#!/bin/bash
set -e

# Configuration
KERNEL_VERSION="vmlinuz-generic" # We will try to grab the host kernel or ask user
# Get the directory where this script is located (distro/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$SCRIPT_DIR"
ROOTFS="$WORK_DIR/rootfs"
BASE_DIR="$(dirname "$SCRIPT_DIR")"  # Project root directory (parent of distro/)

echo "[1/6] Preparing Build Environment..."

# Check Dependencies
for cmd in gcc wget cpio gzip xorriso; do
    if ! command -v $cmd >/dev/null 2>&1; then
        echo "Error: Required command '$cmd' not found. Please install it."
        exit 1
    fi
done
# Only clear if we really need to, otherwise keep for cache
if [ ! -d "$ROOTFS/root/.ollama" ]; then
    rm -rf "$ROOTFS"
    mkdir -p "$ROOTFS"/{bin,dev,proc,sys,etc,usr/bin,sbin,tmp,root,lib,lib64,usr/lib,var/log,var/www}
else
    echo "   (Using cached RootFS... Skipping full wipe)"
    # Clean just binaries to be safe
    rm -f "$ROOTFS/bin/nexos" "$ROOTFS/init"
fi

# Create Web Server Assets
mkdir -p "$ROOTFS/var/www"
if [ ! -f "$ROOTFS/var/www/index.html" ]; then
cat > "$ROOTFS/var/www/index.html" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NexOS Web Server</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            color: white;
        }
        .container {
            text-align: center;
            padding: 40px;
            background: rgba(255,255,255,0.1);
            border-radius: 20px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
            max-width: 600px;
        }
        h1 {
            font-size: 3em;
            margin-bottom: 20px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .status {
            background: #00ff88;
            color: #1a1a2e;
            padding: 10px 30px;
            border-radius: 50px;
            display: inline-block;
            font-weight: bold;
            margin: 20px 0;
        }
        .links { margin-top: 30px; }
        .links a {
            color: white;
            text-decoration: none;
            padding: 12px 25px;
            margin: 5px;
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            display: inline-block;
            transition: all 0.3s;
        }
        .links a:hover {
            background: rgba(255,255,255,0.4);
            transform: translateY(-2px);
        }
        .info {
            margin-top: 30px;
            font-size: 0.9em;
            opacity: 0.8;
        }
        .visitor-info {
            margin-top: 20px;
            padding: 15px;
            background: rgba(0,0,0,0.2);
            border-radius: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🖥️ NexOS Server</h1>
        <div class="status">● ONLINE</div>
        <p>Welcome to NexOS Public Web Server</p>
        
        <div class="links">
            <a href="/reports/">📊 Analytics Dashboard</a>
            <a href="/about.html">ℹ️ About</a>
            <a href="/api/status">📡 API Status</a>
            <a href="/login" style="background: rgba(255,100,100,0.3)">🔒 Admin Login</a>
        </div>
        
        <div style="margin-top: 30px; text-align: left; background: rgba(0,0,0,0.3); padding: 20px; border-radius: 10px;">
            <h3 style="color:#00ff88; margin-bottom:10px;">System Logs</h3>
            <ul style="list-style: none;">
                <li><a href="access.log" style="color:cyan; text-decoration:none;">📄 View Access Logs</a></li>
                <li><a href="ollama.log" style="color:yellow; text-decoration:none;">🤖 View AI Logs</a></li>
                <li><a href="dhcp.log" style="color:white; text-decoration:none;">🌐 View Network Logs</a></li>
            </ul>
        </div>
        
        <div class="visitor-info">
            <p>Your visit has been logged for analytics.</p>
            <p id="timestamp"></p>
        </div>
        
        <div class="info">
            <p>Operating System: NexOS v1.1</p>
        </div>
    </div>
    
    <script>
        document.getElementById('timestamp').textContent = 
            'Access Time: ' + new Date().toLocaleString();
    </script>
</body>
</html>
EOF
fi

# Create Web Server Log Symlinks (Pre-build)
mkdir -p "$ROOTFS/var/log/httpd"
touch "$ROOTFS/var/log/httpd/access.log"
ln -sf "/var/log/httpd/access.log" "$ROOTFS/var/www/access.log"
echo "Ollama AI Log - No active session." > "$ROOTFS/var/www/ollama.log"
echo "DHCP Network Log - Monitoring active." > "$ROOTFS/var/www/dhcp.log"

echo "[2/6] Compiling NexOS (Static)..."
# We use -static so it doesn't need external .so libraries
gcc -static "$BASE_DIR/src/"*.c -o "$ROOTFS/bin/nexos"

echo "[3/6] Setting up BusyBox..."
# Download static busybox if not exists
if [ ! -f "busybox" ]; then
    echo "Downloading BusyBox..."
    wget -q https://busybox.net/downloads/binaries/1.35.0-x86_64-linux-musl/busybox -O busybox
    fi
cp busybox "$ROOTFS/bin/busybox"
chmod +x "$ROOTFS/bin/busybox"

# 3.1 Bundle Terminfo (Critical for htop/TUI)
echo "   -> Bundling Terminfo..."
mkdir -p "$ROOTFS/usr/share/terminfo"
# Copy essential terminals (linux, xterm, vt100)
for term in l/linux x/xterm v/vt100 a/ansi; do
    if [ -d "/usr/share/terminfo/$(dirname $term)" ]; then
        mkdir -p "$ROOTFS/usr/share/terminfo/$(dirname $term)"
        cp -r "/usr/share/terminfo/$term" "$ROOTFS/usr/share/terminfo/$term" 2>/dev/null || true
    elif [ -d "/lib/terminfo/$(dirname $term)" ]; then
        mkdir -p "$ROOTFS/usr/share/terminfo/$(dirname $term)"
        cp -r "/lib/terminfo/$term" "$ROOTFS/usr/share/terminfo/$term" 2>/dev/null || true
    fi
done
# Manually create sh copy (symlinks might be fragile in some initramfs implementations)
rm -f "$ROOTFS/bin/sh"
cp busybox "$ROOTFS/bin/sh"
chmod +x "$ROOTFS/bin/sh"

# Fix clear command (ollama uses it)
ln -sf /bin/busybox "$ROOTFS/bin/clear"

echo "[3.5/6] Bundling AI (Ollama)..."
OLLAMA_BIN=$(which ollama 2>/dev/null || echo "")
if [ -n "$OLLAMA_BIN" ] && [ -f "$OLLAMA_BIN" ]; then
    cp "$OLLAMA_BIN" "$ROOTFS/bin/ollama"
    chmod +x "$ROOTFS/bin/ollama"
    
    # Copy Shared Libraries
    echo "   -> Copying dependencies..."
    mkdir -p "$ROOTFS/lib" "$ROOTFS/lib64"
    ldd "$OLLAMA_BIN" 2>/dev/null | grep "=> /" | awk '{print $3}' | while read lib; do
        if [ -f "$lib" ]; then
            cp -L "$lib" "$ROOTFS/lib/" 2>/dev/null || true
            cp -L "$lib" "$ROOTFS/lib64/" 2>/dev/null || true
        fi
    done
    # Copy ld-linux
    ldd "$OLLAMA_BIN" 2>/dev/null | grep "/lib64/ld-linux" | awk '{print $1}' | while read lib; do
        if [ -f "$lib" ]; then
            cp -L "$lib" "$ROOTFS/lib64/" 2>/dev/null || true
        fi
    done

    # Bundle Custom GGUF Model
    echo "   -> Bundling Custom Model 'nexos-expert' from GGUF..."
    
    # Clean old GGUF files to prevent ISO bloat
    rm -f "$ROOTFS/root/"*.gguf
    
    GGUF_SRC="$BASE_DIR/finetune/qwen2-1.5b-instruct.Q4_K_M.gguf"
    if [ -f "$GGUF_SRC" ]; then
        cp "$GGUF_SRC" "$ROOTFS/root/nexos-expert.gguf"
        echo "      + Copied GGUF: $GGUF_SRC -> nexos-expert.gguf"
    else
        echo "      WARNING: GGUF file not found at $GGUF_SRC"
        echo "      AI features may not work offline!"
    fi
    
    # Create Modelfile
    echo "   -> Creating Modelfile..."
    cat <<EOF > "$ROOTFS/root/Modelfile"
FROM /root/nexos-expert.gguf

# Set parameters to match our strict parsing needs
PARAMETER temperature 0.1
PARAMETER top_p 0.9

# System Prompt
SYSTEM "You are NexOS Assistant, a specialized AI for a custom Linux distro. You ONLY output BusyBox commands in the format: COMMAND: <cmd>\\nEXPLANATION: <text>. You are helpful, safe, and concise."
EOF
    echo "      + Modelfile created at /root/Modelfile"
    
    # Clean old models directory to save space
    rm -rf "$ROOTFS/root/.ollama/models"
    mkdir -p "$ROOTFS/root/.ollama"
    
    echo "   -> Ollama bundling complete!"
else
    echo "   WARNING: Ollama not found on host system!"
    echo "   AI features will not be available."
fi
echo "[3.6/6] Bundling Extras (Ngrok & Htop)..."
# Download Ngrok
if [ ! -f "ngrok.tgz" ]; then
    echo "   -> Downloading Ngrok..."
    wget -q https://bin.equinox.io/c/bNyj1mQVY4c/ngrok-v3-stable-linux-amd64.tgz -O ngrok.tgz
fi
if [ -f "ngrok.tgz" ]; then
    tar -xzf ngrok.tgz
    cp ngrok "$ROOTFS/bin/ngrok"
    chmod +x "$ROOTFS/bin/ngrok"
fi

# Download Static htop (Try multiple sources)
if [ ! -f "htop-static" ]; then
    echo "   -> Downloading htop (Static)..."
    # Try Source 1
    wget -q --no-check-certificate https://github.com/7heo/htop-static/releases/download/v2.2.0/htop-static -O htop-static || \
    # Try Source 2 (Backup)
    wget -q https://github.com/ferment7/htop-static/releases/download/3.2.1/htop-3.2.1-x86_64 -O htop-static || true
    
    if [ -s "htop-static" ]; then
         chmod +x htop-static
    else
         echo "WARNING: Failed to download htop. Skipping."
         rm -f htop-static
    fi
fi
if [ -f "htop-static" ]; then
    cp htop-static "$ROOTFS/bin/htop"
    chmod +x "$ROOTFS/bin/htop"
fi

# Copy SSL Certs (Required for Ngrok/HTTPS)
if [ -d "/etc/ssl/certs" ]; then
    echo "   -> Bundling SSL Certificates..."
    mkdir -p "$ROOTFS/etc/ssl"
    cp -r /etc/ssl/certs "$ROOTFS/etc/ssl/"
fi

echo "[4/6] Installing Init Script..."
cp "$WORK_DIR/init" "$ROOTFS/init"
chmod +x "$ROOTFS/init"

# Install DHCP Script
cp "$WORK_DIR/udhcp.script" "$ROOTFS/etc/udhcp.script"
chmod +x "$ROOTFS/etc/udhcp.script"

# Install NexOS Scripts
echo "   -> Installing utility scripts..."
mkdir -p "$ROOTFS/opt/nexos"
if [ -d "$BASE_DIR/scripts" ]; then
    cp "$BASE_DIR/scripts/"*.sh "$ROOTFS/opt/nexos/" 2>/dev/null || true
    chmod +x "$ROOTFS/opt/nexos/"*.sh 2>/dev/null
fi

# Install Network Drivers (VirtualBox e1000)
echo "   -> Bundling network drivers..."
mkdir -p "$ROOTFS/lib/modules"
DRIVER_PATH=$(find /lib/modules/$(uname -r) -name "e1000.ko*" | head -n 1)
if [ -n "$DRIVER_PATH" ]; then
    echo "      Found: $DRIVER_PATH"
    if [[ "$DRIVER_PATH" == *.zst ]]; then
        zstd -df "$DRIVER_PATH" -o "$ROOTFS/lib/modules/e1000.ko"
    else
        cp "$DRIVER_PATH" "$ROOTFS/lib/modules/e1000.ko"
    fi
else
    echo "WARNING: e1000 driver not found! Network might fail."
fi

echo "[5/6] Creating Initramfs..."
cd "$ROOTFS"
find . -print0 | cpio --null -o --format=newc | gzip > "$WORK_DIR/initramfs.cpio.gz"
cd "$WORK_DIR"

echo "[6/6] Generating ISO..."
mkdir -p iso/boot/isolinux

# Copy Host Kernel (Trying to find a generic one)
if [ -f iso/boot/kernel ]; then
    echo "Using existing kernel at iso/boot/kernel"
elif [ -f /boot/vmlinuz-$(uname -r) ]; then
    cp "/boot/vmlinuz-$(uname -r)" iso/boot/kernel
else
    echo "WARNING: Could not find host kernel at /boot/vmlinuz-$(uname -r)"
    echo "Please copy a linux kernel to distro/iso/boot/kernel manually."
    # Create a dummy file so xorriso doesn't fail, but it won't boot without real kernel
    touch iso/boot/kernel
fi

cp "$WORK_DIR/initramfs.cpio.gz" iso/boot/

# Copy Syslinux Modules (Required for newer versions)
for module in isolinux.bin ldlinux.c32 libutil.c32 libcom32.c32; do
    src=$(find /usr/lib -name "$module" | head -n 1)
    if [ -n "$src" ]; then
        cp "$src" iso/boot/isolinux/
    else
        echo "WARNING: Module $module not found!"
    fi
done

# Create Isolinux Config
cat > iso/boot/isolinux/isolinux.cfg <<EOF
DEFAULT nexos
LABEL nexos
    LINUX /boot/kernel
    INITRD /boot/initramfs.cpio.gz
    APPEND console=ttyS0,115200n8 console=tty0 nomodeset
EOF

# Build ISO
# Requires: xorriso, isolinux (syslinux-utils)
# We use a simple xorriso command that usually works for hybrid ISOs
xorriso -as mkisofs \
    -o nexos.iso \
    -b boot/isolinux/isolinux.bin \
    -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    iso

echo "======================================"
echo "SUCCESS: nexos.iso created in distro/"
echo "======================================"
