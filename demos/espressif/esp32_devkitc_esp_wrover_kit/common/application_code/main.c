#include "FreeRTOS.h"
#include "task.h"

#include "aws_logging_task.h"
#include "aws_system_init.h"



#include "task_config.h"
#include "flags.h"
#include "nvs_storage.h"
#include "spiffs_storage.h"
#include "authentication.h"
#include "aws_dev_mode_key_provisioning.h"
#include "wifi_config.h"
#include "mqtt_config.h"
#include "gpio_handler.h"
#include "rtc_config.h"
//#include "ota_client.h"
#include "analog_handler.h"
#include "queue_conf.h"
#include "ntp.h"

/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 32 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 6 )
#define mainDEVICE_NICK_NAME                "AcuaMatic"

int app_main( void ){
    xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
							tskIDLE_PRIORITY + 5,
							mainLOGGING_MESSAGE_QUEUE_LENGTH );
    
    if(SYSTEM_Init() == pdPASS){
        flags_init();
        //spiffs_storage_init();
        nvs_storage_init();    
        authentication_init();
        queue_conf_init();
        mqtt_config_init();
        rtc_config_init();
        wifi_config_init();
        //ota_client_init();
        gpio_handler_init();
        //analog_handler_init();
        ntp_init();

        WIFIReturnCode_t xWifiStatus = wifi_config_start_driver();

        if(xWifiStatus == eWiFiSuccess){
            ( void ) xTaskCreate( wifi_config_task,
                                TASK_WIFI_NAME,
                                TASK_WIFI_STACK_SIZE,
                                NULL,
                                TASK_WIFI_PRIORITY,
                                NULL );
        
            
            ( void ) xTaskCreate( mqtt_config_task,
                                TASK_MQTT_SUBS_NAME,
                                TASK_MQTT_SUBS_STACK_SIZE,
                                NULL,
                                TASK_MQTT_SUBS_PRIORITY,
                                NULL );               
            
        } 
        
         
    }
    else{
        printf("SYSTEM_Init Fail\n");
    }
    
    return 0;
}
