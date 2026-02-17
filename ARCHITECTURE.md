# DIAMANT v2.1 REV E - System Architecture

## 🏗️ High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      ESP32-C6-MINI-1-N4 (RISC-V 160MHz)                │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                      FreeRTOS Kernel                             │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                    │
│  │ Task Fusion │  │  Task MRAM  │  │Task WebServ │                    │
│  │ Priority: 3 │  │ Priority: 2 │  │ Priority: 2 │                    │
│  │  500ms ⏱    │  │   30s ⏱     │  │Event-driven │                    │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘                    │
│         │                │                 │                           │
│         ▼                ▼                 ▼                           │
│  ┌──────────────────────────────────────────────────┐                  │
│  │            Global Fusion Data Structure          │                  │
│  │  ┌────────┬────────┬─────────┬────────┬────────┐ │                  │
│  │  │Weather │Calendar│  Astro  │  BMS   │ System │ │                  │
│  │  │  Data  │  Data  │  Data   │  Data  │  Data  │ │                  │
│  │  └────────┴────────┴─────────┴────────┴────────┘ │                  │
│  └──────────────────────────────────────────────────┘                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
            │                    │                    │
            ▼                    ▼                    ▼
      ┌─────────┐          ┌─────────┐         ┌─────────┐
      │ I2C Bus │          │ MRAM    │         │ WiFi AP │
      │ 400kHz  │          │ Storage │         │  HTTP   │
      └────┬────┘          └────┬────┘         └────┬────┘
           │                    │                   │
    ┌──────┴──────┐       ┌─────┴─────┐       ┌────┴────┐
    ▼      ▼      ▼       ▼           ▼       ▼         ▼
  TSL   TMP    ADS     AT24C256    Config   REST     Web
  2591  117×2  1115    32KB EEPROM  CRC16   API    Dashboard
  Lux   Temp   ADC     Circular Buf Stats  JSON    Chart.js
```

## 📊 Data Flow Diagram

```
                        ┌─────────────────────┐
                        │   External World    │
                        │  (Sensors, Grid)    │
                        └──────────┬──────────┘
                                   │
                    ┌──────────────┼──────────────┐
                    │              │              │
                    ▼              ▼              ▼
            ┌────────────┐  ┌────────────┐  ┌────────────┐
            │  I2C Bus   │  │   BMS      │  │    RTC     │
            │  Sensors   │  │  (SPI)     │  │  (Time)    │
            └──────┬─────┘  └──────┬─────┘  └──────┬─────┘
                   │                │                │
                   └────────────────┼────────────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │   TASK FUSION         │
                        │   (500ms cycle)       │
                        │                       │
                        │  ┌─────────────────┐  │
                        │  │ Astro Engine    │  │
                        │  └─────────────────┘  │
                        │  ┌─────────────────┐  │
                        │  │ Calendar Engine │  │
                        │  └─────────────────┘  │
                        │  ┌─────────────────┐  │
                        │  │ Weather Engine  │  │
                        │  └─────────────────┘  │
                        │  ┌─────────────────┐  │
                        │  │ Fusion Logic    │  │
                        │  │ • V2G Profile   │  │
                        │  │ • LED Profile   │  │
                        │  └─────────────────┘  │
                        └───────────┬───────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                    ▼               ▼               ▼
            ┌────────────┐  ┌────────────┐  ┌────────────┐
            │ TASK MRAM  │  │TASK WEBSRV │  │  Actuators │
            │  (30s)     │  │(Event)     │  │  (V2G/LED) │
            │            │  │            │  │            │
            │ • Log Data │  │ • WiFi AP  │  │ • LT8708   │
            │ • Stats    │  │ • REST API │  │ • LT8391   │
            │ • Predict  │  │ • WebSocket│  │            │
            └──────┬─────┘  └──────┬─────┘  └────────────┘
                   │                │
                   ▼                ▼
            ┌────────────┐  ┌────────────┐
            │  AT24C256  │  │   Clients  │
            │   EEPROM   │  │ (Web/REST) │
            │   32KB     │  │            │
            │            │  │ • Browser  │
            │ • Config   │  │ • Mobile   │
            │ • History  │  │ • External │
            │ • Stats    │  │   Systems  │
            └────────────┘  └────────────┘
