/**
 * @file    SGP40.c
 * @brief   Driver for SGP40 VOC sensor using I2C communication.
 *
 * Provides functions to initialize the sensor, perform default VOC measurements,
 * and convert raw VOC output into a normalized VOC index.
 * Designed for integration with FreeRTOS and the I2cDriver interface.
 */

#include "SGP40.h"
#include "i2c_master.h"
#include "i2c_master_interrupt.h"
#include "I2cDriver\I2cDriver.h"
#include "stdint.h"
#include "SerialConsole.h"

I2C_Data SGP40Data;
/**
 * @fn      int SGP40_Init(void)
 * @brief   Initializes the SGP40 VOC sensor by reading its serial ID.
 * @details Sends a "get serial ID" command and prints the 9-byte response.
 *          Assumes I2C interface is already configured.
 * 
 * @return  Returns 0 if successful, otherwise error code from I2C layer.
 */
int SGP40_Init(void) {
    uint8_t buffer[64];     // Buffer for incoming I2C data
    uint8_t buffer1[64];    // Buffer for formatted serial number

    // SGP40 "get serial ID" command (2 bytes)
    uint8_t cmd[] = {SGP40_CMD_GET_SERIAL_ID1, SGP40_CMD_GET_SERIAL_ID2};

    // Prepare I2C transaction
    SGP40Data.address = SGP40_ADDR;
    SGP40Data.msgOut = cmd;
    SGP40Data.lenOut = sizeof(cmd);
    SGP40Data.msgIn = buffer;
    SGP40Data.lenIn = 9;  // Expecting 9 bytes back

    int error = I2cReadDataWait(&SGP40Data, WAIT_TIME, WAIT_TIME);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("SGP Get Serial Fail!\r\n");
        return error;
    }

    // Format serial bytes as hexadecimal string
    char *bufPtr = (char *)buffer1;
    int len = 0;
    for (size_t i = 0; i < 9; ++i) {
        if (len < sizeof(buffer1) - 3) {
            len += snprintf(bufPtr + len, sizeof(buffer1) - len, "%02X", buffer[i]);
        }
    }

    SerialConsoleWriteString("Serial Number: ");
    SerialConsoleWriteString(buffer1);
    SerialConsoleWriteString("\r\n");

    return error;
}


/**
 * @fn      int32_t SGP40_Read_Default_Data(uint8_t *buffer, uint8_t count)
 * @brief   Sends a default measurement command to SGP40 and reads VOC raw data.
 * @details Uses default temperature (25¡ãC) and RH (50%) with CRC, in raw mode.
 * 
 * @param[out] buffer - Pointer to array to store received data
 * @param[in]  count  - Number of expected bytes (should match buffer size)
 * @return     Returns 0 if successful, otherwise error code.
 */
int32_t SGP40_Read_Default_Data(uint8_t *buffer, uint8_t count) {
    // Command: Measure VOC with default 25¡ãC, 50%RH, including CRCs
    uint8_t cmd[] = {
        SGP40_CMD_MEASURE_RAW1, SGP40_CMD_MEASURE_RAW2,
        0x80, 0x00, 0xA2,       // 25¡ãC with CRC
        0x66, 0x66, 0x93        // 50%RH with CRC
    };

    // Prepare I2C transfer
    SGP40Data.address = SGP40_ADDR;
    SGP40Data.msgOut = cmd;
    SGP40Data.lenOut = sizeof(cmd);
    SGP40Data.msgIn = buffer;
    SGP40Data.lenIn = count;

    int error = I2cReadDataWait(&SGP40Data, WAIT_TIME, WAIT_TIME);
    if (ERROR_NONE != error) {
        SerialConsoleWriteString("Error reading SGP data!\r\n");
    }
    return error;
}


/**
 * @fn      void Voc_process(const uint16_t voc_raw, int *voc_index)
 * @brief   Processes raw VOC sensor value and maps it to a VOC index.
 * @details A simple linear mapping from VOC raw data to index scale.
 *          The index is inverted: higher raw values result in lower indices.
 * 
 * @param[in]  voc_raw    - Raw VOC output from sensor (0¨C65535)
 * @param[out] voc_index  - Pointer to output VOC index (0¨CMAX_VOC_INDEX)
 */
void Voc_process(const uint16_t voc_raw, int *voc_index) {
    double scale_factor = 1.0;

    // Map raw VOC reading to VOC index (inverse scale)
    *voc_index = (int)(500 - (voc_raw * ((double)MAX_VOC_INDEX / MAX_VOC_RAW) * scale_factor));

    // Clamp index to valid range
    if (*voc_index > MAX_VOC_INDEX) {
        *voc_index = 0;
    } else if (*voc_index < 0) {
        *voc_index = MAX_VOC_INDEX;
    }
}
