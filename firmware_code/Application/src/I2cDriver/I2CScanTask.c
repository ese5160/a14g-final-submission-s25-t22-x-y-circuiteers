#include "I2CScanTask.h"
#include "SerialConsole.h"
#include "i2c_master.h"  

extern struct i2c_master_module i2cSensorBusInstance;

/**
 * @fn      void vI2CScanTask(void *pvParameters)
 * @brief   FreeRTOS task to scan the I2C bus for connected devices.
 * @details Iterates over all 7-bit addresses (0x01¨C0x7E), sending empty write packets to probe devices.
 *          Prints found addresses over serial console.
 *
 * @param   pvParameters - Unused
 */
void vI2CScanTask(void *pvParameters) {
    char msg[64];

    SerialConsoleWriteString("\r\nI2C Scanner Started:\r\n");

    // Loop through 7-bit address range: 0x01 to 0x7E
    for (uint8_t addr = 1; addr < 127; addr++) {
        struct i2c_master_packet packet;
        uint8_t dummy = 0;

        packet.address           = addr;
        packet.data              = &dummy;      // Send empty data
        packet.data_length       = 0;           // No data, just probing
        packet.ten_bit_address   = false;
        packet.high_speed        = false;
        packet.hs_master_code    = 0x0;

        // Try sending a write to the device address
        enum status_code result = i2c_master_write_packet_wait(&i2cSensorBusInstance, &packet);

        if (result == STATUS_OK) {
            snprintf(msg, sizeof(msg), "Found device at 0x%02X\r\n", addr);
            SerialConsoleWriteString(msg);
        }

        vTaskDelay(pdMS_TO_TICKS(20));  // Small delay between address probes
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    SerialConsoleWriteString("I2C scan complete.\r\n");
    vTaskDelay(pdMS_TO_TICKS(100));

    vTaskDelete(NULL);  // Self-delete task
}
