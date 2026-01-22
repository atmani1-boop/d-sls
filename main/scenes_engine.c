/**
 * @file scenes_engine.c
 * @brief Scene management engine implementation
 */

#include "scenes_engine.h"
#include "sacn_receiver.h"
#include "config_manager.h"
#include "logger.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SCENES";

// NVS namespace
#define NVS_NAMESPACE "scenes"

// Global state
static scene_t g_scenes[MAX_SCENES];
static uint8_t g_active_scene = 255;  // 255 = no scene active
static bool g_scenes_running = false;

/**
 * @brief Create default scenes
 */
static void create_default_scenes(void)
{
    // Scene 0: Blackout
    snprintf(g_scenes[0].name, SCENE_NAME_LEN, "Blackout");
    memset(g_scenes[0].fixtures, 0, sizeof(g_scenes[0].fixtures));
    g_scenes[0].fade_time_ms = 1000;
    g_scenes[0].enabled = true;
    
    // Scene 1: Full White
    snprintf(g_scenes[1].name, SCENE_NAME_LEN, "Full White");
    for (int i = 0; i < NUM_FIXTURES; i++) {
        g_scenes[1].fixtures[i].red = 255;
        g_scenes[1].fixtures[i].green = 255;
        g_scenes[1].fixtures[i].blue = 255;
        g_scenes[1].fixtures[i].white = 255;
    }
    g_scenes[1].fade_time_ms = 2000;
    g_scenes[1].enabled = true;
    
    // Scene 2: Warm
    snprintf(g_scenes[2].name, SCENE_NAME_LEN, "Warm");
    for (int i = 0; i < NUM_FIXTURES; i++) {
        g_scenes[2].fixtures[i].red = 255;
        g_scenes[2].fixtures[i].green = 180;
        g_scenes[2].fixtures[i].blue = 0;
        g_scenes[2].fixtures[i].white = 200;
    }
    g_scenes[2].fade_time_ms = 2000;
    g_scenes[2].enabled = true;
    
    // Scene 3: Cool
    snprintf(g_scenes[3].name, SCENE_NAME_LEN, "Cool");
    for (int i = 0; i < NUM_FIXTURES; i++) {
        g_scenes[3].fixtures[i].red = 0;
        g_scenes[3].fixtures[i].green = 180;
        g_scenes[3].fixtures[i].blue = 255;
        g_scenes[3].fixtures[i].white = 150;
    }
    g_scenes[3].fade_time_ms = 2000;
    g_scenes[3].enabled = true;
    
    // Initialize remaining scenes as disabled
    for (int i = 4; i < MAX_SCENES; i++) {
        snprintf(g_scenes[i].name, SCENE_NAME_LEN, "Scene %d", i);
        memset(g_scenes[i].fixtures, 0, sizeof(g_scenes[i].fixtures));
        g_scenes[i].fade_time_ms = 1000;
        g_scenes[i].enabled = false;
    }
    
    ESP_LOGI(TAG, "Default scenes created");
}

/**
 * @brief Scenes engine task - monitors sACN failover
 */
static void task_scenes(void *pvParameters)
{
    ESP_LOGI(TAG, "Scenes engine task started");
    
    const system_config_t *config = config_manager_get();
    bool sacn_was_active = false;
    
    while (g_scenes_running) {
        bool sacn_active = sacn_receiver_is_active();
        
        // Check for sACN timeout (failover condition)
        if (!sacn_active && sacn_was_active) {
            // sACN just timed out - activate default scene
            ESP_LOGW(TAG, "sACN timeout - activating default scene %d", config->default_scene);
            scenes_engine_activate(config->default_scene);
        } else if (sacn_active && !sacn_was_active) {
            // sACN recovered
            ESP_LOGI(TAG, "sACN recovered - resuming streaming mode");
            g_active_scene = 255;  // No scene active (streaming mode)
        }
        
        sacn_was_active = sacn_active;
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    ESP_LOGI(TAG, "Scenes engine task stopped");
    vTaskDelete(NULL);
}

esp_err_t scenes_engine_init(void)
{
    ESP_LOGI(TAG, "Initializing scenes engine");
    
    // Create default scenes
    create_default_scenes();
    
    // Try to load from NVS
    esp_err_t err = scenes_engine_load_all();
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Using default scenes (NVS load failed)");
    }
    
    ESP_LOGI(TAG, "Scenes engine initialized with %d scenes", MAX_SCENES);
    return ESP_OK;
}

esp_err_t scenes_engine_start(void)
{
    if (g_scenes_running) {
        ESP_LOGW(TAG, "Scenes engine already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting scenes engine");
    g_scenes_running = true;
    
    // Create scenes task
    BaseType_t ret = xTaskCreate(
        task_scenes,
        "scenes",
        3072,
        NULL,
        5,  // Priority 5
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create scenes task");
        g_scenes_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Scenes engine started");
    return ESP_OK;
}

esp_err_t scenes_engine_stop(void)
{
    ESP_LOGI(TAG, "Stopping scenes engine");
    g_scenes_running = false;
    return ESP_OK;
}

esp_err_t scenes_engine_activate(uint8_t scene_id)
{
    if (scene_id >= MAX_SCENES) {
        ESP_LOGE(TAG, "Invalid scene ID: %d", scene_id);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_scenes[scene_id].enabled) {
        ESP_LOGW(TAG, "Scene %d is disabled", scene_id);
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Activating scene %d: %s", scene_id, g_scenes[scene_id].name);
    
    // TODO: Implement fade transition
    // For now, apply scene directly
    lighting_engine_set_all_fixtures(g_scenes[scene_id].fixtures);
    
    g_active_scene = scene_id;
    
    return ESP_OK;
}

esp_err_t scenes_engine_get(uint8_t scene_id, scene_t *scene)
{
    if (scene_id >= MAX_SCENES || !scene) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(scene, &g_scenes[scene_id], sizeof(scene_t));
    return ESP_OK;
}

esp_err_t scenes_engine_save(uint8_t scene_id, const scene_t *scene)
{
    if (scene_id >= MAX_SCENES || !scene) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Update in memory
    memcpy(&g_scenes[scene_id], scene, sizeof(scene_t));
    
    // Save to NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    char key[16];
    snprintf(key, sizeof(key), "scene_%d", scene_id);
    
    err = nvs_set_blob(handle, key, scene, sizeof(scene_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Scene %d saved: %s", scene_id, scene->name);
    } else {
        ESP_LOGE(TAG, "Failed to save scene %d: %s", scene_id, esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t scenes_engine_load_all(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    
    int loaded_count = 0;
    
    for (int i = 0; i < MAX_SCENES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "scene_%d", i);
        
        size_t required_size = sizeof(scene_t);
        err = nvs_get_blob(handle, key, &g_scenes[i], &required_size);
        
        if (err == ESP_OK) {
            loaded_count++;
        }
    }
    
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Loaded %d scenes from NVS", loaded_count);
    return ESP_OK;
}

esp_err_t scenes_engine_save_all(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    for (int i = 0; i < MAX_SCENES; i++) {
        char key[16];
        snprintf(key, sizeof(key), "scene_%d", i);
        nvs_set_blob(handle, key, &g_scenes[i], sizeof(scene_t));
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "All scenes saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save scenes: %s", esp_err_to_name(err));
    }
    
    return err;
}

uint8_t scenes_engine_get_active(void)
{
    return g_active_scene;
}
