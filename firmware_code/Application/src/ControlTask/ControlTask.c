/**
 * @file    ControlTask.c
 * @brief   Handles robot motion control based on sensor inputs and state machine.
 *
 * Defines motion sequences and plays them using PCA9685 servo controller.
 * Reacts to environmental data (e.g., distance sensor, touch input) and
 * executes motions accordingly (e.g., walk, dance, push-up).
 *
 * Integrates with EnvSensorTask and AT42QT1010 touch sensor for reactive behaviors.
 */


#include "ControlTask.h"
#include "PCA9685.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SerialConsole.h"
#include "AT42QT1010.h"
#include "WifiHandlerThread/WifiHandler.h" 
#include "EnvTask/EnvSensorTask.h" 

// Standby
const int Standby[][9] = {
	{140, 90, 90, 40,  40, 90, 90, 140, 100}
};
// Forward 
const int Forward[][9] = {
	{140,90,90,40,40,90,90,140,50},
	{90,45,90,40,40,90,90,90,50},
	{140,45,90,40,40,90,90,140,50},
	{140,45,90,90,90,90,90,140,50},
	{140,90,90,90,90,135,45,140,50},
	{140,90,90,40,40,135,45,140,50},
	{90,90,90,40,40,135,90,90,50},
	{90,90,135,40,40,90,90,90,50},
	{140,90,135,40,40,90,90,140,50},
	{140,90,135,90,40,90,90,140,50},
	{140,90,90,40,40,90,90,140,150}
};
// Backward 
const int Backward[][9] = {
  {140, 90, 90, 40,  40, 90, 90, 140, 50},
  { 90, 90, 90, 40, 40, 90, 45, 90, 50 },
  { 140, 90, 90, 40, 40, 90, 45, 140, 50},
  { 140, 90, 90, 90, 90, 90, 45, 140, 50 },
  { 140, 45, 135, 90, 90, 90, 90, 140, 50 },
  { 140, 45, 135, 40, 40, 90, 90, 140, 50 },
  { 90, 90, 135, 40, 40, 90, 90, 90, 50 },
  { 90, 90, 90, 40, 40, 135, 90, 90, 50 },
  { 140, 90, 90, 40, 40, 135, 90, 140, 50 },
  { 140, 90, 90, 40, 90, 135, 90, 140, 50 },
  { 140, 90, 90, 40, 40, 90, 90, 140, 50 },
};
// LeftShift 
const int LeftShift[][9] = {
	{140,90,90,40,40,90,90,140,50},
	{90,90,90,40,40,90,90,90,50},
	{90,135,90,40,40,90,135,90,50},
	{140,135,90,40,40,90,135,140,50},
	{140,135,90,90,90,90,135,140,50},
	{140,135,135,90,90,135,135,140,50},
	{140,135,135,40,40,135,135,140,50},
	{140,90,90,40,40,90,90,140,50},
};

// RightShift 
const int RightShift[][9] = {
	{140,90,90,40,40,90,90,140,50},
	{90,90,90,40,40,90,90,90,50},
	{90,45,90,40,40,90,45,90,50},
	{140,45,90,40,40,90,45,140,50},
	{140,45,90,90,90,90,45,140,50},
	{140,45,45,90,90,45,45,140,50},
	{140,45,45,40,40,45,45,140,50},
	{140,90,90,40,40,90,90,140,50},
};

// Say Hi 
const int SayHi[][9] = {
	{140,90,90,90,90,90,90,90,100},
	{30,90,90,90,90,90,90,90,100},
	{30,130,90,90,90,90,90,90,100},
	{30,50,90,90,90,90,90,90,100},
	{30,130,90,90,90,90,90,90,100},
	{30,90,90,90,90,90,90,90,100},
	{140,90,90,90,90,90,90,90,100}
};

// Lie 
const int Lie[][9] = {
	{70,90,90,110,110,90,90,70,500}
};

