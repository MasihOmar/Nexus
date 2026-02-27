#!/bin/sh
# =============================================================================
# NexOS Log Analyzer - Comprehensive Web Log Analysis Tool
# =============================================================================
# Author: NexOS Project
# Description: Modular log analysis with functions, loops, and report generation
# =============================================================================

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------
LOG_DIR="/var/log/httpd"
ACCESS_LOG="${LOG_DIR}/access.log"
REPORT_DIR="/var/www/reports"
HTML_REPORT="${REPORT_DIR}/index.html"
DATE=$(date +"%Y-%m-%d")
TIME=$(date +"%H:%M:%S")

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# -----------------------------------------------------------------------------
# HELPER FUNCTIONS
# -----------------------------------------------------------------------------

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

ensure_directories() {
    # Create necessary directories if they don't exist
    for dir in "$LOG_DIR" "$REPORT_DIR"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            log_info "Created directory: $dir"
        fi
    done
}

# -----------------------------------------------------------------------------
# ANALYSIS FUNCTIONS
# -----------------------------------------------------------------------------

# Function: Get top N visitors by IP address
get_top_ips() {
    local log_file="$1"
    local count="${2:-10}"
    
    if [ ! -f "$log_file" ]; then
        log_error "Log file not found: $log_file"
        return 1
    fi
    
    awk '{print $1}' "$log_file" | \
        sort | \
        uniq -c | \
        sort -rn | \
        head -n "$count"
}

# Function: Get top N most visited pages
get_top_pages() {
    local log_file="$1"
    local count="${2:-10}"
    
    awk '{print $7}' "$log_file" | \
        grep -v "^-$" | \
        sort | \
        uniq -c | \
        sort -rn | \
        head -n "$count"
}

# Function: Get HTTP status code distribution
get_status_codes() {
    local log_file="$1"
    
    awk '{print $9}' "$log_file" | \
        grep -E "^[0-9]{3}$" | \
        sort | \
        uniq -c | \
        sort -rn
}

# Function: Get error requests (4xx and 5xx)
get_error_requests() {
    local log_file="$1"
    local count="${2:-20}"
    
    awk '$9 ~ /^[45][0-9][0-9]$/ {print $9, $7, $1}' "$log_file" | \
        sort | \
        uniq -c | \
        sort -rn | \
        head -n "$count"
}

# Function: Get requests per hour
get_hourly_traffic() {
    local log_file="$1"
    
    awk '{
        # Extract hour from timestamp [DD/Mon/YYYY:HH:MM:SS
        match($4, /:[0-9]{2}:/, arr)
        if (RSTART > 0) {
            hour = substr($4, RSTART+1, 2)
            hours[hour]++
        }
    }
    END {
        for (h in hours) {
            printf "%s:00 - %d requests\n", h, hours[h]
        }
    }' "$log_file" | sort
}

# Function: Get bandwidth usage (approximate)
get_bandwidth() {
    local log_file="$1"
    
    awk '{
        total += $10
    }
    END {
        if (total > 1073741824) {
            printf "%.2f GB\n", total/1073741824
        } else if (total > 1048576) {
            printf "%.2f MB\n", total/1048576
        } else if (total > 1024) {
            printf "%.2f KB\n", total/1024
        } else {
            printf "%d bytes\n", total
        }
    }' "$log_file"
}

# Function: Get unique visitors count
get_unique_visitors() {
    local log_file="$1"
    
    awk '{print $1}' "$log_file" | sort -u | wc -l
}

# Function: Get total requests count
get_total_requests() {
    local log_file="$1"
    
    wc -l < "$log_file"
}

# Function: Get user agents summary
get_user_agents() {
    local log_file="$1"
    local count="${2:-5}"
    
    awk -F'"' '{print $6}' "$log_file" | \
        sort | \
        uniq -c | \
        sort -rn | \
        head -n "$count"
}

# -----------------------------------------------------------------------------
# REPORT GENERATION
# -----------------------------------------------------------------------------

generate_text_report() {
    local log_file="$1"
    local output_file="${REPORT_DIR}/report_${DATE}.txt"
    
    log_info "Generating text report..."
    
    {
        echo "=============================================="
        echo "   NEXOS WEB LOG ANALYSIS REPORT"
        echo "=============================================="
        echo "Generated: ${DATE} ${TIME}"
        echo "Log File: ${log_file}"
        echo ""
        
        echo "--- SUMMARY ---"
        echo "Total Requests: $(get_total_requests "$log_file")"
        echo "Unique Visitors: $(get_unique_visitors "$log_file")"
        echo "Bandwidth Used: $(get_bandwidth "$log_file")"
        echo ""
        
        echo "--- TOP 10 VISITOR IPs ---"
        get_top_ips "$log_file" 10
        echo ""
        
        echo "--- TOP 10 VISITED PAGES ---"
        get_top_pages "$log_file" 10
        echo ""
        
        echo "--- HTTP STATUS CODES ---"
        get_status_codes "$log_file"
        echo ""
        
        echo "--- ERROR REQUESTS (4xx/5xx) ---"
        get_error_requests "$log_file" 10
        echo ""
        
        echo "--- HOURLY TRAFFIC ---"
        get_hourly_traffic "$log_file"
        echo ""
        
        echo "--- TOP USER AGENTS ---"
        get_user_agents "$log_file" 5
        echo ""
        
        echo "=============================================="
        echo "Report generated by NexOS Log Analyzer"
        echo "=============================================="
    } > "$output_file"
    
    log_info "Text report saved to: $output_file"
}

