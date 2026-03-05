/*
 * talq.c — TALQ Smart City Protocol v2.3 gateway implementation
 *
 * Provides:
 *   - In-memory TALQ data model (node, devices, program groups, calendars)
 *   - HTTP REST server exposing the TALQ API on port 80
 *   - Calendar-driven automatic dimming schedule evaluation
 *   - Command execution (set level, activate group, reset, identify)
 *   - Event ring buffer with monotonically increasing sequence numbers
 *
 * Compile with ESP-IDF (esp_http_server, cJSON, freertos/semphr).
 * All public symbols are prefixed with talq_.
 */

#include "talq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "talq";

/* ─── Internal state ────────────────────────────────────────────────────── */

static talq_set_level_fn     s_set_level;
static talq_read_sensors_fn  s_read_sensors;
static talq_get_time_fn      s_get_time;

static talq_node_t           s_node;
static talq_device_t         s_devices[TALQ_CHANNEL_COUNT];
static talq_program_group_t  s_groups[TALQ_MAX_PROGRAM_GROUPS];
static uint8_t               s_group_count;
static talq_calendar_t       s_calendars[TALQ_MAX_CALENDARS];
static uint8_t               s_calendar_count;

static talq_event_t          s_events[TALQ_MAX_EVENTS];
static uint32_t              s_event_head;   /* next write index (mod) */
static uint32_t              s_event_seq;    /* monotonic counter      */
static SemaphoreHandle_t     s_event_lock;

static httpd_handle_t        s_server;
static bool                  s_initialised;

/* ─── Helper: UUID generation (deterministic for tests; replace with esp_random) */

static void uuid_generate(talq_uuid_t out)
{
    /* Use esp_random() in production; a simple counter is fine for the
       initial data model seeding that happens at boot. */
    static uint32_t counter;
    counter++;
    snprintf(out, TALQ_UUID_LEN,
             "%08x-0000-4000-8000-%012x",
             (unsigned)counter,
             (unsigned)(counter * 2654435761u));
}

/* ─── Helper: current UTC timestamp ─────────────────────────────────────── */

static void get_now(char *buf, size_t len)
{
    if (s_get_time) {
        s_get_time(buf, len);
    } else {
        strncpy(buf, "1970-01-01T00:00:00Z", len);
    }
}

/* ─── JSON serialisation helpers ────────────────────────────────────────── */

static cJSON *json_node(const talq_node_t *n)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "@type",             "talq:Node");
    cJSON_AddStringToObject(obj, "talq:id",           n->id);
    cJSON_AddStringToObject(obj, "talq:name",         n->name);
    cJSON_AddStringToObject(obj, "talq:description",  n->description);
    cJSON_AddNumberToObject(obj, "talq:level",        (double)n->currentLevel);
    cJSON_AddNumberToObject(obj, "talq:cct",          n->currentCct);
    cJSON_AddNumberToObject(obj, "talq:powerWatts",   (double)n->powerWatts);
    cJSON_AddNumberToObject(obj, "talq:energyKwh",    (double)n->energyKwh);
    cJSON_AddNumberToObject(obj, "talq:ambientLux",   (double)n->ambientLux);
    cJSON_AddNumberToObject(obj, "talq:temperature",  (double)n->temperatureCelsius);
    cJSON_AddStringToObject(obj, "talq:lastSeen",     n->lastSeen);
    cJSON_AddStringToObject(obj, "talq:activeProgramGroupId",
                            n->activeProgramGroupId);
    const char *status_str[] = { "ok", "warning", "failure", "unreachable" };
    cJSON_AddStringToObject(obj, "talq:status",
                            status_str[n->status]);
    return obj;
}

static cJSON *json_device(const talq_device_t *d)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "@type",             "talq:Device");
    cJSON_AddStringToObject(obj, "talq:id",           d->id);
    cJSON_AddStringToObject(obj, "talq:name",         d->name);
    cJSON_AddNumberToObject(obj, "talq:channel",      d->channel);
    cJSON_AddNumberToObject(obj, "talq:nominalCct",   d->nominalCct);
    cJSON_AddNumberToObject(obj, "talq:maxPowerWatts",(double)d->maxPowerWatts);
    cJSON_AddNumberToObject(obj, "talq:level",        (double)d->currentLevel);
    cJSON_AddBoolToObject  (obj, "talq:fault",        d->fault);
    return obj;
}

static cJSON *json_program_step(const talq_program_step_t *s)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "talq:hour",     s->hour);
    cJSON_AddNumberToObject(obj, "talq:minute",   s->minute);
    cJSON_AddNumberToObject(obj, "talq:level",    (double)s->level);
    cJSON_AddNumberToObject(obj, "talq:cct",      s->cct_kelvin);
    cJSON_AddNumberToObject(obj, "talq:fadeTime", (double)s->fadeTime);
    return obj;
}

static cJSON *json_program_group(const talq_program_group_t *g)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "@type",            "talq:ProgramGroup");
    cJSON_AddStringToObject(obj, "talq:id",          g->id);
    cJSON_AddStringToObject(obj, "talq:name",        g->name);
    cJSON_AddStringToObject(obj, "talq:description", g->description);
    cJSON *steps = cJSON_AddArrayToObject(obj, "talq:steps");
    for (uint8_t i = 0; i < g->stepCount; i++) {
        cJSON_AddItemToArray(steps, json_program_step(&g->steps[i]));
    }
    return obj;
}

