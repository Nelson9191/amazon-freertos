#include "am_ultrasonic.h"
#include "_am_ultrasonic.h"
#include "task_config.h"

#include "gpio_handler.h";
#include "queue_conf.h"
#include "rtc_config.h"


#define TRIGGER_LOW_DELAY 4
#define TRIGGER_HIGH_DELAY 10
#define PING_TIMEOUT 6000
#define ROUNDTRIP 58
#define MAX_DISTANCE_CM     450
#define LEVEL_CMP_RANGE     20

#define TRIGGER_PIN     22 
#define ECHO_PIN        23
#define BLINK_PIN       21

#define READ_PERIOD_MS  30 * 1000        


void am_ultrasonic_init(){
    gpio_handler_write(TRIGGER_PIN, 0);
    
    ( void ) xTaskCreate( _am_ultrasonic_task,
                        TASK_ULTRASONIC_NAME,
                        TASK_ULTRASONIC_STACK_SIZE,
                        NULL,
                        TASK_ULTRASONIC_PRIORITY,
                        NULL );    
}

static bool _am_ultrasonic_measure(int * distance_cm){
    int64_t echo_start = 0;
    int64_t echo_finish = 0;
    int64_t max_timeout  = 0;
    bool time_exired = false;

    *distance_cm = -1;
     // Ping: Low for 2..4 us, then high 10 us
    gpio_handler_write(TRIGGER_PIN, 0);
    ets_delay_us(TRIGGER_LOW_DELAY);
    gpio_handler_write(TRIGGER_PIN, 1);
    ets_delay_us(TRIGGER_HIGH_DELAY);
    gpio_handler_write(TRIGGER_PIN, 0);

    // Previous ping isn't ended
    if (gpio_handler_read(ECHO_PIN))
    {
        return false;
    }

    echo_start = esp_timer_get_time();
    // Wait for echo
    while (!gpio_get_level(ECHO_PIN))
    {
        echo_finish = esp_timer_get_time();

        if (_am_ultrasonic_time_expired(echo_start, PING_TIMEOUT))
        {
            return false;
        }
    }

    //start to measure pulse width
    echo_start = esp_timer_get_time();
    echo_finish = echo_start;
    max_timeout = echo_start + MAX_DISTANCE_CM * ROUNDTRIP;

    while (gpio_get_level(ECHO_PIN))
    {
        echo_finish = esp_timer_get_time();
        if (_am_ultrasonic_time_expired(echo_start, max_timeout))
        {
            return false;
        }
    }



    if (time_exired){
        return -1;
    }
    else
    {
        return (echo_finish - echo_start) / ROUNDTRIP;
    }
    
    *distance_cm = (echo_finish - echo_start) / ROUNDTRIP;
    return true;
}

static inline bool _am_ultrasonic_time_expired(int64_t start_time, int64_t ping_timeout){
    return esp_timer_get_time() - start_time >= ping_timeout;
}

static void _am_ultrasonic_task(void * pvParameters){
    int reported_level = 0;
    int curr_level = 0;
    for (;;){
        if (_am_ultrasonic_measure(&curr_level)){
            if (_am_ultrasonic_compare(LEVEL_CMP_RANGE, reported_level, curr_level)){
                reported_level = curr_level;
                _am_ultrasonic_report(curr_level);
            }
        }

        vTaskDelay(READ_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

static void _am_ultrasonic_report(int level){
    struct MqttMsg mqtt_msg;

    snprintf(mqtt_msg.name, 10, "%s", "Nivel");
    mqtt_msg.status = level;
    mqtt_msg.timestamp = rtc_config_get_time();
    queue_conf_send_mqtt(mqtt_msg);
}

static bool _am_ultrasonic_compare(int range, int past_val, int cur_val){
    if(cur_val >= past_val){
        return cur_val - past_val > range;
    }
    else{
        return past_val - cur_val > range;
    }
}
