#ifndef US100_H_
#define US100_H_

#include <stdint.h>

#define TRIG_PIN  PIN_PA06
#define ECHO_PIN  PIN_PA05
#define TIMER_TC  TC4

void Ultrasonic_Init(void);
void Ultrasonic_Trigger(void);
int32_t Ultrasonic_GetDistanceCM(void);

#endif
