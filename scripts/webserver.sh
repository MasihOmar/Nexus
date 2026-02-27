#!/bin/sh
# =============================================================================
# NexOS Public Web Server Setup
# =============================================================================
# Description: Configure web server for public access with logging
# =============================================================================

# Configuration
WEB_ROOT="${WEB_ROOT:-/var/www}"
LOG_DIR="${LOG_DIR:-/var/log/httpd}"
ACCESS_LOG="${LOG_DIR}/access.log"
CONFIG_FILE="${CONFIG_FILE:-/etc/httpd.conf}"
PORT="${PORT:-${2:-80}}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# -----------------------------------------------------------------------------
# SETUP FUNCTIONS
# -----------------------------------------------------------------------------

setup_directories() {
    log_info "Setting up directories..."
    
    mkdir -p "$WEB_ROOT"/{css,js,images,reports}
    mkdir -p "$LOG_DIR"
    
    # Ensure log file exists
    touch "$ACCESS_LOG"
    chmod 644 "$ACCESS_LOG"
    
    # Symlink logs to Web Root so they are accessible
    log_info "Exposing logs to web root..."
    ln -sf "$ACCESS_LOG" "$WEB_ROOT/access.log"
    
    # Create valid dummy logs for others if missing (to prevent 404)
    [ ! -f "$WEB_ROOT/ollama.log" ] && echo "Ollama AI Log - No active session." > "$WEB_ROOT/ollama.log"
    [ ! -f "$WEB_ROOT/dhcp.log" ] && echo "DHCP Network Log - Monitoring active." > "$WEB_ROOT/dhcp.log"
}

create_httpd_config() {
    log_info "Creating httpd configuration..."
    
    cat > "$CONFIG_FILE" << EOF
# NexOS httpd Configuration
H:${WEB_ROOT}
I:index.html
E404:404.html

# Enable logging
# httpd will log to stdout, we redirect to file
EOF
}

