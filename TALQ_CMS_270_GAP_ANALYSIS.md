# TALQ 2.7.0 CMS Certification — Gap Analysis

**Reference spec:** `talq-api-cms-2-7-0-online.json` (TALQ Consortium, v2.7.0)  
**D-SLS spec:** `dsls_openapi.yaml` (D-SLS DIAMANT gateway, TALQ v2.3.0 based)  
**Generated:** 2026-03-05  

---

## Executive Summary

| Metric | Value |
|--------|-------|
| TALQ 2.7.0 CMS total endpoint operations | **92** |
| D-SLS currently implemented operations | **16** |
| **Completely missing resource groups** | **16** |
| **Partially implemented resource groups** | **2** |
| Coverage | **~17%** |

The D-SLS DIAMANT device implements a **TALQ Gateway API (v2.3.0)** that
exposes node/device control endpoints.  The **TALQ 2.7.0 CMS API** defines a
much larger surface — the management plane that a Central Management Software
must expose.  For full CMS certification the D-SLS implementation must add **88
new endpoint operations**, correct schema/field naming across all existing
endpoints, and add required HTTP headers.

---

## 1  Missing Endpoint Groups (not implemented at all)

### 1.1  `/bracket-types` — Bracket-type catalogue

TALQ 2.7.0 requires the CMS to maintain a catalogue of bracket (mounting-pole)
types used in the outdoor device network.

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/bracket-types` | getBracketTypes | ❌ Missing |
| POST   | `/bracket-types` | addBracketTypes | ❌ Missing |
| PUT    | `/bracket-types` | updateBracketTypes | ❌ Missing |
| GET    | `/bracket-types/count` | getBracketTypesCount | ❌ Missing |
| GET    | `/bracket-types/{bracketTypeAddress}` | getBracketType | ❌ Missing |
| PUT    | `/bracket-types/{bracketTypeAddress}` | updateBracketType | ❌ Missing |
| DELETE | `/bracket-types/{bracketTypeAddress}` | deleteBracketType | ❌ Missing |

---

### 1.2  `/control-device-dali-part103-types` — DALI Part-103 control-device types

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/control-device-dali-part103-types` | getControlDeviceDaliPart103Types | ❌ Missing |
| POST   | `/control-device-dali-part103-types` | addControlDeviceDaliPart103Types | ❌ Missing |
| GET    | `/control-device-dali-part103-types/count` | getControlDeviceDaliPart103TypesCount | ❌ Missing |
| GET    | `/control-device-dali-part103-types/{controldevicedalipart103Address}` | getControlDeviceDaliPart103Type | ❌ Missing |
| PUT    | `/control-device-dali-part103-types/{controldevicedalipart103Address}` | updateControlDeviceDaliPart103Type | ❌ Missing |
| DELETE | `/control-device-dali-part103-types/{controldevicedalipart103Address}` | deleteControlDeviceDaliPart103Type | ❌ Missing |

---

### 1.3  `/control-gear-dali-part102-types` — DALI Part-102 control-gear types

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/control-gear-dali-part102-types` | getControlGearDaliPart102Types | ❌ Missing |
| POST   | `/control-gear-dali-part102-types` | addControlGearDaliPart102Types | ❌ Missing |
| GET    | `/control-gear-dali-part102-types/count` | getControlGearDaliPart102TypesCount | ❌ Missing |
| GET    | `/control-gear-dali-part102-types/{controlgeardalipart102Address}` | getControlGearDaliPart102Type | ❌ Missing |
| PUT    | `/control-gear-dali-part102-types/{controlgeardalipart102Address}` | updateControlGearDaliPart102Type | ❌ Missing |
| DELETE | `/control-gear-dali-part102-types/{controlgeardalipart102Address}` | deleteControlGearDaliPart102Type | ❌ Missing |

---

### 1.4  `/controller-types` — Controller hardware type catalogue

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/controller-types` | getControllerTypes | ❌ Missing |
| POST   | `/controller-types` | addControllerTypes | ❌ Missing |
| PUT    | `/controller-types` | updateControllerTypes | ❌ Missing |
| GET    | `/controller-types/count` | getControllerTypesCount | ❌ Missing |
| GET    | `/controller-types/{controllerTypeAddress}` | getControllerType | ❌ Missing |
| PUT    | `/controller-types/{controllerTypeAddress}` | updateControllerType | ❌ Missing |
| DELETE | `/controller-types/{controllerTypeAddress}` | deleteControllerType | ❌ Missing |

---