static cJSON *json_calendar(const talq_calendar_t *c)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "@type",                  "talq:Calendar");
    cJSON_AddStringToObject(obj, "talq:id",                c->id);
    cJSON_AddStringToObject(obj, "talq:name",              c->name);
    cJSON_AddStringToObject(obj, "talq:programGroupId",    c->programGroupId);
    cJSON_AddStringToObject(obj, "talq:validFrom",         c->validFrom);
    cJSON_AddStringToObject(obj, "talq:validTo",           c->validTo);
    cJSON_AddNumberToObject(obj, "talq:daysOfWeek",        c->daysOfWeek);
    return obj;
}

/* ─── JSON parsing helpers ───────────────────────────────────────────────── */

static bool parse_program_step(const cJSON *obj, talq_program_step_t *step)
{
    const cJSON *h = cJSON_GetObjectItemCaseSensitive(obj, "talq:hour");
    const cJSON *m = cJSON_GetObjectItemCaseSensitive(obj, "talq:minute");
    const cJSON *l = cJSON_GetObjectItemCaseSensitive(obj, "talq:level");
    const cJSON *c = cJSON_GetObjectItemCaseSensitive(obj, "talq:cct");
    const cJSON *f = cJSON_GetObjectItemCaseSensitive(obj, "talq:fadeTime");
    if (!cJSON_IsNumber(h) || !cJSON_IsNumber(m) || !cJSON_IsNumber(l)) {
        return false;
    }
    step->hour      = (uint8_t)h->valueint;
    step->minute    = (uint8_t)m->valueint;
    step->level     = (float)l->valuedouble;
    step->cct_kelvin= c ? (uint16_t)c->valueint : 0;
    step->fadeTime  = f ? (float)f->valuedouble : 0.0f;

    /* clamp */
    if (step->level < 0.0f) step->level = 0.0f;
    if (step->level > 1.0f) step->level = 1.0f;
    if (step->fadeTime < 0.0f) step->fadeTime = 0.0f;
    return true;
}

static bool parse_program_group(const cJSON *obj, talq_program_group_t *g)
{
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(obj, "talq:name");
    const cJSON *desc = cJSON_GetObjectItemCaseSensitive(obj, "talq:description");
    const cJSON *steps= cJSON_GetObjectItemCaseSensitive(obj, "talq:steps");
    if (!cJSON_IsString(name)) return false;
    snprintf(g->name, sizeof(g->name), "%s", name->valuestring);
    if (cJSON_IsString(desc)) {
        snprintf(g->description, sizeof(g->description), "%s", desc->valuestring);
    }
    g->stepCount = 0;
    if (cJSON_IsArray(steps)) {
        const cJSON *s;
        cJSON_ArrayForEach(s, steps) {
            if (g->stepCount >= TALQ_MAX_PROGRAMS_PER_GROUP) break;
            if (!parse_program_step(s, &g->steps[g->stepCount])) return false;
            g->stepCount++;
        }
    }
    return true;
}

static bool parse_calendar(const cJSON *obj, talq_calendar_t *cal)
{
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(obj, "talq:name");
    const cJSON *pgid = cJSON_GetObjectItemCaseSensitive(obj, "talq:programGroupId");
    const cJSON *vfr  = cJSON_GetObjectItemCaseSensitive(obj, "talq:validFrom");
    const cJSON *vto  = cJSON_GetObjectItemCaseSensitive(obj, "talq:validTo");
    const cJSON *dow  = cJSON_GetObjectItemCaseSensitive(obj, "talq:daysOfWeek");
    if (!cJSON_IsString(name) || !cJSON_IsString(pgid)) return false;
    snprintf(cal->name,           sizeof(cal->name),           "%s", name->valuestring);
    snprintf(cal->programGroupId, sizeof(cal->programGroupId), "%s", pgid->valuestring);
    if (cJSON_IsString(vfr)) {
        strncpy(cal->validFrom, vfr->valuestring, sizeof(cal->validFrom) - 1);
        cal->validFrom[sizeof(cal->validFrom) - 1] = '\0';
    }
    if (cJSON_IsString(vto)) {
        strncpy(cal->validTo, vto->valuestring, sizeof(cal->validTo) - 1);
        cal->validTo[sizeof(cal->validTo) - 1] = '\0';
    }
    cal->daysOfWeek = cJSON_IsNumber(dow) ? (uint8_t)dow->valueint : 0;
    return true;
}

/* ─── HTTP response helpers ──────────────────────────────────────────────── */

#define TALQ_CONTEXT "http://schema.talq-consortium.org/talq-schema.json"

static esp_err_t send_json(httpd_req_t *req, int status, cJSON *body)
{
    char *text = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!text) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    httpd_resp_set_status(req,
        status == 200 ? "200 OK" :
        status == 201 ? "201 Created" :
        status == 400 ? "400 Bad Request" :
        status == 404 ? "404 Not Found" :
        status == 405 ? "405 Method Not Allowed" :
                        "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, text);
    free(text);
    return ESP_OK;
}

static esp_err_t send_error(httpd_req_t *req, int status, const char *msg)
{
    cJSON *err = cJSON_CreateObject();
    cJSON_AddStringToObject(err, "error", msg);
    return send_json(req, status, err);
}

/* Read entire request body (up to max_len bytes) into *out (caller must free). */
static esp_err_t read_body(httpd_req_t *req, char **out, size_t max_len)
{
    size_t len = req->content_len;
    if (len == 0 || len > max_len) {
        *out = NULL;
        return len > max_len ? ESP_ERR_INVALID_SIZE : ESP_OK;
    }
    char *buf = malloc(len + 1);
    if (!buf) return ESP_ERR_NO_MEM;
    int received = httpd_req_recv(req, buf, len);
    if (received <= 0) { free(buf); return ESP_FAIL; }
    buf[received] = '\0';
    *out = buf;
    return ESP_OK;
}

