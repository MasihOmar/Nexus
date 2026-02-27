#!/bin/busybox sh

# analyze_traffic.sh - Modular AI Log Analyzer
# Usage: ./analyze_traffic.sh [command] [log_file]

LOG_DIR="${LOG_DIR:-/var/log/httpd}"
LOG_FILE="${LOG_DIR}/access.log"
SUMMARY_FILE="/tmp/traffic_summary.txt"

# -----------------------------------------------------------------------------
# FUNCTIONS
# -----------------------------------------------------------------------------

generate_summary() {
    local log=$1
    echo "Generating Summary from $log..."
    
    # Header
    echo "--- WEBSERVER TRAFFIC SUMMARY ---" > "$SUMMARY_FILE"
    echo "Date: $(date)" >> "$SUMMARY_FILE"
    
    # Modular extraction using a loop for different metrics
    for metric in "IPs" "Paths" "Errors"; do
        echo "" >> "$SUMMARY_FILE"
        echo "--- TOP $metric ---" >> "$SUMMARY_FILE"
        
        if grep -q "url:" "$log"; then
            # BusyBox Verbose Format (IP:PORT: url:Path)
            if [ "$metric" = "IPs" ]; then
                grep "url:" "$log" | awk '{print $1}' | sed 's/:$//' | sort | uniq -c | sort -nr | head -n 5 >> "$SUMMARY_FILE"
            elif [ "$metric" = "Paths" ]; then
                grep "url:" "$log" | awk -F'url:' '{print $2}' | sort | uniq -c | sort -nr | head -n 5 >> "$SUMMARY_FILE"
            elif [ "$metric" = "Errors" ]; then
                grep "response:" "$log" | grep -vE "response:200|response:30[0-9]" | awk -F'response:' '{print $2}' | sort | uniq -c >> "$SUMMARY_FILE" || echo "No errors found." >> "$SUMMARY_FILE"
            fi
        else
            # Standard CLF Format
            if [ "$metric" = "IPs" ]; then
                awk '{print $1}' "$log" | sort | uniq -c | sort -nr | head -n 5 >> "$SUMMARY_FILE"
            elif [ "$metric" = "Paths" ]; then
                awk '{print $7}' "$log" | sort | uniq -c | sort -nr | head -n 5 >> "$SUMMARY_FILE"
            elif [ "$metric" = "Errors" ]; then
                grep -E " 40[0-9] | 50[0-9] " "$log" | awk '{print $9}' | sort | uniq -c >> "$SUMMARY_FILE" || echo "No errors found." >> "$SUMMARY_FILE"
            fi
        fi
    done
    
    cat "$SUMMARY_FILE"
}

detect_threats() {
    local log=$1
    echo "" >> "$SUMMARY_FILE"
    echo "--- SECURITY ALERTS ---" >> "$SUMMARY_FILE"
    
    if grep -qE "(\.\./|etc/passwd|union select|<script>|eval\()" "$log"; then
        echo "[CRITICAL] Attack Attempts Detected:" >> "$SUMMARY_FILE"
        grep -E "(\.\./|etc/passwd|union select|<script>|eval\()" "$log" | head -n 3 >> "$SUMMARY_FILE"
    else
        echo "[OK] No common attack signatures found." >> "$SUMMARY_FILE"
    fi
    
    if grep -qE "(admin|login|config|\.env|\.git)" "$log"; then
        echo "[WARNING] Access to Sensitive Paths:" >> "$SUMMARY_FILE"
        grep -E "(admin|login|config|\.env|\.git)" "$log" | head -n 3 >> "$SUMMARY_FILE"
    fi
    
    # Brute Force Detection (Repeated 401s)
    local failed_logins=$(grep "response:401" "$log" | grep "url:.*login" | wc -l)
    if [ "$failed_logins" -gt 2 ]; then
        echo "[CRITICAL] Potential Brute Force Attack detected on /login ($failed_logins failures):" >> "$SUMMARY_FILE"
        grep "response:401" "$log" | grep "url:.*login" | tail -n 3 >> "$SUMMARY_FILE"
    fi
}

show_user_journey() {
    local log=$1
    echo "" >> "$SUMMARY_FILE"
    echo "--- USER JOURNEYS (Last 10 Actions) ---" >> "$SUMMARY_FILE"
    if grep -q "url:" "$log"; then
       tail -n 20 "$log" | while read -r line; do
           if echo "$line" | grep -q "url:"; then
                ip=$(echo "$line" | awk -F': ' '{print $1}')
                url=$(echo "$line" | awk -F'url:' '{print $2}')
                echo "[$ip] VISITED $url" >> "$SUMMARY_FILE"
           fi
       done
    else
       tail -n 10 "$log" | awk '{print "["$1"] VISITED "$7}' >> "$SUMMARY_FILE"
    fi
}