generate_html_report() {
    local log_file="$1"
    
    log_info "Generating HTML report..."
    
    local total_requests=$(get_total_requests "$log_file")
    local unique_visitors=$(get_unique_visitors "$log_file")
    local bandwidth=$(get_bandwidth "$log_file")
    
    cat > "$HTML_REPORT" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="refresh" content="30">
    <title>NexOS Log Analysis Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 {
            text-align: center;
            color: #00ff88;
            margin-bottom: 30px;
            text-shadow: 0 0 10px rgba(0,255,136,0.5);
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .stat-card {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            padding: 25px;
            text-align: center;
            border: 1px solid rgba(255,255,255,0.1);
            backdrop-filter: blur(10px);
        }
        .stat-card h3 { color: #888; font-size: 0.9em; margin-bottom: 10px; }
        .stat-card .value { font-size: 2.5em; color: #00ff88; font-weight: bold; }
        .section {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .section h2 {
            color: #00aaff;
            margin-bottom: 15px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
            padding-bottom: 10px;
        }
        table { width: 100%; border-collapse: collapse; }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        th { color: #00ff88; }
        tr:hover { background: rgba(255,255,255,0.05); }
        .footer {
            text-align: center;
            color: #666;
            margin-top: 30px;
            font-size: 0.9em;
        }
        .live-indicator {
            display: inline-block;
            width: 10px;
            height: 10px;
            background: #00ff88;
            border-radius: 50%;
            margin-right: 10px;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1><span class="live-indicator"></span>NexOS Log Analysis Dashboard</h1>
        
        <div class="stats-grid">
            <div class="stat-card">
                <h3>Total Requests</h3>
                <div class="value">${total_requests}</div>
            </div>
            <div class="stat-card">
                <h3>Unique Visitors</h3>
                <div class="value">${unique_visitors}</div>
            </div>
            <div class="stat-card">
                <h3>Bandwidth Used</h3>
                <div class="value">${bandwidth}</div>
            </div>
            <div class="stat-card">
                <h3>Last Updated</h3>
                <div class="value" style="font-size: 1.2em;">${DATE} ${TIME}</div>
            </div>
        </div>
        
        <div class="section">
            <h2>Top 10 Visitor IPs</h2>
            <table>
                <tr><th>Count</th><th>IP Address</th></tr>
EOF
    
    # Add top IPs to HTML
    get_top_ips "$log_file" 10 | while read count ip; do
        echo "                <tr><td>${count}</td><td>${ip}</td></tr>" >> "$HTML_REPORT"
    done
    
    cat >> "$HTML_REPORT" << EOF
            </table>
        </div>
        
        <div class="section">
            <h2>Top 10 Visited Pages</h2>
            <table>
                <tr><th>Count</th><th>Page</th></tr>
EOF
    
    # Add top pages to HTML
    get_top_pages "$log_file" 10 | while read count page; do
        echo "                <tr><td>${count}</td><td>${page}</td></tr>" >> "$HTML_REPORT"
    done
    
    cat >> "$HTML_REPORT" << EOF
            </table>
        </div>
        
        <div class="section">
            <h2>HTTP Status Codes</h2>
            <table>
                <tr><th>Count</th><th>Status Code</th></tr>
EOF
    
    # Add status codes to HTML
    get_status_codes "$log_file" | while read count code; do
        echo "                <tr><td>${count}</td><td>${code}</td></tr>" >> "$HTML_REPORT"
    done
    
    cat >> "$HTML_REPORT" << EOF
            </table>
        </div>
        
        <div class="footer">
            <p>Auto-refreshes every 30 seconds | Powered by NexOS Log Analyzer</p>
        </div>
    </div>
</body>
</html>
EOF
    
    log_info "HTML report saved to: $HTML_REPORT"
}

# -----------------------------------------------------------------------------
# CRON SETUP
# -----------------------------------------------------------------------------

setup_cron() {
    local interval="${1:-hourly}"
    local script_path=$(readlink -f "$0")
    local cron_entry=""
    
    case "$interval" in
        hourly)
            cron_entry="0 * * * * $script_path --auto"
            ;;
        daily)
            cron_entry="0 0 * * * $script_path --auto"
            ;;
        weekly)
            cron_entry="0 0 * * 0 $script_path --auto"
            ;;
        *)
            log_error "Unknown interval: $interval (use: hourly, daily, weekly)"
            return 1
            ;;
    esac
    
    # Check if cron entry already exists
    if crontab -l 2>/dev/null | grep -q "$script_path"; then
        log_warn "Cron job already exists for this script"
    else
        (crontab -l 2>/dev/null; echo "$cron_entry") | crontab -
        log_info "Cron job added: $cron_entry"
    fi
}

# -----------------------------------------------------------------------------
# MAIN EXECUTION
# -----------------------------------------------------------------------------

show_help() {
    echo "NexOS Log Analyzer - Comprehensive Web Log Analysis Tool"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --log <file>     Specify log file (default: $ACCESS_LOG)"
    echo "  --text           Generate text report only"
    echo "  --html           Generate HTML report only"
    echo "  --all            Generate both reports (default)"
    echo "  --auto           Auto mode for cron jobs"
    echo "  --setup-cron <interval>   Setup cron job (hourly/daily/weekly)"
    echo "  --top-ips <n>    Show top N IPs"
    echo "  --top-pages <n>  Show top N pages"
    echo "  --status         Show status code distribution"
    echo "  --errors         Show error requests"
    echo "  --live           Start live monitoring"
    echo "  --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --all                    # Generate full reports"
    echo "  $0 --log /path/to/access.log --html"
    echo "  $0 --setup-cron hourly      # Run every hour"
    echo "  $0 --top-ips 20             # Show top 20 IPs"
}

live_monitor() {
    local log_file="$1"
    
    log_info "Starting live monitoring (Ctrl+C to stop)..."
    
    while true; do
        clear
        echo -e "${BLUE}========================================${NC}"
        echo -e "${GREEN}   NEXOS LIVE LOG MONITOR${NC}"
        echo -e "${BLUE}========================================${NC}"
        echo -e "Time: $(date)"
        echo -e "Log: $log_file"
        echo ""
        echo -e "${YELLOW}--- Current Stats ---${NC}"
        echo "Total Requests: $(get_total_requests "$log_file")"
        echo "Unique Visitors: $(get_unique_visitors "$log_file")"
        echo ""
        echo -e "${YELLOW}--- Last 10 Requests ---${NC}"
        tail -10 "$log_file" 2>/dev/null || echo "No log data"
        echo ""
        echo -e "${YELLOW}--- Top 5 IPs ---${NC}"
        get_top_ips "$log_file" 5
        sleep 5
    done
}

main() {
    ensure_directories
    
    local log_file="$ACCESS_LOG"
    local action="all"
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --log)
                log_file="$2"
                shift 2
                ;;
            --text)
                action="text"
                shift
                ;;
            --html)
                action="html"
                shift
                ;;
            --all)
                action="all"
                shift
                ;;
            --auto)
                action="auto"
                shift
                ;;
            --setup-cron)
                setup_cron "$2"
                exit 0
                ;;
            --top-ips)
                get_top_ips "$log_file" "${2:-10}"
                exit 0
                ;;
            --top-pages)
                get_top_pages "$log_file" "${2:-10}"
                exit 0
                ;;
            --status)
                get_status_codes "$log_file"
                exit 0
                ;;
            --errors)
                get_error_requests "$log_file"
                exit 0
                ;;
            --live)
                live_monitor "$log_file"
                exit 0
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Check if log file exists
    if [ ! -f "$log_file" ]; then
        log_warn "Log file not found: $log_file"
        log_info "Creating sample log file for demo..."
        mkdir -p "$(dirname "$log_file")"
        echo '192.168.1.100 - - [18/Dec/2025:10:00:00 +0300] "GET /index.html HTTP/1.1" 200 1024 "-" "Mozilla/5.0"' > "$log_file"
        echo '192.168.1.101 - - [18/Dec/2025:10:01:00 +0300] "GET /about.html HTTP/1.1" 200 512 "-" "Chrome/90.0"' >> "$log_file"
        echo '192.168.1.100 - - [18/Dec/2025:10:02:00 +0300] "GET /admin.php HTTP/1.1" 404 128 "-" "Mozilla/5.0"' >> "$log_file"
    fi
    
    # Execute action
    case "$action" in
        text)
            generate_text_report "$log_file"
            ;;
        html)
            generate_html_report "$log_file"
            ;;
        all|auto)
            generate_text_report "$log_file"
            generate_html_report "$log_file"
            ;;
    esac
    
    log_info "Analysis complete!"
}

# Run main function with all arguments
main "$@"