/* ─── URI helper ─────────────────────────────────────────────────────────── */

/* Return pointer to the last path component (after the final '/'), or NULL
   if the URI contains no '/' character. */
static const char *last_segment(const char *uri)
{
    const char *p = strrchr(uri, '/');
    return p ? p + 1 : NULL;
}

/* ─── Handler: GET /talq ─────────────────────────────────────────────────── */

static esp_err_t handler_gateway_get(httpd_req_t *req)
{
    char now[21];
    get_now(now, sizeof(now));

    cJSON *gw = cJSON_CreateObject();
    cJSON_AddStringToObject(gw, "@context",              TALQ_CONTEXT);
    cJSON_AddStringToObject(gw, "@type",                 "talq:Gateway");
    cJSON_AddStringToObject(gw, "talq:id",               s_node.id);
    cJSON_AddStringToObject(gw, "talq:name",             "D-SLS DIAMANT v2.1");
    cJSON_AddStringToObject(gw, "talq:description",
                            "Smart Street Lighting System · ESP32-C6 · Milan");
    cJSON_AddStringToObject(gw, "talq:specVersion",      TALQ_SPEC_VERSION);
    cJSON_AddStringToObject(gw, "talq:implVersion",      TALQ_IMPL_VERSION);
    cJSON_AddStringToObject(gw, "talq:timestamp",        now);
    cJSON_AddNumberToObject(gw, "talq:nodeCount",        1);
    cJSON_AddNumberToObject(gw, "talq:programGroupCount",s_group_count);
    cJSON_AddNumberToObject(gw, "talq:calendarCount",    s_calendar_count);

    /* advertise supported commands */
    cJSON *caps = cJSON_AddArrayToObject(gw, "talq:supportedCommands");
    cJSON_AddItemToArray(caps, cJSON_CreateString("commandSetAbsoluteLevel"));
    cJSON_AddItemToArray(caps, cJSON_CreateString("commandActivateGroup"));
    cJSON_AddItemToArray(caps, cJSON_CreateString("commandReset"));
    cJSON_AddItemToArray(caps, cJSON_CreateString("commandIdentify"));

    return send_json(req, 200, gw);
}

/* ─── Handlers: /talq/nodes ──────────────────────────────────────────────── */

static esp_err_t handler_nodes_get(httpd_req_t *req)
{
    /* refresh sensors before responding */
    if (s_read_sensors) s_read_sensors(&s_node);
    get_now(s_node.lastSeen, sizeof(s_node.lastSeen));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON *arr = cJSON_AddArrayToObject(root, "talq:nodes");
    cJSON_AddItemToArray(arr, json_node(&s_node));
    return send_json(req, 200, root);
}

static esp_err_t handler_node_get(httpd_req_t *req)
{
    /* Extract {nodeId} from URI: /talq/nodes/<id> */
    const char *id = last_segment(req->uri);
    if (!id || strcmp(id, s_node.id) != 0) {
        return send_error(req, 404, "node not found");
    }
    if (s_read_sensors) s_read_sensors(&s_node);
    get_now(s_node.lastSeen, sizeof(s_node.lastSeen));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON_AddItemToObject(root, "talq:node", json_node(&s_node));
    return send_json(req, 200, root);
}

static esp_err_t handler_node_put(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || strcmp(id, s_node.id) != 0) {
        return send_error(req, 404, "node not found");
    }

    char *body = NULL;
    if (read_body(req, &body, 4096) != ESP_OK || !body) {
        return send_error(req, 400, "bad request body");
    }
    cJSON *obj = cJSON_Parse(body);
    free(body);
    if (!obj) return send_error(req, 400, "invalid JSON");

    /* Allow updating the active program group */
    const cJSON *pgid = cJSON_GetObjectItemCaseSensitive(obj, "talq:activeProgramGroupId");
    if (cJSON_IsString(pgid)) {
        /* verify the group exists */
        bool found = false;
        for (uint8_t i = 0; i < s_group_count; i++) {
            if (strcmp(s_groups[i].id, pgid->valuestring) == 0) {
                strncpy(s_node.activeProgramGroupId, pgid->valuestring,
                        TALQ_UUID_LEN - 1);
                found = true;
                break;
            }
        }
        if (!found) {
            cJSON_Delete(obj);
            return send_error(req, 400, "programGroup not found");
        }
    }
    cJSON_Delete(obj);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON_AddItemToObject(root, "talq:node", json_node(&s_node));
    talq_post_event(TALQ_EVENT_CONFIG_CHANGED, s_node.id, "node updated");
    return send_json(req, 200, root);
}

/* ─── Handlers: /talq/devices ────────────────────────────────────────────── */

static esp_err_t handler_devices_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON *arr = cJSON_AddArrayToObject(root, "talq:devices");
    for (uint8_t i = 0; i < TALQ_CHANNEL_COUNT; i++) {
        cJSON_AddItemToArray(arr, json_device(&s_devices[i]));
    }
    return send_json(req, 200, root);
}

static esp_err_t handler_device_get(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || *id == '\0') return send_error(req, 404, "device not found");
    for (uint8_t i = 0; i < TALQ_CHANNEL_COUNT; i++) {
        if (strcmp(s_devices[i].id, id) == 0) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
            cJSON_AddItemToObject(root, "talq:device", json_device(&s_devices[i]));
            return send_json(req, 200, root);
        }
    }
    return send_error(req, 404, "device not found");
}

/* ─── Handlers: /talq/programGroups ─────────────────────────────────────── */

