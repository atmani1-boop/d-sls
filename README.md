# DIAMANT v2.1 - Multi-Sensor Fusion System

ESP32-C6 based intelligent lighting control system with multi-sensor fusion, MRAM storage, and web interface.

## Features

- **Multi-Sensor Fusion**: Combines astronomical calculations, weather data, and calendar events
- **Profile-Based Control**: 8 distinct lighting profiles (Work, Gaming, Sleep, etc.)
- **Persistent Storage**: MRAM-based data logging with I2C EEPROM
- **Web Dashboard**: Real-time monitoring and configuration via WiFi
- **CLI Interface**: Console commands for system management and MRAM operations
- **FreeRTOS Tasks**: Three concurrent tasks for fusion, storage, and web serving

## Hardware Requirements

- ESP32-C6 Development Board
- AT24C256 EEPROM (I2C, 32KB)
- I2C connections: GPIO20 (SDA), GPIO21 (SCL)
- WiFi connectivity for web interface

## Build System

### Prerequisites

- ESP-IDF v5.0 or later
- CMake 3.16+
- Python 3.7+

### Configuration

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure project (optional - uses sdkconfig.defaults)
idf.py menuconfig

# Set WiFi credentials in "DIAMANT v2.1 WiFi Configuration"
```

### Building

```bash
# Build the project
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash

# Monitor output
idf.py -p /dev/ttyUSB0 monitor
```

## Project Structure

```
.
├── CMakeLists.txt              # Root project configuration
├── sdkconfig.defaults          # Default SDK configuration
├── main/
│   ├── CMakeLists.txt          # Component build configuration
│   ├── Kconfig.projbuild       # WiFi configuration menu
│   ├── main.c                  # Application entry point
│   ├── astro.c                 # Astronomical calculations
│   ├── calendar.c              # Calendar/event management
│   ├── weather.c               # Weather data processing
│   ├── fusion.c                # Multi-sensor fusion engine
│   ├── mram_storage.c          # MRAM storage operations
│   ├── cli_mram.c              # CLI commands for MRAM
│   ├── web_server.c            # HTTP server implementation
│   ├── drivers/
│   │   └── at24c256.c          # I2C EEPROM driver
│   ├── include/                # Header files
│   ├── tasks/                  # FreeRTOS task implementations
│   │   ├── task_fusion.c       # Fusion task (500ms cycle)
│   │   ├── task_mram.c         # MRAM task (30s cycle)
│   │   └── task_webserver.c    # Web server task
│   └── www/
│       └── dashboard.html      # Web dashboard (embedded)
└── tools/
    └── html_to_header.py       # HTML to C header converter
```

## System Architecture

### Task Priorities

1. **Fusion Task** (Priority 3, 500ms cycle)
   - Reads sensor data
   - Performs profile selection
   - Updates global state

2. **Web Server Task** (Priority 2)
   - Manages HTTP server
   - Serves dashboard
   - Handles REST API

3. **MRAM Task** (Priority 2, 30s cycle)
   - Logs historical data
   - Calculates statistics
   - Manages predictive analytics

### Initialization Sequence

1. NVS initialization
2. I2C bus setup (GPIO20/21, 400kHz)
3. MRAM storage initialization
4. WiFi STA mode connection
5. Task creation (MRAM → WebServer → Fusion)
6. Console initialization
7. CLI command registration

## CLI Commands

Access the console via serial port (115200 baud):

```
# MRAM Operations
mram status              # Show MRAM status and statistics
mram read <addr> <len>   # Read from MRAM address
mram write <addr> <val>  # Write to MRAM address
mram clear               # Clear all MRAM data
mram test                # Run MRAM test pattern

# System Commands
help                     # Show all available commands
restart                  # Restart the system
```

## Web Interface

After connecting to WiFi, access the dashboard at:

```
http://<esp32-ip>/
```

The dashboard displays:
- Current profile and sensor states
- Real-time fusion data
- Historical statistics
- System configuration

## Configuration

### WiFi Settings

Edit in `sdkconfig.defaults` or via menuconfig:

```
CONFIG_ESP_WIFI_SSID="DIAMANT"
CONFIG_ESP_WIFI_PASSWORD="diamant2024"
CONFIG_ESP_MAXIMUM_RETRY=5
```

### Task Cycle Times

Edit in task header files:

```c
#define FUSION_TASK_CYCLE_MS    500     // Fusion cycle
#define MRAM_TASK_CYCLE_MS      30000   // MRAM cycle
#define MRAM_LOG_INTERVAL_SEC   1800    // Log interval (30 min)
```

### I2C Configuration

Edit in `main.c`:

```c
#define I2C_MASTER_SCL_IO       21      // SCL GPIO
#define I2C_MASTER_SDA_IO       20      // SDA GPIO
#define I2C_MASTER_FREQ_HZ      400000  // Clock frequency
```

## Development

### Adding New Profiles

Edit `main/include/profiles.h` and add to the `LightingProfile` enum.

### Modifying Fusion Logic

Edit `main/fusion.c` - implements `calculate_lighting_profile()`.

### Adding CLI Commands

1. Add command handler in `main/cli_mram.c`
2. Register in `register_mram_commands()`

### Updating Web Dashboard

1. Edit `main/www/dashboard.html`
2. Run `tools/html_to_header.py` to regenerate header
3. Rebuild project

## Troubleshooting

### Build Errors

```bash
# Clean build
idf.py fullclean
idf.py build
```

### MRAM Not Detected

- Check I2C connections (GPIO20/21)
- Verify EEPROM address (0x50)
- Check pull-up resistors on SDA/SCL

### WiFi Connection Failed

- Verify SSID and password in menuconfig
- Check WiFi auth mode (WPA2-PSK default)
- Increase retry count in configuration

## License

See DIAMANT_v2.1_REV_F_SPECIFICATION.txt for system specifications.

## References

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [ESP32-C6 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf)
- [AT24C256 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/doc0670.pdf)