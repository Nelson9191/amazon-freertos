#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "string.h"
//#include "uart.h"
//#include "utils.h"

#include "am_level.h"

#define LEVEL_TXD  GPIO_NUM_17
#define LEVEL_RXD  GPIO_NUM_16

#define TICKS_TO_WAIT   20
#define BUF_SIZE        2000
uint8_t data_buffer[BUF_SIZE];

uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

static int reported_level;


void am_level_init(){
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, LEVEL_TXD, LEVEL_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);
}

bool am_level_read()
{
    if (am_level_is_response_available() && am_level_recv())
    {
        int curr_level = am_level_get_lvl_from_buffer();
        if (curr_level < 1)
            return false;

        if (am_level_compare(LEVEL_RANGE, reported_level, curr_level)){
            reported_level = curr_level;

            snprintf(mqtt_msg.name, 10, "%s", "Nivel");
            mqtt_msg.status = reported_level;
            mqtt_msg.timestamp = rtc_config_get_time();
            queue_conf_send_mqtt(mqtt_msg);
        }
    }
}

static inline bool am_level_is_response_available(){
    size_t len = 0;
    if(uart_get_buffered_data_len(UART_NUM_1, &len) < 0){
        return false;
    }
    else{
        return len > 2;
    }
}

static bool am_level_recv(bool waitForResponse){
    memset(data_buffer, '\0', BUF_SIZE);

    int len = uart_read_bytes(UART_NUM_1, data_buffer, BUF_SIZE, TICKS_TO_WAIT);
    if(len <= 0){
        return false;
    }
    else
    {
        printf("--  %.*s [%d]\n", (len > 2 ? len - 2 : len), data_buffer, len);
        return true;
    }
}

static bool am_level_compare(uint32_t range, uint32_t past_val, uint32_t cur_val){
    if(cur_val >= past_val){
        return cur_val - past_val > range;
    }
    else{
        return past_val - cur_val > range;
    }
}

static int am_level_get_lvl_from_buffer(const char * buffer){
    char * ptr1, ptr2;
    int level_cm = 0;
    
    ptr1 = strstr(buffer, "_*_");
    if (ptr1){
        ptr1 += 3;
        ptr2 = strstr(ptr1 + 1, "_*_");
    }

    // valida que la trama está bien formada, y que la distancia medida sea menor a 1000
    if (ptr1 && ptr2 && ptr2 - ptr1 < 4){
        *ptr2 = '\0';
        if(sscanf(ptr1, "%d", &level_cm) > 0)
        {
            return level_cm;
        }
    }

    return -1;
}