#include <asf.h>                
#include "AT42QT1010.h"         

/**
 * @fn      void AT42QT1010_Init(void)
 * @brief   Initialize the GPIO pin connected to AT42QT1010 as input.
 * @details Sets the TOUCH_PIN as a digital input without pull-up or pull-down.
 *          Assumes that ASF system clock and GPIO modules are already initialized.
 * @return  None.
 */
void AT42QT1010_Init(void)
{
    struct port_config pin_conf;

    // Load default configuration into the structure
    port_get_config_defaults(&pin_conf);

    // Set the pin direction as input
    pin_conf.direction = PORT_PIN_DIR_INPUT;

    // Disable internal pull-up/down resistors
    pin_conf.input_pull = PORT_PIN_PULL_NONE;

    // Apply configuration to the specified touch pin
    port_pin_set_config(TOUCH_PIN, &pin_conf);
}

/**
 * @fn      bool AT42QT1010_IsTouched(void)
 * @brief   Check the current state of the touch sensor.
 * @details Reads the digital logic level of the TOUCH_PIN.
 *          If the sensor is being touched, the pin typically goes HIGH.
 * @return  true  - if touch detected (logic HIGH)
 *          false - if not touched (logic LOW)
 */
bool AT42QT1010_IsTouched(void)
{
    // Read and return the logic level of the pin
    return port_pin_get_input_level(TOUCH_PIN);
}