// Fighting 
const int Fighting[][9] = {
	{110,90,90,40,70,90,90,140,200},
	{110,60,60,40,70,60,60,140,200},
	{110,120,120,40,60,120,120,140,200},
	{110,60,60,40,70,60,60,140,200},
	{110,120,120,40,60,120,120,140,200},
	{140,90,90,70,40,90,90,110,200},
	{140,60,60,70,40,60,60,110,200},
	{140,120,120,70,40,120,120,110,200},
	{140,60,60,70,40,60,60,120,200},
	{140,120,120,70,40,120,120,110,200},
	{140,90,90,70,40,90,90,110,200}
};

// PushUp
const int PushUp[][9] = {
	{140,90,90,40,40,90,90,140,300},
	{110,90,160,40,70,90,20,140,300},
	{140,90,160,40,40,90,20,140,300},
	{110,90,160,40,70,90,20,140,300},
	{140,90,160,40,40,90,20,140,300},
	{110,90,160,40,70,90,20,140,300},
	{140,90,160,40,40,90,20,140,500},
	{45,90,160,135,135,90,20,45,800},
	{140,90,160,135,120,90,20,45,200},
	{140,90,160,135,40,90,20,45,200},
	{140,90,160,40,40,90,20,140,200}
};

// Sleep 
const int Sleep[][9] = {
	{170,90,90,10,10,90,90,170,700},
	{170,45,135,10,10,135,45,170,700}
};

// Dance1
const int Dance1[][9] = {
	{170,90,90,10,60,90,90,110,50},
	{155,90,90,25,50,90,90,125,50},
	{140,90,90,40,40,90,90,140,50},
	{125,90,90,55,30,90,90,155,50},
	{110,90,90,50,20,90,90,170,50},
	{125,90,90,55,30,90,90,155,50},
	{140,90,90,40,40,90,90,140,50},
	{155,90,90,25,50,90,90,125,50},
	{170,90,90,10,60,90,90,110,50},
	{155,90,90,25,50,90,90,125,50},
	{140,90,90,40,40,90,90,140,50},
	{170,90,90,10,60,90,90,110,50},
	{155,90,90,25,50,90,90,125,50},
	{140,90,90,40,40,90,90,140,50},
	{125,90,90,55,30,90,90,155,50},
	{110,90,90,50,20,90,90,170,50},
	{125,90,90,55,30,90,90,155,50},
	{140,90,90,40,40,90,90,140,50},
	{155,90,90,25,50,90,90,125,50},
	{170,90,90,10,60,90,90,110,50},
	{155,90,90,25,50,90,90,125,50},
	{140,90,90,40,40,90,90,140,50},	

};

// Dance2
const int Dance2[][9] = {
	{140,45,135,40,40,135,45,140,200},
	{65,45,135,115,40,135,45,140,200},
	{140,45,135,40,115,135,45,65,200},
	{65,45,135,115,40,135,45,140,200},
	{140,45,135,40,115,135,45,65,200},
	{65,45,135,115,40,135,45,140,200},
	{140,45,135,40,115,135,45,65,200},
	{65,45,135,115,40,135,45,140,200},
	{140,45,135,65,40,135,45,140,200}
};

// Dance3
const int Dance3[][9] = {
	{140,45,135,40,40,135,45,140,200},
	{70,45,135,110,120,135,45,140,200},
	{140,45,135,40,40,135,45,140,200},
	{70,45,135,40,120,135,45,60,200},
	{140,45,135,40,40,135,45,140,200},
	{70,45,135,110,120,135,45,140,200},
	{140,45,135,40,40,135,45,140,200},
	{70,45,135,40,120,135,45,60,200},
	{140,45,135,40,40,135,45,140,200},
	{140,90,90,40,40,90,90,140,200}
};

// Global robot state variable
RobotState current_state = STATE_IDLE;


/**
 * @fn      void PlayMotion(const int motion[][9], int steps)
 * @brief   Play a predefined motion sequence by setting servo angles.
 * @details Each row in the motion array represents one step.
 *          The first 8 values are servo angles, and the 9th is the delay in ms.
 * 
 * @param   motion - 2D array of motion steps [step][8 servo angles + 1 delay]
 * @param   steps  - Number of steps in the motion
 * @return  None
 */
