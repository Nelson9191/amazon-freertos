#include "acua_serial.h"
#include "uart.h"

#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"

#define ECHO_TEST_TXD  GPIO_NUM_4
#define ECHO_TEST_RXD  GPIO_NUM_5
#define ECHO_TEST_RTS  UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE 1024

static uint8_t * data_buffer;

uart_config_t uart_config = {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
void acua_serial_init(){
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    data_buffer = (uint8_t *) malloc(BUF_SIZE);

    ( void ) xTaskCreate( acua_serial_task,
                        "serial",
                        500,
                        NULL,
                        2,
                        NULL );
}

void acua_serial_task(void * pvParameters){
    for(;;){
        int len = uart_read_bytes(UART_NUM_1, data_buffer, BUF_SIZE, 20 / portTICK_RATE_MS);

        if(len > 0){
            printf("** %.*s\n", len, data_buffer);
        }
        // Write data back to the UART
        //uart_write_bytes(UART_NUM_1, (const char *) "AT", 2);  

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf(".\n");
    }
}
