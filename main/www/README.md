# Web Server Module

The web server module provides a WiFi Access Point and HTTP server with a beautiful, real-time monitoring dashboard for the DIAMANT v2.1 system.

## Features

### WiFi Access Point
- **SSID**: DIAMANT_AP
- **Password**: diamant2026
- **IP Address**: 192.168.4.1
- **Maximum Connections**: 4

### REST API Endpoints

#### `GET /api/status`
Real-time system status with all sensor data:
- Weather (ambient light, temperature, PV voltage, weather state)
- Astronomical data (sunrise, sunset, twilight times, current period)
- Calendar data (season, weekend/holiday status)
- BMS data (SOC, voltage, current, temperature)
- System temperatures (board, heatsink)
- Active V2G and LED profiles

#### `GET /api/config`
MRAM configuration data:
- Device identity and location
- System configuration parameters
- Operational thresholds
- Runtime statistics

#### `GET /api/stats`
Predictive statistics:
- PV energy production (today, weekly, peak power)
- Battery statistics (cycles, health, charge/discharge)
- V2G export/import statistics
- LED energy consumption
- System uptime and errors
- Predictive maintenance forecasting

#### `GET /api/history?hours=N`
Historical data query (default: last hour):
- Time-series sensor data
- Battery SOC and temperature history
- Power flow history
- LED intensity history

#### `GET /`
Serves the interactive web dashboard

### Web Dashboard

Beautiful, modern dashboard featuring:

#### Design
- **Dark Theme**: Gradient background (#0a0e27 to #1a1f3a)
- **Glass-morphism Cards**: Transparent cards with backdrop blur
- **Responsive Grid Layout**: Mobile-friendly design
- **Color Palette**: 
  - Blue (#3b82f6) - Primary UI elements
  - Green (#10b981) - Success states
  - Yellow (#f59e0b) - Warnings
  - Red (#ef4444) - Alerts

#### Metric Cards (6 cards)
1. **Solar PV**: PV voltage, ambient light, weather state
2. **Battery BMS**: SOC, voltage/current, temperature
3. **LED System**: Active profile, warm/cool LED intensity
4. **V2G System**: Active profile, power target, mode
5. **Astronomical**: Current period, sunrise/sunset, season
6. **System Stats**: Uptime, board/heatsink temperatures

#### Chart Cards (3 interactive charts)
1. **Power Flow History**: V2G power (W) and PV voltage (V)
2. **Battery SOC & Temperature**: Dual Y-axis chart
3. **LED & Ambient Light**: LED intensities and ambient lux

#### Features
- Real-time updates every 2 seconds
- Historical chart updates every 30 seconds
- Connection status indicator with pulse animation
- Loading animations
- Smooth chart transitions with Chart.js

## Build Process

### Prerequisites
- Python 3.x (for HTML-to-header conversion)
- ESP-IDF (for compiling ESP32 code)

### Generate Dashboard Header

When you modify the dashboard HTML, regenerate the C header:

```bash
python3 tools/html_to_header.py main/www/dashboard.html main/www/dashboard.html.h
```

This will:
1. Read `dashboard.html`
2. Compress with gzip (level 9)
3. Generate `dashboard.html.h` with embedded compressed data
4. Typical compression: ~82% (22KB → 4KB)

### Integration

Include the web server in your main application:

```c
#include "include/web_server.h"

// In app_main():
web_server_wifi_init();  // Initialize WiFi AP
web_server_start();      // Start HTTP server
```

### External Dependencies

The web server requires:
- `fusion_data_t g_fusion_data` - Global fusion state (from fusion.c)
- MRAM storage functions (from mram_storage.c)
- Profile functions (from profiles.h/fusion.c)

## Usage

1. **Power on the ESP32-C6**
2. **Connect to WiFi AP**: 
   - SSID: `DIAMANT_AP`
   - Password: `diamant2026`
3. **Open Dashboard**: 
   - Navigate to `http://192.168.4.1/` in any web browser
4. **Monitor Real-time Data**:
   - Metrics update every 2 seconds
   - Charts update every 30 seconds
5. **Query API Endpoints**:
   - Status: `http://192.168.4.1/api/status`
   - Config: `http://192.168.4.1/api/config`
   - Stats: `http://192.168.4.1/api/stats`
   - History: `http://192.168.4.1/api/history?hours=2`

## File Structure

```
main/
├── web_server.c              # HTTP server implementation
├── include/
│   └── web_server.h          # Web server header
└── www/
    ├── dashboard.html        # Dashboard HTML source
    └── dashboard.html.h      # Generated compressed header

tools/
└── html_to_header.py         # Build tool for HTML compression
```

## API Response Examples

### Status Response
```json
{
  "device_id": "DIAMANT_v2.1_REV_F",
  "timestamp": 1707337200,
  "uptime_sec": 3600,
  "weather": {
    "state": "Clear",
    "ambient_lux": 45000.0,
    "temperature_c": 25.3,
    "pv_voltage": 18.5,
    "pv_oscillating": false
  },
  "bms": {
    "soc_percent": 75.5,
    "voltage": 12.6,
    "current": 2.3,
    "temperature": 22.1
  },
  "v2g": {
    "id": 3,
    "name": "Solar Export Maximum",
    "power_target": 500
  }
}
```

### History Response
```json
[
  {
    "timestamp": 1707337200,
    "ambient_lux": 45000,
    "temperature": 25.3,
    "pv_voltage": 18.5,
    "bms_soc": 75.5,
    "v2g_power": 500,
    "led_warm": 0,
    "led_cool": 0
  }
]
```

## Security Considerations

- Change default WiFi password in production
- Implement authentication for sensitive endpoints
- Use HTTPS for production deployments
- Limit API access to trusted networks
- Monitor for unauthorized access attempts

## Troubleshooting

### Dashboard Won't Load
- Verify WiFi connection to `DIAMANT_AP`
- Check ESP32 serial logs for errors
- Ensure `dashboard.html.h` is generated correctly

### API Returns 500 Error
- Check MRAM initialization status
- Verify fusion data is being updated
- Check ESP32 logs for detailed error messages

### Charts Not Updating
- Verify `/api/history` endpoint returns data
- Check browser console for JavaScript errors
- Ensure historical logging is enabled

## Future Enhancements

- WebSocket support for real-time updates
- User authentication and authorization
- Configuration editing through web interface
- Data export functionality (CSV, JSON)
- Mobile app integration via REST API
- OTA firmware updates via web interface
