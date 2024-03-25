/**
 * @file sensor.c
 * @brief Implementation file containing functions for sensor-related functionality.
 *
 * This file contains the implementation of functions for initializing the sensor and reading sensor data.
 * It is intended to be compiled and linked with other source files that need to use sensor functionality.
 *
 */
#include "sensor.h"
#include "driver/gpio.h"
#include "esp_log.h" 


#define FLAT_PLATE_SENSOR GPIO_NUM_21 
#define DEEP_SESOR GPIO_NUM_13 
#define BASKET_SENSOR GPIO_NUM_17

/**
 * initializing the sensor
*/
void initializeSensor(void) {
    gpio_reset_pin(FLAT_PLATE_SENSOR);
    gpio_set_direction(FLAT_PLATE_SENSOR, GPIO_MODE_INPUT);
    gpio_reset_pin(DEEP_SESOR);
    gpio_set_direction(DEEP_SESOR, GPIO_MODE_INPUT);  
    gpio_reset_pin(BASKET_SENSOR);
    gpio_set_direction(BASKET_SENSOR, GPIO_MODE_INPUT);  
}

/**
 * finction that treads the flat sensor (flat plates sensor)output and retur true in case of high vlaues(1)
 * and false in case of low(0)
*/
bool flatSensorDetect(void) {
    int flat_val = gpio_get_level(FLAT_PLATE_SENSOR); 
    if(flat_val == 0){
        return true;
    }
    return false;
}

/**
 * finction that treads the deep sensor (deep plates sensor)output and retur true in case of high vlaues(1)
 * and false in case of low(0)
*/
bool deepSensorDetect(void) {
    int deep_sensor = gpio_get_level(DEEP_SESOR);
    if(deep_sensor == 0){
        return true;
    }
    return false;
}

/**
 * finction that treads the basket sensor output and retur true in case of high vlaues(1)
 * and false in case of low(0)
*/
bool basketSensorDetect(void) {
    int basket_sensor = gpio_get_level(BASKET_SENSOR);
    if(basket_sensor == 0) {
        return true;
    }
    return false;
}
