#ifndef _TASK_CONFIG_H_
#define _TASK_CONFIG_H_

#define TASK_WIFI_NAME              "WiFi"
#define TASK_WIFI_STACK_SIZE        2000
#define TASK_WIFI_PRIORITY          5

#define TASK_GPIO_READ_NAME              "GPIO_READ"
#define TASK_GPIO_READ_STACK_SIZE        3500
#define TASK_READ_GPIO_PRIORITY          3

#define TASK_GPIO_WRITE_NAME              "GPIO_WRITE"
#define TASK_GPIO_WRITE_STACK_SIZE        3500
#define TASK_WRITE_GPIO_PRIORITY          2

#define TASK_MQTT_SUBS_NAME          "MQTT Subscribe"
#define TASK_MQTT_SUBS_STACK_SIZE    3500
#define TASK_MQTT_SUBS_PRIORITY      2

#define TASK_ANALOG_NAME            "Analog Read"
#define TASK_ANALOG_STACK_SIZE      3500
#define TASK_ANALOG_PRIORITY        4

#define TASK_NTP_NAME            "NTP"
#define TASK_NTP_STACK_SIZE      1000
#define TASK_NTP_PRIORITY        2

#define TASK_MONITOR_NAME            "Memory Monitor"
#define TASK_MONITOR_STACK_SIZE      2000
#define TASK_MONITOR_PRIORITY        1



#endif