#!/bin/bash

# Long-running test script for LODA
# This script runs long-running tests and uploads results to Discord
# Environment variables:
#   LODA_DISCORD_WEBHOOK - Discord webhook URL for uploading results

set -e

# Constants
OUTPUT_LINES=20  # Number of lines of test output to show
DISCORD_OUTPUT_LIMIT=1900  # Safe limit for Discord message output to avoid breaking UTF-8

# Accumulated test results for final summary
TEST_RESULTS=()

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LODA_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LODA_BIN="$LODA_DIR/loda"

# Enable testing with external tools
export LODA_TEST_WITH_EXTERNAL_TOOLS=1

# Create output directory with timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="$SCRIPT_DIR/test_output_$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Check if loda binary exists
if [ ! -f "$LODA_BIN" ]; then
    echo -e "${RED}Error: loda binary not found at $LODA_BIN${NC}"
    echo "Please build the project first by running: cd src && make -f Makefile.<platform>.mk"
    exit 1
fi

# Check for required external tools
echo "Checking for required external tools..."
MISSING_TOOLS=()
if ! command -v curl &> /dev/null; then
    MISSING_TOOLS+=("curl")
fi
if ! command -v jq &> /dev/null; then
    MISSING_TOOLS+=("jq")
fi
if ! command -v gp &> /dev/null; then
    MISSING_TOOLS+=("gp (PARI/GP)")
fi
if ! command -v lean &> /dev/null; then
    MISSING_TOOLS+=("lean")
fi
if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo -e "${RED}Error: The following required tools are not installed:${NC}"
    for tool in "${MISSING_TOOLS[@]}"; do
        echo -e "${RED}  - $tool${NC}"
    done
    echo ""
    echo "Please install the missing tools:"
    echo "  - curl: brew install curl (on macOS) or apt-get install curl (on Linux)"
    echo "  - jq: brew install jq (on macOS) or apt-get install jq (on Linux)"
    echo "  - gp (PARI/GP): brew install pari (on macOS) or apt-get install pari-gp (on Linux)"
    echo "  - lean: https://leanprover-community.github.io/get_started.html"
    exit 1
fi
echo -e "${GREEN}✓ All required tools are installed${NC}"

# Function to send message to Discord
send_to_discord() {
    local message="$1"
    
    # Check if Discord webhook is set
    if [ -z "$LODA_DISCORD_WEBHOOK" ]; then
        echo -e "${YELLOW}Warning: LODA_DISCORD_WEBHOOK not set, skipping Discord notification${NC}"
        return 0
    fi
    
    # Escape special characters for JSON
    # Use jq to properly encode the message as JSON, which handles all escaping correctly
    local json_payload
    json_payload=$(jq -n --arg content "$message" '{content: $content}')
    
    # Send to Discord using curl
    local curl_error
    curl_error=$(mktemp)
    if ! curl -fsSL -X POST -H "Content-Type: application/json" -d "$json_payload" "$LODA_DISCORD_WEBHOOK" 2>"$curl_error"; then
        echo -e "${RED}✗ Failed to send message to Discord${NC}"
        if [ -s "$curl_error" ]; then
            echo -e "${RED}Error details: $(cat "$curl_error")${NC}"
        fi
    fi
    
    # Clean up
    rm -f "$curl_error"
}

