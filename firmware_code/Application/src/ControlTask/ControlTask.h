/**
 * @file    ControlTask.h
 * @brief   Declarations for robot motion control and state management.
 *
 * Defines the RobotState enum and exposes motion sequences and the ControlTask function.
 * Used to coordinate servo-based motions like walking, dancing, and reacting to sensor input.
 */


#ifndef CONTROL_TASK_H
#define CONTROL_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
	#endif
	#define PCA9685_FREQ        50
	typedef enum {
		STATE_IDLE,
		STATE_FORWARD,
		STATE_BACKWARD,
		STATE_LEFT_SHIFT,
		STATE_RIGHT_SHIFT,
		STATE_SAY_HI,
		STATE_LIE,
		STATE_FIGHTING,
		STATE_PUSHUP,
		STATE_SLEEP,
		STATE_DANCE1,
		STATE_DANCE2,
		STATE_DANCE3
	} RobotState;

	extern RobotState current_state;
	extern const int Forward[][9];
	extern const int Backward[][9];
	extern const int LeftShift[][9];
	extern const int RightShift[][9];
	extern const int SayHi[][9];
	extern const int Lie[][9];
	extern const int Fighting[][9];
	extern const int PushUp[][9];
	extern const int Sleep[][9];
	extern const int Dance1[][9];
	extern const int Dance2[][9];
	extern const int Dance3[][9];
	extern const int Standby[][9];

	void ControlTask(void *pvParameters);


#ifdef __cplusplus
}
#endif

#endif // CONTROL_TASK_H
