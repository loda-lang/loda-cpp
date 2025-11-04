#!/bin/bash

# Long-running test script for LODA
# This script runs long-running tests and uploads results to Discord
# Environment variables:
#   LODA_DISCORD_WEBHOOK - Discord webhook URL for uploading results

set -e

# Constants
DISCORD_OUTPUT_LIMIT=1400  # Safe limit for Discord message output to avoid breaking UTF-8

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LODA_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LODA_BIN="$LODA_DIR/loda"

# Set test environment flags
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

# Function to send message to Discord
send_to_discord() {
    local message="$1"
    
    # Check if Discord webhook is set
    if [ -z "$LODA_DISCORD_WEBHOOK" ]; then
        echo -e "${YELLOW}Warning: LODA_DISCORD_WEBHOOK not set, skipping Discord upload${NC}"
        return 0
    fi
    
    # Escape special characters for JSON
    # Replace backslashes first, then quotes, then newlines
    local escaped_message
    escaped_message=$(echo "$message" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | awk '{printf "%s\\n", $0}' | sed 's/\\n$//')
    
    # Create temporary JSON file
    local tmp_file
    tmp_file=$(mktemp)
    echo "{\"content\":\"$escaped_message\"}" > "$tmp_file"
    
    # Send to Discord using curl
    local curl_error
    curl_error=$(mktemp)
    if curl -fsSL -X POST -H "Content-Type: application/json" -d @"$tmp_file" "$LODA_DISCORD_WEBHOOK" 2>"$curl_error"; then
        echo -e "${GREEN}✓ Results uploaded to Discord${NC}"
    else
        echo -e "${RED}✗ Failed to upload results to Discord${NC}"
        if [ -s "$curl_error" ]; then
            echo -e "${RED}Error details: $(cat "$curl_error")${NC}"
        fi
    fi
    
    # Clean up
    rm -f "$tmp_file" "$curl_error"
}

# Function to run a test and upload results
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo ""
    echo "=========================================="
    echo "Running: $test_name"
    echo "Command: $test_command"
    echo "=========================================="
    echo ""
    
    # Create output file in the timestamped directory
    local output_file="$OUTPUT_DIR/${test_name}.txt"
    
    # Run the test and capture output and exit code
    local start_time
    start_time=$(date +%s)
    set +e
    $test_command > "$output_file" 2>&1
    local exit_code=$?
    set -e
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Determine status
    local status
    local status_emoji
    if [ $exit_code -eq 0 ]; then
        status="PASSED"
        status_emoji="✅"
        echo -e "${GREEN}✓ Test passed (exit code: $exit_code, duration: ${duration}s)${NC}"
    else
        status="FAILED"
        status_emoji="❌"
        echo -e "${RED}✗ Test failed (exit code: $exit_code, duration: ${duration}s)${NC}"
    fi
    
    # Print last 20 lines of output
    echo ""
    echo "Last 20 lines of output:"
    echo "----------------------------------------"
    tail -20 "$output_file"
    echo "----------------------------------------"
    
    # Prepare Discord message with safe truncation (avoid breaking multi-byte characters)
    # Use head -c to limit bytes, but be conservative to avoid breaking UTF-8
    local output_tail
    output_tail=$(tail -20 "$output_file" | head -c "$DISCORD_OUTPUT_LIMIT")
    local discord_message
    discord_message="$status_emoji **$test_name** - $status\nExit code: $exit_code\nDuration: ${duration}s\n\nLast 20 lines of output:\n\`\`\`\n${output_tail}\n\`\`\`"
    
    # Upload to Discord
    send_to_discord "$discord_message"
    
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
run_test "test-inceval" "$LODA_BIN test-inceval"
run_test "test-vireval" "$LODA_BIN test-vireval"
run_test "test-pari" "$LODA_BIN test-pari"
run_test "test-lean" "$LODA_BIN test-lean"
run_test "test-formula-parser" "$LODA_BIN test-formula-parser"

echo ""
echo "=========================================="
echo "All tests completed!"
echo "Output files saved in: $OUTPUT_DIR"
echo "=========================================="
