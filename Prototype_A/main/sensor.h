/**
 * @file sensor.h
 * @brief Header file containing function for sensor-related functionality.
 *
 * This header file provides function prototypes for initializing the sensor and reading sensor data.
 * It is intended to be included in files that need to use sensor functionality.
 */
#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

bool flatSensorDetect(void);
bool deepSensorDetect(void);
bool basketSensorDetect(void);
void initializeSensor(void);

#endif // SENSORS_H