static esp_err_t handler_groups_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON *arr = cJSON_AddArrayToObject(root, "talq:programGroups");
    for (uint8_t i = 0; i < s_group_count; i++) {
        cJSON_AddItemToArray(arr, json_program_group(&s_groups[i]));
    }
    return send_json(req, 200, root);
}

static esp_err_t handler_groups_post(httpd_req_t *req)
{
    if (s_group_count >= TALQ_MAX_PROGRAM_GROUPS) {
        return send_error(req, 400, "max program groups reached");
    }
    char *body = NULL;
    if (read_body(req, &body, 8192) != ESP_OK || !body) {
        return send_error(req, 400, "bad request body");
    }
    cJSON *obj = cJSON_Parse(body);
    free(body);
    if (!obj) return send_error(req, 400, "invalid JSON");

    talq_program_group_t *g = &s_groups[s_group_count];
    memset(g, 0, sizeof(*g));
    uuid_generate(g->id);
    if (!parse_program_group(obj, g)) {
        cJSON_Delete(obj);
        return send_error(req, 400, "invalid program group");
    }
    cJSON_Delete(obj);
    s_group_count++;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON_AddItemToObject(root, "talq:programGroup", json_program_group(g));
    talq_post_event(TALQ_EVENT_CONFIG_CHANGED, g->id, "program group created");
    return send_json(req, 201, root);
}

static esp_err_t handler_group_get(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || *id == '\0') return send_error(req, 400, "missing id");
    for (uint8_t i = 0; i < s_group_count; i++) {
        if (strcmp(s_groups[i].id, id) == 0) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
            cJSON_AddItemToObject(root, "talq:programGroup",
                                  json_program_group(&s_groups[i]));
            return send_json(req, 200, root);
        }
    }
    return send_error(req, 404, "program group not found");
}

static esp_err_t handler_group_put(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || *id == '\0') return send_error(req, 400, "missing id");
    for (uint8_t i = 0; i < s_group_count; i++) {
        if (strcmp(s_groups[i].id, id) == 0) {
            char *body = NULL;
            if (read_body(req, &body, 8192) != ESP_OK || !body) {
                return send_error(req, 400, "bad request body");
            }
            cJSON *obj = cJSON_Parse(body);
            free(body);
            if (!obj) return send_error(req, 400, "invalid JSON");
            /* preserve ID */
            talq_uuid_t saved_id;
            strncpy(saved_id, s_groups[i].id, TALQ_UUID_LEN);
            if (!parse_program_group(obj, &s_groups[i])) {
                cJSON_Delete(obj);
                return send_error(req, 400, "invalid program group");
            }
            strncpy(s_groups[i].id, saved_id, TALQ_UUID_LEN);
            cJSON_Delete(obj);
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
            cJSON_AddItemToObject(root, "talq:programGroup",
                                  json_program_group(&s_groups[i]));
            talq_post_event(TALQ_EVENT_CONFIG_CHANGED, id, "program group updated");
            return send_json(req, 200, root);
        }
    }
    return send_error(req, 404, "program group not found");
}

/* ─── Handlers: /talq/calendars ─────────────────────────────────────────── */

static esp_err_t handler_calendars_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON *arr = cJSON_AddArrayToObject(root, "talq:calendars");
    for (uint8_t i = 0; i < s_calendar_count; i++) {
        cJSON_AddItemToArray(arr, json_calendar(&s_calendars[i]));
    }
    return send_json(req, 200, root);
}

static esp_err_t handler_calendars_post(httpd_req_t *req)
{
    if (s_calendar_count >= TALQ_MAX_CALENDARS) {
        return send_error(req, 400, "max calendars reached");
    }
    char *body = NULL;
    if (read_body(req, &body, 4096) != ESP_OK || !body) {
        return send_error(req, 400, "bad request body");
    }
    cJSON *obj = cJSON_Parse(body);
    free(body);
    if (!obj) return send_error(req, 400, "invalid JSON");

    talq_calendar_t *cal = &s_calendars[s_calendar_count];
    memset(cal, 0, sizeof(*cal));
    uuid_generate(cal->id);
    if (!parse_calendar(obj, cal)) {
        cJSON_Delete(obj);
        return send_error(req, 400, "invalid calendar");
    }
    cJSON_Delete(obj);
    s_calendar_count++;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON_AddItemToObject(root, "talq:calendar", json_calendar(cal));
    talq_post_event(TALQ_EVENT_CONFIG_CHANGED, cal->id, "calendar created");
    return send_json(req, 201, root);
}

static esp_err_t handler_calendar_get(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || *id == '\0') return send_error(req, 400, "missing id");
    for (uint8_t i = 0; i < s_calendar_count; i++) {
        if (strcmp(s_calendars[i].id, id) == 0) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
            cJSON_AddItemToObject(root, "talq:calendar",
                                  json_calendar(&s_calendars[i]));
            return send_json(req, 200, root);
        }
    }
    return send_error(req, 404, "calendar not found");
}

