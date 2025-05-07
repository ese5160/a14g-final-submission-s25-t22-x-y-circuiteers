#ifndef MAIN_H
#define MAIN_H

#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t xSensorQueue;

typedef struct {
	int temp;
	int rh;
	int voc;
	int dist_cm;
	int touch; 
} SensorData;

#endif /* MAIN_H */
