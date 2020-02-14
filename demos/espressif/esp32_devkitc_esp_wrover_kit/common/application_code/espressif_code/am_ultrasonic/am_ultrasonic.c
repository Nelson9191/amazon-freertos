#include "am_ultrasonic.h"
#include "_am_ultrasonic.h"
#include "task_config.h"
#include "am_level_info.h"

#include "gpio_handler.h";
#include "queue_conf.h"
#include "rtc_config.h"
#include "gpio_info.h"


#define TRIGGER_LOW_DELAY 4
#define TRIGGER_HIGH_DELAY 10
#define PING_TIMEOUT 6000
#define ROUNDTRIP 583
#define MAX_DISTANCE_CM     450

#define BLINK_PIN       21


#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_ENTER_CRITICAL portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL portEXIT_CRITICAL(&mux)
#define RETURN_CRITICAL(RES) do { PORT_EXIT_CRITICAL; return RES; } while(0)

void am_ultrasonic_init(){
    gpio_handler_write(ULTRASONIC_TRIGGER, 1);
    
    ( void ) xTaskCreate( _am_ultrasonic_task,
                        TASK_ULTRASONIC_NAME,
                        TASK_ULTRASONIC_STACK_SIZE,
                        NULL,
                        TASK_ULTRASONIC_PRIORITY,
                        NULL );    
}

static bool _am_ultrasonic_measure(int * distance_cm){
    static bool blink;
    int64_t echo_start = 0;
    int64_t echo_finish = 0;
    int64_t max_timeout  = 0;
    bool ok = false;

    *distance_cm = -1;

    if (blink){
        blink = false;
        gpio_handler_write(ULTRASONIC_BLINK, 1);
    }
    else{
        blink = true;
        gpio_handler_write(ULTRASONIC_BLINK, 0);
    }

    PORT_ENTER_CRITICAL;

     // Ping: Low for 2..4 us, then high 10 us
    gpio_handler_write(ULTRASONIC_TRIGGER, 0);
    ets_delay_us(TRIGGER_LOW_DELAY);
    gpio_handler_write(ULTRASONIC_TRIGGER, 1);
    ets_delay_us(TRIGGER_HIGH_DELAY);
    gpio_handler_write(ULTRASONIC_TRIGGER, 0);

    // Previous ping isn't ended
    if (gpio_handler_read(ULTRASONIC_ECHO))
    {
        goto end_measure;
    }

    echo_start = esp_timer_get_time();
    // Wait for echo
    while (!gpio_get_level(ULTRASONIC_ECHO))
    {
        
        if (timeout_expired(echo_start, PING_TIMEOUT*100))
        {
            echo_start = esp_timer_get_time();
            goto end_measure;
        } 
    }

    //start to measure pulse width
    echo_start = esp_timer_get_time();
    echo_finish = echo_start;
    max_timeout = echo_start + MAX_DISTANCE_CM * ROUNDTRIP / 10;

    while (gpio_get_level(ULTRASONIC_ECHO))
    {
        echo_finish = esp_timer_get_time();
        if (timeout_expired(echo_start, max_timeout))
        {
            goto end_measure;
        }
    }

    ok = true;

end_measure:

    PORT_EXIT_CRITICAL;
    *distance_cm = (echo_finish - echo_start) * 100 / ROUNDTRIP;
    return ok;
}

static bool _am_ultrasonic_time_expired(int64_t start_time, int64_t ping_timeout){
    return (esp_timer_get_time() - start_time) >= ping_timeout;
}

static void _am_ultrasonic_task(void * pvParameters){
    bool b = false;
    int reported_level = 0;
    int curr_level = 0;
    printf("ULTRASONIC created\n");
    for (;;){
        if (_am_ultrasonic_measure(&curr_level)){
            printf("level: %d mm\n", curr_level);
            if (_am_ultrasonic_compare(LEVEL_CMP_RANGE_MM, reported_level, curr_level)){
                reported_level = curr_level;
                _am_ultrasonic_report(curr_level);
            }
        }

        vTaskDelay(READ_PERIOD_S / portTICK_PERIOD_MS);
    }
}

static void _am_ultrasonic_report(int level){
    struct MqttMsg mqtt_msg;

    snprintf(mqtt_msg.name, 10, "%s", "AI01");
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
