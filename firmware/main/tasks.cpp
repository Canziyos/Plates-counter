#include "tasks.h"
#include "app_config.h"
#include "dish_counter_defaults.h"
#include "dish_counter_logic.h"

#include <atomic>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace {

constexpr DishClassificationConfig kDishClassificationConfig = {
    .minimumDistance = dish_counter_defaults::minimumDistance,
    .maximumDistance = dish_counter_defaults::maximumDistance,
    .requiredConsecutiveSamples = dish_counter_defaults::requiredConsecutiveSamples,
};

const char *kLogTag = "dish_counter";

std::atomic<uint32_t> dishCount{0};

bool waitForLevel(gpio_num_t gpio, int level, int64_t timeoutUs)
{
    const int64_t deadline = esp_timer_get_time() + timeoutUs;
    while (gpio_get_level(gpio) != level) {
        if (esp_timer_get_time() >= deadline) {
            return false;
        }
    }
    return true;
}

int measureDistance()
{
    if (!waitForLevel(
            app_config::pins::distanceSensor,
            1,
            app_config::timing::pulseWaitTimeoutUs)) {
        return -1;
    }
    const int64_t pulseStart = esp_timer_get_time();

    if (!waitForLevel(
            app_config::pins::distanceSensor,
            0,
            app_config::timing::pulseWaitTimeoutUs)) {
        return -1;
    }
    const int64_t pulseDuration = esp_timer_get_time() - pulseStart;
    if (pulseDuration <= 0 || pulseDuration > app_config::timing::maximumDistancePulseUs) {
        return -1;
    }

    const int distance = static_cast<int>(
        (pulseDuration - app_config::distance_sensor::pulseZeroOffsetUs)
        * app_config::distance_sensor::distanceScaleNumerator
        / app_config::distance_sensor::distanceScaleDenominator);
    return distance < 0 ? 0 : distance;
}

} // namespace

void initialize_gpio(void)
{
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << app_config::pins::irSensor)
        | (1ULL << app_config::pins::distanceSensor);
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));
}

void monitorIRSensorTask(void *)
{
    const TickType_t delay = pdMS_TO_TICKS(app_config::timing::irDebounceMs);
    const TickType_t idleInterval = pdMS_TO_TICKS(app_config::timing::idleLogIntervalMs);
    TickType_t lastDetection = xTaskGetTickCount();
    bool lastTrayState = false;

    while (true) {
        const bool trayPresent = gpio_get_level(app_config::pins::irSensor) == 0;
        if (trayPresent != lastTrayState) {
            lastTrayState = trayPresent;
            if (trayPresent) {
                lastDetection = xTaskGetTickCount();
                ESP_LOGI(kLogTag, "Tray detected");
                xSemaphoreGive(trayDetectionSemaphore);
            } else {
                ESP_LOGI(kLogTag, "Tray removed");
            }
        }

        if ((xTaskGetTickCount() - lastDetection) > idleInterval) {
            ESP_LOGI(
                kLogTag,
                "Idle: no tray detected for %lu seconds",
                static_cast<unsigned long>(app_config::timing::idleLogIntervalMs / 1000));
            lastDetection = xTaskGetTickCount();
        }
        vTaskDelay(delay);
    }
}

void readPulseTask(void *)
{
    const TickType_t delay = pdMS_TO_TICKS(app_config::timing::measurementIntervalMs);
    const TickType_t window = pdMS_TO_TICKS(app_config::timing::measurementWindowMs);
    ScalarKalmanFilter distanceFilter(dish_counter_defaults::kalmanMeasuredError);

    while (true) {
        if (xSemaphoreTake(trayDetectionSemaphore, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        ESP_LOGI(kLogTag, "Starting distance measurement");
        const TickType_t started = xTaskGetTickCount();
        DishClassifier classifier(kDishClassificationConfig);

        while ((xTaskGetTickCount() - started) <= window) {
            const int rawDistance = measureDistance();
            if (rawDistance >= 0) {
                const int filteredDistance = static_cast<int>(distanceFilter.update(rawDistance));
                if (classifier.recordSample(filteredDistance)) {
                    const uint32_t newCount = dishCount.fetch_add(1) + 1;
                    ESP_LOGI(kLogTag, "Tray with dishes counted; total=%lu",
                             static_cast<unsigned long>(newCount));
                    break;
                }
            } else {
                classifier.recordSample(rawDistance);
            }
            vTaskDelay(delay);
        }

        if (!classifier.classified()) {
            ESP_LOGW(kLogTag, "Measurement window expired without a count");
        }
    }
}

uint32_t dishCounterGetCount(void)
{
    return dishCount.load();
}
