/*******************************************************************************
 *
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 *
 * Copyright (c) 2021 Manuel Bleichenbacher
 *
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 ********************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensor.h"
#include "nvs_flash.h"
#include "ttn.h"

/**
 * introduce a time delay or pause in the execution of tasks or processes.
 */
#define SENSOR_WAIT_TIME_MS 100

/**
 * initiating Global variable to be used in the program
 */
static bool flatSensorDetected = false;
static bool secondSensorDetected = false;
static bool dataSent = true;
static bool loraWAnJoined = false;

// AppEUI (sometimes called JoinEUI)
const char *appEui = "0000000000001234";
// DevEUI
const char *devEui = "70B3D57ED0064FE3";
// AppKey
const char *appKey = "9FA04AC323CE2B5CB24987E14350571B";

// Pins and other resources
#define TTN_SPI_HOST SPI2_HOST
#define TTN_SPI_DMA_CHAN SPI_DMA_DISABLED
#define TTN_PIN_SPI_SCLK 5
#define TTN_PIN_SPI_MOSI 27
#define TTN_PIN_SPI_MISO 19
#define TTN_PIN_NSS 18
#define TTN_PIN_RXTX TTN_NOT_CONNECTED
#define TTN_PIN_RST 14
#define TTN_PIN_DIO0 26
#define TTN_PIN_DIO1 35

#define TX_INTERVAL 30


/**
 * globala counter
*/
static int flatTemporaryCounter = 0;
static int deepTemppraryCounter = 0;
static int basketFlatCounter = 0;
static int basketDeepCounter = 0;
static int total_flat_plate = 0;
static int total_deep_plate = 0;
static int total_plate = 0;
static uint16_t numberToSend = 0;

/**
 * initiating the semaphore.
 */
SemaphoreHandle_t basketDetectedSemaphore;
SemaphoreHandle_t deepDetectedSemaphore;
SemaphoreHandle_t analysisSemaphore;
SemaphoreHandle_t sendingData;

/**
 * imporint the fucntion that are responsable to get the sensors statue.
 */
extern bool flatSensorDetect(void);
extern bool deepSensorDetect(void);
extern bool basketSensorDetect(void);
extern void initializeSensor(void);

TimerHandle_t noDetectionTimerHandle = NULL;

/**
 * call back function which will be called when the timer is finished.
*/
void vNoDetectionTimerCallback(TimerHandle_t xTimer)
{
    ESP_LOGI("SensorTask", "Basket was not detected for over x minutes.");
    dataSent = false;
    numberToSend = total_plate;
    ESP_LOGI("checking", "Number of the baskets with flat plate: %d", numberToSend);
    xSemaphoreGive(sendingData);
}

/**
 * The task responsible for detecting the basket and give the appropriate semaphore.
*/
void basketTask(void *pvParameter)
{
    bool previouslyDetected = false;

    while (1)
    {
        if (previouslyDetected == false && basketSensorDetect())
        {
            ESP_LOGI("Basket detection", "Basket detected.");
            previouslyDetected = true;
            dataSent = true;
            xTimerStop(noDetectionTimerHandle, 0);
            xSemaphoreGive(basketDetectedSemaphore);
        }

        else if (previouslyDetected == true && basketSensorDetect())
        {
            dataSent = true;
            xSemaphoreGive(basketDetectedSemaphore);
            xTimerReset(noDetectionTimerHandle, 0);
        }

        else if (previouslyDetected == true && !basketSensorDetect())
        {

            ESP_LOGI("SensorTask", "Basket is no longer detected.");
            // If less than 1 minute, give resultSemaphore, 1 minute is just for the test it should be longer time to know that the washing process is finished
            previouslyDetected = false;
            dataSent = true;
            xTimerStart(noDetectionTimerHandle, 0);
            xSemaphoreGive(analysisSemaphore);
            
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_WAIT_TIME_MS));
    }
}

/**
 * deep plate detection task.
 * this task is responsable to increase the global variable wish rfepresent the number of detected deep plate
 * in case of detection the the semaphore will be handeld to the flat plate task.
 */
