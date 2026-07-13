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
    if (xTaskCreate(readPulseTask, "readPulseTask", 2048, nullptr, 4, &distanceTask) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create distance measurement task");
        vSemaphoreDelete(trayDetectionSemaphore);
        trayDetectionSemaphore = nullptr;
        return false;
    }

    if (xTaskCreate(monitorIRSensorTask, "monitorIRSensorTask", 2048, nullptr, 5, nullptr) != pdPASS) {
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