### 1.5  `/data-packages` — Firmware / configuration data packages

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/data-packages` | getDataPackages | ❌ Missing |
| GET    | `/data-packages/{packageAddress}` | getDataPackage | ❌ Missing |

---

### 1.6  `/device-classes` — Device classification

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| POST   | `/device-classes` | addDeviceClasses | ❌ Missing |
| PUT    | `/device-classes` | updateDeviceClasses | ❌ Missing |
| PUT    | `/device-classes/{className}` | updateDeviceClass | ❌ Missing |
| DELETE | `/device-classes/{className}` | deleteDeviceClass | ❌ Missing |

---

### 1.7  `/diagnostics-and-maintenance-data-dali-part253-types` — DALI Part-253 D&M types

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/diagnostics-and-maintenance-data-dali-part253-types` | getDiagnosticsAndMaintenanceDataDaliPart253Types | ❌ Missing |
| POST   | `/diagnostics-and-maintenance-data-dali-part253-types` | addDiagnosticsAndMaintenanceDataDaliPart253Types | ❌ Missing |
| GET    | `/diagnostics-and-maintenance-data-dali-part253-types/count` | getDiagnosticsAndMaintenanceDataDaliPart253TypesCount | ❌ Missing |
| GET    | `/diagnostics-and-maintenance-data-dali-part253-types/{…Address}` | getDiagnosticsAndMaintenanceDataDaliPart253Type | ❌ Missing |
| PUT    | `/diagnostics-and-maintenance-data-dali-part253-types/{…Address}` | updateDiagnosticsAndMaintenanceDataDaliPart253Type | ❌ Missing |
| DELETE | `/diagnostics-and-maintenance-data-dali-part253-types/{…Address}` | deleteDiagnosticsAndMaintenanceDataDaliPart253Type | ❌ Missing |

---

### 1.8  `/driver-types` — LED driver type catalogue

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/driver-types` | getDriverTypes | ❌ Missing |
| POST   | `/driver-types` | addDriverTypes | ❌ Missing |
| PUT    | `/driver-types` | updateDriverTypes | ❌ Missing |
| GET    | `/driver-types/count` | getDriverTypesCount | ❌ Missing |
| GET    | `/driver-types/{driverTypeAddress}` | getDriverType | ❌ Missing |
| PUT    | `/driver-types/{driverTypeAddress}` | updateDriverType | ❌ Missing |
| DELETE | `/driver-types/{driverTypeAddress}` | deleteDriverType | ❌ Missing |

---

### 1.9  `/energy-data-dali-part252-types` — DALI Part-252 energy data types

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/energy-data-dali-part252-types` | getEnergyDataDaliPart252Types | ❌ Missing |
| POST   | `/energy-data-dali-part252-types` | addEnergyDataDaliPart252Types | ❌ Missing |
| GET    | `/energy-data-dali-part252-types/count` | getEnergyDataDaliPart252TypesCount | ❌ Missing |
| GET    | `/energy-data-dali-part252-types/{energydatadalipart252Address}` | getEnergyDataDaliPart252Type | ❌ Missing |
| PUT    | `/energy-data-dali-part252-types/{energydatadalipart252Address}` | updateEnergyDataDaliPart252Type | ❌ Missing |
| DELETE | `/energy-data-dali-part252-types/{energydatadalipart252Address}` | deleteEnergyDataDaliPart252Type | ❌ Missing |

---

### 1.10  `/lamp-types` — Lamp/LED module type catalogue

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| GET    | `/lamp-types` | getLampTypes | ❌ Missing |
| POST   | `/lamp-types` | addLampTypes | ❌ Missing |
| PUT    | `/lamp-types` | updateLampTypes | ❌ Missing |
| GET    | `/lamp-types/count` | getLampTypesCount | ❌ Missing |
| GET    | `/lamp-types/{lampTypeAddress}` | getLampType | ❌ Missing |
| PUT    | `/lamp-types/{lampTypeAddress}` | updateLampType | ❌ Missing |
| DELETE | `/lamp-types/{lampTypeAddress}` | deleteLampType | ❌ Missing |

---

### 1.11  `/log-reports` and `/logger-configs` — Logging

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| POST   | `/log-reports` | addLogReports | ❌ Missing |
| GET    | `/logger-configs/{loggerAddress}` | getLoggerConfig | ❌ Missing |

---

### 1.12  `/luminaire-*` — Luminaire type catalogues (DALI Parts 251 & 351)

#### `/luminaire-data-dali-part251-types`

