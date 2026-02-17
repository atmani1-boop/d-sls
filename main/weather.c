/**
 * @file weather.c
 * @brief Local weather state detection from sensor data
 * 
 * Classifies weather conditions using sensor inputs from:
 *   - TSL2591: Ambient light sensor (lux)
 *   - TMP117: High-precision temperature sensor
 *   - ADS1115: ADC for PV panel voltage monitoring
 * 
 * Weather states: Clear, Cloudy, Intermittent, Rain, Fog
 * 
 * @note Currently uses simulated sensor readings for development
 */

#include "include/profiles.h"
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WEATHER";

// Weather classification thresholds
#define LUX_THRESHOLD_CLEAR       30000.0f  /**< Clear sky: > 30k lux */
#define LUX_THRESHOLD_CLOUDY      10000.0f  /**< Cloudy: 10k-30k lux */
#define LUX_THRESHOLD_OVERCAST    1000.0f   /**< Overcast/Rain: < 10k lux */
#define LUX_THRESHOLD_FOG         500.0f    /**< Fog: < 500 lux during day */

#define TEMP_RAIN_MAX             15.0f     /**< Rain likely if temp < 15°C + low lux */
#define TEMP_FOG_RANGE_LOW        0.0f      /**< Fog temperature range: 0-10°C */
#define TEMP_FOG_RANGE_HIGH       10.0f

#define PV_VOLTAGE_MIN_CLEAR      12.0f     /**< Minimum PV voltage for clear sky */
#define PV_VOLTAGE_OVERCAST       6.0f      /**< PV voltage in overcast conditions */

#define PV_OSCILLATION_THRESHOLD  0.5f      /**< Voltage change threshold for cloud edge (V) */
#define PV_OSCILLATION_COUNT      3         /**< Number of oscillations to detect intermittent */

/**
 * @brief PV oscillation detection buffer
 */
typedef struct {
    float history[10];          /**< Recent PV voltage readings */
    uint8_t index;              /**< Current index in circular buffer */
    uint8_t count;              /**< Number of valid samples */
    uint32_t last_update_ms;    /**< Last update timestamp */
} pv_oscillation_detector_t;

static pv_oscillation_detector_t pv_detector = {0};

/**
 * @brief Simulate TSL2591 light sensor reading
 * 
 * @return Ambient light in lux
 * @note TODO: Replace with actual I2C driver implementation
 */
static float tsl2591_read_lux(void) {
    // Simulated reading - in production, read from I2C device
    static uint32_t counter = 0;
    counter++;
    
    // Simulate varying light conditions
    if (counter % 100 < 30) {
        return 35000.0f + (counter % 1000) * 10.0f; // Clear
    } else if (counter % 100 < 60) {
        return 15000.0f + (counter % 500) * 5.0f;   // Cloudy
    } else if (counter % 100 < 80) {
        return 5000.0f + (counter % 300) * 3.0f;    // Overcast
    } else {
        return 2000.0f + (counter % 200) * 2.0f;    // Intermittent/Rain
    }
}

/**
 * @brief Simulate TMP117 temperature sensor reading
 * 
 * @return Temperature in Celsius
 * @note TODO: Replace with actual I2C driver implementation
 */
static float tmp117_read_temperature(void) {
    // Simulated reading - in production, read from I2C device
    static float base_temp = 18.0f;
    static int32_t drift = 0;
    
    drift += (rand() % 3) - 1; // Random walk
    if (drift > 50) drift = 50;
    if (drift < -50) drift = -50;
    
    return base_temp + (drift / 10.0f);
}

/**
 * @brief Simulate ADS1115 ADC reading for PV voltage
 * 
 * @return PV panel voltage in volts
 * @note TODO: Replace with actual I2C driver implementation
 */
static float ads1115_read_pv_voltage(void) {
    // Simulated reading - in production, read from I2C device via ADS1115
    static float base_voltage = 13.5f;
    static int32_t noise = 0;
    
    // Add some realistic noise and variation
    noise = (rand() % 100) - 50; // -0.5V to +0.5V
    
    float voltage = base_voltage + (noise / 100.0f);
    
    // Clamp to realistic range
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > 18.0f) voltage = 18.0f;
    
    return voltage;
}

/**
 * @brief Update PV oscillation detector with new voltage reading
 * 
 * @param voltage Current PV voltage
 * @return Number of detected oscillations in recent history
 */
static uint8_t update_pv_oscillation_detector(float voltage) {
    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Add to circular buffer
    pv_detector.history[pv_detector.index] = voltage;
    pv_detector.index = (pv_detector.index + 1) % 10;
    if (pv_detector.count < 10) {
        pv_detector.count++;
    }
    pv_detector.last_update_ms = now_ms;
    
    // Need at least 4 samples to detect oscillations
    if (pv_detector.count < 4) {
        return 0;
    }
    
    // Count voltage swings above threshold
    uint8_t oscillation_count = 0;
    for (uint8_t i = 1; i < pv_detector.count; i++) {
        uint8_t prev_idx = (pv_detector.index - i - 1 + 10) % 10;
        uint8_t curr_idx = (pv_detector.index - i + 10) % 10;
        
        float delta = fabsf(pv_detector.history[curr_idx] - pv_detector.history[prev_idx]);
        if (delta > PV_OSCILLATION_THRESHOLD) {
            oscillation_count++;
        }
    }
    
    return oscillation_count;
}

