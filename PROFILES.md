# DIAMANT v2.1 REV E - Complete Profile Specification

## Overview

The DIAMANT fusion system implements 38 unique profiles across 7 categories, each with hexadecimal IDs for hardware traceability and priority-based conflict resolution.

## Profile Categories

### 1. LED Profiles (0x00-0x09) - 10 profiles
Controls LED output with adaptive brightness and color temperature.

| ID   | Profile           | Intensity | CCT   | Priority | Night-Saver | Activation Conditions |
|------|-------------------|-----------|-------|----------|-------------|-----------------------|
| 0x00 | SUNSET_ON         | 100%      | 2700K | P5       | ✅          | Au coucher du soleil |
| 0x01 | TWILIGHT_SOFT     | 80%       | 2800K | P5       | ✅          | Crépuscule civil +30min |
| 0x02 | MIDNIGHT_ECO      | 60%       | 3000K | P5       | ✅          | 00:00-03:00 · SOC>40% |
| 0x03 | EARLY_MORNING     | 40%       | 4000K | P5       | ✅          | 03:00→sunrise |
| 0x04 | FOG_BOOST         | 100%      | 5000K | P2       | ❌          | TSL2591<0.5lux+PV low |
| 0x05 | CLEAR_NIGHT       | 60%       | 3000K | P5       | ✅          | Nuit+ciel clair |
| 0x06 | LOW_SOC_PROTECT   | 30%       | 2700K | P3       | ❌          | SOC<20% |
| 0x07 | WINTER_MODE       | 70%       | 3000K | P6       | ✅          | Saison hiver |
| 0x08 | SUMMER_MODE       | 50%       | 2700K | P6       | ✅          | Saison été |
| 0x09 | RAIN_ALERT        | 90%       | 5000K | P2       | ❌          | PV drop>60% <5min |

**Night-Saver Algorithm:**
- SOC ≥60%: 100% brightness (1.0×)
- SOC 40-59%: 80% brightness (0.8×)
- SOC 20-39%: 60% brightness (0.6×)
- SOC <20%: 30% brightness (0.3×) - minimum for safety

**Dynamic CCT Control:**
- Linear interpolation between warm (2700K) and cool (5000K) LEDs
- Automatic mixing ratio calculation based on target CCT
- Hardware: LT8391 #1 (warm) + LT8391 #2 (cool)

### 2. MPPT Profiles (0x10-0x16) - 7 profiles
Solar charge controller optimization based on weather and light conditions.

| ID   | Profile           | Step Mult | Period Mult | Max I-Limit | Conditions |
|------|-------------------|-----------|-------------|-------------|------------|
| 0x10 | AGGRESSIVE_SUNNY  | 0.5×      | 1.0×        | 1.0 (100%)  | Ciel clair·PV stable |
| 0x11 | ADAPTIVE_CLOUDY   | 1.0×      | 1.0×        | 1.0 (100%)  | Nuages variables |
| 0x12 | LOW_LIGHT         | 2.0×      | 3.0×        | 1.0 (100%)  | Matin/soir/hiver |
| 0x13 | WINTER_PROTECT    | 1.0×      | 1.0×        | 0.5 (50%)   | Froid TMP117<5°C |
| 0x14 | SAFE_CHARGE       | 1.0×      | 1.0×        | 0.1 (10%)   | T<0°C LFP/5°C NMC |
| 0x15 | CLOUD_EDGE_BOOST  | 1.0×      | 0.5×        | 1.0 (100%)  | Pics PV courts |
| 0x16 | EVENING_SAVE      | 0.0×      | 10.0×       | 0.1 (10%)   | Après sunset·PV<10% |

**P&O Algorithm Adaptation:**
- Step Multiplier: Adjusts voltage step size for MPPT tracking
- Period Multiplier: Changes update frequency (lower = faster tracking)
- Max I-Limit: Thermal/safety current limiting

### 3. V2G Profiles (0x20-0x26) - 7 profiles
Bidirectional grid integration for vehicle-to-grid and grid-to-vehicle power flow.

