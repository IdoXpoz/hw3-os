#!/bin/bash
echo "Setting up message slot testing environment..."

# Load kernel module
echo "Loading kernel module..."
sudo insmod message_slot.ko
if [ $? -ne 0 ]; then
    echo "Failed to load kernel module!"
    exit 1
fi

# Create device files
echo "Creating device files..."
sudo mknod /dev/slot0 c 235 0 2>/dev/null
sudo mknod /dev/slot1 c 235 1 2>/dev/null
sudo chmod 666 /dev/slot0 /dev/slot1

# Verify setup
echo "Verifying setup..."
if [ -c /dev/slot0 ] && [ -c /dev/slot1 ]; then
    echo "✅ Device files created successfully"
    ls -la /dev/slot*
else
    echo "❌ Failed to create device files"
    exit 1
fi

if lsmod | grep -q message_slot; then
    echo "✅ Kernel module loaded successfully"
else
    echo "❌ Kernel module not loaded"
    exit 1
fi

echo "Setup complete! You can now run tests."