analyze_with_ai() {
    echo "----------------------------------------"
    echo " AI Security Analysis (Qwen 1.5b)..."
    echo "----------------------------------------"
    
    local summary_text=$(cat "$SUMMARY_FILE")
    local prompt="Analyze this web traffic summary. Identify unusual patterns, potential attacks, or performance issues. Keep it brief. Summary: $summary_text"
    
    # Check if Ollama is running
    if pgrep ollama >/dev/null; then
         # JAILBREAK ATTEMPT: We use the fine-tuned model but extract the EXPLANATION.
         OUTPUT=$(/bin/ollama run nexos-expert "SYSTEM: Analyze traffic logs. USER: $prompt" 2>/dev/null)
         
         CLEANED_OUTPUT=$(echo "$OUTPUT" | grep "EXPLANATION:" | sed 's/EXPLANATION: //g')
         if [ -n "$CLEANED_OUTPUT" ]; then
             echo "$CLEANED_OUTPUT"
         else
             echo "$OUTPUT"
         fi
    else
         echo "Error: AI Service (Ollama) is not running."
    fi
}

setup_cron() {
    echo "Setting up Cron Job for automated analysis..."
    
    # Check if crond is running
    if ! pgrep crond >/dev/null; then
        echo "Starting crond..."
        crond -b -L /var/log/cron.log
    fi
    
    # Add to crontab (Run every 5 minutes)
    # writes to /var/spool/cron/crontabs/root
    mkdir -p /var/spool/cron/crontabs
    echo "*/5 * * * * /opt/nexos/analyze_traffic.sh run >> /var/log/traffic_analysis.log 2>&1" > /var/spool/cron/crontabs/root
    chmod 600 /var/spool/cron/crontabs/root
    
    echo "Success! Traffic analysis will run every 5 minutes."
    echo "Logs: /var/log/traffic_analysis.log"
}

show_help() {
    echo "Usage: $0 {run|cron|help} [log_file]"
    echo "  run   : Run immediate analysis"
    echo "  cron  : Install automated cron job (every 5 mins)"
    echo "  help  : Show this help"
}

# -----------------------------------------------------------------------------
# MAIN LOOP
# -----------------------------------------------------------------------------

COMMAND=${1:-"run"}
CUSTOM_LOG=$2

if [ -n "$CUSTOM_LOG" ]; then
    LOG_FILE="$CUSTOM_LOG"
fi

case "$COMMAND" in
    monitor)
        while true; do
            clear
            echo "=== NexOS Security Monitor (Update every 30s) ==="
            echo "Press Ctrl+C to stop."
            if [ -f "$LOG_FILE" ]; then
                generate_summary "$LOG_FILE"
                detect_threats "$LOG_FILE"
                show_user_journey "$LOG_FILE"
                cat "$SUMMARY_FILE"
            else
                echo "Waiting for log file..."
            fi
            sleep 30
        done
        ;;
    run)
        if [ ! -f "$LOG_FILE" ]; then
            mkdir -p "$(dirname "$LOG_FILE")"
            touch "$LOG_FILE"
            echo "Warning: Log file created empty. Start 'web' to generate traffic." > "$SUMMARY_FILE"
        fi
        
        # 1. Generate Text Summary (for CLI)
        generate_summary "$LOG_FILE"
        detect_threats "$LOG_FILE"
        
        # 2. Console Output
        cat "$SUMMARY_FILE"
        echo ""
        
        # 3. AI Analysis (Console)
        # We capture this to a variable to reuse in HTML
        AI_OUTPUT=""
        if pgrep ollama >/dev/null; then
             echo "----------------------------------------"
             echo " AI Security Analysis (Qwen 1.5b)..."
             echo "----------------------------------------"
             
             local summary_text=$(cat "$SUMMARY_FILE")
             local prompt="Analyze this web traffic summary. Identify unusual patterns, potential attacks, or performance issues. Keep it brief. Summary: $summary_text"
             
             RAW_AI=$(/bin/ollama run nexos-expert "SYSTEM: Analyze traffic logs. USER: $prompt" 2>/dev/null)
             AI_OUTPUT=$(echo "$RAW_AI" | grep "EXPLANATION:" | sed 's/EXPLANATION: //g')
             if [ -z "$AI_OUTPUT" ]; then AI_OUTPUT="$RAW_AI"; fi
             
             echo "$AI_OUTPUT"
        else
             echo "AI Service not running. Skipping AI analysis."
        fi

        # 4. Generate HTML Report (For Web Dashboard)
        publish_html_report "$LOG_FILE" "$AI_OUTPUT"
        ;;
    cron)
        setup_cron
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "Unknown command: $COMMAND"
        show_help
        exit 1
        ;;
esac

# -----------------------------------------------------------------------------
# HTML REPORT GENERATION
# -----------------------------------------------------------------------------

