# FreeRTOS Tasks for DIAMANT v2.1

This directory contains three main FreeRTOS tasks for the ESP32-C6 DIAMANT v2.1 system.

## Tasks Overview

### 1. Fusion Task (`task_fusion.c`)
**Priority:** 3 (Highest)  
**Cycle Time:** 500ms

Performs intelligent sensor fusion and profile selection:
- Updates astronomical data (sunrise/sunset/twilight)
- Updates calendar data (season/holidays/weekend)
- Updates weather detection from sensors
- Updates BMS battery data
- Determines current time period
- Applies V2G and LED profiles using fusion engine
- Publishes global fusion data (`g_fusion_data`)

**Functions:**
```c
esp_err_t task_fusion_start(void);
void task_fusion_stop(void);
TaskHandle_t task_fusion_get_handle(void);
```

### 2. MRAM Task (`task_mram.c`)
**Priority:** 2  
**Cycle Time:** 30 seconds

Manages persistent storage operations:
- Logs data every 30 minutes (configurable)
- Calculates statistics every hour
- Updates configuration every 24 hours
- Displays predictive analytics in logs

**Functions:**
```c
esp_err_t task_mram_start(void);
void task_mram_stop(void);
TaskHandle_t task_mram_get_handle(void);
esp_err_t task_mram_log_now(void);  // Trigger immediate log
```

### 3. Web Server Task (`task_webserver.c`)
**Priority:** 2  
**Cycle Time:** Event-driven

Manages WiFi Access Point and HTTP web server:
- Initializes WiFi AP on boot
- Starts HTTP web server
- Handles client connect/disconnect events
- Monitors server health

**Functions:**
```c
esp_err_t task_webserver_start(void);
void task_webserver_stop(void);
TaskHandle_t task_webserver_get_handle(void);
bool task_webserver_is_running(void);
```

**WiFi Credentials:**
- SSID: `DIAMANT_AP`
- Password: `diamant2026`
- IP Address: `192.168.4.1`

## Usage Example

```c
#include "tasks/task_fusion.h"
#include "tasks/task_mram.h"
#include "tasks/task_webserver.h"

void app_main(void)
{
    // Initialize NVS
    nvs_flash_init();
    
    // Start all tasks
    task_fusion_start();
    task_mram_start();
    task_webserver_start();
    
    // Tasks now run independently
}
```

## Dependencies

### External Functions Required:
- `astro_calculate()` - Calculate astronomical data
- `astro_update_period()` - Update time period
- `calendar_calculate()` - Calculate calendar data
- `weather_update()` - Update weather state
- `fusion_update()` - Run fusion engine
- `mram_storage_init()` - Initialize MRAM
- `mram_config_load()` - Load configuration
- `mram_config_save()` - Save configuration
- `mram_history_add()` - Add history entry
- `mram_stats_calculate()` - Calculate statistics
- `mram_prediction_generate()` - Generate predictions
- `web_server_wifi_init()` - Initialize WiFi AP
- `web_server_start()` - Start HTTP server
- `web_server_stop()` - Stop HTTP server

### External Variables:
- `fusion_data_t g_fusion_data` - Global fusion state (exported by task_fusion.c)

## Configuration

### Fusion Task:
```c
#define FUSION_TASK_PRIORITY        3
#define FUSION_TASK_STACK_SIZE      4096
#define FUSION_TASK_CYCLE_MS        500
```

### MRAM Task:
```c
#define MRAM_TASK_PRIORITY          2
#define MRAM_TASK_STACK_SIZE        4096
#define MRAM_TASK_CYCLE_MS          30000
#define MRAM_LOG_INTERVAL_SEC       1800    // 30 minutes
#define MRAM_STATS_INTERVAL_SEC     3600    // 1 hour
#define MRAM_CONFIG_UPDATE_SEC      86400   // 24 hours
```

### Web Server Task:
```c
#define WEBSERVER_TASK_PRIORITY     2
#define WEBSERVER_TASK_STACK_SIZE   4096
```

## Task Priorities

FreeRTOS uses higher numbers for higher priority:
- **Priority 3** (Fusion): Highest - real-time sensor fusion
- **Priority 2** (MRAM & Web): Medium - periodic operations
- **Priority 1** (Optional): Low priority background tasks
- **Priority 0** (Idle): System idle task

## Memory Usage

Approximate stack usage:
- Fusion Task: 4KB
- MRAM Task: 4KB
- Web Server Task: 4KB
- **Total:** ~12KB

## Error Handling

All tasks include:
- ESP_LOG for debugging
- Proper error checking
- Graceful shutdown support
- Task state validation

## Notes

1. **Task Start Order:** Start fusion task first, then MRAM and web server
2. **BMS Simulation:** BMS data is currently simulated - replace with actual driver
3. **Temperature Sensors:** Board/heatsink temps are simulated - replace with actual sensors
4. **Global Data:** `g_fusion_data` is shared between tasks (fusion publishes, others consume)
5. **Thread Safety:** Consider adding mutexes if modifying shared data from multiple tasks

## Testing

See `tasks_example_main.c` for a complete example of initializing and monitoring all tasks.

## ESP-IDF Version

Compatible with ESP-IDF v5.0 and later for ESP32-C6.
