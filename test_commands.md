# Message Slot Testing Commands
## 1. Initial Setup (as root/sudo)

```bash
# Compile the kernel module
make

# Load the module
sudo insmod message_slot.ko

# Verify module is loaded
lsmod | grep message_slot

# Check kernel logs for successful loading
dmesg | tail -5
```

## 2. Create Device Files (as root/sudo)

```bash
# Create message slot device files
sudo mknod /dev/slot0 c 235 0
sudo mknod /dev/slot1 c 235 1
sudo mknod /dev/slot2 c 235 2

# Change permissions so regular user can access
sudo chmod 666 /dev/slot0
sudo chmod 666 /dev/slot1
sudo chmod 666 /dev/slot2

# Verify device files were created
ls -l /dev/slot*
```

## 3. Compile User Programs

```bash
# Compile message sender and reader
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader

# Verify compilation succeeded
ls -l message_sender message_reader
```

## 4. Basic Functionality Tests

### Test 1: Simple message without censorship
```bash
# Send a message to channel 1 without censorship
./message_sender /dev/slot0 1 0 "Hello World"

# Read the message back
./message_reader /dev/slot0 1

# Expected output: Hello World
```

### Test 2: Simple message with censorship
```bash
# Send a message to channel 2 with censorship enabled
./message_sender /dev/slot0 2 1 "Hello World"

# Read the message back
./message_reader /dev/slot0 2

# Expected output: He#lo #or#d (every 3rd character replaced with #)
```

### Test 3: Multiple channels on same device
```bash
# Send different messages to different channels
./message_sender /dev/slot0 10 0 "Channel 10 message"
./message_sender /dev/slot0 20 0 "Channel 20 message"
./message_sender /dev/slot0 30 1 "Channel 30 censored"

# Read messages back
./message_reader /dev/slot0 10  # Expected: Channel 10 message
./message_reader /dev/slot0 20  # Expected: Channel 20 message
./message_reader /dev/slot0 30  # Expected: Ch#nn#l #0 #en#or#d
```

### Test 4: Multiple devices
```bash
# Send messages to different devices
./message_sender /dev/slot0 1 0 "Device 0 message"
./message_sender /dev/slot1 1 0 "Device 1 message"
./message_sender /dev/slot2 1 0 "Device 2 message"

# Read messages back
./message_reader /dev/slot0 1  # Expected: Device 0 message
./message_reader /dev/slot1 1  # Expected: Device 1 message
./message_reader /dev/slot2 1  # Expected: Device 2 message
```

## 5. Message Persistence Tests

### Test 5: Message overwrites
```bash
# Send first message
./message_sender /dev/slot0 100 0 "First message"
./message_reader /dev/slot0 100  # Expected: First message

# Overwrite with second message
./message_sender /dev/slot0 100 0 "Second message"
./message_reader /dev/slot0 100  # Expected: Second message

# Read again (should still be second message)
./message_reader /dev/slot0 100  # Expected: Second message
```

## 6. Error Testing

### Test 6: Invalid channel ID (should fail)
```bash
# Try to use channel ID 0 (invalid)
./message_sender /dev/slot0 0 0 "Should fail"
# Expected: Error message and exit code 1

echo $?  # Should be 1
```

### Test 7: Reading from non-existent channel (should fail)
```bash
# Try to read from channel that has no message
./message_reader /dev/slot0 999
# Expected: Error message and exit code 1

echo $?  # Should be 1
```

### Test 8: Empty message (should fail)
```bash
# Try to send empty message
./message_sender /dev/slot0 1 0 ""
# Expected: Error message and exit code 1

echo $?  # Should be 1
```

### Test 9: Message too long (should fail)
```bash
# Try to send message longer than 128 bytes
./message_sender /dev/slot0 1 0 "$(python3 -c 'print("x" * 129)')"
# Expected: Error message and exit code 1

echo $?  # Should be 1
```

### Test 10: Wrong number of arguments (should fail)
```bash
# Test with wrong number of arguments
./message_sender /dev/slot0 1 0
# Expected: Error message and exit code 1

./message_reader /dev/slot0
# Expected: Error message and exit code 1
```

### Test 11: Non-existent device file (should fail)
```bash
# Try to access non-existent device
./message_sender /dev/nonexistent 1 0 "test"
# Expected: Error message and exit code 1

./message_reader /dev/nonexistent 1
# Expected: Error message and exit code 1
```

## 7. Buffer Size Tests

### Test 12: Maximum size message (128 bytes)
```bash
# Send exactly 128-byte message
./message_sender /dev/slot0 1 0 "$(python3 -c 'print("x" * 128)')"

# Read it back
./message_reader /dev/slot0 1
# Should succeed and print 128 x's
```

