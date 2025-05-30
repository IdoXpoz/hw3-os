#!/bin/bash

# Test script for message slot implementation
echo "Starting Message Slot Tests..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0

# Function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_output="$3"
    
    echo -e "\n${YELLOW}Test: $test_name${NC}"
    echo "Command: $command"
    
    output=$(eval "$command" 2>&1)
    exit_code=$?
    
    if [ "$expected_output" = "ERROR" ]; then
        if [ $exit_code -ne 0 ]; then
            echo -e "${GREEN}PASS${NC} - Expected error occurred"
            ((PASS_COUNT++))
        else
            echo -e "${RED}FAIL${NC} - Expected error but command succeeded"
            ((FAIL_COUNT++))
        fi
    else
        if [ $exit_code -eq 0 ] && [ "$output" = "$expected_output" ]; then
            echo -e "${GREEN}PASS${NC} - Output matches expected"
            ((PASS_COUNT++))
        else
            echo -e "${RED}FAIL${NC}"
            echo "Expected: '$expected_output'"
            echo "Got: '$output'"
            echo "Exit code: $exit_code"
            ((FAIL_COUNT++))
        fi
    fi
}

# Test 1: Basic functionality
run_test "Basic send and receive" \
    "./message_sender /dev/slot0 1 0 'Hello World' && ./message_reader /dev/slot0 1" \
    "Hello World"

# Test 2: Multiple channels
./message_sender /dev/slot0 10 0 "Channel Ten" 2>/dev/null
./message_sender /dev/slot0 20 0 "Channel Twenty" 2>/dev/null
run_test "Multiple channels - channel 10" \
    "./message_reader /dev/slot0 10" \
    "Channel Ten"
run_test "Multiple channels - channel 20" \
    "./message_reader /dev/slot0 20" \
    "Channel Twenty"

# Test 3: Message overwriting  
./message_sender /dev/slot0 5 0 "First" 2>/dev/null
./message_sender /dev/slot0 5 0 "Second" 2>/dev/null
run_test "Message overwriting" \
    "./message_reader /dev/slot0 5" \
    "Second"

# Test 4: Censorship
run_test "Censorship feature" \
    "./message_sender /dev/slot0 30 1 'Hello World' && ./message_reader /dev/slot0 30" \
    "He#lo #or#d"

# Test 5: Different device files
./message_sender /dev/slot1 1 0 "Slot One Message" 2>/dev/null
run_test "Different device files" \
    "./message_reader /dev/slot1 1" \
    "Slot One Message"

# Test 6: Error cases
run_test "Invalid channel 0" \
    "./message_sender /dev/slot0 0 0 'test'" \
    "ERROR"

run_test "Read from non-existent channel" \
    "./message_reader /dev/slot0 999" \
    "ERROR"

run_test "Empty message" \
    "./message_sender /dev/slot0 1 0 ''" \
    "ERROR"

# Test 7: Maximum size message
MAX_MSG=$(printf 'A%.0s' {1..128})
run_test "Maximum size message (128 chars)" \
    "./message_sender /dev/slot0 40 0 '$MAX_MSG' && ./message_reader /dev/slot0 40" \
    "$MAX_MSG"

# Test 8: Oversized message
OVER_MSG=$(printf 'A%.0s' {1..129})
run_test "Oversized message (129 chars)" \
    "./message_sender /dev/slot0 41 0 '$OVER_MSG'" \
    "ERROR"

# Test 9: Complex censorship
run_test "Complex censorship test" \
    "./message_sender /dev/slot0 50 1 'abcdefghijk' && ./message_reader /dev/slot0 50" \
    "ab#de#gh#jk"

# Test 10: Wrong number of arguments (should fail)
run_test "Sender wrong number of arguments" "./message_sender /dev/slot0 1 0" "ERROR"
run_test "Reader wrong number of arguments" "./message_reader /dev/slot0" "ERROR"

# Test 11: Non-existent device file (should fail)
run_test "Sender non-existent device file" "./message_sender /dev/nonexistent 1 0 'test'" "ERROR"
run_test "Reader non-existent device file" "./message_reader /dev/nonexistent 1" "ERROR"

# Test 12: Multiple devices
./message_sender /dev/slot0 1 0 "Device 0 message" 2>/dev/null
./message_sender /dev/slot1 1 0 "Device 1 message" 2>/dev/null
./message_sender /dev/slot2 1 0 "Device 2 message" 2>/dev/null
run_test "Multiple devices - slot0" "./message_reader /dev/slot0 1" "Device 0 message"
run_test "Multiple devices - slot1" "./message_reader /dev/slot1 1" "Device 1 message"
run_test "Multiple devices - slot2" "./message_reader /dev/slot2 1" "Device 2 message"

# Test 13: Various message sizes
./message_sender /dev/slot0 1 0 "a" 2>/dev/null
run_test "Message size 1" "./message_reader /dev/slot0 1" "a"
./message_sender /dev/slot0 2 0 "$(python3 -c 'print("b" * 50)')" 2>/dev/null
run_test "Message size 50" "./message_reader /dev/slot0 2" "$(python3 -c 'print("b" * 50)')"
./message_sender /dev/slot0 3 0 "$(python3 -c 'print("c" * 100)')" 2>/dev/null
run_test "Message size 100" "./message_reader /dev/slot0 3" "$(python3 -c 'print("c" * 100)')"

# Test 14: Numeric censorship pattern
./message_sender /dev/slot0 2 1 "123456789" 2>/dev/null
run_test "Censorship pattern numeric" "./message_reader /dev/slot0 2" "12#45#78#"

# Test 15: Many channels
for i in {1..50}; do
    ./message_sender /dev/slot0 $i 0 "Message for channel $i" 2>/dev/null
done
for i in {1..50}; do
    run_test "Many channels - channel $i" "./message_reader /dev/slot0 $i" "Message for channel $i"
done

# Test 16: Large channel IDs
./message_sender /dev/slot0 1000000 0 "Large channel ID" 2>/dev/null
run_test "Large channel ID" "./message_reader /dev/slot0 1000000" "Large channel ID"
./message_sender /dev/slot0 4294967295 0 "Max channel ID" 2>/dev/null
run_test "Max channel ID" "./message_reader /dev/slot0 4294967295" "Max channel ID"

# Summary
echo -e "\n${YELLOW}=== Test Summary ===${NC}"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed. Please check your implementation.${NC}"
    exit 1
fi