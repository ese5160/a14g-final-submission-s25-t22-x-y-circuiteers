#ifndef APDS9960_H_
#define APDS9960_H_

#include <stdint.h>
#include <stdbool.h>

/* I2C address */
#define APDS9960_I2C_ADDR  0x39

/* Acceptable device IDs */
#define APDS9960_ID_1      0xAB
#define APDS9960_ID_2      0x9C

/* Gesture Parameters */
#define GESTURE_THRESHOLD_OUT   10
#define GESTURE_SENSITIVITY_1   40
#define GESTURE_SENSITIVITY_2   20
#define FIFO_PAUSE_TIME         30

/* Register addresses */
#define APDS9960_ENABLE         0x80
#define APDS9960_ATIME          0x81
#define APDS9960_WTIME          0x83
#define APDS9960_PPULSE         0x8E
#define APDS9960_GPULSE         0xA6
#define APDS9960_GCONF1         0xA2
#define APDS9960_GCONF2         0xA3
#define APDS9960_GCONF3         0xAA
#define APDS9960_GCONF4         0xAB
#define APDS9960_GSTATUS        0xAF
#define APDS9960_GFLVL          0xAE
#define APDS9960_GFIFO_U        0xFC
#define APDS9960_ID             0x92
#define APDS9960_CONFIG2        0x90
#define APDS9960_CONTROL        0x8F
#define APDS9960_GPENTH         0xA0
#define APDS9960_GEXTH          0xA1

/* Bit fields */
#define APDS9960_PON            0b00000001
#define APDS9960_WEN            0b00001000
#define APDS9960_PEN            0b00000100
#define APDS9960_GEN            0b01000000
#define APDS9960_GVALID         0b00000001

/* Default values */
#define DEFAULT_PGAIN           2   // PGAIN_4X
#define DEFAULT_AGAIN           1   // AGAIN_4X
#define DEFAULT_GPENTH          30
#define DEFAULT_GEXTH           20
#define DEFAULT_GPULSE          0xC9  // 32us, 10 pulses

/* Direction Definitions */
#define DIR_NONE  0
#define DIR_LEFT  1
#define DIR_RIGHT 2
#define DIR_UP    3
#define DIR_DOWN  4
#define DIR_NEAR  5
#define DIR_FAR   6
#define DIR_ALL   7

/* Gesture data struct */
typedef struct {
	uint8_t u_data[32];
	uint8_t d_data[32];
	uint8_t l_data[32];
	uint8_t r_data[32];
	uint8_t index;
	uint8_t total_gestures;
} gesture_data_t;

/* Function Prototypes */
bool APDS9960_Init(void);
bool APDS9960_IsGestureAvailable(void);
bool APDS9960_ReadGesture(int *gesture);

#endif // APDS9960_H_
