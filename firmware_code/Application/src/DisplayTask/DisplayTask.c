/**
 * @file    DisplayTask.c
 * @brief   Displays real-time sensor readings and system state on the LCD.
 *
 * Reads SensorData from a FreeRTOS queue and updates the ST7735 screen
 * with temperature, humidity, VOC index, distance, and robot mode.
 * Designed for continuous execution as a FreeRTOS task.
 */


#include "DisplayTask.h"
#include "ST7735.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "EnvTask/EnvSensorTask.h"
#include "ControlTask/ControlTask.h"

/**
 * @fn      void vDisplayTask(void *pvParameters)
 * @brief   FreeRTOS task to display sensor readings and system state on LCD.
 * @details Waits for incoming SensorData from a queue and updates the screen.
 *          Displays temperature, humidity, VOC, distance, and current robot state.
 * 
 * @param   pvParameters - Not used (can be NULL)
 * @return  None (runs indefinitely)
 */
void vDisplayTask(void *pvParameters) {
	SensorData d;

	// Initialize LCD display
	LCD_init();
	LCD_clearScreen(BLACK);

	// Draw static labels
	drawString(10, 20,  "Temp:", WHITE, BLACK);
	drawString(10, 40,  "Humi:", WHITE, BLACK);
	drawString(10, 60,   "VOC:", WHITE, BLACK);
	drawString(10, 80,  "Dist:", WHITE, BLACK);
	drawString(10, 100, "Mode:", WHITE, BLACK);

	// Draw initial mode status background
	drawRectangle(70, 100, _GRAMWIDTH - 1, 107, BLACK);
	drawString(70, 100, "IDLE", WHITE, BLACK);

	// Main display update loop
	for (;;) {
		// Wait indefinitely for new sensor data from the queue
		if (xQueueReceive(xSensorQueue, &d, portMAX_DELAY) == pdPASS) {
			char buffer[20];
			memset(buffer, 0, sizeof(buffer));

			// --- Temperature ---
			drawRectangle(70, 20, 120, 30, BLACK);  // Clear previous value
			snprintf(buffer, sizeof(buffer), "%d.%02dC", d.temp / 100, abs(d.temp) % 100);
			drawString(70, 20, buffer, WHITE, BLACK);

			// --- Humidity ---
			drawRectangle(70, 40, 120, 60, BLACK);
			snprintf(buffer, sizeof(buffer), "%d.%02d%%", d.rh / 100, abs(d.rh) % 100);
			drawString(70, 40, buffer, WHITE, BLACK);

			// --- VOC ---
			drawRectangle(70, 60, 120, 90, BLACK);
			snprintf(buffer, sizeof(buffer), "%d.%02d", d.voc / 100, abs(d.voc) % 100);
			drawString(70, 60, buffer, WHITE, BLACK);

			// --- Distance ---
			drawRectangle(70, 80, 120, 120, BLACK);
			if (d.dist_cm >= 0) {
				snprintf(buffer, sizeof(buffer), "%d.%02dcm", d.dist_cm / 100, abs(d.dist_cm) % 100);
			} else {
				snprintf(buffer, sizeof(buffer), "--.--cm");
			}
			drawString(70, 80, buffer, WHITE, BLACK);

			// --- Mode Display ---
			if (current_state == STATE_IDLE) {
				drawRectangle(70, 100, _GRAMWIDTH - 1, 107, BLACK);
				drawString(70, 100, "IDLE", WHITE, BLACK);
			}
		}
	}
}