static esp_err_t handler_calendar_put(httpd_req_t *req)
{
    const char *id = last_segment(req->uri);
    if (!id || *id == '\0') return send_error(req, 400, "missing id");
    for (uint8_t i = 0; i < s_calendar_count; i++) {
        if (strcmp(s_calendars[i].id, id) == 0) {
            char *body = NULL;
            if (read_body(req, &body, 4096) != ESP_OK || !body) {
                return send_error(req, 400, "bad request body");
            }
            cJSON *obj = cJSON_Parse(body);
            free(body);
            if (!obj) return send_error(req, 400, "invalid JSON");
            talq_uuid_t saved_id;
            strncpy(saved_id, s_calendars[i].id, TALQ_UUID_LEN);
            if (!parse_calendar(obj, &s_calendars[i])) {
                cJSON_Delete(obj);
                return send_error(req, 400, "invalid calendar");
            }
            strncpy(s_calendars[i].id, saved_id, TALQ_UUID_LEN);
            cJSON_Delete(obj);
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
            cJSON_AddItemToObject(root, "talq:calendar",
                                  json_calendar(&s_calendars[i]));
            talq_post_event(TALQ_EVENT_CONFIG_CHANGED, id, "calendar updated");
            return send_json(req, 200, root);
        }
    }
    return send_error(req, 404, "calendar not found");
}

/* ─── Handler: POST /talq/commands ──────────────────────────────────────── */

static esp_err_t handler_commands_post(httpd_req_t *req)
{
    char *body = NULL;
    if (read_body(req, &body, 4096) != ESP_OK || !body) {
        return send_error(req, 400, "bad request body");
    }
    cJSON *obj = cJSON_Parse(body);
    free(body);
    if (!obj) return send_error(req, 400, "invalid JSON");

    const cJSON *type_j  = cJSON_GetObjectItemCaseSensitive(obj, "talq:commandType");
    const cJSON *node_j  = cJSON_GetObjectItemCaseSensitive(obj, "talq:nodeId");
    if (!cJSON_IsString(type_j) || !cJSON_IsString(node_j)) {
        cJSON_Delete(obj);
        return send_error(req, 400, "missing talq:commandType or talq:nodeId");
    }
    if (strcmp(node_j->valuestring, s_node.id) != 0) {
        cJSON_Delete(obj);
        return send_error(req, 404, "node not found");
    }

    talq_command_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    strncpy(cmd.nodeId, node_j->valuestring, TALQ_UUID_LEN - 1);

    const char *ct = type_j->valuestring;
    if (strcmp(ct, "commandSetAbsoluteLevel") == 0) {
        cmd.type = TALQ_CMD_SET_ABSOLUTE_LEVEL;
        const cJSON *lv = cJSON_GetObjectItemCaseSensitive(obj, "talq:level");
        const cJSON *cc = cJSON_GetObjectItemCaseSensitive(obj, "talq:cct");
        const cJSON *ft = cJSON_GetObjectItemCaseSensitive(obj, "talq:fadeTime");
        cmd.level    = cJSON_IsNumber(lv) ? (float)lv->valuedouble : 1.0f;
        cmd.cct      = cJSON_IsNumber(cc) ? (uint16_t)cc->valueint : 4000;
        cmd.fadeTime = cJSON_IsNumber(ft) ? (float)ft->valuedouble : 1.0f;
    } else if (strcmp(ct, "commandActivateGroup") == 0) {
        cmd.type = TALQ_CMD_ACTIVATE_GROUP;
        const cJSON *pgid = cJSON_GetObjectItemCaseSensitive(obj, "talq:programGroupId");
        if (!cJSON_IsString(pgid)) {
            cJSON_Delete(obj);
            return send_error(req, 400, "missing talq:programGroupId");
        }
        strncpy(cmd.programGroupId, pgid->valuestring, TALQ_UUID_LEN - 1);
    } else if (strcmp(ct, "commandReset") == 0) {
        cmd.type = TALQ_CMD_RESET;
    } else if (strcmp(ct, "commandIdentify") == 0) {
        cmd.type = TALQ_CMD_IDENTIFY;
    } else {
        cJSON_Delete(obj);
        return send_error(req, 400, "unknown command type");
    }
    cJSON_Delete(obj);

    int rc = talq_command_execute(&cmd);
    if (rc == -ENOTSUP) return send_error(req, 405, "command not supported");
    if (rc != 0)        return send_error(req, 400, "command failed");

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "@context", TALQ_CONTEXT);
    cJSON_AddStringToObject(resp, "talq:status", "accepted");
    return send_json(req, 200, resp);
}

/* ─── Handler: GET /talq/events ──────────────────────────────────────────── */

static esp_err_t handler_events_get(httpd_req_t *req)
{
    /* Optional ?since=<seqNo> query parameter */
    uint32_t since = 0;
    char query[32];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[16];
        if (httpd_query_key_value(query, "since", val, sizeof(val)) == ESP_OK) {
            since = (uint32_t)strtoul(val, NULL, 10);
        }
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "@context", TALQ_CONTEXT);
    cJSON *arr = cJSON_AddArrayToObject(root, "talq:events");

    xSemaphoreTake(s_event_lock, portMAX_DELAY);
    for (uint32_t i = 0; i < TALQ_MAX_EVENTS; i++) {
        const talq_event_t *ev = &s_events[i];
        if (ev->seqNo > since && ev->seqNo != 0) {
            cJSON *e = cJSON_CreateObject();
            cJSON_AddNumberToObject(e, "talq:seqNo",     (double)ev->seqNo);
            cJSON_AddStringToObject(e, "talq:timestamp", ev->timestamp);
            const char *ev_type_str[] = {
                "levelChanged", "statusChanged", "faultRaised",
                "faultCleared", "configChanged"
            };
            cJSON_AddStringToObject(e, "talq:type",      ev_type_str[ev->type]);
            cJSON_AddStringToObject(e, "talq:sourceId",  ev->sourceId);
            cJSON_AddStringToObject(e, "talq:detail",    ev->detail);
            cJSON_AddItemToArray(arr, e);
        }
    }
    xSemaphoreGive(s_event_lock);

    return send_json(req, 200, root);
}