publish_html_report() {
    local log=$1
    local ai_text=$2
    local report_file="/var/www/reports/index.html"
    
    mkdir -p "$(dirname "$report_file")"
    
    echo "Generating Web Report at $report_file..."
    
    # --- HTML HEADER & CSS ---
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NexOS Security Analytics</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1a1a2e; color: #e0e0e0; padding: 20px; line-height: 1.6; }
        .container { max-width: 900px; margin: 0 auto; }
        h1 { color: #00ff88; border-bottom: 2px solid #00ff88; padding-bottom: 10px; }
        h2 { color: #00d4ff; margin-top: 30px; }
        .card { background: rgba(255,255,255,0.05); padding: 20px; border-radius: 10px; margin-bottom: 20px; border: 1px solid rgba(255,255,255,0.1); }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { text-align: left; padding: 12px; border-bottom: 1px solid rgba(255,255,255,0.1); }
        th { color: #00ff88; }
        .alert { background: rgba(255, 50, 50, 0.2); border-left: 4px solid #ff3333; padding: 15px; margin: 10px 0; }
        .ai-box { background: linear-gradient(45deg, rgba(100,0,255,0.2), rgba(0,200,255,0.1)); border: 1px solid #667eea; padding: 20px; border-radius: 15px; }
        .timestamp { color: #888; font-size: 0.9em; float: right; }
        a.back { display: inline-block; margin-top: 20px; color: #fff; text-decoration: none; padding: 10px 20px; background: #333; border-radius: 5px; }
        a.back:hover { background: #555; }
    </style>
</head>
<body>
    <div class="container">
        <span class="timestamp">Last Updated: $(date)</span>
        <h1>📊 NexOS Security Report</h1>
        
        <div class="card">
            <h2>🌍 Traffic Overview</h2>
            <table>
                <tr><th>Top IPs</th><th>Count</th></tr>
EOF

    # --- TOP IPs (AWK) ---
    if grep -q "url:" "$log"; then
        grep "url:" "$log" | awk '{print $1}' | sed 's/:$//' | sort | uniq -c | sort -nr | head -n 5 | \
        awk '{print "<tr><td>"$2"</td><td>"$1"</td></tr>"}' >> "$report_file"
    else
        awk '{print $1}' "$log" | sort | uniq -c | sort -nr | head -n 5 | \
        awk '{print "<tr><td>"$2"</td><td>"$1"</td></tr>"}' >> "$report_file"
    fi

    cat >> "$report_file" << EOF
            </table>
            
            <h3 style="color:#00d4ff; margin-top:20px">📂 Top Accessed Paths</h3>
            <table>
                <tr><th>Path</th><th>Requests</th></tr>
EOF
    
    # --- TOP PATHS ---
    if grep -q "url:" "$log"; then
        grep "url:" "$log" | awk -F'url:' '{print $2}' | sort | uniq -c | sort -nr | head -n 5 | \
        awk '{print "<tr><td>"$2"</td><td>"$1"</td></tr>"}' >> "$report_file"
    else
        awk '{print $7}' "$log" | sort | uniq -c | sort -nr | head -n 5 | \
        awk '{print "<tr><td>"$2"</td><td>"$1"</td></tr>"}' >> "$report_file"
    fi

    echo "            </table>" >> "$report_file"
    echo "        </div>" >> "$report_file"

    # --- SECURITY ALERTS ---
    echo '        <div class="card">' >> "$report_file"
    echo '            <h2>🛡️ Security Alerts</h2>' >> "$report_file"
    
    ALERTS_FOUND=0
    
    # Check 1: 404/500 Errors
    ERRORS=$(grep -E " 40[0-9] | 50[0-9] " "$log" | wc -l)
    if [ "$ERRORS" -gt 0 ]; then
        echo "<div class='alert'><strong>⚠️ High Error Rate:</strong> $ERRORS failed requests (404/500) detected.</div>" >> "$report_file"
        ALERTS_FOUND=1
    fi
    
    # Check 2: Brute Force
    FAILURES=$(grep "response:401" "$log" | wc -l)
    if [ "$FAILURES" -gt 2 ]; then
        echo "<div class='alert'><strong>🚨 BRUTE FORCE DETECTED:</strong> $FAILURES failed login attempts detected on /login.</div>" >> "$report_file"
        ALERTS_FOUND=1
    fi

    # Check 3: Sensitive Files
    if grep -qE "(admin|config|\.env)" "$log"; then
         echo "<div class='alert'><strong>🚫 Sensitive Access:</strong> Attempts to access admin/config files detected.</div>" >> "$report_file"
         ALERTS_FOUND=1
    fi

    if [ "$ALERTS_FOUND" -eq 0 ]; then
        echo "<p style='color:#00ff88'>✅ System Status Normal. No immediate threats detected.</p>" >> "$report_file"
    fi
    echo "        </div>" >> "$report_file"

    # --- AI ANALYSIS ---
    if [ -n "$ai_text" ]; then
        cat >> "$report_file" << EOF
        <div class="card ai-box">
            <h2>🧠 AI Analyst Insights</h2>
            <p><strong>Diagnosis:</strong></p>
            <p>${ai_text}</p>
        </div>
EOF
    fi

    # --- FOOTER ---
    cat >> "$report_file" << EOF
        <a href="/" class="back">← Back to Home</a>
        <br><br>
        <div style="text-align:center; opacity:0.5; font-size:0.8em">
            Generated by NexOS Traffic Analyzer v1.0
        </div>
    </div>
</body>
</html>
EOF
}

# -----------------------------------------------------------------------------
# MAIN LOOP
# -----------------------------------------------------------------------------