| ID   | Profile           | Power Flow | Priority | Conditions |
|------|-------------------|------------|----------|------------|
| 0x20 | EXPORT_DAY_SUNNY  | +500W      | P0       | Ciel clair+PV fort+SOC>80% |
| 0x21 | EXPORT_CLOUD_EDGE | +200W      | P0       | Nuages+SOC>60% |
| 0x22 | IMPORT_LOW_SOC    | -300W      | P3       | SOC<20% |
| 0x23 | IMPORT_WINTER     | -150W      | P6       | Hiver·PV faible·SOC<60% |
| 0x24 | HOLD_BALANCED     | 0W         | P7       | SOC 40-70%·pas de condition |
| 0x25 | DISCHARGE_PEAK    | +350W      | P6       | Heures de pointe réseau |
| 0x26 | EMERGENCY_ISLAND  | 0W         | P0       | Perte réseau (LTC4364) |

**Power Flow Convention:**
- Positive (+): Export to grid (discharge battery)
- Negative (-): Import from grid (charge battery)
- Zero (0): Hold/island mode (no grid interaction)

### 4. Battery Profiles (0x30-0x33) - 4 profiles
Thermal and SOC-based battery protection.

| ID   | Profile           | Charge Limit | Priority | Conditions |
|------|-------------------|--------------|----------|------------|
| 0x30 | SAFE_CHARGE       | 0.1 (10%)    | P0       | T<0°C LFP ou T<5°C NMC |
| 0x31 | SUMMER_LIMIT      | 0.5 (50%)    | P1       | T>40°C dissipateur |
| 0x32 | WINTER_LIMIT      | 0.7 (70%)    | P2       | Saison hiver |
| 0x33 | CRITICAL_SOC      | 0.05 (5%)    | P0       | SOC<10% protection |

### 5. Weather Detection Profiles (0x40-0x43) - 4 profiles
Environmental condition detection for adaptive control.

| ID   | Profile           | Detection Criteria | Sensors |
|------|-------------------|--------------------|---------|
| 0x40 | FOG_DETECT        | TSL2591<0.5lux + PV faible | TSL2591+ADS1115 |
| 0x41 | CLOUD_DETECT      | PV ratio<0.6 fluctuant | ADS1115 PV |
| 0x42 | RAIN_DETECT       | PV chute>60% en <5min | ADS1115+TSL2591 |
| 0x43 | HEAT_ALERT        | TMP117>45°C surchauffe | TMP117#2 heatsink |

### 6. Astronomical Profiles (0x50-0x52) - 3 profiles
Solar position calculations for time-based automation.

| ID   | Profile           | Function | Algorithm |
|------|-------------------|----------|-----------|
| 0x50 | SUNRISE           | Sunrise calculation | Meeus+DS3231 |
| 0x51 | SUNSET            | Sunset calculation | Meeus+DS3231 |
| 0x52 | DAY_LENGTH        | Day duration | Calcul durée jour |

### 7. Calendar Profiles (0x60-0x62) - 3 profiles
Time-based scheduling and seasonal adjustments.

| ID   | Profile           | Detection | Storage |
|------|-------------------|-----------|---------|
| 0x60 | WEEKEND           | Samedi/Dimanche | DS3231 RTC |
| 0x61 | HOLIDAY           | Jours fériés | MRAM bitfield |
| 0x62 | SEASON_CHANGE     | Solstice/équinoxe | Astronomique |

## Priority System (P0-P7)

The fusion system uses 8 priority levels to resolve conflicts when multiple profiles are applicable:

- **P0 (Highest)**: Emergency/Safety
  - Island mode (grid loss)
  - Critical battery temperature
  - Critical SOC (<10%)
  
- **P1**: Thermal Limits
  - Overheat protection
  - Cold weather charging limits
  
- **P2**: Weather Safety
  - Fog boost (visibility)
  - Rain alert
  - Heat warnings
  
- **P3**: SOC Protection
  - Low battery protection (<20%)
  
