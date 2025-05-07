/**
 * @file    PCA9685.c
 * @brief   Driver functions for controlling servo motors via the PCA9685 PWM controller.
 *
 * Provides I2C-based configuration of PWM frequency and servo angle output.
 * Includes register-level access for prescaler and pulse timing setup.
 * Used in robotic motion control to drive up to 16 servo channels.
 */

#include "PCA9685.h"
#include "I2cDriver/I2cDriver.h"
#include "SerialConsole.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h> 

I2C_Data PCA9685Data; // Global I2C communication data structure for PCA9685

/**
 * @fn      static long map(long x, long in_min, long in_max, long out_min, long out_max)
 * @brief   Maps a value from one range to another.
 * @param   x        - Input value
 * @param   in_min   - Input range minimum
 * @param   in_max   - Input range maximum
 * @param   out_min  - Output range minimum
 * @param   out_max  - Output range maximum
 * @return  Mapped value in the output range
 */
static long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/**
 * @fn      static int32_t PCA9685_WriteCommand(uint8_t reg, uint8_t value)
 * @brief   Sends a single-byte write command to the PCA9685 via I2C.
 * @param   reg   - Target register address
 * @param   value - Value to write to the register
 * @return  I2C communication result (0 on success)
 */
static int32_t PCA9685_WriteCommand(uint8_t reg, uint8_t value) {
	uint8_t cmd[2] = { reg, value };

	memset(&PCA9685Data, 0, sizeof(PCA9685Data));
	PCA9685Data.address = PCA9685_I2C_ADDRESS;
	PCA9685Data.msgOut = cmd;
	PCA9685Data.lenOut = sizeof(cmd);
	PCA9685Data.lenIn = 0;

	return I2cWriteDataWait(&PCA9685Data, 0xFF); 
}
/**
 * @fn      void PCA9685_SetPWMFreq(uint8_t freq_hz)
 * @brief   Sets the PWM frequency of the PCA9685.
 * @details Configures the internal prescaler to generate a desired frequency.
 * @param   freq_hz - Desired PWM frequency in Hz (typically 50Hz for servos)
 */
void PCA9685_SetPWMFreq(uint8_t freq_hz) {
	float prescaleval = 25000000.0 / (4096.0 * freq_hz) - 1.0;
	uint8_t prescale = (uint8_t)(prescaleval + 0.5f);

	// Enter sleep mode before setting prescaler
	PCA9685_WriteCommand(0x00, 0x10);
	vTaskDelay(pdMS_TO_TICKS(1));

	// Set the prescaler
	PCA9685_WriteCommand(0xFE, prescale);

	// Wake up
	PCA9685_WriteCommand(0x00, 0x00);
	vTaskDelay(pdMS_TO_TICKS(1));

	// Restart with auto-increment enabled
	PCA9685_WriteCommand(0x00, 0xA1);
}

/**
 * @fn      void pca9685_init(void)
 * @brief   Initializes the PCA9685 by resetting MODE1 register.
 * @return  None.
 */
void pca9685_init(void) {
	PCA9685_WriteCommand(0x00, 0x00);  // Reset mode
	vTaskDelay(pdMS_TO_TICKS(5));
	SerialConsoleWriteString("PCA9685 Initialized\r\n");
}

/**
 * @fn      int32_t set_servo_angle(uint8_t channel, int angle)
 * @brief   Sets a servo motor to a specific angle on the given channel.
 * @details Maps angle [0,180] to corresponding PWM pulse and writes to PCA9685 registers.
 * @param   channel - PCA9685 output channel (0¨C15)
 * @param   angle   - Desired servo angle in degrees (0¨C180)
 * @return  I2C communication result (0 on success)
 */
int32_t set_servo_angle(uint8_t channel, int angle) {
	// Clamp angle to [0, 180]
	if (angle < 0) angle = 0;
	if (angle > 180) angle = 180;

	// Map angle to pulse width (typically 500¨C2500us scaled to 12-bit value)
	int pulse = map(angle, 0, 180, PCA9685_SERVO_MIN, PCA9685_SERVO_MAX);

	uint8_t data[5] = {
		(uint8_t)(0x06 + 4 * channel),  // Start register address for LEDn_ON_L
		0x00, 0x00,                     // LEDn_ON = 0 (start of cycle)
		(uint8_t)(pulse & 0xFF),        // LEDn_OFF_L
		(uint8_t)(pulse >> 8)           // LEDn_OFF_H
	};

	memset(&PCA9685Data, 0, sizeof(PCA9685Data));
	PCA9685Data.address = PCA9685_I2C_ADDRESS;
	PCA9685Data.msgOut = data;
	PCA9685Data.lenOut = sizeof(data);
	PCA9685Data.lenIn = 0;

	return I2cWriteDataWait(&PCA9685Data, 0xFF);
}
