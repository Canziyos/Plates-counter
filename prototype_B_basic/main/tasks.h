#ifndef SENSOR_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include <stdio.h>
//#include "lvgl.h"
#include "driver/gpio.h"

// Declaration of IR and Distance sensor GPIOs.
extern const gpio_num_t IR_SENSOR_GPIO;
extern const gpio_num_t DISTANCE_SENSOR_GPIO;

// External global variables
extern volatile bool isTrayPresent;
extern int count_object; // Count of objects detected by the sensor.
extern int signals; // Number of signals detected by the sensor
extern SemaphoreHandle_t trayDetectionSemaphore;
// Constants for distance measurement
extern const int DISTANCE_MIN; // Minimum distance for detection.
extern const int DISTANCE_MAX; // Maximum distance for detection.
extern bool systemIdle;
// Function declarations for sensor tasks and application logic
void initialize_gpio(void);
int measureDistance(void);
void monitorIRSensorTask(void* pvParameter);
void readPulseTask(void* pvParameter);
float kalmanFilter(int measurement);
void calibrateThresholds(void);

void sendMessages(void* pvParameter);

#endif // SENSOR_TASKS_H
