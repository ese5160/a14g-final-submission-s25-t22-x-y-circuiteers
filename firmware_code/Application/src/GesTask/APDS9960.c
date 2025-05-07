#include "APDS9960.h"
#include "I2cDriver/I2cDriver.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>

// Gesture state cache and tracking variables
static gesture_data_t gesture_cache;
static int delta_ud, delta_lr;        // Directional deltas (up-down, left-right)
static int count_ud, count_lr;        // Counted movement direction
static int count_near, count_far;     // For near/far detection
static int current_state, current_gesture;

static I2C_Data i2c_msg;              // I2C communication struct

#define LOOP_TIMEOUT 10               // Max retry loops for gesture read

// Forward declarations
static bool analyzeGestureData(void);
static bool classifyGesture(void);

/**
 * @fn      static bool write_apds9960(uint8_t reg, uint8_t val)
 * @brief   Writes a byte to the specified APDS9960 register.
 * @param   reg - Register address
 * @param   val - Value to write
 * @return  true if write successful, false otherwise
 */
static bool write_apds9960(uint8_t reg, uint8_t val) {
    uint8_t buffer[2] = { reg, val };
    i2c_msg.address = APDS9960_I2C_ADDR;
    i2c_msg.msgOut = buffer;
    i2c_msg.lenOut = 2;
    i2c_msg.msgIn = NULL;
    i2c_msg.lenIn = 0;
    return (I2cWriteDataWait(&i2c_msg, portMAX_DELAY) == 0);
}

/**
 * @fn      static bool read_apds9960(uint8_t reg, uint8_t *val)
 * @brief   Reads one byte from the specified APDS9960 register.
 * @param   reg - Register address
 * @param   val - Pointer to store read value
 * @return  true if read successful, false otherwise
 */
static bool read_apds9960(uint8_t reg, uint8_t *val) {
    i2c_msg.address = APDS9960_I2C_ADDR;
    i2c_msg.msgOut = &reg;
    i2c_msg.lenOut = 1;
    i2c_msg.msgIn = val;
    i2c_msg.lenIn = 1;
    return (I2cReadDataWait(&i2c_msg, 0, portMAX_DELAY) == 0);
}

/**
 * @fn      static int read_apds9960_block(uint8_t reg, uint8_t *buf, uint16_t len)
 * @brief   Reads multiple bytes from APDS9960 starting at a given register.
 * @param   reg - Starting register address
 * @param   buf - Buffer to hold read data
 * @param   len - Number of bytes to read
 * @return  Number of bytes read, or -1 on error
 */
static int read_apds9960_block(uint8_t reg, uint8_t *buf, uint16_t len) {
    i2c_msg.address = APDS9960_I2C_ADDR;
    i2c_msg.msgOut = &reg;
    i2c_msg.lenOut = 1;
    i2c_msg.msgIn = buf;
    i2c_msg.lenIn = len;
    return (I2cReadDataWait(&i2c_msg, 0, portMAX_DELAY) == 0) ? len : -1;
}

/**
 * @fn      static void reset_gesture_cache(void)
 * @brief   Clears all cached gesture data and resets related variables.
 */
static void reset_gesture_cache(void) {
    memset(&gesture_cache, 0, sizeof(gesture_cache));
    delta_ud = delta_lr = 0;
    count_ud = count_lr = 0;
    count_near = count_far = 0;
    current_state = 0;
    current_gesture = DIR_NONE;
}

/**
 * @fn      bool APDS9960_Init(void)
 * @brief   Initializes the APDS9960 gesture engine.
 * @details Verifies device ID, then configures gesture-related registers.
 * @return  true if initialization successful, false otherwise.
 */
bool APDS9960_Init(void) {
    uint8_t chip_id = 0;
    if (!read_apds9960(APDS9960_ID, &chip_id)) return false;
    if (!(chip_id == APDS9960_ID_1 || chip_id == APDS9960_ID_2)) return false;

    // Disable features before config
    if (!write_apds9960(APDS9960_ENABLE, 0x00)) return false;

    // Gesture engine timing and pulse configs
    if (!write_apds9960(APDS9960_ATIME, 219)) return false;           // ALS time
    if (!write_apds9960(APDS9960_WTIME, 246)) return false;           // Wait time
    if (!write_apds9960(APDS9960_PPULSE, 0x89)) return false;         // Proximity pulse
    if (!write_apds9960(APDS9960_GPULSE, 0xC9)) return false;         // Gesture pulse

    // Gesture mode config
    if (!write_apds9960(APDS9960_GCONF1, 0x40)) return false;         // FIFO threshold

    // Gain settings: Proximity gain (PGAIN) and ALS gain (AGAIN)
    if (!write_apds9960(APDS9960_CONTROL, (DEFAULT_PGAIN << 2) | DEFAULT_AGAIN)) return false;

    // Misc config: LED drive, proximity gain, etc.
    if (!write_apds9960(APDS9960_CONFIG2, 0b01000001)) return false;

    // Gesture mode: LED drive strength, gain, wait time
    if (!write_apds9960(APDS9960_GCONF2, (2 << 5) | (0 << 3) | 1)) return false;

    // Gesture proximity entry and exit thresholds
    if (!write_apds9960(APDS9960_GPENTH, 30)) return false;
    if (!write_apds9960(APDS9960_GEXTH, 20)) return false;

    // Enable Gesture, Proximity, Wait, and Power ON
    if (!write_apds9960(APDS9960_ENABLE, APDS9960_PON | APDS9960_WEN | APDS9960_PEN | APDS9960_GEN)) return false;

    reset_gesture_cache();
    return true;
}