void deepPlateTask(void *pvParameter)
{
    while (1)
    {
        if (xSemaphoreTake(basketDetectedSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (deepSensorDetect())
            {
                ESP_LOGI("Deep plate task", "deep plate detected");
                deepTemppraryCounter++;
                xSemaphoreGive(deepDetectedSemaphore);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_WAIT_TIME_MS));
    }
}

/***
 * this task is responsable of detecing the flate plates.
 * this task is responsable to increase the global variable wish rfepresent the number of detected flat plate.
 * after the increasing the semaphore will be handled to analys task.
 */
void flatPlateTask(void *pvParameter)
{
    while (1)
    {
        if (xSemaphoreTake(deepDetectedSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (flatSensorDetect())
            {
                ESP_LOGI("Deep plate task", "flate plate detected");
                flatTemporaryCounter++;
                xSemaphoreGive(analysisSemaphore);
            }
            // else if (!flatSensorDetect())
            // {
            //     xSemaphoreGive(analysisSemaphore);
            // }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_WAIT_TIME_MS));
    }
}

/**
 * this task is responsable to analys the data that coming from athore tasks.
 * when there is no basket the program will check the global counter we had "temproray flat and deep counters"
 * to check if the basket contain deep or flat
 * to finaly icrease the number of basket with the detected type.
 */
void analysationTask(void *pvParameter)
{
    bool showed = false;
    while (1)
    {
        if (xSemaphoreTake(analysisSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (basketSensorDetect())
            {
                ESP_LOGI("BasketCounter", "deciding and counting is on ");
            }
            else if (!basketSensorDetect())
            {
                if (flatTemporaryCounter > 0 && deepTemppraryCounter > 0)
                {
                    basketFlatCounter++;
                    showed = false;
                }
                else if (flatTemporaryCounter == 0 && deepTemppraryCounter > 0)
                {
                    basketDeepCounter++;
                    showed = false;
                }

                if (showed == false)
                {
                    ESP_LOGI("BasketCounter", "im in showing case");
                    ESP_LOGI("BasketCounter", "Number of the baskets with flat plate: %d", basketFlatCounter);
                    ESP_LOGI("BasketCounter", "Number of the baskets with deep plate: %d", basketDeepCounter);
                    showed = true;
                }
                total_flat_plate = basketFlatCounter * 18;
                total_deep_plate = basketDeepCounter *12;
                total_plate = total_flat_plate + total_deep_plate;
                flatTemporaryCounter = 0;
                deepTemppraryCounter = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_WAIT_TIME_MS));
    }
}

/**
 * task that responsoble to send the number of plates after convet the number to bytes
 * which typically requires fewer bytes than transmitting the integer as a string or in another format.
*/
void sendDataTask(void *pvParameters)
{
    
    while (1)
    {
        if (xSemaphoreTake(sendingData, portMAX_DELAY) == pdTRUE)
        {
            if (!dataSent)
            {
                printf("Sending message...\n");
                uint8_t msgData[2];

                msgData[0] = (uint8_t)(numberToSend & 0xFF);
                msgData[1] = ((uint8_t)(numberToSend >> 8) & 0xFF);
                ttn_response_code_t res = ttn_transmit_message(msgData, sizeof(msgData), 1, false);
                printf(res == TTN_SUCCESSFUL_TRANSMISSION ? "Number sent.\n" : "Transmission failed.\n");
                numberToSend = 0;
                basketFlatCounter = 0;
                basketDeepCounter = 0;
                total_plate = 0;
                dataSent = true;
            }
            else
            {
                printf("Sending message...\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_WAIT_TIME_MS));
    }
}

void messageReceived(const uint8_t *message, size_t length, ttn_port_t port)
{
    printf("Message of %d bytes received on port %d:", length, port);
    for (int i = 0; i < length; i++)
        printf(" %02x", message[i]);
    printf("\n");
}


/***
 * This is the main function responsible for creating semaphores and tasks,
 * creating semaphores that will be used,
 * creating the counter,
 * and initiating the connection to TTN.
 */
void app_main(void)
{
    sendingData = xSemaphoreCreateBinary();
    basketDetectedSemaphore = xSemaphoreCreateBinary();
    deepDetectedSemaphore = xSemaphoreCreateBinary();
    analysisSemaphore = xSemaphoreCreateBinary();

    if (basketDetectedSemaphore == NULL || deepDetectedSemaphore == NULL || analysisSemaphore == NULL || sendingData == NULL)
    {
        ESP_LOGE("Main", "Semaphore creation failed");
        return;
    }

    noDetectionTimerHandle = xTimerCreate("NoDetectionTimer",
                                          pdMS_TO_TICKS(30000),
                                          pdFALSE,
                                          (void *)0,
                                          vNoDetectionTimerCallback);

    if (noDetectionTimerHandle == NULL)
    {
        ESP_LOGE("Main", "Timer creation failed");
        return;
    }

    ///////////////////////
    // ttn configeration //
    /////////////////////

    esp_err_t err;
    // Initialize the GPIO ISR handler service
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    // Initialize SPI bus
    spi_bus_config_t spi_bus_config = {
        .miso_io_num = TTN_PIN_SPI_MISO,
        .mosi_io_num = TTN_PIN_SPI_MOSI,
        .sclk_io_num = TTN_PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};
    err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(err);

    // Initialize TTN
    ttn_init();

    // Configure the SX127x pins
    ttn_configure_pins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn_provision(devEui, appEui, appKey);

    // Register callback for received messages
    ttn_on_message(messageReceived);

    printf("Joining...\n");
    if (ttn_join())
    {
        printf("Joined.\n");
        xTaskCreatePinnedToCore(basketTask, "BasketSensorTask", 2048, NULL, 6, NULL, 0);
        xTaskCreatePinnedToCore(deepPlateTask, "FirstSensorTask", 2048, NULL, 5, NULL, 0);
        xTaskCreatePinnedToCore(flatPlateTask, "SecondSensorTask", 2048, NULL, 4, NULL, 0);
        xTaskCreatePinnedToCore(analysationTask, "DetectionAnalysisTask", 2048, NULL, 3, NULL, 0);
        xTaskCreatePinnedToCore(sendDataTask, "sendDataTask", 1024 * 4, (void *)0, 2, NULL, 0);
    }
    else
    {
        printf("Join failed. Goodbye\n");
    }
}
