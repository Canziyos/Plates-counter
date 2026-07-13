#include "lorawan_service.h"

#include "TheThingsNetwork.h"
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
constexpr TickType_t JOIN_RETRY_DELAY = pdMS_TO_TICKS(30000);
constexpr UBaseType_t SERVICE_PRIORITY = 3;
constexpr uint32_t SERVICE_STACK_SIZE = 4096;

constexpr spi_host_device_t TTN_SPI_HOST = HSPI_HOST;
constexpr int TTN_SPI_DMA_CHANNEL = 1;
constexpr gpio_num_t TTN_PIN_SPI_SCLK = GPIO_NUM_5;
constexpr gpio_num_t TTN_PIN_SPI_MOSI = GPIO_NUM_27;
constexpr gpio_num_t TTN_PIN_SPI_MISO = GPIO_NUM_19;
constexpr gpio_num_t TTN_PIN_NSS = GPIO_NUM_18;
constexpr int TTN_PIN_RXTX = TTN_NOT_CONNECTED;
constexpr gpio_num_t TTN_PIN_RST = GPIO_NUM_14;
constexpr gpio_num_t TTN_PIN_DIO0 = GPIO_NUM_26;
constexpr gpio_num_t TTN_PIN_DIO1 = GPIO_NUM_35;

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
    busConfig.miso_io_num = TTN_PIN_SPI_MISO;
    busConfig.mosi_io_num = TTN_PIN_SPI_MOSI;
    busConfig.sclk_io_num = TTN_PIN_SPI_SCLK;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;

    const esp_err_t result = spi_bus_initialize(TTN_SPI_HOST, &busConfig, TTN_SPI_DMA_CHANNEL);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed: %s", esp_err_to_name(result));
        return false;
    }

    ttn.configurePins(
        TTN_SPI_HOST,
        TTN_PIN_NSS,
        TTN_PIN_RXTX,
        TTN_PIN_RST,
        TTN_PIN_DIO0,
        TTN_PIN_DIO1);
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
        ESP_LOGW(TAG, "LoRaWAN join failed; retrying in 30 seconds");
        vTaskDelay(JOIN_RETRY_DELAY);
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
               SERVICE_STACK_SIZE,
               nullptr,
               SERVICE_PRIORITY,
               nullptr)
        == pdPASS;
}
