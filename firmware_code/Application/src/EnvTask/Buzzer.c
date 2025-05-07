#include <asf.h>
#include "Buzzer.h"

/**
 * @fn      void buzzer_gpio_init(void)
 * @brief   Initializes the GPIO pin used to drive the buzzer.
 * @details Configures BUZZER_PIN as an output and sets it to low initially (off).
 */
void buzzer_gpio_init(void)
{
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);               // Load default pin config
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;          // Set as output
    port_pin_set_config(BUZZER_PIN, &pin_conf);        // Apply config to buzzer pin
    port_pin_set_output_level(BUZZER_PIN, false);      // Set low (buzzer off)
}

/**
 * @fn      void BuzzerPWM_Init(void)
 * @brief   Initializes the buzzer (currently via GPIO only).
 * @details Calls buzzer_gpio_init(). Placeholder if PWM implementation is added later.
 */
void BuzzerPWM_Init(void)
{
    buzzer_gpio_init();  // Configure buzzer GPIO
}

/**
 * @fn      void BuzzerPWM_Start(void)
 * @brief   Starts the buzzer sound (constant ON).
 * @details Sets the buzzer pin high. Assumes GPIO-based control.
 */
void BuzzerPWM_Start(void)
{
    port_pin_set_output_level(BUZZER_PIN, true);  // Set high (buzzer ON)
}

/**
 * @fn      void BuzzerPWM_Stop(void)
 * @brief   Stops the buzzer sound.
 * @details Sets the buzzer pin low to disable sound.
 */
void BuzzerPWM_Stop(void)
{
    port_pin_set_output_level(BUZZER_PIN, false); // Set low (buzzer OFF)
}