# Function to send output lines to Discord one by one
send_output_to_discord() {
    local output_file="$1"
    local line_count=0
    local current_message=""
    local max_message_length=1800  # Safe limit to avoid hitting Discord's 2000 character limit
    
    while IFS= read -r line; do
        line_count=$((line_count + 1))
        
        # If adding this line would exceed the limit, send the current message first
        if [ $((${#current_message} + ${#line} + 1)) -gt $max_message_length ] && [ -n "$current_message" ]; then
            send_to_discord $'```\n'"${current_message}"$'\n```'
            current_message=""
            # Small delay to avoid rate limiting
            sleep 0.5
        fi
        
        # Add line to current message
        if [ -z "$current_message" ]; then
            current_message="$line"
        else
            current_message="${current_message}
${line}"
        fi
    done < <(tail -$OUTPUT_LINES "$output_file")
    
    # Send any remaining message
    if [ -n "$current_message" ]; then
        send_to_discord $'```\n'"${current_message}"$'\n```'
    fi
}

# Function to run a test and upload results
run_test() {
    local test_name="$1"
    
    echo ""
    echo "=========================================="
    echo "Running: $test_name"
    echo "=========================================="
    echo ""

    send_to_discord "▶️ **Starting $test_name**"
    
    # Create output file in the timestamped directory
    local output_file="$OUTPUT_DIR/${test_name}.txt"
    
    # Heartbeat to show progress every 10 minutes
    local heartbeat_pid
    (
        while true; do
            sleep 3600
            send_output_to_discord "$output_file"
        done
    ) &
    heartbeat_pid=$!

    # Run the test and capture output and exit code
    local start_time
    start_time=$(date +%s)
    set +e
    # Change to the main loda directory before running the test
    (cd "$LODA_DIR" && ./loda $test_name) > "$output_file" 2>&1
    local exit_code=$?
    set -e

    # Stop heartbeat
    if [ -n "$heartbeat_pid" ]; then
        kill "$heartbeat_pid" 2>/dev/null || true
        wait "$heartbeat_pid" 2>/dev/null || true
    fi

    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Format duration in human-readable format
    local hours=$((duration / 3600))
    local minutes=$(((duration % 3600) / 60))
    local seconds=$((duration % 60))
    local duration_str=""
    if [ $hours -gt 0 ]; then
        duration_str="${hours}h ${minutes}m ${seconds}s"
    elif [ $minutes -gt 0 ]; then
        duration_str="${minutes}m ${seconds}s"
    else
        duration_str="${seconds}s"
    fi
    
    # Determine status
    local status
    local status_emoji
    if [ $exit_code -eq 0 ]; then
        status="PASSED"
        status_emoji="✅"
        echo -e "${GREEN}✓ Test passed (exit code: $exit_code, duration: ${duration_str})${NC}"
    else
        status="FAILED"
        status_emoji="❌"
        echo -e "${RED}✗ Test failed (exit code: $exit_code, duration: ${duration_str})${NC}"
    fi
    
    # Print last lines of output
    echo ""
    echo "Last $OUTPUT_LINES lines of output:"
    echo "----------------------------------------"
    tail -$OUTPUT_LINES "$output_file"
    echo "----------------------------------------"
    
    # Upload to Discord
    local discord_message
    discord_message="$status_emoji **Finished $test_name - ${status}**
Exit code: $exit_code
Duration: ${duration_str}"
    send_to_discord "$discord_message"
    
    # Send output lines one by one to avoid size limits
    send_to_discord "Last $OUTPUT_LINES lines of output:"
    send_output_to_discord "$output_file"

    # Store result for final summary
    TEST_RESULTS+=("${status_emoji} ${test_name} - ${status} (exit code: ${exit_code}, duration: ${duration_str})")
    
    echo ""
}

# Main execution
echo "=========================================="
echo "LODA Long-Running Tests"
echo "=========================================="
echo "LODA binary: $LODA_BIN"
echo "Output directory: $OUTPUT_DIR"
echo "Discord webhook: ${LODA_DISCORD_WEBHOOK:+configured}"

# Run the tests
# run_test "test-analyzer"
# run_test "test-formula-parser"
# run_test "test-inceval"
# run_test "test-vireval"
# run_test "test-recursion"
run_test "find-slow-formulas"
# run_test "test-pari"
run_test "test-lean"

# Print and send final summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
for summary_line in "${TEST_RESULTS[@]}"; do
    echo "${summary_line}"
done

# Send summary to Discord as a single message
if [ ${#TEST_RESULTS[@]} -gt 0 ]; then
    summary_payload="Test Summary:\n"
    for summary_line in "${TEST_RESULTS[@]}"; do
        summary_payload+="${summary_line}\n"
    done
    send_to_discord "${summary_payload}"
fi

echo ""
echo "=========================================="
echo "All tests completed!"
echo "Output files saved in: $OUTPUT_DIR"
echo "=========================================="
