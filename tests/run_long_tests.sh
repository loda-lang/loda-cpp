#!/bin/bash

# Long-running test script for LODA
# This script runs long-running tests and uploads results to Discord
# Environment variables:
#   LODA_DISCORD_WEBHOOK - Discord webhook URL for uploading results

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LODA_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LODA_BIN="$LODA_DIR/loda"

# Check if loda binary exists
if [ ! -f "$LODA_BIN" ]; then
    echo -e "${RED}Error: loda binary not found at $LODA_BIN${NC}"
    echo "Please build the project first by running: cd src && make -f Makefile.linux-x86.mk"
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
    # Replace backslashes first, then quotes and newlines
    local escaped_message=$(echo "$message" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | awk '{printf "%s\\n", $0}' | sed 's/\\n$//')
    
    # Create temporary JSON file
    local tmp_file=$(mktemp /tmp/loda_discord_XXXXXX.json)
    echo "{\"content\":\"$escaped_message\"}" > "$tmp_file"
    
    # Send to Discord using curl
    if curl -fsSL -X POST -H "Content-Type: application/json" -d @"$tmp_file" "$LODA_DISCORD_WEBHOOK" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Results uploaded to Discord${NC}"
    else
        echo -e "${RED}✗ Failed to upload results to Discord${NC}"
    fi
    
    # Clean up
    rm -f "$tmp_file"
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
    
    # Create temporary file for output
    local output_file=$(mktemp /tmp/loda_test_output_XXXXXX.txt)
    
    # Run the test and capture output and exit code
    local start_time=$(date +%s)
    set +e
    $test_command > "$output_file" 2>&1
    local exit_code=$?
    set -e
    local end_time=$(date +%s)
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
    
    # Prepare Discord message
    local discord_message="$status_emoji **$test_name** - $status\nExit code: $exit_code\nDuration: ${duration}s\n\nLast 20 lines of output:\n\`\`\`\n$(tail -20 "$output_file" | head -c 1500)\n\`\`\`"
    
    # Upload to Discord
    send_to_discord "$discord_message"
    
    # Clean up
    rm -f "$output_file"
    
    echo ""
}

# Main execution
echo "=========================================="
echo "LODA Long-Running Tests"
echo "=========================================="
echo "LODA binary: $LODA_BIN"
echo "Discord webhook: ${LODA_DISCORD_WEBHOOK:+configured}"

# Run the tests
run_test "test-inceval" "$LODA_BIN test-inceval"
run_test "test-pari" "$LODA_BIN test-pari"
run_test "test-analyzer" "$LODA_BIN test-analyzer"
run_test "test-lean" "$LODA_BIN test-lean"

echo ""
echo "=========================================="
echo "All tests completed!"
echo "=========================================="
