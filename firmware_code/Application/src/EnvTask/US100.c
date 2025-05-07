#include "US100.h"
#include "tc.h"
#include "port.h"
#include "extint.h"
#include "delay.h"

static struct tc_module echo_timer;  // TC instance for measuring pulse width

static volatile uint16_t start_time = 0;
static volatile uint16_t end_time = 0;
static volatile bool edge_rising = true;     // Rising/falling edge flag
static volatile int32_t distance_cm = -1;    // Latest measured distance in cm

/**
 * @fn      int32_t Ultrasonic_GetDistanceCM(void)
 * @brief   Returns the latest measured distance in centimeters.
 * @return  Distance in cm if valid, -1 if out of range or invalid.
 */
int32_t Ultrasonic_GetDistanceCM(void)
{
    return distance_cm;
}

/**
 * @fn      void Ultrasonic_Trigger(void)
 * @brief   Sends a 10us pulse to the ultrasonic sensor's TRIG pin.
 * @details TRIG pin must be held high for 10?s to initiate a measurement.
 */
void Ultrasonic_Trigger(void)
{
    port_pin_set_output_level(TRIG_PIN, true);  // Set TRIG high

    // Approximate 10?s delay (depending on CPU speed, ~1000 iterations)
    for (volatile int i = 0; i < 1000; i++);

    port_pin_set_output_level(TRIG_PIN, false); // Set TRIG low
}

/**
 * @fn      void ECHO_PIN_ISR(void)
 * @brief   Interrupt Service Routine for the ECHO pin signal edge.
 * @details Measures time between rising and falling edge to compute distance.
 *          Uses TC counter to time the echo pulse duration.
 */
void ECHO_PIN_ISR(void)
{
    if (edge_rising) {
        // On rising edge: record start time
        start_time = tc_get_count_value(&echo_timer);
        edge_rising = false;
    } else {
        // On falling edge: record end time and compute duration
        end_time = tc_get_count_value(&echo_timer);
        edge_rising = true;

        uint16_t duration = (end_time >= start_time)
                          ? (end_time - start_time)
                          : (0xFFFF - start_time + end_time);  // handle timer overflow

        float duration_us = duration / 48.0f;  // Assuming 48 MHz clock
        distance_cm = (int32_t)(duration_us * 0.0343f / 2.0f * 100);  // Sound speed: 343 m/s

        // Reject invalid readings
        if (distance_cm < 2 || distance_cm > 400) {
            distance_cm = -1;
        }
    }
}

/**
 * @fn      void Ultrasonic_Init(void)
 * @brief   Initializes the TRIG pin, timer counter, and ECHO pin interrupt.
 */
void Ultrasonic_Init(void)
{
    // --- Configure TRIG pin as output ---
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(TRIG_PIN, &pin_conf);
    port_pin_set_output_level(TRIG_PIN, false);  // Default low

    // --- Configure TC (Timer/Counter) for echo timing ---
    struct tc_config config_tc;
    tc_get_config_defaults(&config_tc);
    config_tc.counter_size = TC_COUNTER_SIZE_16BIT;
    config_tc.clock_source = GCLK_GENERATOR_0;
    config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1;

    tc_init(&echo_timer, TIMER_TC, &config_tc);
    tc_enable(&echo_timer);

    // --- Configure ECHO pin for external interrupt (EIC) ---
    struct extint_chan_conf config_extint;
    extint_chan_get_config_defaults(&config_extint);
    config_extint.gpio_pin             = ECHO_PIN;
    config_extint.gpio_pin_mux         = MUX_PA05A_EIC_EXTINT5;
    config_extint.gpio_pin_pull        = EXTINT_PULL_NONE;
    config_extint.filter_input_signal  = true;
    config_extint.detection_criteria   = EXTINT_DETECT_BOTH;  // Detect rising & falling edges

    extint_chan_set_config(5, &config_extint);
    extint_register_callback(ECHO_PIN_ISR, 5, EXTINT_CALLBACK_TYPE_DETECT);
    extint_chan_enable_callback(5, EXTINT_CALLBACK_TYPE_DETECT);
}

