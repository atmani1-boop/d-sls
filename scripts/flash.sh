#!/bin/bash
#
# flash.sh - Automated flash script for Atmani Smart Lighting Node
#
# Usage: ./scripts/flash.sh [PORT]
#   PORT: Serial port (default: /dev/ttyUSB0)
#

set -e

# Default port
PORT="${1:-/dev/ttyUSB0}"

echo "========================================"
echo "  Atmani Smart Lighting - Flash Script"
echo "========================================"
echo ""
echo "Target port: $PORT"
echo ""

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Must run from project root directory"
    exit 1
fi

# Check if firmware exists
if [ ! -f "build/atmani-smart-lighting.bin" ]; then
    echo "ERROR: Firmware not found. Run ./scripts/build.sh first"
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

# Flash
echo ""
echo "→ Flashing firmware to $PORT..."
idf.py -p "$PORT" flash

# Monitor
echo ""
echo "========================================"
echo "✓ Flash completed successfully!"
echo "========================================"
echo ""
echo "Starting serial monitor..."
echo "Press Ctrl+] to exit monitor"
echo ""
sleep 2

idf.py -p "$PORT" monitor
