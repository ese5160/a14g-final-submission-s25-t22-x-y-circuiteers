/**
 * @file    EnvSensorTask.c
 * @brief   Reads sensors (Temp/RH, VOC, Distance, Touch) and triggers alarms.
 *
 * Periodically reads data from SHTC3, SGP40, US100, and AT42QT1010.
 * Sends results to a queue and activates buzzer/LCD alert if thresholds are exceeded.
 * Updates global flag `distance_safe` for other task use.
 */

#include "EnvSensorTask.h"
#include "SerialConsole.h"
#include "SHTC3.h"
#include "SGP40.h"
#include "US100.h"
#include "Buzzer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>  
#include "I2cDriver/I2cDriver.h"
#include "DisplayTask/ST7735.h"
#include "ControlTask/AT42QT1010.h"
#include "WifiHandlerThread/WifiHandler.h"

// Environmental threshold values
float TEMP_THRESHOLD = 50.0f;
float RH_THRESHOLD   = 40.0f;
float VOC_THRESHOLD  = 290.0f;
int   DIST_THRESHOLD = 400;

volatile bool distance_safe = true;  // Shared flag for distance status

/**
 * @fn      void vEnvSensorTask(void *pvParameters)
 * @brief   FreeRTOS task that periodically reads environment sensors and handles safety logic.
 * @details Reads SHTC3 (Temp/RH), SGP40 (VOC), ultrasonic (distance), and touch status,
 *          then sends results to a queue and triggers alarms if thresholds are exceeded.
 */
void vEnvSensorTask(void *pvParameters)
{
    float temp = 25.0f, rh = 50.0f;
    uint16_t voc_raw = 0;
    int voc_index = 0;
    int32_t dist_cm = 0;
    char msg[128];

    uint8_t shtc3_buf[SHTC3_READ_BUF_SIZE] = {0};
    uint8_t sgp40_buf[SGP40_READ_BUF_SIZE] = {0};

    // Delay for hardware readiness
    vTaskDelay(pdMS_TO_TICKS(1000));
    SerialConsoleWriteString("Initializing sensors...\r\n");

    // Initialize SHTC3 and SGP40, abort if failed
    if (SHTC3_Init() != ERROR_NONE || SGP40_Init() != ERROR_NONE) {
        vTaskDelete(NULL);
    }

    // Seed random (if needed)
    srand((unsigned int)xTaskGetTickCount());

    // Initialize remaining hardware
    Ultrasonic_Init();
    BuzzerPWM_Init();
    AT42QT1010_Init();

    SerialConsoleWriteString("All sensors initialized.\r\n");

    while (1)
    {
        // --- Temperature & Humidity ---
        if (SHTC3_Read_Data(shtc3_buf, SHTC3_READ_BUF_SIZE) != ERROR_NONE) {
            SerialConsoleWriteString("Temp/RH read error\r\n");
        } else {
            uint16_t raw_temp = ((uint16_t)shtc3_buf[0] << 8) | shtc3_buf[1];
            uint16_t raw_rh   = ((uint16_t)shtc3_buf[3] << 8) | shtc3_buf[4];

            temp = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
            rh   = 100.0f * ((float)raw_rh / 65535.0f);
        }

        // --- VOC ---
        if (SGP40_Read_Default_Data(sgp40_buf, SGP40_READ_BUF_SIZE) != ERROR_NONE) {
            SerialConsoleWriteString("VOC read error\r\n");
            voc_index = 0;
        } else {
            voc_raw = ((uint16_t)sgp40_buf[0] << 8) | sgp40_buf[1];
            Voc_process(voc_raw, &voc_index);
        }

        // --- Distance ---
        Ultrasonic_Trigger();
        vTaskDelay(pdMS_TO_TICKS(50));  // Wait for echo
        dist_cm = Ultrasonic_GetDistanceCM();

        // --- Data Conversion ---
        int temp_int = (int)(temp * 100);
        int rh_int   = (int)(rh * 100);
        int voc_int  = (int)(voc_index * 100);
        int dist_int = dist_cm;
        bool touched = AT42QT1010_IsTouched();
        int touch_int = touched ? 1 : 0;

        // --- Send to Queue ---
        {
            SensorData sensor_data;
            sensor_data.temp    = temp_int;
            sensor_data.rh      = rh_int;
            sensor_data.voc     = voc_int;
            sensor_data.dist_cm = dist_int;
            sensor_data.touch   = touch_int;

            if (xQueueSend(xSensorQueue, &sensor_data, portMAX_DELAY) != pdPASS) {
                SerialConsoleWriteString("Failed to send Env data to queue\r\n");
            }
        }

        // --- Print to Serial ---
        snprintf(msg, sizeof(msg),
                 "Temp: %d.%02dC  RH: %d.%02d%%  VOC: %d.%02d  ",
                 temp_int / 100, temp_int % 100,
                 rh_int / 100, rh_int % 100,
                 voc_int / 100, voc_int % 100);
        SerialConsoleWriteString(msg);

        if (dist_int < 0) {
            SerialConsoleWriteString("Distance: Out of range\r\n");
        } else {
            snprintf(msg, sizeof(msg), "Distance: %d.%02d cm\r\n",
                     dist_int / 100, dist_int % 100);
            SerialConsoleWriteString(msg);
        }

        // --- Alarm Conditions ---
        bool temp_alarm = (temp > TEMP_THRESHOLD);
        bool rh_alarm   = (rh > RH_THRESHOLD);
        bool voc_alarm  = (voc_index > VOC_THRESHOLD);
        bool dis_alarm  = (dist_int > 0 && dist_int < DIST_THRESHOLD);

        // --- Actuation ---
        if (temp_alarm || rh_alarm || voc_alarm || dis_alarm) {
            BuzzerPWM_Start();
            drawRectangle(70, 100, _GRAMWIDTH - 1, 107, BLACK);
            drawString(70, 100, "Backward", WHITE, BLACK);
        } else {
            BuzzerPWM_Stop();
        }

        distance_safe = !dis_alarm;  // Shared flag for other tasks

        vTaskDelay(pdMS_TO_TICKS(1000));  // Loop interval
    }
}