/**
 * @fn      bool APDS9960_IsGestureAvailable(void)
 * @brief   Checks if new gesture data is available from the sensor.
 * @return  true if gesture data ready, false otherwise.
 */
bool APDS9960_IsGestureAvailable(void) {
    uint8_t stat = 0;
    if (!read_apds9960(APDS9960_GSTATUS, &stat)) return false;
    return (stat & APDS9960_GVALID) != 0;
}


/**
 * @fn      bool APDS9960_ReadGesture(int *gesture)
 * @brief   Reads and processes gesture data from the APDS9960 sensor.
 * @details Polls gesture FIFO data until complete, filters and stores in cache,
 *          then attempts to classify the gesture direction.
 * 
 * @param[out] gesture - Pointer to store resulting gesture direction (DIR_LEFT / DIR_RIGHT / DIR_NONE)
 * @return     true if a gesture is successfully classified, false otherwise.
 */
bool APDS9960_ReadGesture(int *gesture) {
    uint8_t fifo_lvl = 0;          // FIFO level (number of data sets in FIFO)
    uint8_t data_buf[128];         // Temporary buffer to read FIFO data
    uint8_t gstat = 0;             // Gesture status register
    int read_len = 0;              // Number of bytes read from FIFO
    int loop = 0;                  // Loop counter to prevent infinite polling

    // Reset gesture cache before collecting new data
    reset_gesture_cache();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(FIFO_PAUSE_TIME));  // Wait between polls

        // Read gesture status register to check for valid data
        if (!read_apds9960(APDS9960_GSTATUS, &gstat))
            return false;

        // Exit if gesture data no longer valid
        if ((gstat & APDS9960_GVALID) == 0)
            break;

        // Read FIFO level (how many sets of gesture data available)
        if (!read_apds9960(APDS9960_GFLVL, &fifo_lvl))
            return false;

        if (fifo_lvl > 0) {
            // Read FIFO data: each gesture set contains U, D, L, R bytes
            read_len = read_apds9960_block(APDS9960_GFIFO_U, data_buf, fifo_lvl * 4);
            if (read_len < 0)
                return false;

            // Process each 4-byte gesture data group
            for (int i = 0; i < read_len; i += 4) {
                // Skip invalid data (all 255s)
                if (data_buf[i] == 255 && data_buf[i + 1] == 255 &&
                    data_buf[i + 2] == 255 && data_buf[i + 3] == 255)
                    continue;

                gesture_cache.u_data[gesture_cache.index] = data_buf[i];
                gesture_cache.d_data[gesture_cache.index] = data_buf[i + 1];
                gesture_cache.l_data[gesture_cache.index] = data_buf[i + 2];
                gesture_cache.r_data[gesture_cache.index] = data_buf[i + 3];

                gesture_cache.index++;
                gesture_cache.total_gestures++;
            }
        }

        // Prevent infinite loop in case sensor keeps reporting GVALID
        if (++loop > 40) {
            *gesture = DIR_NONE;
            return false;
        }
    }

    // If no gesture data was captured, return failure
    if (gesture_cache.total_gestures == 0) {
        *gesture = DIR_NONE;
        return false;
    }

    // Analyze and classify gesture
    if (analyzeGestureData() && classifyGesture()) {
        *gesture = current_gesture;
    } else {
        *gesture = DIR_NONE;
    }

    return true;
}


/**
 * @fn      static bool analyzeGestureData(void)
 * @brief   Analyzes gesture sensor cached data to compute lateral movement (left vs. right).
 * @details Compares the difference in left/right sensor values from the start and end of the gesture.
 *          Uses this difference to compute a directional bias (left or right).
 * 
 * @return  true if enough data to analyze, false otherwise.
 */
static bool analyzeGestureData(void) {
    int l_start = 0, r_start = 0;  // Initial left/right values
    int l_end = 0, r_end = 0;      // Final left/right values

    // Require at least 5 samples to ensure reliable gesture
    if (gesture_cache.total_gestures <= 4) return false;

    // Find first valid frame from the start
    for (int i = 0; i < gesture_cache.total_gestures; i++) {
        if (gesture_cache.l_data[i] > GESTURE_THRESHOLD_OUT &&
            gesture_cache.r_data[i] > GESTURE_THRESHOLD_OUT) {
            l_start = gesture_cache.l_data[i];
            r_start = gesture_cache.r_data[i];
            break;
        }
    }

    // Find last valid frame from the end
    for (int i = gesture_cache.total_gestures - 1; i >= 0; i--) {
        if (gesture_cache.l_data[i] > GESTURE_THRESHOLD_OUT &&
            gesture_cache.r_data[i] > GESTURE_THRESHOLD_OUT) {
            l_end = gesture_cache.l_data[i];
            r_end = gesture_cache.r_data[i];
            break;
        }
    }

    // Compute the change in left-right difference over time
    int lr_diff = (l_end - r_end) - (l_start - r_start);

    // Accumulate difference into global delta_lr
    delta_lr += lr_diff;

    // Determine if gesture is strong enough to count as left/right
    count_lr = (delta_lr >= GESTURE_SENSITIVITY_1) ? 1 :
               (delta_lr <= -GESTURE_SENSITIVITY_1) ? -1 : 0;

    return true;
}


/**
 * @fn      static bool classifyGesture(void)
 * @brief   Classifies the gesture direction based on the accumulated left-right count.
 * @details Maps count_lr to gesture direction: left, right, or none.
 * 
 * @return  true if gesture is classified (left or right), false if no gesture.
 */
static bool classifyGesture(void) {
    if (count_lr == 1) {
        current_gesture = DIR_RIGHT;
    } else if (count_lr == -1) {
        current_gesture = DIR_LEFT;
    } else {
        current_gesture = DIR_NONE;
        return false;
    }
    return true;
}
