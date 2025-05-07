/**
 * @file    SHTC3.h
 * @brief   Interface for SHTC3 temperature and humidity sensor driver.
 *
 * Provides definitions and function prototypes for:
 * - Sensor wake-up and measurement commands
 * - Data acquisition via I2C
 */

#ifndef SHTC3_H
#define SHTC3_H

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
#define SHTC3_ADDR 0x70
#define SHTC3_WAKEUP_CMD 0x3517
#define SHTC3_WAKEUP_CMD1 0x35
#define SHTC3_WAKEUP_CMD2 0x17
#define SHTC3_SLEEP_CMD 0xB098
#define SHTC3_SOFT_RESET_CMD 0x805D
#define SHTC3_ID_REG 0xEFC8

#define WAIT_TIME 0xff

#define SHT3_TH_NM_NCS_MEASURE_CMD 0x7866  
#define SHT3_TH_NM_NCS_MEASURE_CMD1 0x78  
#define SHT3_TH_NM_NCS_MEASURE_CMD2 0x66 

#define SHT3_TH_LPM_NCS_MEASURE_CMD 0x609C 

#define SHT3_HT_NM_NCS_MEASURE_CMD 0x58E0 
#define SHT3_HT_LPM_NCS_MEASURE_CMD 0x401A  

#define SHT3_TH_NM_CS_MEASURE_CMD 0x7CA2 
#define SHT3_TH_LPM_CS_MEASURE_CMD 0x6458 

#define SHT3_HT_NM_CS_MEASURE_CMD 0x5C24 
#define SHT3_HT_LPM_CS_MEASURE_CMD 0x44DE 

int SHTC3_Init(void);
int32_t SHTC3_Read_Data(uint8_t *buffer, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif /*SHTC3_DRIVER_H */

