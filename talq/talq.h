/*
 * talq.h — TALQ Smart City Protocol v2.3 conformance layer
 *
 * Implements the TALQ gateway interface for the D-SLS DIAMANT v2.1 device.
 * The gateway exposes a RESTful HTTP API that allows a Central Management
 * Software (CMS) to discover, configure, monitor, and control the luminaire.
 *
 * Architecture mapping:
 *   TALQ Gateway  →  D-SLS DIAMANT v2.1 (ESP32-C6)
 *   TALQ Node     →  Single tunable-white luminaire (400 W)
 *   TALQ Device   →  LT8391 #1 (warm 2700 K) + LT8391 #2 (cool 5000 K)
 *   TALQ Program  →  Dimming/CCT schedule entry (level 0-100 %, CCT)
 *   TALQ Calendar →  Date-based mapping of program groups to active time
 *
 * Specification reference: TALQ Consortium specification 2.3.0
 * https://github.com/TALQ-consortium/TALQ_specification
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ─── Version ───────────────────────────────────────────────────────────── */

#define TALQ_SPEC_VERSION   "2.3.0"
#define TALQ_IMPL_VERSION   "1.0.0"
#define TALQ_BASE_PATH      "/talq"

/* ─── Limits ────────────────────────────────────────────────────────────── */

#define TALQ_MAX_PROGRAMS_PER_GROUP  24   /* max dimming steps per group    */
#define TALQ_MAX_PROGRAM_GROUPS       8   /* max concurrent program groups  */
#define TALQ_MAX_CALENDARS            4   /* max active calendars           */
#define TALQ_MAX_EVENTS             128   /* event ring-buffer size         */
#define TALQ_UUID_LEN                37   /* "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0" */

/* ─── LED channel index ─────────────────────────────────────────────────── */

#define TALQ_CHANNEL_WARM   0   /* LT8391 #1 · 2700 K */
#define TALQ_CHANNEL_COOL   1   /* LT8391 #2 · 5000 K */
#define TALQ_CHANNEL_COUNT  2

/* ─── Types ─────────────────────────────────────────────────────────────── */

/** ISO-8601 timestamp string "YYYY-MM-DDTHH:MM:SSZ\0" */
typedef char talq_timestamp_t[21];

/** Universally unique identifier string "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0" */
typedef char talq_uuid_t[TALQ_UUID_LEN];

/**
 * TALQ dimming program step.
 * Represents one time slot within a 24-hour dimming schedule.
 */
typedef struct {
    uint8_t  hour;          /**< UTC hour 0-23                              */
    uint8_t  minute;        /**< UTC minute 0-59                            */
    float    level;         /**< Relative light output 0.0-1.0 (0-100 %)   */
    uint16_t cct_kelvin;    /**< Colour temperature 2700-5000 K (0=bypass)  */
    float    fadeTime;      /**< Transition duration in seconds (≥0)        */
} talq_program_step_t;

/**
 * TALQ Program Group.
 * A named collection of program steps that define a 24-hour dimming curve.
 */
typedef struct {
    talq_uuid_t         id;
    char                name[64];
    char                description[128];
    uint8_t             stepCount;
    talq_program_step_t steps[TALQ_MAX_PROGRAMS_PER_GROUP];
} talq_program_group_t;

/**
 * TALQ Calendar entry.
 * Associates a program group with a date range.
 */
typedef struct {
    talq_uuid_t      id;
    char             name[64];
    talq_uuid_t      programGroupId;
    char             validFrom[11];   /**< "YYYY-MM-DD\0"   */
    char             validTo[11];     /**< "YYYY-MM-DD\0"   */
    uint8_t          daysOfWeek;      /**< Bitmask Mon=bit0 … Sun=bit6 (0=all) */
} talq_calendar_t;

/** Operational status of the luminaire node */
typedef enum {
    TALQ_NODE_STATUS_OK          = 0,
    TALQ_NODE_STATUS_WARNING     = 1,
    TALQ_NODE_STATUS_FAILURE     = 2,
    TALQ_NODE_STATUS_UNREACHABLE = 3,
} talq_node_status_t;

/**
 * TALQ Node.
 * Represents the managed luminaire endpoint.
 */
typedef struct {
    talq_uuid_t         id;
    char                name[64];
    char                description[128];
    talq_uuid_t         activeProgramGroupId;
    talq_node_status_t  status;
    float               currentLevel;       /**< 0.0-1.0          */
    uint16_t            currentCct;         /**< Kelvin            */
    float               powerWatts;         /**< Measured power    */
    float               energyKwh;          /**< Cumulative energy */
    float               ambientLux;         /**< TSL2591 reading   */
    float               temperatureCelsius; /**< TMP117 reading    */
    talq_timestamp_t    lastSeen;
} talq_node_t;

/**
 * TALQ Device.
 * Represents one LED driver channel (warm or cool).
 */
