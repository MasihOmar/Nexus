#!/bin/busybox sh

# doctor.sh - AI System Health Check
# Usage: ./doctor.sh

LOG_FILE="/tmp/system_health.txt"

echo "----------------------------------------"
echo " 🚑 AI SYSTEM DOCTOR - Diagnosis Started"
echo "----------------------------------------"
echo "Gathering vitals..."

# Check Dependencies
if ! pgrep ollama >/dev/null; then
    echo "⚠️  WARNING: AI Engine (Ollama) is NOT running!"
    echo "   Diagnosis usually requires AI."
    echo "   Try starting it: ollama serve &"
fi

# Collect Vitals
echo "=== UPTIME ===" > "$LOG_FILE"
uptime >> "$LOG_FILE"

echo -e "\n=== MEMORY (MB) ===" >> "$LOG_FILE"
free -m >> "$LOG_FILE"

echo -e "\n=== DISK USAGE ===" >> "$LOG_FILE"
df -h >> "$LOG_FILE"

echo -e "\n=== KERNEL LOGS (Last 10) ===" >> "$LOG_FILE"
dmesg | tail -n 10 >> "$LOG_FILE"

echo -e "\n=== LOAD AVERAGE ===" >> "$LOG_FILE"
cat /proc/loadavg >> "$LOG_FILE"

# Show gathered info briefly
cat "$LOG_FILE"

echo "----------------------------------------"
echo " 🧠 AI Analyzing Symptoms..."
echo "----------------------------------------"

HEALTH_DATA=$(cat "$LOG_FILE")
PROMPT="You are a Linux System Administrator. Analyze this system health report. Check for high load, memory shortages, full disks, or kernel errors. Give a short diagnosis and recommendation. Report: $HEALTH_DATA"

if pgrep ollama >/dev/null; then
    # The model is fine-tuned to ALWAYS output COMMAND + EXPLANATION.
    # We strip the COMMAND line and only show the explanation for the report.
    OUTPUT=$(/bin/ollama run nexos-expert "SYSTEM: Analyze system health. USER: $PROMPT" 2>/dev/null)
    
    # Try to extract EXPLANATION if present (Fine-tuned behavior)
    CLEANED_OUTPUT=$(echo "$OUTPUT" | grep "EXPLANATION:" | sed 's/EXPLANATION: //g')
    
    if [ -n "$CLEANED_OUTPUT" ]; then
        echo "$CLEANED_OUTPUT"
    else
        # Fallback: Just show the raw output if the model didn't follow the format
        # This fixes the "empty output" bug when the model is chatty or ignores instructions
        echo "$OUTPUT"
    fi
else
    echo "Error: AI Brain (Ollama) is dead! Resuscitate it with 'ollama serve'."
fi
