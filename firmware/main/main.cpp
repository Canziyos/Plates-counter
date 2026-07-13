#include "app_config.h"
#include "lorawan_service.h"
#include "tasks.h"

#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

SemaphoreHandle_t trayDetectionSemaphore;

namespace {

constexpr char TAG[] = "main";

bool startDishCounterTasks()
{
    trayDetectionSemaphore = xSemaphoreCreateBinary();
    if (trayDetectionSemaphore == nullptr) {
        ESP_LOGE(TAG, "Failed to create tray detection semaphore");
        return false;
    }

    TaskHandle_t distanceTask = nullptr;
    if (xTaskCreate(
            readPulseTask,
            "readPulseTask",
            app_config::tasks::distanceMeasurementStackSize,
            nullptr,
            app_config::tasks::distanceMeasurementPriority,
            &distanceTask)
        != pdPASS) {
        ESP_LOGE(TAG, "Failed to create distance measurement task");
        vSemaphoreDelete(trayDetectionSemaphore);
        trayDetectionSemaphore = nullptr;
        return false;
    }

    if (xTaskCreate(
            monitorIRSensorTask,
            "monitorIRSensorTask",
            app_config::tasks::trayMonitorStackSize,
            nullptr,
            app_config::tasks::trayMonitorPriority,
            nullptr)
        != pdPASS) {
        ESP_LOGE(TAG, "Failed to create tray monitoring task");
        vTaskDelete(distanceTask);
        vSemaphoreDelete(trayDetectionSemaphore);
        trayDetectionSemaphore = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Dish counter tasks started");
    return true;
}

} // namespace

extern "C" void app_main(void)
{
    initialize_gpio();

    if (!startDishCounterTasks()) {
        return;
    }

    if (!lorawanServiceStart()) {
        ESP_LOGW(TAG, "LoRaWAN service unavailable; dish counting continues");
    }
}