create_website() {
    log_info "Creating website files..."
    
    # Main index page
    cat > "${WEB_ROOT}/index.html" << 'EOF'
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

    # About page
    cat > "${WEB_ROOT}/about.html" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>About - NexOS</title>
    <style>
        body { font-family: sans-serif; background: #1a1a2e; color: #eee; padding: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #00ff88; }
        a { color: #00aaff; }
    </style>
</head>
<body>
    <div class="container">
        <h1>About NexOS</h1>
        <p>NexOS is an educational operating system project featuring:</p>
        <ul>
            <li>Custom Virtual File System (SFS)</li>
            <li>AI-powered Command Assistant</li>
            <li>Web Log Analysis Tools</li>
            <li>Network Management</li>
        </ul>
        <p><a href="/">← Back to Home</a></p>
    </div>
</body>
</html>
EOF

    # 404 Error page
    cat > "${WEB_ROOT}/404.html" << EOF
<!DOCTYPE html>
<html lang="en">
<head><title>404</title></head>
<body><h1>404 Not Found</h1></body>
</html>
EOF

    # Dashboard
    mkdir -p "${WEB_ROOT}/reports"
    cat > "${WEB_ROOT}/reports/index.html" << EOF
<!DOCTYPE html>
<html>
<head><title>Analytics Dashboard</title></head>
<body style="background:#1a1a2e;color:white;font-family:sans-serif;padding:40px;">
    <h1>📊 Analytics Dashboard</h1>
    <p>Traffic analysis loaded from server logs.</p>
    <div style="background:#333;padding:20px;border-radius:10px;">
        <h3>Run 'analyze' in terminal to update this report.</h3>
    </div>
    <br>
    <a href="/" style="color:#00ff88">Back to Home</a>
</body>
</html>
EOF

    # API Status
    mkdir -p "${WEB_ROOT}/api"
    echo '{"status": "online", "system": "NexOS", "uptime": "check terminal"}' > "${WEB_ROOT}/api/status"
}

get_network_info() {
    log_info "Detecting network configuration..."
    
    # Get all IP addresses
    echo ""
    echo -e "${BLUE}=== Network Information ===${NC}"
    
    # Local IP
    # Local IP detection (Robust)
    local_ip=$(ip addr show | grep 'inet ' | grep -v '127.0.0.1' | awk '{print $2}' | cut -d/ -f1 | head -n 1)
    if [ -z "$local_ip" ]; then
        local_ip=$(ifconfig | grep -A 1 'eth0' | grep 'inet' | awk '{print $2}' | cut -d: -f2)
    fi
    # Fallback
    if [ -z "$local_ip" ]; then
        local_ip=$(hostname -i 2>/dev/null)
    fi

    if [ -n "$local_ip" ]; then
        echo -e "Local IP:     ${GREEN}${local_ip}${NC}"
    fi
    
    # Try to get public IP (requires internet)
    public_ip=$(wget -qO- http://ipinfo.io/ip 2>/dev/null || curl -s http://ipinfo.io/ip 2>/dev/null)
    if [ -n "$public_ip" ]; then
        echo -e "Public IP:    ${GREEN}${public_ip}${NC}"
        echo ""
        echo -e "${YELLOW}Access URLs:${NC}"
        echo -e "  Local:  http://${local_ip}:${PORT}/"
        echo -e "  Public: http://${public_ip}:${PORT}/"
    else
        echo -e "Public IP:    ${RED}Unable to detect (no internet?)${NC}"
        echo ""
        echo -e "${YELLOW}Access URL:${NC}"
        echo -e "  Local:  http://${local_ip}:${PORT}/"
    fi
    
    echo ""
    echo -e "${YELLOW}Note:${NC} For external access, ensure:"
    echo "  1. Port ${PORT} is open in firewall"
    echo "  2. Router port forwarding is configured (if behind NAT)"
    echo ""
}

start_server() {
    log_info "Starting web server on port ${PORT}..."
    
    # Kill existing httpd if running
    killall httpd 2>/dev/null
    pkill -f "busybox httpd" 2>/dev/null
    # Force kill anything on the port (Robustness)
    fuser -k -n tcp ${PORT} 2>/dev/null
    
    # Check for busybox presence
    HTTPD_CMD="httpd"
    if ! command -v httpd >/dev/null 2>&1; then
        if [ -x "./busybox" ]; then
             HTTPD_CMD="./busybox httpd"
        elif command -v busybox >/dev/null 2>&1; then
             HTTPD_CMD="busybox httpd"
        else
             log_error "BusyBox httpd not found! Please install busybox."
             return 1
        fi
    fi
    
    # Create dummy config for Basic Auth (Protect /login)
    # Generate MD5 hash for password "123"
    PASS_HASH=$($HTTPD_CMD -m "123")
    # Protect ENTIRE site (/) with Basic Auth
    echo "/:admin:$PASS_HASH" > /tmp/httpd.conf
    
    echo "Executing: $HTTPD_CMD -p 0.0.0.0:${PORT} -h \"$WEB_ROOT\" -c /tmp/httpd.conf -f -vv"
    $HTTPD_CMD -p 0.0.0.0:${PORT} -h "$WEB_ROOT" -c /tmp/httpd.conf -f -vv >> "$ACCESS_LOG" 2>&1 &
    SERVER_PID=$!
    
    sleep 1
    
    if kill -0 $SERVER_PID 2>/dev/null; then
        log_info "Web server started successfully! (PID: $SERVER_PID)"
        echo ""
        echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║     WEB SERVER IS NOW SECURED          ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
        
        # Self-Check (Must use credentials now)
        (sleep 2; echo "Running Self-Check..."; wget -O /dev/null --http-user=admin --http-password=123 http://127.0.0.1:${PORT}/ 2>>/tmp/wget_error.log) &
        
        # AUTO-ANALYZE: Generate valid report immediately so dashboard isn't empty
        (sleep 3; /opt/nexos/analyze_traffic.sh run >/dev/null 2>&1) &
        
        # Show Access URLs at the end
        get_network_info
    else
        log_error "Failed to start web server (Process died)"
        return 1
    fi
}

stop_server() {
    log_info "Stopping web server..."
    killall httpd 2>/dev/null
    log_info "Server stopped."
}

show_status() {
    echo -e "${BLUE}=== Server Status ===${NC}"
    
    if pgrep -x httpd > /dev/null; then
        echo -e "Status: ${GREEN}RUNNING${NC}"
        echo "PID: $(pgrep -x httpd)"
    else
        echo -e "Status: ${RED}STOPPED${NC}"
    fi
    
    echo ""
    echo "Log file: $ACCESS_LOG"
    if [ -f "$ACCESS_LOG" ]; then
        echo "Log size: $(du -h "$ACCESS_LOG" | cut -f1)"
        echo "Total requests: $(wc -l < "$ACCESS_LOG")"
    fi
}

tail_logs() {
    log_info "Tailing access logs (Ctrl+C to stop)..."
    tail -f "$ACCESS_LOG"
}

# -----------------------------------------------------------------------------
# MAIN
# -----------------------------------------------------------------------------

show_help() {
    echo "NexOS Public Web Server"
    echo ""
    echo "Usage: $0 [command] [port]"
    echo ""
    echo "Commands:"
    echo "  start [port]   Start web server (default port: 80)"
    echo "  stop           Stop web server"
    echo "  restart        Restart web server"
    echo "  status         Show server status"
    echo "  logs           Tail access logs"
    echo "  setup          Setup directories and files only"
    echo "  network        Show network information"
    echo "  help           Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 start 8080   # Start on port 8080"
    echo "  $0 logs         # Watch access logs"
}

main() {
    local command="${1:-start}"
    
    case "$command" in
        start)
            setup_directories
            create_httpd_config
            create_website
            get_network_info
            start_server
            ;;
        stop)
            stop_server
            ;;
        restart)
            stop_server
            sleep 1
            setup_directories
            start_server
            ;;
        status)
            show_status
            get_network_info
            ;;
        logs)
            tail_logs
            ;;
        setup)
            setup_directories
            create_httpd_config
            create_website
            log_info "Setup complete. Run '$0 start' to start server."
            ;;
        network)
            get_network_info
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
