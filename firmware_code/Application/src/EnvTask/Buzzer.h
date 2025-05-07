#ifndef BUZZER_PWM_H_
#define BUZZER_PWM_H_

#define BUZZER_PIN PIN_PB03

void BuzzerPWM_Init(void);   
void BuzzerPWM_Start(void);  
void BuzzerPWM_Stop(void);   
void buzzer_gpio_init(void); 

#endif