```

## 🔄 Profile Selection Flow

```
                    ┌────────────────────────┐
                    │   Sensor Inputs        │
                    │  • Weather State       │
                    │  • Time Period         │
                    │  • Calendar Data       │
                    │  • BMS State           │
                    └───────────┬────────────┘
                                │
                                ▼
                    ┌────────────────────────┐
                    │   Priority Evaluation  │
                    │   (Highest to Lowest)  │
                    └───────────┬────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ V2G Profiles  │     │ LED Profiles  │     │ System State  │
│               │     │               │     │               │
│ Pri 255: EMERG│     │ Pri 255: FAULT│     │ • Uptime      │
│ Pri 200: PEAK │     │ Pri 200: SUNSET│     │ • Temperature │
│ Pri 180: SOLAR│     │ Pri 190: TWILIT│     │ • Errors      │
│ Pri 160: NIGHT│     │ Pri 180: ECO  │     │               │
│ ...           │     │ ...           │     │               │
│ Pri 50: DFLT  │     │ Pri 50: DFLT  │     │               │
└───────┬───────┘     └───────┬───────┘     └───────────────┘
        │                     │
        └──────────┬──────────┘
                   │
                   ▼
        ┌────────────────────────┐
        │  Active Profile        │
        │  Application           │
        │                        │
        │  • V2G: Power Target   │
        │  • LED: Intensities    │
        │  • Logging: Reason     │
        └────────────┬───────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │  Hardware Actuation    │
        │                        │
        │  • LT8708 (V2G)        │
        │  • LT8391 (LED)        │
        └────────────────────────┘
