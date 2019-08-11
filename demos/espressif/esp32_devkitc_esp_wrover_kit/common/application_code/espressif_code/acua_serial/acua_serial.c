#include "acua_serial.h"
#include "at_commands.h"
#include "queue_conf.h"

#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"

#define ECHO_TEST_TXD  GPIO_NUM_10
#define ECHO_TEST_RXD  GPIO_NUM_9
#define ECHO_TEST_RTS  UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE 1024
#define TICKS_TO_WAIT   20

#define SHORT_DELAY   2000
#define LONG_DELAY    20000

uint8_t data_buffer[BUF_SIZE];
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
                        2000,
                        NULL,
                        2,
                        NULL );           
}

void acua_serial_task(void * pvParameters){
    struct ATMsg msg;
    int len;
    int ctr = 0;

    acua_serial_verify_status();
    for(;;){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

bool acua_serial_verify_status(){
    int len = -1;
    bool ok = true;
    enum eGPRSStatus response;
    
    response = acua_serial_send_command(AT, AT_OK, SHORT_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    //Valida estado de GPRS
    do{
        response = acua_serial_send_command(CREG, CREG_OK, SHORT_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);

    //Valida estado de GPRS
    do{
        response = acua_serial_send_command(CGREG, CGREG_OK, SHORT_DELAY);
        vTaskDelay(3000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);

/*
    response = acua_serial_send_command(COPS, AT_OK, SHORT_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_serial_send_command(COPS_LIST, AT_OK, LONG_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS); */
    
    response = !acua_serial_send_command(CSQ, CSQ_ERROR, SHORT_DELAY); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    response = acua_serial_send_command(CGATT, CGATT_OK, LONG_DELAY); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);      
    response = acua_serial_send_command(CGATT_ACTIVATE, AT_OK, LONG_DELAY); 
    vTaskDelay(9000 / portTICK_PERIOD_MS);    
    response = acua_serial_send_command(CGATT, CGATT_OK, LONG_DELAY); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
    response = acua_serial_send_command(SET_CGDCONT, AT_OK, SHORT_DELAY); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    
    response = acua_serial_send_command(CGDCONT_LIST, CGDCONT_OK, SHORT_DELAY); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_serial_send_command(CGACT_LIST, CGACT_LIST_OK, SHORT_DELAY); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    response = acua_serial_send_command(SET_CGACT, CGACT_OK, LONG_DELAY); 
    vTaskDelay(9000 / portTICK_PERIOD_MS);  


    response = acua_serial_send_command(CGPADDR, CGPADDR_FAIL, SHORT_DELAY);
    vTaskDelay(9000 / portTICK_PERIOD_MS);    

    return ok;
}

size_t acua_serial_AT_len(const char * AT_command){
    return strlen(AT_command) + 2;
}

enum eGPRSStatus acua_serial_write(const char * msg){
    int len;
    char * buff = malloc(sizeof(char) * 2000);
    if (buff == NULL){
        printf("MEM error\n");
        return GPRS_ERROR;
    }
    memset(buff, '\0', sizeof(char) * 2000);
    snprintf(buff, 2000, "%s\r\n", msg);

    len = uart_write_bytes(UART_NUM_1, buff, strlen(buff));
    free(buff);
    printf("%s >> written: %d\n", msg, len);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    return len >= 0 ? OK : GPRS_HARDWARE_ERROR;
}

enum eGPRSStatus acua_serial_recv(bool loop){
    int ctr = 0;
    int len;

    memset(data_buffer, '\0', BUF_SIZE);
    while(!acua_serial_response_available()){
        if(ctr++ > 100){
            printf("NO response\n");
            if(loop){
                ctr = 0;
            }
            else
            {
                return GPRS_NO_RESPONSE;
            }
            
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    len = uart_read_bytes(UART_NUM_1, data_buffer, BUF_SIZE, TICKS_TO_WAIT);
    if(len < 0){
        return GPRS_HARDWARE_ERROR;
    }
    else
    {
        printf("--  %.*s [%d]\n", (len > 2 ? len - 2 : len), data_buffer, len);
        return GPRS_OK;
    }
    
    
}

enum eGPRSStatus acua_serial_send_command(const char * command, const char * validation_response, int wait_ms){
    uart_flush_input(UART_NUM_1);
    if(acua_serial_write(command) == GPRS_ERROR){
        return GPRS_HARDWARE_ERROR;
    }

    vTaskDelay(wait_ms / portTICK_PERIOD_MS);

    if (acua_serial_recv(false) == GPRS_NO_RESPONSE){
        return GPRS_HARDWARE_ERROR;
    }

    return acua_serial_verify_ok((const char *)data_buffer, validation_response) ? GPRS_OK : GPRS_ERROR;
}

bool acua_serial_verify_ok(const char * resp, const char * pattern_ok){
    return strstr(resp, pattern_ok) != NULL ? true : false;
}

bool acua_serial_response_available(){
    int len = 0;
    int er;
    er = uart_get_buffered_data_len(UART_NUM_1, (size_t *)&len);
    if(er < 0){
        printf("error buf\n");
    }
    return len > 0;
}