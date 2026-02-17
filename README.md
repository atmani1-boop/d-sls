# DIAMANT v2.1 REV E - Fusion System

Advanced solar LED controller with 35-profile specification, Night-Saver algorithm, and dynamic CCT control.

## 🎯 Features

### Profile System
- **35 unique profiles** with hexadecimal IDs (0x00-0x62)
- **9 LED profiles** (0x00-0x09): Adaptive lighting with CCT control
- **7 MPPT profiles** (0x10-0x16): Solar charge optimization
- **7 V2G profiles** (0x20-0x26): Bidirectional grid integration
- **4 Battery profiles** (0x30-0x33): Thermal protection
- **4 Weather Detection profiles** (0x40-0x43): Environmental adaptation
- **3 Astronomical profiles** (0x50-0x52): Sun position calculations
- **3 Calendar profiles** (0x60-0x62): Seasonal adjustments

### Intelligent Algorithms
- **Night-Saver**: SOC-based LED dimming (30-100% range)
  - ≥60% SOC → 100% brightness
  - 40-59% SOC → 80% brightness
  - 20-39% SOC → 60% brightness
  - <20% SOC → 30% brightness (minimum)
- **Dynamic CCT Control**: Warm (2700K) to Cool (5000K) LED mixing
- **Priority-Based Fusion**: P0-P7 priority system for conflict resolution

### Hardware Support
- ESP32-C6-MINI-1-N4 (RISC-V 160MHz)
- Dual LT8391 LED drivers (400W total)
- BQ76952 BMS (3-16S battery support)
- LT8708 MPPT + V2G converters
- 8 I²C sensors (light, temperature, RTC, etc.)

## 📁 Project Structure

```
d-sls/
├── main/
│   ├── include/
│   │   ├── profiles.h         # All 35 profile definitions
│   │   └── mram_storage.h     # Typed log structures
│   ├── tasks/
│   │   └── task_led.c         # LED control with Night-Saver + CCT
│   ├── www/
│   │   └── dashboard.html     # Web dashboard UI
│   ├── night_saver.c          # SOC-based dimming algorithm
│   ├── cct_control.c          # Color temperature mixing
│   ├── fusion.c               # Priority-based profile selection
│   ├── web_server.c           # HTTP API server
│   ├── main.c                 # Application entry point
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

## 🚀 Quick Start

### Prerequisites
- ESP-IDF v5.0 or later
- ESP32-C6 development board or DIAMANT hardware

### Build
```bash
# Set up ESP-IDF environment
. $IDF_PATH/export.sh

# Configure project
idf.py set-target esp32c6

# Build firmware
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash monitor
```

### Testing (without hardware)
```bash
# Compile test version
gcc -o test_diamant \
    main/main.c \
    main/night_saver.c \
    main/cct_control.c \
    main/fusion.c \
    -I main -lm

# Run tests
./test_diamant
```

## 📊 Profile Details

### LED Profiles (0x00-0x09)
| ID   | Name            | CCT  | Intensity | Priority | Night-Saver |
|------|-----------------|------|-----------|----------|-------------|
| 0x00 | SUNSET_ON       | 2700K| 100%      | P5       | ✅          |
| 0x01 | TWILIGHT_SOFT   | 2800K| 80%       | P5       | ✅          |
| 0x02 | MIDNIGHT_ECO    | 3000K| 60%       | P5       | ✅          |
| 0x03 | EARLY_MORNING   | 4000K| 40%       | P5       | ✅          |
| 0x04 | FOG_BOOST       | 5000K| 100%      | P2       | ❌          |
| 0x05 | CLEAR_NIGHT     | 3000K| 60%       | P5       | ✅          |
| 0x06 | LOW_SOC_PROTECT | 2700K| 30%       | P3       | ❌          |
| 0x07 | WINTER_MODE     | 3000K| 70%       | P6       | ✅          |
| 0x08 | SUMMER_MODE     | 2700K| 50%       | P6       | ✅          |
| 0x09 | RAIN_ALERT      | 5000K| 90%       | P2       | ❌          |

### Priority System
- **P0**: Emergency (island mode, critical SOC)
- **P1**: Thermal limits
- **P2**: Weather safety (fog, rain, heat)
- **P3**: SOC protection
- **P4**: Battery protection
- **P5**: Astronomical events
- **P6**: Calendar events
- **P7**: Default/balanced operation

## 🌐 Web Dashboard

Access the dashboard at `http://diamant.local` (or device IP) to monitor:
- Active LED profile with CCT and Night-Saver status
- MPPT profile and solar charging parameters
- V2G profile and grid power flow
- Battery SOC, voltage, and current
- Real-time profile switching based on conditions

### API Endpoints
- `GET /api/profiles` - Get all active profile information
- `GET /api/status` - Legacy endpoint (redirects to /api/profiles)

## 🔧 Configuration

### Night-Saver Thresholds
Edit `main/night_saver.c` to customize SOC thresholds:
```c
float night_saver_get_soc_factor(float soc_percent) {
    if (soc_percent >= 60.0f) return 1.0f;   // Full brightness
    else if (soc_percent >= 40.0f) return 0.8f;  // 80%
    else if (soc_percent >= 20.0f) return 0.6f;  // 60%
    else return 0.3f;  // 30% minimum
}
```

### CCT Range
Edit `main/cct_control.c` to adjust color temperature range:
```c
#define CCT_WARM 2700  // Warm LED
#define CCT_COOL 5000  // Cool LED
```

## 📝 MRAM Logging

Structured logging with typed entries:
- **Type 1**: MPPT tracking data
- **Type 2**: V2G power flow
- **Type 3**: BMS battery data
- **Type 7**: Thermal monitoring
- **Type 9**: Astronomical events
- **Type 15**: System events

Each log entry includes:
- Timestamp
- Profile ID (0x00-0x62)
- Priority level (P0-P7)
- Type-specific data

## 🧪 Testing

### Unit Tests
```bash
# Test Night-Saver algorithm
./test_diamant

Expected output:
  SOC 65%, base 100% → 100% (expected 100%)
  SOC 45%, base 100% → 80% (expected 80%)
  SOC 25%, base 100% → 60% (expected 60%)
  SOC 15%, base 100% → 30% (expected 30%)
```

### Hardware Tests
1. Monitor LED PWM outputs on GPIO18 (warm) and GPIO19 (cool)
2. Verify CCT transitions at different profile switches
3. Test Night-Saver by varying battery SOC
4. Confirm priority-based profile selection

## 📄 License

Copyright (c) 2026 D-SLS SRL, Milan

## 🤝 Contributing

This is a production firmware project. Contact D-SLS for contribution guidelines.

## 📧 Support

For technical support or hardware inquiries, contact: info@d-sls.com