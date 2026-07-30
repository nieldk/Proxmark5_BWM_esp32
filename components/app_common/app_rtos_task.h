#ifndef APP_RTOS_TASK_
#define APP_RTOS_TASK_

#include "freertos/FreeRTOS.h"

void wait_for_rtos_task_exit(int timeout_ms, TaskHandle_t *task_handle);

#endif
