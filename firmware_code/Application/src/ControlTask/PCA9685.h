/**
 * @file    PCA9685.h
 * @brief   Interface for controlling PCA9685 16-channel PWM driver.
 *
 * Provides functions to initialize the device, set PWM frequency,
 * and control servo angles on individual channels.
 */

#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#define PCA9685_I2C_ADDRESS 0x40
#define PCA9685_FREQ        50
#define PCA9685_SERVO_MIN   150
#define PCA9685_SERVO_MAX   600

#ifdef __cplusplus
extern "C" {
	#endif
	
	void pca9685_init(void);
	int32_t set_servo_angle(uint8_t channel, int angle);
	void PCA9685_SetPWMFreq(uint8_t freq_hz);

	#ifdef __cplusplus
}
#endif

#endif  // PCA9685_H