/* ─── HTTP server URI registrations ─────────────────────────────────────── */

/* Wildcard handler for collection + item endpoints (/talq/nodes  and
   /talq/nodes/<id>) routed by URI prefix matching. */

static esp_err_t router(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *method = (req->method == HTTP_GET)  ? "GET"  :
                         (req->method == HTTP_POST) ? "POST" :
                         (req->method == HTTP_PUT)  ? "PUT"  : "?";

    ESP_LOGI(TAG, "%s %s", method, uri);

    /* /talq (gateway info) */
    if (strcmp(uri, TALQ_BASE_PATH) == 0 ||
        strcmp(uri, TALQ_BASE_PATH "/") == 0) {
        if (req->method == HTTP_GET) return handler_gateway_get(req);
    }
    /* /talq/nodes[/<id>] */
    else if (strncmp(uri, TALQ_BASE_PATH "/nodes",
                     sizeof(TALQ_BASE_PATH "/nodes") - 1) == 0) {
        const char *after = uri + sizeof(TALQ_BASE_PATH "/nodes") - 1;
        if (*after == '\0' || strcmp(after, "/") == 0) {
            if (req->method == HTTP_GET) return handler_nodes_get(req);
        } else {
            if (req->method == HTTP_GET) return handler_node_get(req);
            if (req->method == HTTP_PUT) return handler_node_put(req);
        }
    }
    /* /talq/devices[/<id>] */
    else if (strncmp(uri, TALQ_BASE_PATH "/devices",
                     sizeof(TALQ_BASE_PATH "/devices") - 1) == 0) {
        const char *after = uri + sizeof(TALQ_BASE_PATH "/devices") - 1;
        if (*after == '\0' || strcmp(after, "/") == 0) {
            if (req->method == HTTP_GET) return handler_devices_get(req);
        } else {
            if (req->method == HTTP_GET) return handler_device_get(req);
        }
    }
    /* /talq/programGroups[/<id>] */
    else if (strncmp(uri, TALQ_BASE_PATH "/programGroups",
                     sizeof(TALQ_BASE_PATH "/programGroups") - 1) == 0) {
        const char *after = uri + sizeof(TALQ_BASE_PATH "/programGroups") - 1;
        if (*after == '\0' || strcmp(after, "/") == 0) {
            if (req->method == HTTP_GET)  return handler_groups_get(req);
            if (req->method == HTTP_POST) return handler_groups_post(req);
        } else {
            if (req->method == HTTP_GET) return handler_group_get(req);
            if (req->method == HTTP_PUT) return handler_group_put(req);
        }
    }
    /* /talq/calendars[/<id>] */
    else if (strncmp(uri, TALQ_BASE_PATH "/calendars",
                     sizeof(TALQ_BASE_PATH "/calendars") - 1) == 0) {
        const char *after = uri + sizeof(TALQ_BASE_PATH "/calendars") - 1;
        if (*after == '\0' || strcmp(after, "/") == 0) {
            if (req->method == HTTP_GET)  return handler_calendars_get(req);
            if (req->method == HTTP_POST) return handler_calendars_post(req);
        } else {
            if (req->method == HTTP_GET) return handler_calendar_get(req);
            if (req->method == HTTP_PUT) return handler_calendar_put(req);
        }
    }
    /* /talq/commands */
    else if (strcmp(uri, TALQ_BASE_PATH "/commands") == 0) {
        if (req->method == HTTP_POST) return handler_commands_post(req);
    }
    /* /talq/events */
    else if (strncmp(uri, TALQ_BASE_PATH "/events",
                     sizeof(TALQ_BASE_PATH "/events") - 1) == 0) {
        if (req->method == HTTP_GET) return handler_events_get(req);
    }

    return send_error(req, 404, "not found");
}

static const httpd_uri_t s_uri_catch_all = {
    .uri      = TALQ_BASE_PATH "/*",
    .method   = HTTP_GET,   /* registered multiple times below */
    .handler  = router,
    .user_ctx = NULL,
};

/* ─── Default data model seeding ─────────────────────────────────────────── */

