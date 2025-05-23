#!/bin/bash

echo "Cleaning up message slot testing environment..."

# Remove kernel module
echo "Unloading kernel module..."
if lsmod | grep -q message_slot; then
    sudo rmmod message_slot
    if [ $? -eq 0 ]; then
        echo "✅ Kernel module unloaded successfully"
    else
        echo "❌ Failed to unload kernel module"
        echo "Try: sudo rmmod -f message_slot"
    fi
else
    echo "ℹ️  Kernel module not loaded"
fi

# Remove device files
echo "Removing device files..."
sudo rm -f /dev/slot0 /dev/slot1
if [ ! -e /dev/slot0 ] && [ ! -e /dev/slot1 ]; then
    echo "✅ Device files removed successfully"
else
    echo "❌ Failed to remove device files"
fi

# Clean build files (optional)
echo "Cleaning build files..."
make clean > /dev/null 2>&1
echo "✅ Build files cleaned"

# Verify cleanup
echo ""
echo "Verification:"
if lsmod | grep -q message_slot; then
    echo "❌ Module still loaded"
else
    echo "✅ Module unloaded"
fi

if [ -e /dev/slot0 ] || [ -e /dev/slot1 ]; then
    echo "❌ Device files still exist"
else
    echo "✅ Device files removed"
fi

echo "Cleanup complete!"