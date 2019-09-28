#include "memory_monitor.h"
#include "task_config.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void memoryMonitorInit(){
    ( void ) xTaskCreate( memoryMonitorTask,
                            TASK_MONITOR_NAME,
                            TASK_MONITOR_STACK_SIZE,
                            NULL,
                            TASK_MONITOR_PRIORITY,
                            NULL );
}

void memoryMonitorTask(void * pvParameters){
    for(;;){
        //printf("Free memory: \n");
        printf("Free memory: %u\n", xPortGetFreeHeapSize());
        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}