### Test 13: Various message sizes
```bash
# Test different message sizes
./message_sender /dev/slot0 1 0 "a"
./message_reader /dev/slot0 1  # Expected: a

./message_sender /dev/slot0 2 0 "$(python3 -c 'print("b" * 50)')"
./message_reader /dev/slot0 2  # Expected: 50 b's

./message_sender /dev/slot0 3 0 "$(python3 -c 'print("c" * 100)')"
./message_reader /dev/slot0 3  # Expected: 100 c's
```

## 8. Censorship Pattern Tests

### Test 14: Verify censorship pattern
```bash
# Test censorship on specific patterns
./message_sender /dev/slot0 1 1 "abcdefghijk"
./message_reader /dev/slot0 1
# Expected: ab#de#gh#jk (positions 2,5,8 replaced)

./message_sender /dev/slot0 2 1 "123456789"
./message_reader /dev/slot0 2
# Expected: 12#45#78# (positions 2,5,8 replaced)
```

## 9. Stress Tests

### Test 15: Many channels
```bash
# Create messages on many different channels
for i in {1..50}; do
    ./message_sender /dev/slot0 $i 0 "Message for channel $i"
done

# Read them back
for i in {1..50}; do
    echo -n "Channel $i: "
    ./message_reader /dev/slot0 $i
done
```

### Test 16: Large channel IDs
```bash
# Test with large channel IDs
./message_sender /dev/slot0 1000000 0 "Large channel ID"
./message_reader /dev/slot0 1000000
# Expected: Large channel ID

./message_sender /dev/slot0 4294967295 0 "Max channel ID"  # Max uint
./message_reader /dev/slot0 4294967295
# Expected: Max channel ID
```

## 10. Concurrent Access Simulation

### Test 17: Multiple processes (run in background)
```bash
# Send messages in background
./message_sender /dev/slot0 1 0 "Process 1" &
./message_sender /dev/slot0 2 0 "Process 2" &
./message_sender /dev/slot0 3 0 "Process 3" &

# Wait for completion
wait

# Read results
./message_reader /dev/slot0 1
./message_reader /dev/slot0 2
./message_reader /dev/slot0 3
```

## 11. Cleanup and Verification

### Test 18: Module unloading
```bash
# Check memory usage before unload
cat /proc/meminfo | grep -i slab

# Remove device files
sudo rm /dev/slot0 /dev/slot1 /dev/slot2

# Unload module
sudo rmmod message_slot

# Check kernel logs
dmesg | tail -5

# Verify module is unloaded
lsmod | grep message_slot  # Should return nothing
```

## 12. Automated Test Script

```bash
#!/bin/bash
# Save as test_all.sh and make executable: chmod +x test_all.sh

set -e  # Exit on any error

echo "=== Message Slot Comprehensive Test ==="

# Setup
echo "Setting up..."
make clean && make
sudo insmod message_slot.ko
sudo mknod /dev/slot0 c 235 0
sudo chmod 666 /dev/slot0
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader

# Basic tests
echo "Testing basic functionality..."
./message_sender /dev/slot0 1 0 "Hello World"
result=$(./message_reader /dev/slot0 1)
if [ "$result" = "Hello World" ]; then
    echo "✓ Basic test passed"
else
    echo "✗ Basic test failed: expected 'Hello World', got '$result'"
fi

# Censorship test
echo "Testing censorship..."
./message_sender /dev/slot0 2 1 "abcdefghijk"
result=$(./message_reader /dev/slot0 2)
if [ "$result" = "ab#de#gh#jk" ]; then
    echo "✓ Censorship test passed"
else
    echo "✗ Censorship test failed: expected 'ab#de#gh#jk', got '$result'"
fi

# Error tests
echo "Testing error cases..."
if ./message_sender /dev/slot0 0 0 "test" 2>/dev/null; then
    echo "✗ Channel 0 test failed: should have returned error"
else
    echo "✓ Channel 0 error test passed"
fi

# Cleanup
echo "Cleaning up..."
sudo rm /dev/slot0
sudo rmmod message_slot

echo "=== All tests completed ==="
```

## 13. Debug Commands

```bash
# Monitor kernel messages in real time
sudo dmesg -w

# Check module information
modinfo message_slot.ko

# Check device file details
file /dev/slot0
stat /dev/slot0

# Monitor system calls (if strace is available)
strace -e trace=openat,ioctl,read,write,close ./message_sender /dev/slot0 1 0 "test"
```

## Notes:
- Run setup commands as root/sudo
- Test commands can be run as regular user after proper permissions are set
- Check `echo $?` after each command to verify exit codes
- Use `dmesg` to monitor kernel messages for debugging
- The automated test script provides a quick way to verify basic functionality