static void seed_default_model(void)
{
    /* Node */
    memset(&s_node, 0, sizeof(s_node));
    uuid_generate(s_node.id);
    strncpy(s_node.name,        "Luminaire-01", sizeof(s_node.name) - 1);
    strncpy(s_node.description, "400W Tunable White · D-SLS DIAMANT v2.1",
            sizeof(s_node.description) - 1);
    s_node.status = TALQ_NODE_STATUS_OK;

    /* Devices */
    memset(s_devices, 0, sizeof(s_devices));
    uuid_generate(s_devices[TALQ_CHANNEL_WARM].id);
    strncpy(s_devices[TALQ_CHANNEL_WARM].name, "LED-Warm-2700K",
            sizeof(s_devices[TALQ_CHANNEL_WARM].name) - 1);
    s_devices[TALQ_CHANNEL_WARM].channel       = TALQ_CHANNEL_WARM;
    s_devices[TALQ_CHANNEL_WARM].nominalCct    = 2700;
    s_devices[TALQ_CHANNEL_WARM].maxPowerWatts = 200.0f;

    uuid_generate(s_devices[TALQ_CHANNEL_COOL].id);
    strncpy(s_devices[TALQ_CHANNEL_COOL].name, "LED-Cool-5000K",
            sizeof(s_devices[TALQ_CHANNEL_COOL].name) - 1);
    s_devices[TALQ_CHANNEL_COOL].channel       = TALQ_CHANNEL_COOL;
    s_devices[TALQ_CHANNEL_COOL].nominalCct    = 5000;
    s_devices[TALQ_CHANNEL_COOL].maxPowerWatts = 200.0f;

    /* Default program group: Street lighting profile
       22:00→100% warm, 23:00→70%, 02:00→40%, 05:00→70%, 06:00 OFF */
    s_group_count = 1;
    talq_program_group_t *pg = &s_groups[0];
    memset(pg, 0, sizeof(*pg));
    uuid_generate(pg->id);
    strncpy(pg->name, "Default-Street-Profile", sizeof(pg->name) - 1);
    strncpy(pg->description,
            "Standard street lighting profile: dusk-to-dawn with midnight dimming",
            sizeof(pg->description) - 1);
    pg->stepCount = 5;
    pg->steps[0] = (talq_program_step_t){ 22, 0, 1.00f, 2700, 30.0f };
    pg->steps[1] = (talq_program_step_t){ 23, 0, 0.70f, 3000, 30.0f };
    pg->steps[2] = (talq_program_step_t){  2, 0, 0.40f, 3500, 30.0f };
    pg->steps[3] = (talq_program_step_t){  5, 0, 0.70f, 4000, 30.0f };
    pg->steps[4] = (talq_program_step_t){  6, 0, 0.00f,    0, 30.0f };
    strncpy(s_node.activeProgramGroupId, pg->id, TALQ_UUID_LEN - 1);

    /* Default calendar: apply the default program all year */
    s_calendar_count = 1;
    talq_calendar_t *cal = &s_calendars[0];
    memset(cal, 0, sizeof(*cal));
    uuid_generate(cal->id);
    strncpy(cal->name, "Default-Calendar", sizeof(cal->name) - 1);
    strncpy(cal->programGroupId, pg->id, TALQ_UUID_LEN - 1);
    strncpy(cal->validFrom, "2000-01-01", sizeof(cal->validFrom) - 1);
    strncpy(cal->validTo,   "2099-12-31", sizeof(cal->validTo)   - 1);
    cal->daysOfWeek = 0; /* all days */
}

/* ─── Public API ─────────────────────────────────────────────────────────── */

int talq_init(talq_set_level_fn    set_level,
              talq_read_sensors_fn read_sensors,
              talq_get_time_fn     get_time)
{
    if (!set_level || !read_sensors || !get_time) {
        ESP_LOGE(TAG, "talq_init: NULL callback");
        return -EINVAL;
    }
    if (s_initialised) return 0;

    s_set_level    = set_level;
    s_read_sensors = read_sensors;
    s_get_time     = get_time;

    s_event_lock = xSemaphoreCreateMutex();
    if (!s_event_lock) return -ENOMEM;

    memset(s_events, 0, sizeof(s_events));
    s_event_head = 0;
    s_event_seq  = 0;

    seed_default_model();
    s_initialised = true;
    ESP_LOGI(TAG, "TALQ %s gateway initialised · node id: %s",
             TALQ_SPEC_VERSION, s_node.id);
    return 0;
}

int talq_httpd_start(void)
{
    if (!s_initialised) {
        ESP_LOGE(TAG, "talq_httpd_start called before talq_init");
        return -EINVAL;
    }
    if (s_server) return 0; /* already running */

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn   = httpd_uri_match_wildcard;
    cfg.max_uri_handlers = 8;
    cfg.server_port    = 80;
    cfg.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return -EIO;
    }

    /* Register one wildcard handler per HTTP method */
    static const httpd_method_t methods[] = {
        HTTP_GET, HTTP_POST, HTTP_PUT
    };
    for (int i = 0; i < (int)(sizeof(methods)/sizeof(methods[0])); i++) {
        httpd_uri_t u = s_uri_catch_all;
        u.method = methods[i];
        httpd_register_uri_handler(s_server, &u);
    }

    ESP_LOGI(TAG, "TALQ HTTP server listening on port %d", cfg.server_port);
    return 0;
}

void talq_httpd_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "TALQ HTTP server stopped");
    }
}

