#ifndef DISH_COUNTER_TASKS_H
#define DISH_COUNTER_TASKS_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

extern SemaphoreHandle_t trayDetectionSemaphore;

void initialize_gpio(void);
void monitorIRSensorTask(void *parameter);
void readPulseTask(void *parameter);

uint32_t dishCounterGetCount(void);

#endif
