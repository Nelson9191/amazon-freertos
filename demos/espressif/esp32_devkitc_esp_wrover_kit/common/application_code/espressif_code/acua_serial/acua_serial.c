#include "acua_serial.h"
#include "queue_conf.h"

#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"

#define ECHO_TEST_TXD  GPIO_NUM_10
#define ECHO_TEST_RXD  GPIO_NUM_9
#define ECHO_TEST_RTS  UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE 1024

static uint8_t data_buffer[BUF_SIZE];
QueueHandle_t at_queue;

uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
void acua_serial_init(){
    
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);


    //data_buffer = (uint8_t *) malloc(BUF_SIZE);

    ( void ) xTaskCreate( acua_serial_task,
                        "serial",
                        800,
                        NULL,
                        2,
                        NULL );


    ( void ) xTaskCreate( prueba,
                        "serial2",
                        600,
                        NULL,
                        2,
                        NULL );              
}

void acua_serial_task(void * pvParameters){
    struct ATMsg msg;
    int len;
    int ctr = 0;
    uart_write_bytes(UART_NUM_1, (const char *) "AT+CGATT?\r\n", 11);  
    bool b = false;

    for(;;){

        uart_get_buffered_data_len(UART_NUM_1, (size_t *)&len);
        //len = 0;

        if(len > 0){
            len = uart_read_bytes(UART_NUM_1, data_buffer, BUF_SIZE, 20);
            //uart_flush(UART_NUM_1);
        }

        if(len > 0){
            msg.buf = (char *)data_buffer;
            msg.len = len;
            //queue_conf_send_AT(msg);
            printf("* %.*s\n", len, data_buffer); 
        }

            if(b){
                b = false;
                uart_write_bytes(UART_NUM_1, (const char *) "AT+CGATT?\r\n", 11);  
            }
            else {
                b = true;
                uart_write_bytes(UART_NUM_1, (const char *) "AT\r\n", 4);  
            }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void prueba(void * pvParameters){
    (void) pvParameters;
    struct ATMsg at_msg;

    for(;;){
        while(xQueueReceive(at_queue, &at_msg, NULL) != errQUEUE_EMPTY){
            //printf("* %.*s\n", at_msg.len, at_msg.buf); 
            printf("hola\n");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
