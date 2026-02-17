#!/bin/bash
###############################################################################
# build_with_dashboard.sh
# 
# Automated build script for DIAMANT v2.1 ESP-IDF project
# - Converts HTML dashboard to C header with gzip compression
# - Builds ESP32-C6 firmware
# - Displays connection instructions
###############################################################################

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         DIAMANT v2.1 - Build Script with Dashboard          ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Step 1: Convert HTML to header
echo -e "${YELLOW}[1/3] Converting dashboard HTML to C header...${NC}"
if [ -f tools/html_to_header.py ]; then
    python3 tools/html_to_header.py main/www/dashboard.html main/www/dashboard.html.h
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ HTML converted successfully${NC}"
        
        # Display compression statistics
        if [ -f main/www/dashboard.html ] && [ -f main/www/dashboard.html.h ]; then
            ORIGINAL_SIZE=$(wc -c < main/www/dashboard.html)
            COMPRESSED_SIZE=$(grep -o 'Length: [0-9]*' main/www/dashboard.html.h | grep -o '[0-9]*')
            if [ -n "$COMPRESSED_SIZE" ]; then
                RATIO=$(echo "scale=1; 100 - ($COMPRESSED_SIZE * 100 / $ORIGINAL_SIZE)" | bc)
                echo -e "  Original: ${ORIGINAL_SIZE} bytes"
                echo -e "  Compressed: ${COMPRESSED_SIZE} bytes"
                echo -e "  Compression: ${RATIO}%"
            fi
        fi
    else
        echo -e "${RED}✗ HTML conversion failed${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}⚠ tools/html_to_header.py not found, skipping HTML conversion${NC}"
fi
echo ""

# Step 2: Build firmware
echo -e "${YELLOW}[2/3] Building ESP32-C6 firmware...${NC}"
if command -v idf.py &> /dev/null; then
    idf.py build
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Firmware built successfully${NC}"
        
        # Display binary sizes
        if [ -f build/diamant-v2.bin ]; then
            FIRMWARE_SIZE=$(wc -c < build/diamant-v2.bin)
            echo -e "  Firmware size: ${FIRMWARE_SIZE} bytes"
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ idf.py not found. Please activate ESP-IDF environment.${NC}"
    echo -e "  Run: . \$IDF_PATH/export.sh"
    exit 1
fi
echo ""

# Step 3: Display connection instructions
echo -e "${YELLOW}[3/3] Build complete! Connection instructions:${NC}"
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                  Connection Instructions                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}1. Flash firmware:${NC}"
echo -e "   $ idf.py flash monitor"
echo ""
echo -e "${BLUE}2. Connect to WiFi Access Point:${NC}"
echo -e "   SSID:     ${GREEN}DIAMANT_AP${NC}"
echo -e "   Password: ${GREEN}diamant2026${NC}"
echo ""
echo -e "${BLUE}3. Open web browser:${NC}"
echo -e "   URL: ${GREEN}http://192.168.4.1/${NC}"
echo ""
echo -e "${BLUE}4. Console commands (via USB serial):${NC}"
echo -e "   mram-info        - Display MRAM configuration"
echo -e "   mram-stats       - Show statistics and predictions"
echo -e "   mram-dump <n>    - Dump last n historical entries"
echo ""
echo -e "${BLUE}5. API endpoints:${NC}"
echo -e "   GET ${GREEN}http://192.168.4.1/api/status${NC}   - Real-time status"
echo -e "   GET ${GREEN}http://192.168.4.1/api/config${NC}   - Configuration"
echo -e "   GET ${GREEN}http://192.168.4.1/api/stats${NC}    - Statistics"
echo -e "   GET ${GREEN}http://192.168.4.1/api/history${NC}  - Historical data"
echo ""
echo -e "${GREEN}✓ Build successful!${NC}"
echo ""
