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
        //printf("Monitor Task -- bytes not used: %u\n", uxTaskGetStackHighWaterMark(NULL)*4);
        printf("Free memory: %u\n", xPortGetFreeHeapSize());
        printf("minn %d\n", configMINIMAL_STACK_SIZE);
        vTaskDelay(40000 / portTICK_PERIOD_MS);  
    }
}
