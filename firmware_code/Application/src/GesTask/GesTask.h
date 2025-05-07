#ifndef GES_TASK_H_
#define GES_TASK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus


extern "C" {
	#endif
extern volatile bool gestureEnabled;

	void GesTask(void *pvParameters);

	#ifdef __cplusplus
}
#endif

#endif // GES_TASK_H_
