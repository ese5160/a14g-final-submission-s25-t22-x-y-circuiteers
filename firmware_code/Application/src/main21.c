/**
 * @file      main.c
 * @brief     Main application entry point
 * @author    Eduardo Garcia
 * @author    Nick M-G
 * @date      2022-04-14
 ******************************************************************************/

/****
 * Includes
 ******************************************************************************/
#include <errno.h>

#include "CliThread/CliThread.h"
#include "FreeRTOS.h"
#include "I2cDriver\I2cDriver.h"
#include "I2cDriver\I2CScanTask.h"
#include "SerialConsole.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "main.h"
#include "stdio_serial.h"
#include "EnvTask/EnvSensorTask.h" 
#include "DisplayTask/DisplayTask.h"  
#include "ControlTask/ControlTask.h"
#include "GesTask/GesTask.h"
#include "EnvTask/SHTC3.h"
#include "EnvTask/SGP40.h"
/****
 * Defines and Types
 ******************************************************************************/
#define APP_TASK_ID 0 /**< @brief ID for the application task */
#define CLI_TASK_ID 1 /**< @brief ID for the command line interface task */
#define ENV_TASK_SIZE 300
#define ENV_PRIORITY (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_SIZE 380 
#define DISPLAY_PRIORITY (tskIDLE_PRIORITY + 1) 
#define CONTROL_TASK_SIZE 430  /**< @brief Size for the control task */
#define CONTROL_TASK_PRIORITY (tskIDLE_PRIORITY + 2) /**< @brief Priority for the control task */
#define GES_TASK_SIZE          180  /**< @brief Size for the gesture task */
#define GES_TASK_PRIORITY      (tskIDLE_PRIORITY + 2) /**< @brief Priority for the gesture task */

/****
 * Local Function Declaration
 ******************************************************************************/
void vApplicationIdleHook(void);
//!< Initial task used to initialize HW before other tasks are initialized
static void StartTasks(void);
void vApplicationDaemonTaskStartupHook(void);

void vApplicationStackOverflowHook(void);
void vApplicationMallocFailedHook(void);
void vApplicationTickHook(void);

/****
 * Variables
 ******************************************************************************/
static TaskHandle_t cliTaskHandle = NULL;       //!< CLI task handle
static TaskHandle_t daemonTaskHandle = NULL;    //!< Daemon task handle
static TaskHandle_t wifiTaskHandle = NULL;      //!< Wifi task handle
static TaskHandle_t uiTaskHandle = NULL;        //!< UI task handle
static TaskHandle_t controlTaskHandle = NULL;   //!< Control task handle
static TaskHandle_t envTaskHandle = NULL;       //!< Env task handle
static TaskHandle_t displayTaskHandle = NULL;   //!< Display task handle

char bufferPrint[64];   ///< Buffer for daemon task
//Env Queue
QueueHandle_t xSensorQueue;

/**
 * @brief Main application function.
 * Application entry point.
 * @return int
 */
int main(void) {
    /* Initialize the board. */
    system_init();

    // Initialize trace capabilities
    vTraceEnable(TRC_START);
    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;   // Will not get here
}

/**
 * function          vApplicationDaemonTaskStartupHook
 * @brief            Initialization code for all subsystems that require FreeRToS
 * @details			This function is called from the FreeRToS timer task. Any code
 *					here will be called before other tasks are initilized.
 * @param[in]        None
 * @return           None
 */
void vApplicationDaemonTaskStartupHook(void) {
	// Initialize the UART console here so heap is ready
    InitializeSerialConsole();
    SerialConsoleWriteString("\r\n\r\n-----ESE516 Main Program-----\r\n");

    // Initialize HW that needs FreeRTOS Initialization
    SerialConsoleWriteString("\r\n\r\nInitialize HW...\r\n");
    if (I2cInitializeDriver() != STATUS_OK) {
        SerialConsoleWriteString("Error initializing I2C Driver!\r\n");
    } else {
        SerialConsoleWriteString("Initialized I2C Driver!\r\n");
    }
	
    StartTasks();

    vTaskSuspend(daemonTaskHandle);
}

/**
 * function          StartTasks
 * @brief            Initialize application tasks
 * @details
 * @param[in]        None
 * @return           None
 */
static void StartTasks(void) {
	snprintf(bufferPrint, 64, "Heap before starting tasks: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	// Initialize Tasks here
	if (xTaskCreate(vCommandConsoleTask, "CLI_TASK", CLI_TASK_SIZE, NULL, CLI_PRIORITY, &cliTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: CLI task could not be initialized!\r\n");
	}

	snprintf(bufferPrint, 64, "Heap after starting CLI: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
	
 	// Env
	xSensorQueue = xQueueCreate(5, sizeof(SensorData));
    if (xSensorQueue == NULL) {
        SerialConsoleWriteString("ERR: could not create EnvSensor queue!\r\n");
    }
	if (xTaskCreate(vEnvSensorTask, "ENV_TASK", ENV_TASK_SIZE, NULL, ENV_PRIORITY, &envTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: ENV task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting ENV: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
	
	// WI-FI
	if (xTaskCreate(vWifiTask, "WIFI_TASK", WIFI_TASK_SIZE, NULL, WIFI_PRIORITY, &wifiTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: WIFI task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting WIFI: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

 	// Control
	if (xTaskCreate(ControlTask, "CONTROL_TASK", CONTROL_TASK_SIZE, NULL, CONTROL_TASK_PRIORITY, &controlTaskHandle) != pdPASS) {
			SerialConsoleWriteString("ERR: Control task could not be initialized!\r\n");
		}
		snprintf(bufferPrint, 64, "Heap after starting CONTROL_TASK: %d\r\n", xPortGetFreeHeapSize());
		SerialConsoleWriteString(bufferPrint);
		
	// Gesture Task
	if (xTaskCreate(GesTask, "GES_TASK", GES_TASK_SIZE, NULL, GES_TASK_PRIORITY, NULL) != pdPASS) {
		SerialConsoleWriteString("ERR: GES task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting GES_TASK: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	// Display
	if (xTaskCreate(vDisplayTask, "DISPLAY_TASK", DISPLAY_TASK_SIZE, NULL, DISPLAY_PRIORITY, &displayTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: DISPLAY task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting DISPLAY: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

} 


void vApplicationMallocFailedHook(void) {
    SerialConsoleWriteString("Error on memory allocation on FREERTOS!\r\n");
    while (1)
        ;
}

void vApplicationStackOverflowHook(void) {
    SerialConsoleWriteString("Error on stack overflow on FREERTOS!\r\n");
    while (1)
        ;
}

#include "MCHP_ATWx.h"
void vApplicationTickHook(void) { SysTick_Handler_MQTT(); }