```

## 🗄️ MRAM Storage Architecture

```
AT24C256 (32KB) Memory Map
┌────────────────────────────────────────────────┐ 0x0000
│  Configuration Header (256 bytes)              │
│  ┌──────────────────────────────────────────┐  │
│  │ Magic: 0x44494D41 ("DIMA")               │  │
│  │ Version: 1                                │  │
│  │ CRC16: Checksum                           │  │
│  │ Device ID, Location, Coordinates          │  │
│  │ Thresholds, Intervals, Counters           │  │
│  │ History Buffer State (write idx, count)   │  │
│  └──────────────────────────────────────────┘  │
├────────────────────────────────────────────────┤ 0x0100
│  Client Profiles (256 bytes)                   │
│  ┌──────────────────────────────────────────┐  │
│  │ Profile 0: 64 bytes                       │  │
│  │ Profile 1: 64 bytes                       │  │
│  │ Profile 2: 64 bytes                       │  │
│  │ Profile 3: 64 bytes                       │  │
│  └──────────────────────────────────────────┘  │
├────────────────────────────────────────────────┤ 0x0200
│  Historical Logs (7680 bytes)                  │
│  Circular Buffer: 240 entries × 32 bytes       │
│  ┌──────────────────────────────────────────┐  │
│  │ Entry 0:  timestamp, sensors, profiles    │  │
│  │ Entry 1:  ...                             │  │
│  │ Entry 2:  ...                             │  │
│  │ ...                                       │  │
│  │ Entry 239: ...                            │  │
│  └──────────────────────────────────────────┘  │
│  (Wraps around, oldest overwritten)            │
├────────────────────────────────────────────────┤ 0x2000
│  Prediction Models (4KB)                       │
│  • PV Production Coefficients                  │
│  • Battery Health Models                       │
│  • Maintenance Predictions                     │
├────────────────────────────────────────────────┤ 0x3000
│  Maintenance Logs (4KB)                        │
│  • Event timestamps                            │
│  • Maintenance actions                         │
│  • Error logs                                  │
├────────────────────────────────────────────────┤ 0x4000
│  Extended Data (16KB)                          │
│  • D4i protocol data                           │
│  • OTA update metadata                         │
│  • User configurations                         │
└────────────────────────────────────────────────┘ 0x7FFF
```

## 🌐 Web Server Architecture

```
┌──────────────────────────────────────────────────────────┐
│                     WiFi Access Point                    │
│              SSID: DIAMANT_AP (192.168.4.1)             │
└───────────────────────────┬──────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────┐
│                    ESP HTTP Server                       │
│                  (esp_http_server)                       │
└───────────────────────────┬──────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ GET /        │  │ GET /api/*   │  │ POST /api/*  │
│              │  │              │  │              │
│ Dashboard    │  │ REST API     │  │ Config       │
│ HTML         │  │ Endpoints    │  │ Update       │
│              │  │              │  │              │
│ • Gzipped    │  │ • /status    │  │ (Future)     │
│ • 4KB        │  │ • /config    │  │              │
│ • Embedded   │  │ • /stats     │  │              │
│              │  │ • /history   │  │              │
└──────────────┘  └──────┬───────┘  └──────────────┘
                         │
                         ▼
              ┌────────────────────┐
              │  cJSON Serializer  │
              │                    │
              │ • Real-time data   │
              │ • Historical data  │
              │ • Statistics       │
              │ • Predictions      │
              └──────────┬─────────┘
                         │
                         ▼
              ┌────────────────────┐
              │   HTTP Response    │
              │                    │
              │ • Content-Type     │
              │ • CORS Headers     │
              │ • Cache Control    │
              │ • Gzip Encoding    │
              └────────────────────┘
```

## 📱 Dashboard UI Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    DIAMANT v2.1 Dashboard                   │
│                    ──────────────────────                   │
│                    🟢 Connected                             │
└─────────────────────────────────────────────────────────────┘
┌───────────────────┐ ┌───────────────────┐ ┌───────────────┐
│   Solar PV        │ │  Battery BMS      │ │  LED System   │
│                   │ │                   │ │               │
│ 🌞 Power: 150 W   │ │ 🔋 SOC: 75.5%    │ │ 💡 Warm: 80%  │
│ ⚡ Voltage: 42.5V │ │ ⚡ Voltage: 50.4V │ │ 💡 Cool: 60%  │
│ ☀️  Lux: 35000    │ │ ⚡ Current: 3.2A  │ │ 📋 Profile:   │
│ 🌤️  Weather: Clear│ │ 🌡️  Temp: 25.8°C  │ │    TWILIGHT   │
└───────────────────┘ └───────────────────┘ └───────────────┘
┌───────────────────┐ ┌───────────────────┐ ┌───────────────┐
│   V2G System      │ │  Astronomical     │ │ System Stats  │
│                   │ │                   │ │               │
│ ↔️  Power: 100W   │ │ 🌅 Sunrise: 06:30 │ │ ⏱  Uptime:    │
│ 📊 Mode: EXPORT   │ │ 🌇 Sunset: 20:15  │ │    24h 15m    │
│ 📋 Profile:       │ │ 🌆 Period:        │ │ 🌡️  Board:     │
│    SOLAR_EXPORT   │ │    TWILIGHT       │ │    35.2°C     │
└───────────────────┘ └───────────────────┘ └───────────────┘
┌─────────────────────────────────────────────────────────────┐
│              📈 Power Flow Chart (24h)                      │
│  400W ┤                          ╱╲                         │
│       │                         ╱  ╲                        │
│  200W ┤        ╱╲              ╱    ╲                       │
│       │       ╱  ╲            ╱      ╲                      │
│     0 ┼──────╱────╲──────────╱────────╲────────────────    │
│       │           ╲        ╱            ╲                   │
│ -200W ┤            ╲      ╱              ╲                  │
│       └────────────────────────────────────────────────────│
│         PV Power (yellow)   V2G Power (blue)                │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│           📉 Battery SOC & Temperature (24h)                │
│  100% ┤──────────────────────────────                       │
│   75% ┤                          ╱──────      45°C ┤        │
│   50% ┤               ╱─────────╱             30°C ┤────    │
│   25% ┤     ╱────────╱                        15°C ┤        │
│    0% ┴────────────────────────────────            └────────│
│         SOC (green, left)   Temperature (red, right)        │
└─────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────┐
│         💡 LED Output & Ambient Light (24h)                 │
│  100% ┤                                                      │
│   75% ┤    ╱────╲                                           │
│   50% ┤───╱      ╲─────                  60k ┤──────╲       │
│   25% ┤                ╲────                 ┤       ╲      │
│    0% ┴─────────────────────╲───         0k ┴────────╲──   │
│     Warm (orange)  Cool (cyan)  Ambient Lux (purple, right)│
└─────────────────────────────────────────────────────────────┘
```

## 🔧 Build & Deployment Flow

```
┌────────────────────┐
│   Source Code      │
│   • main/*.c       │
│   • www/*.html     │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│ html_to_header.py  │
│ • Read HTML        │
│ • Gzip compress    │
│ • Generate .h      │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│   idf.py build     │
│   • CMake          │
│   • Ninja          │
│   • ESP-IDF        │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│  diamant-v2.bin    │
│  • Firmware        │
│  • ~800KB          │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│   idf.py flash     │
│   • Serial         │
│   • USB-C          │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│  ESP32-C6 Device   │
│  • Boot            │
│  • WiFi AP         │
│  • HTTP Server     │
└────────────────────┘
```

---

**Document Version**: 1.0  
**Last Updated**: 2026-02-17  
**Status**: ✅ Complete