int talq_command_execute(const talq_command_t *cmd)
{
    if (!cmd) return -EINVAL;
    if (strcmp(cmd->nodeId, s_node.id) != 0) return -EINVAL;

    switch (cmd->type) {
    case TALQ_CMD_SET_ABSOLUTE_LEVEL: {
        float level = cmd->level;
        if (level < 0.0f) level = 0.0f;
        if (level > 1.0f) level = 1.0f;

        /* Mix warm/cool channels based on CCT target */
        float warm = 1.0f, cool = 0.0f;
        if (cmd->cct > 0 && level > 0.0f) {
            /* Linear CCT mixing between 2700 K (warm=1, cool=0)
               and 5000 K (warm=0, cool=1) */
            float t = (float)(cmd->cct - 2700) / (float)(5000 - 2700);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            warm = (1.0f - t) * level;
            cool = t            * level;
        } else {
            warm = level;
            cool = level;
        }

        s_set_level(TALQ_CHANNEL_WARM, warm, cmd->fadeTime);
        s_set_level(TALQ_CHANNEL_COOL, cool, cmd->fadeTime);
        s_devices[TALQ_CHANNEL_WARM].currentLevel = warm;
        s_devices[TALQ_CHANNEL_COOL].currentLevel = cool;
        s_node.currentLevel = level;
        s_node.currentCct   = cmd->cct > 0 ? cmd->cct : 4000;

        char detail[64];
        snprintf(detail, sizeof(detail), "level=%.2f cct=%uK fade=%.1fs",
                 (double)level, cmd->cct, (double)cmd->fadeTime);
        talq_post_event(TALQ_EVENT_LEVEL_CHANGED, s_node.id, detail);
        ESP_LOGI(TAG, "Set level: warm=%.2f cool=%.2f cct=%uK",
                 (double)warm, (double)cool, cmd->cct);
        return 0;
    }

    case TALQ_CMD_ACTIVATE_GROUP: {
        for (uint8_t i = 0; i < s_group_count; i++) {
            if (strcmp(s_groups[i].id, cmd->programGroupId) == 0) {
                strncpy(s_node.activeProgramGroupId, cmd->programGroupId,
                        TALQ_UUID_LEN - 1);
                talq_post_event(TALQ_EVENT_CONFIG_CHANGED,
                                s_node.id, "program group activated");
                ESP_LOGI(TAG, "Activated program group: %s", s_groups[i].name);
                return 0;
            }
        }
        return -EINVAL;
    }

    case TALQ_CMD_RESET:
        s_node.status = TALQ_NODE_STATUS_OK;
        s_devices[TALQ_CHANNEL_WARM].fault = false;
        s_devices[TALQ_CHANNEL_COOL].fault = false;
        talq_post_event(TALQ_EVENT_FAULT_CLEARED, s_node.id, "manual reset");
        ESP_LOGI(TAG, "Node reset");
        return 0;

    case TALQ_CMD_IDENTIFY:
        /* Flash both channels 3× for 100 ms each to identify the fixture */
        for (int pulse = 0; pulse < 3; pulse++) {
            s_set_level(TALQ_CHANNEL_WARM, 1.0f, 0.0f);
            s_set_level(TALQ_CHANNEL_COOL, 1.0f, 0.0f);
            vTaskDelay(pdMS_TO_TICKS(100));
            s_set_level(TALQ_CHANNEL_WARM, s_devices[TALQ_CHANNEL_WARM].currentLevel, 0.0f);
            s_set_level(TALQ_CHANNEL_COOL, s_devices[TALQ_CHANNEL_COOL].currentLevel, 0.0f);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        talq_post_event(TALQ_EVENT_STATUS_CHANGED, s_node.id, "identify executed");
        return 0;

    default:
        return -ENOTSUP;
    }
}

void talq_tick(void)
{
    if (!s_initialised) return;

    /* Refresh sensor data */
    if (s_read_sensors) {
        s_read_sensors(&s_node);
        for (uint8_t i = 0; i < TALQ_CHANNEL_COUNT; i++) {
            /* Update per-device fault state from node status */
            if (s_node.status == TALQ_NODE_STATUS_FAILURE) {
                if (!s_devices[i].fault) {
                    s_devices[i].fault = true;
                    talq_post_event(TALQ_EVENT_FAULT_RAISED,
                                    s_devices[i].id, "driver fault detected");
                }
            }
        }
    }

    /* Calendar-driven schedule evaluation:
       Find the active calendar entry for today and determine which program
       step should be active right now, then apply it if it has changed. */
    if (s_get_time) {
        char now_str[21];
        s_get_time(now_str, sizeof(now_str));
        /* Parse HH:MM from "YYYY-MM-DDTHH:MM:SSZ" */
        int hour = 0, minute = 0;
        if (strlen(now_str) >= 16) {
            hour   = (now_str[11] - '0') * 10 + (now_str[12] - '0');
            minute = (now_str[14] - '0') * 10 + (now_str[15] - '0');
        }
        int now_minutes = hour * 60 + minute;

        const talq_program_group_t *active = talq_get_active_program_group();
        if (active && active->stepCount > 0) {
            /* Find the most recent step that has passed */
            const talq_program_step_t *target = &active->steps[0];
            for (uint8_t i = 0; i < active->stepCount; i++) {
                int step_minutes = active->steps[i].hour * 60
                                 + active->steps[i].minute;
                if (step_minutes <= now_minutes) {
                    target = &active->steps[i];
                }
            }
            /* Apply if different from current level */
            float eps = 0.005f;
            if (fabsf(target->level - s_node.currentLevel) > eps ||
                (target->cct_kelvin > 0 &&
                 target->cct_kelvin != s_node.currentCct)) {
                talq_command_t cmd = {
                    .type     = TALQ_CMD_SET_ABSOLUTE_LEVEL,
                    .level    = target->level,
                    .cct      = target->cct_kelvin,
                    .fadeTime = target->fadeTime,
                };
                strncpy(cmd.nodeId, s_node.id, TALQ_UUID_LEN - 1);
                talq_command_execute(&cmd);
            }
        }
    }
}

const talq_node_t *talq_get_node(void)
{
    return &s_node;
}

const talq_program_group_t *talq_get_active_program_group(void)
{
    if (!s_initialised) return NULL;
    for (uint8_t i = 0; i < s_group_count; i++) {
        if (strcmp(s_groups[i].id, s_node.activeProgramGroupId) == 0) {
            return &s_groups[i];
        }
    }
    return NULL;
}

void talq_post_event(talq_event_type_t type,
                     const char        *sourceId,
                     const char        *detail)
{
    if (!s_event_lock) return;
    xSemaphoreTake(s_event_lock, portMAX_DELAY);

    talq_event_t *ev = &s_events[s_event_head % TALQ_MAX_EVENTS];
    ev->seqNo = ++s_event_seq;
    get_now(ev->timestamp, sizeof(ev->timestamp));
    ev->type  = type;
    if (sourceId) strncpy(ev->sourceId, sourceId, TALQ_UUID_LEN - 1);
    if (detail)   strncpy(ev->detail,   detail,   sizeof(ev->detail) - 1);
    s_event_head++;

    xSemaphoreGive(s_event_lock);
}