void PlayMotion(const int motion[][9], int steps) {
	for (int i = 0; i < steps; ++i) {
		for (int ch = 0; ch < 8; ++ch) {
			// Set each servo to its target angle for this step
			if (set_servo_angle(ch, motion[i][ch]) != 0) {
				// Optional: handle servo failure
			}
		}
		// Wait for the specified time before next step
		vTaskDelay(pdMS_TO_TICKS(motion[i][8]));
	}
}

/**
 * @fn      void ControlTask(void *pvParameters)
 * @brief   Main FreeRTOS task for controlling the robot's behavior.
 * @details Initializes peripherals, sets the robot to standby pose,
 *          and executes reactive behaviors based on sensors.
 * 
 * @param   pvParameters - Not used
 * @return  None (FreeRTOS task runs in an infinite loop)
 */
void ControlTask(void *pvParameters) {
	SerialConsoleWriteString("ControlTask started...\r\n");
	// Initialize PWM controller (e.g., PCA9685) and set frequency
	pca9685_init();
	PCA9685_SetPWMFreq(50);  // Set PWM frequency to 50Hz (standard for servos)

	// Initialize capacitive touch sensor
	AT42QT1010_Init();

	// Small delay to stabilize hardware
	vTaskDelay(pdMS_TO_TICKS(500));

	// Move all 8 servos to neutral (90 degrees)
	for (int ch = 0; ch < 8; ++ch) {
		set_servo_angle(ch, 90);
	}

	while (1) {
		// If an obstacle is detected too close
		if (!distance_safe) {
			SerialConsoleWriteString("Obstacle too close, direct Backward.\r\n");
			PlayMotion(Backward, sizeof(Backward)/sizeof(Backward[0]));

			current_state = STATE_IDLE;
			vTaskDelay(pdMS_TO_TICKS(500));
			continue;
		}

		// If the touch sensor is triggered
		if (AT42QT1010_IsTouched()) {
			SerialConsoleWriteString("Touch detected! Trigger Dance1.\r\n");
			PlayMotion(Dance1, sizeof(Dance1)/sizeof(Dance1[0]));
			vTaskDelay(pdMS_TO_TICKS(500));
			continue;
		}

		// State machine handling
		switch (current_state) {
			
			case STATE_IDLE:
			PlayMotion(Standby, sizeof(Standby)/sizeof(Standby[0]));
			break;
		
			case STATE_FORWARD:
			PlayMotion(Forward, sizeof(Forward)/sizeof(Forward[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_BACKWARD:
			PlayMotion(Backward, sizeof(Backward)/sizeof(Backward[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_LEFT_SHIFT:
			PlayMotion(LeftShift, sizeof(LeftShift)/sizeof(LeftShift[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_RIGHT_SHIFT:
			PlayMotion(RightShift, sizeof(RightShift)/sizeof(RightShift[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_SAY_HI:
			PlayMotion(SayHi, sizeof(SayHi)/sizeof(SayHi[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_LIE:
			PlayMotion(Lie, sizeof(Lie)/sizeof(Lie[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_FIGHTING:
			PlayMotion(Fighting, sizeof(Fighting)/sizeof(Fighting[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_PUSHUP:
			PlayMotion(PushUp, sizeof(PushUp)/sizeof(PushUp[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_SLEEP:
			PlayMotion(Sleep, sizeof(Sleep)/sizeof(Sleep[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_DANCE1:
			PlayMotion(Dance1, sizeof(Dance1)/sizeof(Dance1[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_DANCE2:
			PlayMotion(Dance2, sizeof(Dance2)/sizeof(Dance2[0]));
			current_state = STATE_IDLE;
			break;

			case STATE_DANCE3:
			PlayMotion(Dance3, sizeof(Dance3)/sizeof(Dance3[0]));
			current_state = STATE_IDLE;
			break;
		}

	}
}