- **P4**: Battery Management
  - Routine battery protection
  
- **P5**: Astronomical Events
  - Sunrise/sunset timing
  - Time-of-day profiles
  
- **P6**: Calendar Events
  - Seasonal modes
  - Weekend/holiday schedules
  
- **P7 (Lowest)**: Default Operation
  - Balanced/normal mode

**Conflict Resolution:**
When multiple profiles are active, the system selects the profile with the **lowest priority number** (highest priority). Within the same priority level, more recent conditions take precedence.

## MRAM Logging Structure

All profile activations and system events are logged to MRAM with structured data:

### Log Types
- **Type 1**: MPPT tracking (voltage, current, duty, efficiency)
- **Type 2**: V2G power flow (power, voltage, mode, flags)
- **Type 3**: BMS battery data (SOC, voltage, current, temps)
- **Type 7**: Thermal monitoring (board, heatsink, battery temps)
- **Type 9**: Astronomical events (sunrise/sunset, weather, calendar)
- **Type 15**: System events (faults, mode changes)

### Log Entry Format (16 bytes)
```c
struct {
    uint32_t timestamp;      // Unix timestamp
    uint8_t log_type;        // 1-15
    uint8_t profile_id;      // 0x00-0x62
    uint8_t priority;        // P0-P7
    uint8_t reserved;
    uint8_t data[8];         // Type-specific data
}
```

## Web API

### Endpoints

**GET /api/profiles**
Returns active profile information in JSON format:
```json
{
  "led_profile": {
    "profile_id": 0,
    "profile_name": "SUNSET_ON",
    "priority": 5,
    "cct_kelvin": 2700,
    "base_intensity": 100,
    "night_saver_enabled": true,
    "conditions": "Au coucher du soleil"
  },
  "mppt_profile": { ... },
  "v2g_profile": { ... },
  "bms": {
    "soc_percent": 65.5,
    "voltage": 48.2,
    "current": 5.5
  }
}
```

## Hardware Integration

### GPIO Mapping
- **GPIO18**: Warm LED PWM (LT8391 #1, 2700K)
- **GPIO19**: Cool LED PWM (LT8391 #2, 5000K)
- **GPIO4-6,22**: SPI (BQ76952 BMS + SSD1306 OLED)
- **GPIO20-21**: I²C (8 sensors @ 400kHz)

### PWM Configuration
- Frequency: 5 kHz
- Resolution: 13-bit (0-8191)
- Duty cycle calculation: `percentage × 8191 / 100`

### I²C Sensors
- **0x29**: TSL2591 (light sensor)
- **0x48**: TMP117 #1 (board temp)
- **0x49**: ADS1115 (16-bit ADC)
- **0x4A**: TMP117 #2 (heatsink temp)
- **0x50**: M24M02 (EEPROM)
- **0x57**: ST25DV64K (NFC)
- **0x68**: DS3231 (RTC)
- **0x6C**: MCP4725 (DAC)

## Performance Characteristics

- **Profile switching**: <10ms latency
- **CCT transitions**: Smooth linear interpolation
- **Night-Saver update**: 100ms cycle
- **MPPT tracking**: 10-300ms depending on profile
- **V2G response**: <50ms for grid events
- **Web API response**: <200ms typical

## Safety Features

1. **Priority Override**: P0 safety profiles always win
2. **Night-Saver Bypass**: Weather/safety profiles ignore SOC dimming
3. **Thermal Protection**: Automatic current limiting
4. **Watchdog**: Task monitoring with auto-recovery
5. **Graceful Degradation**: Falls back to safe defaults on sensor failure

## Testing & Validation

Run the test suite to validate all profiles:
```bash
gcc -o test_profiles test_profiles.c -I main
./test_profiles
```

Expected result: 32+ tests passed, 0 failed

---

**Document Version**: 2.1.0  
**Last Updated**: 2026-02-17  
**System**: DIAMANT v2.1 REV E  
**Copyright**: D-SLS SRL, Milan