/**
 * @brief Classify weather based on sensor readings
 * 
 * @param lux Ambient light level (lux)
 * @param temperature Temperature (°C)
 * @param pv_voltage PV panel voltage (V)
 * @param oscillations Number of detected PV oscillations
 * @param is_daytime true if currently daytime
 * @return Classified weather state
 */
static weather_state_t classify_weather(float lux, float temperature, float pv_voltage,
                                        uint8_t oscillations, bool is_daytime) {
    // Fog detection: low lux + specific temp range during day
    if (is_daytime && lux < LUX_THRESHOLD_FOG &&
        temperature >= TEMP_FOG_RANGE_LOW && temperature <= TEMP_FOG_RANGE_HIGH) {
        ESP_LOGD(TAG, "Fog detected (lux=%.0f, temp=%.1f°C)", lux, temperature);
        return WEATHER_FOG;
    }
    
    // Rain detection: very low lux + low temperature + low PV
    if (is_daytime && lux < LUX_THRESHOLD_OVERCAST &&
        temperature < TEMP_RAIN_MAX && pv_voltage < PV_VOLTAGE_OVERCAST) {
        ESP_LOGD(TAG, "Rain detected (lux=%.0f, temp=%.1f°C, pv=%.2fV)", 
                 lux, temperature, pv_voltage);
        return WEATHER_RAIN;
    }
    
    // Intermittent (cloud edge effects): significant PV oscillations
    if (is_daytime && oscillations >= PV_OSCILLATION_COUNT) {
        ESP_LOGD(TAG, "Intermittent detected (oscillations=%d)", oscillations);
        return WEATHER_INTERMITTENT;
    }
    
    // Clear sky: high lux + good PV voltage
    if (is_daytime && lux >= LUX_THRESHOLD_CLEAR && pv_voltage >= PV_VOLTAGE_MIN_CLEAR) {
        ESP_LOGD(TAG, "Clear sky detected (lux=%.0f, pv=%.2fV)", lux, pv_voltage);
        return WEATHER_CLEAR;
    }
    
    // Cloudy: moderate lux
    if (is_daytime && lux >= LUX_THRESHOLD_CLOUDY) {
        ESP_LOGD(TAG, "Cloudy detected (lux=%.0f)", lux);
        return WEATHER_CLOUDY;
    }
    
    // Default to cloudy during day, unknown at night
    if (is_daytime) {
        return WEATHER_CLOUDY;
    } else {
        return WEATHER_UNKNOWN;
    }
}

/**
 * @brief Read sensors and determine current weather state
 * 
 * @param weather Pointer to weather_data_t structure to fill
 * @param is_daytime true if currently daytime (for accurate classification)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t weather_update(weather_data_t *weather, bool is_daytime) {
    if (!weather) {
        ESP_LOGE(TAG, "NULL weather pointer");
        return ESP_FAIL;
    }
    
    // Read sensors
    weather->ambient_lux = tsl2591_read_lux();
    weather->temperature_c = tmp117_read_temperature();
    weather->pv_voltage = ads1115_read_pv_voltage();
    
    // Update oscillation detector
    uint8_t oscillations = update_pv_oscillation_detector(weather->pv_voltage);
    weather->pv_oscillating = (oscillations >= PV_OSCILLATION_COUNT);
    
    // Classify weather
    weather->state = classify_weather(
        weather->ambient_lux,
        weather->temperature_c,
        weather->pv_voltage,
        oscillations,
        is_daytime
    );
    
    ESP_LOGI(TAG, "Weather update: %s (lux=%.0f, temp=%.1f°C, pv=%.2fV, osc=%s)",
             get_weather_state_str(weather->state),
             weather->ambient_lux,
             weather->temperature_c,
             weather->pv_voltage,
             weather->pv_oscillating ? "Yes" : "No");
    
    return ESP_OK;
}

/**
 * @brief Get weather state from sensor readings without updating internal state
 * 
 * @param lux Ambient light level (lux)
 * @param temperature Temperature (°C)
 * @param pv_voltage PV panel voltage (V)
 * @param is_daytime true if currently daytime
 * @return Classified weather state
 */
weather_state_t weather_classify(float lux, float temperature, float pv_voltage, bool is_daytime) {
    // Simple classification without oscillation detection
    return classify_weather(lux, temperature, pv_voltage, 0, is_daytime);
}

/**
 * @brief Reset PV oscillation detector
 */
void weather_reset_oscillation_detector(void) {
    memset(&pv_detector, 0, sizeof(pv_oscillation_detector_t));
    ESP_LOGD(TAG, "PV oscillation detector reset");
}

/**
 * @brief Get current PV oscillation state
 * 
 * @return true if PV is currently oscillating (cloud edge effects)
 */
bool weather_is_pv_oscillating(void) {
    if (pv_detector.count < 4) {
        return false;
    }
    
    // Check recent oscillations
    uint8_t oscillations = 0;
    for (uint8_t i = 1; i < pv_detector.count && i < 5; i++) {
        uint8_t prev_idx = (pv_detector.index - i - 1 + 10) % 10;
        uint8_t curr_idx = (pv_detector.index - i + 10) % 10;
        
        float delta = fabsf(pv_detector.history[curr_idx] - pv_detector.history[prev_idx]);
        if (delta > PV_OSCILLATION_THRESHOLD) {
            oscillations++;
        }
    }
    
    return (oscillations >= PV_OSCILLATION_COUNT);
}
