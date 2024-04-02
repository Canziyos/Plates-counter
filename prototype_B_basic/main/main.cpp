#include "tasks.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "TheThingsNetwork.h"
#include "oled.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "freertos/semphr.h"
#include <cstring>
#include "driver/rtc_io.h"


// Global variables
SemaphoreHandle_t trayDetectionSemaphore;

// LoRaWAN configuration constants
const char* appEui = "0000000000000000";
const char* devEui = "70B3D57ED0064CC9";
const char* appKey = "A17FB3F70891FC287716982D251F417A";

// SPI and pin configuration for The Things Network
#define TTN_SPI_HOST HSPI_HOST
#define TTN_SPI_DMA_CHAN 1
#define TTN_PIN_SPI_SCLK 5
#define TTN_PIN_SPI_MOSI 27
#define TTN_PIN_SPI_MISO 19
#define TTN_PIN_NSS 18
#define TTN_PIN_RXTX TTN_NOT_CONNECTED
#define TTN_PIN_RST 14
#define TTN_PIN_DIO0 26
#define TTN_PIN_DIO1 35

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 180;

void sendMessages(void* pvParameter)
{
    while (1) {
        printf("Sending message...\n");
        uint8_t data[] = {static_cast<uint8_t>(count_object >> 8), static_cast<uint8_t>(count_object & 0xFF)};
        TTNResponseCode res = ttn.transmitMessage(data, sizeof(data)); // Transmit the message
        printf(res == kTTNSuccessfulTransmission ? "Message sent.\n" : "Transmission failed.\n");

        vTaskDelay(TX_INTERVAL * pdMS_TO_TICKS(1000)); // Delay until next transmission
    }
}

void messageReceived(const uint8_t* message, size_t length, ttn_port_t port)
{
    printf("Message of %d bytes received on port %d:", length, port);
    for (int i = 0; i < length; i++)
        printf(" %02x", message[i]);
    printf("\n");
}
extern "C" void app_main(void) {
    esp_err_t err;

    // Define TAG for logging
    static const char *TAG = "main";

    // Initialize the GPIO ISR handler service
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);
    
    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    // Create semaphore for tray detection
    trayDetectionSemaphore = xSemaphoreCreateBinary();
    if (!trayDetectionSemaphore) {
        ESP_LOGE(TAG, "Semaphore creation failed.");
        return;
    }

    // Initialize SPI bus
    spi_bus_config_t spi_bus_config;
    memset(&spi_bus_config, 0, sizeof(spi_bus_config));
    spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
    spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
    spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
    err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(err);

    // Configure the SX127x pins
    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn.provision(devEui, appEui, appKey);

    // Register callback for received messages
    ttn.onMessage(messageReceived);

    printf("Joining...\n");
    if (ttn.join())
    {
        printf("Joined.\n");
        xTaskCreate(monitorIRSensorTask, "monitorIRSensorTask", 2048, NULL, 5, NULL);
        xTaskCreate(readPulseTask, "readPulseTask", 2048, NULL, 4, NULL);
        //xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void* )0, 3, nullptr);

    }
    else
    {
        printf("Join failed. Goodbye\n");
    }
}
