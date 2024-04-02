#include "tasks.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"


static const char *LOG_TAG = "sensor_manager";

// Global variable definitions
volatile bool isTrayPresent = false;
volatile bool distanceSensorTaskShouldRun = false;
int count_object = 0;
int signals = 0;
bool objectCounted = false;

// Constants
const int DEBOUNCE_DELAY_MS = 50;
const int CALIBRATION_INTERVAL = 5000;
const float MIN_CONFIDENCE = 0.8;
const int DISTANCE_FILTER_SAMPLES = 5;

// Definitions
#define IR_SENSOR_GPIO GPIO_NUM_17
#define DISTANCE_SENSOR_GPIO GPIO_NUM_23
#define MAX_DISTANCE_PULSE_DURATION 1850

static int calibrationCounter = 0;
static int calibrationSamples = 0;
static int minDistance = 200;
static int maxDistance = 250;
// static int distanceFilterBuffer[DISTANCE_FILTER_SAMPLES] = {0};
// static int filterIndex = 0;
// static bool waitForNewTrayDetection = false; // Control waiting for new tray detection

// Kalman filter variables
static float kalmanGain = 0.5;
static float estimatedError = 1.0;
static float measuredError = 0.1;
static float currentEstimate = 0.0;

void initialize_gpio(void)
{
    gpio_config_t ioConfig = {};
    ioConfig.intr_type = GPIO_INTR_DISABLE;
    ioConfig.mode = GPIO_MODE_INPUT;
    ioConfig.pin_bit_mask = (1ULL << IR_SENSOR_GPIO) | (1ULL << DISTANCE_SENSOR_GPIO);
    ioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    ioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&ioConfig);
}

void monitorIRSensorTask(void *pvParameter)
{
    const TickType_t xDelay = pdMS_TO_TICKS(DEBOUNCE_DELAY_MS);
    const TickType_t idleTime = pdMS_TO_TICKS(20000);
    TickType_t lastDetectionTime = xTaskGetTickCount();
    bool lastTrayState = false;

    while (true)
    {
        bool currentTrayState = !gpio_get_level(IR_SENSOR_GPIO);

        if (currentTrayState != lastTrayState)
        {
            lastTrayState = currentTrayState;

            if (currentTrayState)
            {
                ESP_LOGI(LOG_TAG, "Tray detected.");
                lastDetectionTime = xTaskGetTickCount();
                xSemaphoreGive(trayDetectionSemaphore);
                ESP_LOGI(LOG_TAG, "Semaphore given by taskMonitorIRSensor");
            }
            else
            {
                ESP_LOGI(LOG_TAG, "Tray removed.");
            }
        }

        // Checks for idle condition based on last detection time
        if ((xTaskGetTickCount() - lastDetectionTime) > idleTime)
        {
            // Logic for handling idle condition
            ESP_LOGI(LOG_TAG, "System is idle. No tray detected for 20 seconds.");
            // Resets the detection time to avoid repetitive idle logs.
            lastDetectionTime = xTaskGetTickCount();
        }

        vTaskDelay(xDelay);
    }
}

void readPulseTask(void *pvParameter)
{
    const TickType_t xDelay = pdMS_TO_TICKS(100);
    const TickType_t tenSeconds = pdMS_TO_TICKS(10000);

    while (true)
    {
        // Wait for the semaphore indefinitely.
        if (xSemaphoreTake(trayDetectionSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Indicate semaphore obtained
            ESP_LOGI(LOG_TAG, "Semaphore obtained by taskReadPulse");
            bool distanceSensorInRange = false;
            TickType_t lastTrayTime = xTaskGetTickCount();

            while (true)
            {
                int rawDistance = measureDistance();

                if (rawDistance >= 0)
                {
                    float kalmanValue = kalmanFilter(rawDistance);
                    int smoothedDistance = (int)kalmanValue;

                    if (smoothedDistance >= minDistance && smoothedDistance <= maxDistance)
                    {
                        distanceSensorInRange = true;
                    }
                    else
                    {
                        distanceSensorInRange = false;
                    }
                }

                if (distanceSensorInRange)
                {
                    signals++;

                    if (signals >= 5)
                    {
                        // Counting is done only once.
                        count_object++;
                        ESP_LOGI(LOG_TAG, "Tray with dishes detected - Total count: %d", count_object);
                        break;
                    }
                }

                vTaskDelay(xDelay);

                if (xTaskGetTickCount() - lastTrayTime > tenSeconds)
                {
                    ESP_LOGI(LOG_TAG, "Waiting for the next tray...");
                    break;
                }
            }
        }
        else
        {
            ESP_LOGI(LOG_TAG, "Washing session ended. No trays detected for a long time.");
            // PHere the deep sleep or any necessary cleanup or end-of-session actions will be performed
            break;
        }
    }
}


float kalmanFilter(int measurement)
{
    // Prediction update
    float predictedEstimate = currentEstimate;
    float predictedError = estimatedError + measuredError;

    // Measurement update
    kalmanGain = predictedError / (predictedError + measuredError);
    currentEstimate = predictedEstimate + kalmanGain * (measurement - predictedEstimate);
    estimatedError = (1 - kalmanGain) * predictedError;

    return currentEstimate;
}

int measureDistance()
{
    while (gpio_get_level(DISTANCE_SENSOR_GPIO) == 0)
        ; // Wait for pulse start
    int64_t startTimePulse = esp_timer_get_time();

    while (gpio_get_level(DISTANCE_SENSOR_GPIO) == 1); // pulse end
    int64_t endTimePulse = esp_timer_get_time();

    int64_t pulseDuration = endTimePulse - startTimePulse;

    if (pulseDuration <= 0 || pulseDuration > MAX_DISTANCE_PULSE_DURATION)
    {
        ESP_LOGI(LOG_TAG, "Measured distance out of range.");
        return -1; // Out of range
    }

    int distance = (pulseDuration - 1000) * 3 / 4;
    return distance < 0 ? 0 : distance;
}

void calibrateThresholds()
{
    if (calibrationSamples > 0)
    {
        minDistance = (1 - MIN_CONFIDENCE) * minDistance;
        maxDistance = (1 + MIN_CONFIDENCE) * maxDistance;
        ESP_LOGI(LOG_TAG, "Calibrated thresholds: Min: %d, Max: %d", minDistance, maxDistance);
    }
    else
    {
        ESP_LOGI(LOG_TAG, "Calibration failed: No samples collected.");
    }

    // Resets calibration variables
    calibrationCounter = 0;
    calibrationSamples = 0;
}

void updateDistanceStatistics(int distance)
{
    // Updates min and max distances
    minDistance = distance < minDistance ? distance : minDistance;
    maxDistance = distance > maxDistance ? distance : maxDistance;

    // Updates calibration counter and sample count.
    calibrationCounter += pdMS_TO_TICKS(100); // Increment by delay of taskReadPulse
    calibrationSamples++;
}