| Method | Path | Status |
|--------|------|--------|
| GET    | `/luminaire-data-dali-part251-types` | ❌ Missing |
| POST   | `/luminaire-data-dali-part251-types` | ❌ Missing |
| GET    | `/luminaire-data-dali-part251-types/count` | ❌ Missing |
| GET/PUT/DELETE | `/luminaire-data-dali-part251-types/{…Address}` | ❌ Missing |

#### `/luminaire-mounted-control-device-dali-part351-types`

| Method | Path | Status |
|--------|------|--------|
| GET    | `/luminaire-mounted-control-device-dali-part351-types` | ❌ Missing |
| POST   | `/luminaire-mounted-control-device-dali-part351-types` | ❌ Missing |
| GET    | `/luminaire-mounted-control-device-dali-part351-types/count` | ❌ Missing |
| GET/PUT/DELETE | `/luminaire-mounted-control-device-dali-part351-types/{…Address}` | ❌ Missing |

#### `/luminaire-types`

| Method | Path | Status |
|--------|------|--------|
| GET    | `/luminaire-types` | ❌ Missing |
| POST   | `/luminaire-types` | ❌ Missing |
| PUT    | `/luminaire-types` | ❌ Missing |
| GET    | `/luminaire-types/count` | ❌ Missing |
| GET/PUT/DELETE | `/luminaire-types/{luminaireTypeAddress}` | ❌ Missing |

---

### 1.13  `/services` — Service registration

| Method | Path | Operation ID | Status |
|--------|------|-------------|--------|
| POST   | `/services` | addServices | ❌ Missing |

---

## 2  Partially Implemented Endpoints — Missing Methods

### 2.1  `/devices`

The D-SLS exposes `GET /talq/devices` and `GET /talq/devices/{deviceId}` (read-only).  
TALQ 2.7.0 CMS requires full CRUD plus bulk-update support:

| Method | Path | Operation ID | D-SLS | TALQ 2.7.0 |
|--------|------|-------------|-------|------------|
| GET    | `/devices` | getDevices | ⚠️ Exists (wrong path/schema — see §3) | ✅ Required |
| POST   | `/devices` | addDevices | ❌ Missing | ✅ Required |
| PUT    | `/devices` | updateDevices | ❌ Missing | ✅ Required |
| PATCH  | `/devices` | partialUpdateDevices | ❌ Missing | ✅ Required |
| GET    | `/devices/{deviceAddress}` | getDevice | ⚠️ Exists (wrong path/schema) | ✅ Required |
| PUT    | `/devices/{deviceAddress}` | updateDevice | ❌ Missing | ✅ Required |
| PATCH  | `/devices/{deviceAddress}` | partialUpdateDevice | ❌ Missing | ✅ Required |
| DELETE | `/devices/{deviceAddress}` | deleteDevice | ❌ Missing | ✅ Required |
| GET    | `/devices/{deviceAddress}/{functionId}` | getDeviceFunction | ❌ Missing | ✅ Required |
| GET    | `/devices/{deviceAddress}/{functionId}/{attributeName}` | getDeviceAttribute | ❌ Missing | ✅ Required |

### 2.2  `/groups`

D-SLS exposes `GET /talq/programGroups` and friends.  
The CMS API uses a different path (`/groups`) with read-only semantics:

| Method | Path | Operation ID | D-SLS | TALQ 2.7.0 |
|--------|------|-------------|-------|------------|
| GET    | `/groups` | getGroups | ⚠️ Wrong path (`/talq/programGroups`) | ✅ Required |
| GET    | `/groups/{groupAddress}` | getGroup | ⚠️ Wrong path | ✅ Required |

> **Note:** The TALQ 2.7.0 CMS `/groups` endpoint returns a read-only view of
> device groups configured by the Gateway, not program-group schedules.  The
> D-SLS `/talq/programGroups` resource is a different (Gateway-facing) concept
> and does not satisfy this requirement.

---

## 3  Wrong Fields / Schema Mismatches on Existing Endpoints

### 3.1  Base path prefix

| | D-SLS | TALQ 2.7.0 CMS |
|---|---|---|
| Base URL | `/talq/*` | `/` (no prefix) |

**Action required:** Remove the `/talq` prefix from all CMS endpoints, or mount the CMS API at the root path.

---

### 3.2  Required HTTP request headers missing

Every TALQ 2.7.0 CMS operation requires the following headers.  The D-SLS
implementation does not validate or process them:

| Header | Required on | Description |
|--------|-------------|-------------|
| `talq-api-version` | All operations (GET & mutating) | Must be set to `"2.7.0"` |
| `clientAddress` | All operations | UUID of the calling CMS client |
| `talqRequestId` | All mutating operations (POST/PUT/PATCH/DELETE) | UUID for idempotency |
| `talqOriginRequestId` | All mutating operations | Original request UUID for retry tracking |

---

### 3.3  `/devices` — schema field names

The TALQ 2.7.0 CMS `Device` schema (from `talq-data-model-2-7-0-online.json`)
differs significantly from the D-SLS device object:

| Field | D-SLS (`dsls_openapi.yaml`) | TALQ 2.7.0 (`Device` schema) | Issue |
|-------|-----------------------------|------------------------------|-------|
| `talq:id` | UUID string | `address` (URI / IRI string, not UUID) | ❌ Wrong type & name |
| `talq:name` | free string | `name` (no `talq:` prefix in JSON-LD) | ❌ Wrong namespace prefix |
| `talq:channel` | integer 0/1 (custom) | not present — LED channel is a `Function` attribute | ❌ Non-standard field |
| `talq:nominalCct` | integer (custom) | not present at device level | ❌ Non-standard field |
| `talq:maxPowerWatts` | number (custom) | not present at device level | ❌ Non-standard field |
| `talq:level` | 0.0–1.0 float | attribute inside `Function` object | ❌ Wrong location |
| `talq:fault` | boolean (custom) | `deviceStatus` enum (`ok`/`warning`/`failure`) | ❌ Wrong field & type |
| *(absent)* | — | `address` | ❌ Missing required field |
| *(absent)* | — | `deviceClass` | ❌ Missing required field |
| *(absent)* | — | `functions[]` | ❌ Missing required field |
| *(absent)* | — | `controllerTypeAddress` | ❌ Missing required field |

---

### 3.4  `/devices/{deviceAddress}/{functionId}` — `Function` object not implemented

TALQ 2.7.0 models device capabilities as `Function` objects nested under each
device.  For a DALI-2 DT8 luminaire (which the DIAMANT uses) the expected
functions include:

- `LightingOutputFunction` — controls `reportedLevel`, `actualLevelValue`,
  `commandedLevel`, `commandedColourTemperature`, `configuredFadeTime`, etc.
- `SensorFunction` — for the TSL2591 ambient-light sensor
- `EnergyMonitoringFunction` — for cumulative kWh metering (ADS1115)

None of these are modelled in the current D-SLS API.

---

### 3.5  `/nodes` — Non-standard resource

The D-SLS `/talq/nodes` resource has no equivalent in the TALQ 2.7.0 CMS API.  
The CMS API manages **devices** directly.  The notion of a "node" is specific
to the v2.3 draft and is not part of the certified CMS data model.

**Action required:** Replace `/nodes` with a proper `/devices` implementation
that uses the `Device` + `Function` schema from the TALQ 2.7.0 data model.

---

### 3.6  `/programGroups` — Non-standard resource

The D-SLS `/talq/programGroups` (with its 24-step schedule) does not map to any
TALQ 2.7.0 CMS endpoint.

In TALQ 2.7.0, lighting schedules are represented through:
- `Calendar` → references `Programme` objects → references `Programme entries`  
- These are managed through the **Gateway API** (`talq-api-gateway-2-7-0-online.json`),  
  not the CMS API.

**Action required:**  
The CMS API does not expose `/programGroups`.  Remove this endpoint from the
CMS-facing surface.  Schedules must be pushed to the Gateway through the
Gateway API.

---

### 3.7  `/calendars` — Non-standard resource

Same as §3.6.  The TALQ 2.7.0 CMS API does not include a `/calendars`
endpoint.  Calendar/programme management is handled through the Gateway API.

---

### 3.8  `/commands` — Non-standard resource

The D-SLS `/talq/commands` POST endpoint accepts a proprietary command JSON
(`talq:commandType`, `talq:nodeId`, `talq:level`, etc.) that has no equivalent
in the TALQ 2.7.0 CMS API.

In TALQ 2.7.0, commands are issued by writing attribute values to a device's
`Function` via `PATCH /devices/{deviceAddress}` or by calling service
operations via `POST /services`.

**Action required:**  
Replace `/commands` with PATCH-based attribute writes on `/devices/{address}`
and/or a `POST /services` service call.

---

### 3.9  `/events` — Non-standard resource

The D-SLS `/talq/events` polling endpoint has no direct equivalent in the TALQ
2.7.0 CMS API.  In TALQ 2.7.0:

- The **Gateway** pushes log reports to the CMS via `POST /log-reports`
  (CMS exposes this endpoint to receive incoming push reports).
- The **CMS** retrieves logger configuration via `GET /logger-configs/{loggerAddress}`.

**Action required:**  
Implement `POST /log-reports` (receive push from Gateway) and
`GET /logger-configs/{loggerAddress}` instead of the polling `/events` endpoint.

---

### 3.10  `talq:specVersion` field in Gateway response

The gateway object currently advertises `"talq:specVersion": "2.3.0"`.
For TALQ 2.7.0 CMS certification this must be `"2.7.0"`.

---

### 3.11  Error response schema

D-SLS returns a non-standard error body:

```json
{
  "talq:error": "not found",
  "talq:message": "node not found"
}
```

TALQ 2.7.0 requires a `TALQErrorMessage` array response:

```json
[
  {
    "errorCode": "TALQ_NOT_FOUND",
    "errorDescription": "Device address xyz not found",
    "requestId": "uuid-of-the-failed-request"
  }
]
```

**Action required:**  
Change all 4xx error responses to return an array of `TALQErrorMessage` objects
as defined in `talq-data-model-2-7-0-online.json`.

---

### 3.12  HTTP response codes

| Scenario | D-SLS | TALQ 2.7.0 |
|----------|-------|------------|
| Successful creation | `201` | `201` ✅ |
| Resource not found  | `404` | `404` ✅ |
| Validation error    | `400` | `422` ❌ (D-SLS overloads 400 for both) |
| Duplicate / conflict | not returned | `409` ❌ Missing |
| Invalid API version  | not checked | `400` ❌ Missing |

---

## 4  Summary of Required Changes

### 4.1  New endpoint groups to implement (76 operations)

| Group | # Operations |
|-------|-------------|
| `/bracket-types` | 7 |
| `/control-device-dali-part103-types` | 6 |
| `/control-gear-dali-part102-types` | 6 |
| `/controller-types` | 7 |
| `/data-packages` | 2 |
| `/device-classes` | 4 |
| `/devices` (missing methods) | 8 |
| `/diagnostics-and-maintenance-data-dali-part253-types` | 6 |
| `/driver-types` | 7 |
| `/energy-data-dali-part252-types` | 6 |
| `/lamp-types` | 7 |
| `/log-reports` + `/logger-configs` | 2 |
| `/luminaire-data-dali-part251-types` | 6 |
| `/luminaire-mounted-control-device-dali-part351-types` | 6 |
| `/luminaire-types` | 7 |
| `/services` | 1 |
| **Total new operations to implement** | **88** |

### 4.2  Field / schema corrections required

| # | Area | Change |
|---|------|--------|
| 1 | Base path | Remove `/talq` prefix from all CMS endpoints |
| 2 | All endpoints | Add `talq-api-version`, `clientAddress`, `talqRequestId`, `talqOriginRequestId` header validation |
| 3 | `Device` schema | Replace custom fields with TALQ 2.7.0 `Device` + `Function` object model |
| 4 | `/nodes` | Replace with CMS-compliant `/devices` (TALQ data model `Device`) |
| 5 | `/programGroups` | Remove from CMS surface; move schedule management to Gateway API |
| 6 | `/calendars` | Remove from CMS surface; move to Gateway API |
| 7 | `/commands` | Replace with PATCH attribute writes on devices + `POST /services` |
| 8 | `/events` | Replace with `POST /log-reports` + `GET /logger-configs` |
| 9 | Error responses | Return `TALQErrorMessage[]` array instead of custom error object |
| 10 | HTTP status 400 vs 422 | Use `422` for validation/semantic errors, `400` only for malformed syntax |
| 11 | HTTP status 409 | Return `409 Conflict` when a resource already exists |
| 12 | `talq:specVersion` | Change from `"2.3.0"` to `"2.7.0"` |

---

## 5  Files in This Repository

| File | Description |
|------|-------------|
| `dsls_openapi.yaml` | Current D-SLS DIAMANT gateway OpenAPI specification (TALQ v2.3.0 based) |
| `talq-api-cms-2-7-0-online.json` | Official TALQ 2.7.0 CMS API specification (TALQ Consortium) |
| `TALQ_CMS_270_GAP_ANALYSIS.md` | This document — gap analysis for CMS certification |

---

*Reference: TALQ Consortium, https://github.com/TALQ-consortium/TALQ_specification*
