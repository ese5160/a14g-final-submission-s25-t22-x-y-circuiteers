/**
 * @file    EnvSensorTask.h
 * @brief   Interface for the environmental sensing FreeRTOS task.
 *
 * Declares task function and shared flags for distance safety and sensor readiness.
 * Supports reading data from SHTC3 (Temp/RH) and SGP40 (VOC) sensors.
 */


#ifndef ENV_SENSOR_TASK_H
#define ENV_SENSOR_TASK_H

#include <stdbool.h>

#define SHTC3_READ_BUF_SIZE 6
#define SGP40_READ_BUF_SIZE 3

extern volatile bool distance_safe;
extern volatile bool sensor_ready;

void vEnvSensorTask(void *pvParameters);

#endif
