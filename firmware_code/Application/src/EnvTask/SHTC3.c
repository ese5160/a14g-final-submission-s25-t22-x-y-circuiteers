/**
 * @file    SHTC3.c
 * @brief   Driver for SHTC3 temperature and humidity sensor via I2C.
 *
 * Implements functions for sensor initialization and environmental data reading.
 * Communicates using standard I2C protocol and integrates with I2cDriver interface.
 */


#include "SHTC3.h"
#include "i2c_master.h"
#include "i2c_master_interrupt.h"
#include "I2cDriver\I2cDriver.h"
#include "stdint.h"
#include "SerialConsole.h"

I2C_Data SHTC3Data;

/**
 * @fn      int SHTC3_Init(void)
 * @brief   Initializes the SHTC3 temperature and humidity sensor.
 * @details Sends a wakeup command over I2C. Assumes I2C is already initialized.
 *
 * @return  Returns 0 if successful, otherwise I2C error code.
 */
int SHTC3_Init(void) {
    // Wakeup command: 0x35 0x17 (per SHTC3 datasheet)
    uint8_t cmd[] = {SHTC3_WAKEUP_CMD1, SHTC3_WAKEUP_CMD2};

    SHTC3Data.address = SHTC3_ADDR;
    SHTC3Data.msgOut = cmd;
    SHTC3Data.lenOut = sizeof(cmd);
    SHTC3Data.lenIn = 0;

    // Send wakeup command and wait
    int32_t error = I2cWriteDataWait(&SHTC3Data, WAIT_TIME);

    return error;
}

/**
 * @fn      int32_t SHTC3_Read_Data(uint8_t *buffer, uint8_t count)
 * @brief   Reads temperature and humidity data from SHTC3.
 * @details Sends a measurement command (normal mode, no clock stretching),
 *          then reads back the result into the buffer.
 *
 * @param[out] buffer - Pointer to receive data (must be at least 6 bytes)
 * @param[in]  count  - Number of bytes to read from sensor (typically 6)
 * @return     Returns 0 if successful, otherwise I2C error code.
 */
int32_t SHTC3_Read_Data(uint8_t *buffer, uint8_t count) {
    // Command: Measure T first, then RH, normal power, no clock stretching
    uint8_t cmd[] = {SHT3_TH_NM_NCS_MEASURE_CMD1, SHT3_TH_NM_NCS_MEASURE_CMD2};

    SHTC3Data.address = SHTC3_ADDR;
    SHTC3Data.msgOut = cmd;
    SHTC3Data.lenOut = sizeof(cmd);
    SHTC3Data.msgIn = buffer;
    SHTC3Data.lenIn = count;  // should match actual expected count (typically 6)

    int error = I2cReadDataWait(&SHTC3Data, WAIT_TIME, WAIT_TIME);

    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Error reading SHTC3 data!\r\n");
    }

    return error;
}