typedef struct {
    talq_uuid_t         id;
    char                name[64];
    uint8_t             channel;        /**< TALQ_CHANNEL_WARM / COOL */
    uint16_t            nominalCct;     /**< 2700 or 5000 K           */
    float               maxPowerWatts;  /**< 200 W per channel        */
    float               currentLevel;  /**< 0.0-1.0                  */
    bool                fault;
} talq_device_t;

/**
 * TALQ Event.
 * An asynchronous notification stored in the event ring buffer.
 */
typedef enum {
    TALQ_EVENT_LEVEL_CHANGED   = 0,
    TALQ_EVENT_STATUS_CHANGED  = 1,
    TALQ_EVENT_FAULT_RAISED    = 2,
    TALQ_EVENT_FAULT_CLEARED   = 3,
    TALQ_EVENT_CONFIG_CHANGED  = 4,
} talq_event_type_t;

typedef struct {
    uint32_t          seqNo;
    talq_timestamp_t  timestamp;
    talq_event_type_t type;
    talq_uuid_t       sourceId;    /**< ID of node or device that raised the event */
    char              detail[128];
} talq_event_t;

/**
 * TALQ command types supported by this gateway.
 */
typedef enum {
    TALQ_CMD_SET_ABSOLUTE_LEVEL  = 0,  /**< Dim to specific level/CCT     */
    TALQ_CMD_ACTIVATE_GROUP      = 1,  /**< Switch to a program group     */
    TALQ_CMD_RESET               = 2,  /**< Reset fault / restart node    */
    TALQ_CMD_IDENTIFY            = 3,  /**< Flash LED to identify fixture */
} talq_command_type_t;

typedef struct {
    talq_uuid_t         nodeId;
    talq_command_type_t type;
    float               level;          /**< 0.0-1.0 for SET_ABSOLUTE_LEVEL */
    uint16_t            cct;            /**< Kelvin for SET_ABSOLUTE_LEVEL  */
    float               fadeTime;       /**< Seconds for SET_ABSOLUTE_LEVEL */
    talq_uuid_t         programGroupId; /**< For ACTIVATE_GROUP             */
} talq_command_t;

/* ─── Callbacks (platform glue) ─────────────────────────────────────────── */

/**
 * Platform callback: apply a new dimming level to the hardware.
 *
 * @param channel   TALQ_CHANNEL_WARM or TALQ_CHANNEL_COOL
 * @param level     Relative light output 0.0-1.0
 * @param fadeTime  Transition time in seconds
 */
typedef void (*talq_set_level_fn)(uint8_t channel, float level, float fadeTime);

/**
 * Platform callback: read current sensor values into @p node.
 * The callback must populate:
 *   node->powerWatts, node->energyKwh, node->ambientLux,
 *   node->temperatureCelsius
 */
typedef void (*talq_read_sensors_fn)(talq_node_t *node);

/**
 * Platform callback: get current UTC time as an ISO-8601 string.
 *
 * @param buf   Output buffer of at least 21 bytes
 */
typedef void (*talq_get_time_fn)(char *buf, size_t len);

/* ─── Public API ────────────────────────────────────────────────────────── */

/**
 * Initialise the TALQ subsystem.
 *
 * Must be called once before talq_httpd_start().
 *
 * @param set_level    Hardware dimming callback (required)
 * @param read_sensors Sensor reading callback (required)
 * @param get_time     UTC time callback (required)
 * @return 0 on success, negative errno on failure
 */
int talq_init(talq_set_level_fn    set_level,
              talq_read_sensors_fn read_sensors,
              talq_get_time_fn     get_time);

/**
 * Start the TALQ HTTP server on port 80.
 *
 * @return 0 on success, negative errno on failure
 */
int talq_httpd_start(void);

/** Stop the TALQ HTTP server and free resources. */
void talq_httpd_stop(void);

/**
 * Process a TALQ command synchronously (e.g. from internal logic).
 *
 * @return 0 on success, -EINVAL for unknown command/node, -ENOTSUP for
 *         unsupported command type.
 */
int talq_command_execute(const talq_command_t *cmd);

/**
 * Periodic update: refreshes sensor data, evaluates the active calendar
 * program, and posts status-change events.  Call from a FreeRTOS task or
 * a periodic timer every 500 ms.
 */
void talq_tick(void);

/**
 * Read-only access to the live node state (for display/logging).
 *
 * @return Pointer to internal node structure (must not be modified).
 */
const talq_node_t *talq_get_node(void);

/**
 * Read-only access to the active program group (may be NULL).
 */
const talq_program_group_t *talq_get_active_program_group(void);

/**
 * Append an application-generated event to the TALQ event buffer.
 * Thread-safe (uses a FreeRTOS mutex).
 */
void talq_post_event(talq_event_type_t type,
                     const char        *sourceId,
                     const char        *detail);
