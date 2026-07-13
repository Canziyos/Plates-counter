#include "tasks.h"
#include "dish_counter_logic.h"

#include <atomic>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace {

constexpr gpio_num_t kIrSensorGpio = GPIO_NUM_17;
constexpr gpio_num_t kDistanceSensorGpio = GPIO_NUM_23;
constexpr int kDebounceDelayMs = 50;
constexpr int kMeasurementDelayMs = 100;
constexpr int kMeasurementWindowMs = 10000;
constexpr int kIdleLogIntervalMs = 20000;
constexpr DishClassificationConfig kDishClassificationConfig = {
    .minimumDistance = 200,
    .maximumDistance = 250,
    .requiredConsecutiveSamples = 5,
};
constexpr int64_t kPulseWaitTimeoutUs = 3000;
constexpr int64_t kMaxDistancePulseUs = 1850;

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
    if (!waitForLevel(kDistanceSensorGpio, 1, kPulseWaitTimeoutUs)) {
        return -1;
    }
    const int64_t pulseStart = esp_timer_get_time();

    if (!waitForLevel(kDistanceSensorGpio, 0, kPulseWaitTimeoutUs)) {
        return -1;
    }
    const int64_t pulseDuration = esp_timer_get_time() - pulseStart;
    if (pulseDuration <= 0 || pulseDuration > kMaxDistancePulseUs) {
        return -1;
    }

    const int distance = static_cast<int>((pulseDuration - 1000) * 3 / 4);
    return distance < 0 ? 0 : distance;
}

} // namespace

void initialize_gpio(void)
{
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << kIrSensorGpio) | (1ULL << kDistanceSensorGpio);
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));
}

void monitorIRSensorTask(void *)
{
    const TickType_t delay = pdMS_TO_TICKS(kDebounceDelayMs);
    const TickType_t idleInterval = pdMS_TO_TICKS(kIdleLogIntervalMs);
    TickType_t lastDetection = xTaskGetTickCount();
    bool lastTrayState = false;

    while (true) {
        const bool trayPresent = gpio_get_level(kIrSensorGpio) == 0;
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
            ESP_LOGI(kLogTag, "Idle: no tray detected for %d seconds", kIdleLogIntervalMs / 1000);
            lastDetection = xTaskGetTickCount();
        }
        vTaskDelay(delay);
    }
}

void readPulseTask(void *)
{
    const TickType_t delay = pdMS_TO_TICKS(kMeasurementDelayMs);
    const TickType_t window = pdMS_TO_TICKS(kMeasurementWindowMs);
    ScalarKalmanFilter distanceFilter;

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
