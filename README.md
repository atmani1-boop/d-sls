# d-sls

**DIAMANT v2.1** — Smart Street Lighting System by D-SLS SRL · Milan

ESP32-C6 based outdoor luminaire controller with bidirectional PV/battery/grid
power management and tunable-white LED output (2× 200 W).

---

## TALQ Conformance

The `talq/` directory implements the
[TALQ Smart City Protocol v2.3](https://github.com/TALQ-consortium/TALQ_specification)
gateway interface.  It allows any TALQ-compliant Central Management Software
(CMS) to discover, configure, monitor and control the luminaire over HTTP.

### Architecture mapping

| TALQ concept    | D-SLS hardware                                    |
|-----------------|---------------------------------------------------|
| Gateway         | D-SLS DIAMANT v2.1 (ESP32-C6)                    |
| Node            | Single tunable-white luminaire (400 W total)      |
| Device          | LT8391 #1 (warm 2700 K, 200 W) + LT8391 #2 (cool 5000 K, 200 W) |
| Program Group   | 24-hour dimming/CCT schedule                      |
| Calendar        | Date-range selector for a program group           |

### REST API endpoints

All endpoints are served on port 80 under the `/talq` base path.

| Method | Path                          | Description                          |
|--------|-------------------------------|--------------------------------------|
| GET    | `/talq`                       | Gateway identity and capabilities    |
| GET    | `/talq/nodes`                 | List all nodes                       |
| GET    | `/talq/nodes/{id}`            | Get node state and sensor readings   |
| PUT    | `/talq/nodes/{id}`            | Update node (activate program group) |
| GET    | `/talq/devices`               | List all devices (LED drivers)       |
| GET    | `/talq/devices/{id}`          | Get device state                     |
| GET    | `/talq/programGroups`         | List program groups                  |
| POST   | `/talq/programGroups`         | Create program group                 |
| GET    | `/talq/programGroups/{id}`    | Get program group                    |
| PUT    | `/talq/programGroups/{id}`    | Update program group                 |
| GET    | `/talq/calendars`             | List calendars                       |
| POST   | `/talq/calendars`             | Create calendar                      |
| GET    | `/talq/calendars/{id}`        | Get calendar                         |
| PUT    | `/talq/calendars/{id}`        | Update calendar                      |
| POST   | `/talq/commands`              | Send a command to a node             |
| GET    | `/talq/events`                | Retrieve event log (supports `?since=<seqNo>`) |

### Supported commands

| Command                  | Parameters                                  |
|--------------------------|---------------------------------------------|
| `commandSetAbsoluteLevel`| `talq:level` (0–1), `talq:cct` (K), `talq:fadeTime` (s) |
| `commandActivateGroup`   | `talq:programGroupId`                       |
| `commandReset`           | —  (clears faults)                          |
| `commandIdentify`        | —  (flashes fixture 3 × 100 ms)             |

### Integration (ESP-IDF)

```c
#include "talq/talq.h"

// Platform callbacks (implement these for your hardware)
void hw_set_level(uint8_t channel, float level, float fade_s) { /* PWM */ }
void hw_read_sensors(talq_node_t *n)  { /* I²C sensor read */ }
void hw_get_time(char *buf, size_t l) { /* DS3231 RTC read */ }

void app_main(void)
{
    // ... WiFi init ...
    talq_init(hw_set_level, hw_read_sensors, hw_get_time);
    talq_httpd_start();

    while (1) {
        talq_tick();          // call every 500 ms from a FreeRTOS task
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

Add `talq` to your top-level `CMakeLists.txt`:

```cmake
set(EXTRA_COMPONENT_DIRS talq)
```

### Default data model

On first boot the gateway seeds a default street-lighting program group:

| Time  | Level | CCT    | Fade |
|-------|-------|--------|------|
| 22:00 | 100 % | 2700 K | 30 s |
| 23:00 |  70 % | 3000 K | 30 s |
| 02:00 |  40 % | 3500 K | 30 s |
| 05:00 |  70 % | 4000 K | 30 s |
| 06:00 |   0 % | —      | 30 s |

### Files

| File                  | Description                                      |
|-----------------------|--------------------------------------------------|
| `talq/talq.h`         | Data structures, constants, and public API       |
| `talq/talq.c`         | Gateway state management + HTTP REST server      |
| `talq/CMakeLists.txt` | ESP-IDF component build file                     |