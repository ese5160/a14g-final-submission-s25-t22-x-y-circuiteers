#ifndef AT42QT1010_H_
#define AT42QT1010_H_

#include <stdbool.h>

#define TOUCH_PIN PIN_PA10

void AT42QT1010_Init(void);
bool AT42QT1010_IsTouched(void);

#endif /* AT42QT1010_H_ */
