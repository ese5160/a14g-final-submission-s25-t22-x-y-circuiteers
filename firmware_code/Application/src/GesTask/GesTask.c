/**
 * @file    GesTask.c
 * @brief   Gesture recognition task using APDS9960 sensor.
 *
 * This task initializes the APDS9960, checks for gesture events when enabled,
 * and maps detected gestures (left/right) to robot motion states.
 */

#include "SerialConsole.h"
#include "FreeRTOS.h"
#include "task.h"
#include "APDS9960.h"
#include "GesTask.h"
#include "ControlTask/ControlTask.h"   

volatile bool gestureEnabled = false;  // External flag to enable/disable gesture detection

/**
 * @fn      void GesTask(void *pvParameters)
 * @brief   FreeRTOS task for detecting gestures using the APDS9960 sensor.
 * @details Initializes APDS9960, then periodically checks for available gesture data.
 *          On detection, it classifies the gesture and updates current_state accordingly.
 */
void GesTask(void *pvParameters)
{
    // Initialize APDS9960 sensor
    if (!APDS9960_Init()) {
        SerialConsoleWriteString("APDS9960 Init failed!\r\n");
        vTaskDelete(NULL);  // Terminate task on failure
    }

    SerialConsoleWriteString("APDS9960 Ready\r\n");

    int gesture = DIR_NONE;

    while (1) {
        if (gestureEnabled) {
            // Check if gesture data is ready
            if (APDS9960_IsGestureAvailable()) {
                SerialConsoleWriteString("APDS9960\r\n");

                // Read and classify gesture
                if (APDS9960_ReadGesture(&gesture)) {
                    switch (gesture) {
                        case DIR_LEFT:
                            current_state = STATE_LEFT_SHIFT;
                            SerialConsoleWriteString("Left Gesture\r\n");
                            break;

                        case DIR_RIGHT:
                            current_state = STATE_RIGHT_SHIFT;
                            SerialConsoleWriteString("Right Gesture\r\n");
                            break;

                        default:
                            break;  // No valid gesture
                    }
                }
            }
        }

        // Task delay to reduce polling frequency
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
