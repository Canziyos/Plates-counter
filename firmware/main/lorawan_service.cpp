#include "lorawan_service.h"
#include "app_config.h"

#include "TheThingsNetwork.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include <cctype>
#include <cstddef>
#include <cstring>

namespace {

constexpr char TAG[] = "lorawan";

constexpr spi_host_device_t TTN_SPI_HOST = HSPI_HOST;

TheThingsNetwork ttn;

bool isHexCredential(const char *value, size_t expectedLength)
{
    if (value == nullptr || std::strlen(value) != expectedLength) {
        return false;
    }

    for (size_t index = 0; index < expectedLength; ++index) {
        if (!std::isxdigit(static_cast<unsigned char>(value[index]))) {
            return false;
        }
    }
    return true;
}

bool credentialsAreValid()
{
    return isHexCredential(CONFIG_DISH_COUNTER_DEV_EUI, 16)
        && isHexCredential(CONFIG_DISH_COUNTER_APP_EUI, 16)
        && isHexCredential(CONFIG_DISH_COUNTER_APP_KEY, 32);
}

bool initializeNvs()
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires reformatting");
        result = nvs_flash_erase();
        if (result == ESP_OK) {
            result = nvs_flash_init();
        }
    }

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(result));
        return false;
    }
    return true;
}

bool initializeRadio()
{
    spi_bus_config_t busConfig = {};
    busConfig.miso_io_num = app_config::pins::lorawanSpiMiso;
    busConfig.mosi_io_num = app_config::pins::lorawanSpiMosi;
    busConfig.sclk_io_num = app_config::pins::lorawanSpiClock;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;

    const esp_err_t result = spi_bus_initialize(
        TTN_SPI_HOST,
        &busConfig,
        app_config::lorawan::spiDmaChannel);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed: %s", esp_err_to_name(result));
        return false;
    }

    ttn.configurePins(
        TTN_SPI_HOST,
        app_config::pins::lorawanChipSelect,
        TTN_NOT_CONNECTED,
        app_config::pins::lorawanReset,
        app_config::pins::lorawanDio0,
        app_config::pins::lorawanDio1);
    return true;
}

void messageReceived(const uint8_t *message, size_t length, ttn_port_t port)
{
    ESP_LOGI(TAG, "Received %u bytes on port %u", static_cast<unsigned>(length), static_cast<unsigned>(port));
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, message, length, ESP_LOG_INFO);
}

void serviceTask(void *)
{
    if (!initializeNvs() || !initializeRadio()) {
        ESP_LOGW(TAG, "LoRaWAN disabled; dish counting is unaffected");
        vTaskDelete(nullptr);
        return;
    }

    if (!credentialsAreValid()) {
        ESP_LOGE(TAG, "Configure hexadecimal LoRaWAN credentials with idf.py menuconfig");
        vTaskDelete(nullptr);
        return;
    }

    if (!ttn.provision(
            CONFIG_DISH_COUNTER_DEV_EUI,
            CONFIG_DISH_COUNTER_APP_EUI,
            CONFIG_DISH_COUNTER_APP_KEY)) {
        ESP_LOGE(TAG, "LoRaWAN provisioning failed; dish counting continues");
        vTaskDelete(nullptr);
        return;
    }

    ttn.onMessage(messageReceived);

    while (!ttn.join()) {
        ESP_LOGW(
            TAG,
            "LoRaWAN join failed; retrying in %lu seconds",
            static_cast<unsigned long>(app_config::timing::lorawanJoinRetryMs / 1000));
        vTaskDelay(pdMS_TO_TICKS(app_config::timing::lorawanJoinRetryMs));
    }

    ESP_LOGI(TAG, "LoRaWAN joined");
    vTaskDelete(nullptr);
}

} // namespace

bool lorawanServiceStart()
{
    return xTaskCreate(
               serviceTask,
               "lorawanService",
               app_config::tasks::lorawanStackSize,
               nullptr,
               app_config::tasks::lorawanPriority,
               nullptr)
        == pdPASS;
}
