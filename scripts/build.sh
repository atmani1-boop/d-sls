#!/bin/bash
#
# build.sh - Automated build script for Atmani Smart Lighting Node
#
# Usage: ./scripts/build.sh
#

set -e

echo "========================================"
echo "  Atmani Smart Lighting - Build Script"
echo "========================================"
echo ""

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Must run from project root directory"
    exit 1
fi

# Activate ESP-IDF environment
echo "→ Activating ESP-IDF environment..."
if [ -z "$IDF_PATH" ]; then
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        source "$HOME/esp/esp-idf/export.sh"
    else
        echo "ERROR: ESP-IDF not found at $HOME/esp/esp-idf"
        echo "Please install ESP-IDF v5.1+ first"
        exit 1
    fi
else
    echo "  ESP-IDF already activated: $IDF_PATH"
fi

# Verify IDF version
echo ""
echo "→ Checking ESP-IDF version..."
idf.py --version

# Set target
echo ""
echo "→ Setting target to esp32c6..."
idf.py set-target esp32c6

# Build
echo ""
echo "→ Building firmware..."
idf.py build

# Show size
echo ""
echo "→ Firmware size analysis:"
idf.py size

# Success
echo ""
echo "========================================"
echo "✓ Build completed successfully!"
echo "========================================"
echo ""
echo "Firmware location: build/atmani-smart-lighting.bin"
echo ""
echo "Next steps:"
echo "  1. Flash: ./scripts/flash.sh /dev/ttyUSB0"
echo "  2. Or manually: idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
