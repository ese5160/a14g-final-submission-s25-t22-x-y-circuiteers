  /******************************************************************************
/**
 * @file    SGP40.h
 * @brief   Interface for SGP40 VOC sensor driver.
 *
 * Defines I2C command constants and provides function declarations for:
 * - Sensor initialization
 * - Default VOC measurements
 * - VOC raw-to-index processing
 */

  ******************************************************************************/
  #ifndef SGP40_H
  #define SGP40_H

  #ifdef __cplusplus
  extern "C" {
	  #endif
	  
/******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <math.h>

/******************************************************************************
 * Defines
 ******************************************************************************/
#define SGP40_ADDR 0x59
#define SGP40_CMD_Execute_Self_Test 0x280e
#define SGP40_CMD_Execute_Self_Test1 0x28
#define SGP40_CMD_Execute_Self_Test2 0x0e
#define SGP40_CMD_Turn_Heater_Off 0x3615

#define SGP40_CMD_MEASURE_RAW_WORDS 1
#define SGP40_CMD_MEASURE_RAW 0x260f
#define SGP40_CMD_MEASURE_RAW1 0x26
#define SGP40_CMD_MEASURE_RAW2 0x0f

/* command and constants for reading the serial ID */
#define SGP40_CMD_GET_SERIAL_ID_DURATION_US 500
#define SGP40_CMD_GET_SERIAL_ID_WORDS 3
#define SGP40_CMD_GET_SERIAL_ID 0x3682
#define SGP40_CMD_GET_SERIAL_ID1 0x36
#define SGP40_CMD_GET_SERIAL_ID2 0x82

/* command and constants for reading the featureset version */
#define SGP40_CMD_GET_FEATURESET_DURATION_US 1000
#define SGP40_CMD_GET_FEATURESET_WORDS 1
#define SGP40_CMD_GET_FEATURESET 0x202f

/* command and constants for measuring raw signals */
#define SGP40_CMD_MEASURE_RAW_DURATION_US 100000
#define SGP40_DEFAULT_HUMIDITY 0x8000
#define SGP40_DEFAULT_TEMPERATURE 0x6666
#define SGP40_SERIAL_ID_NUM_BYTES 6

#define WAIT_TIME 0xff

#define MAX_VOC_RAW 65535
#define MAX_VOC_INDEX 500

int SGP40_Init(void);
int32_t SGP40_Read_Default_Data(uint8_t *buffer, uint8_t count);
void Voc_process(const uint16_t voc_raw, int *voc_index);

#ifdef __cplusplus
}
#endif

#endif /*SHTC3_DRIVER_H */

