#include "acua_gprs.h"
#include "clientcredential_keys.h"
#include "at_commands.h"
#include "mqtt_info.h"
#include "queue_conf.h"
#include "gpio_info.h"

#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"

#define ECHO_TEST_TXD  GPIO_NUM_10
#define ECHO_TEST_RXD  GPIO_NUM_9
#define ECHO_TEST_RTS  UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE 2000
#define TICKS_TO_WAIT   20

#define SHORT_DELAY   2000
#define LONG_DELAY    15000
#define TOO_LONG_DELAY  65000

uint8_t data_buffer[BUF_SIZE];
QueueHandle_t at_queue;

 char rxbuf[100];

//handle for the interrupt
static intr_handle_t handle_console;

extern int daniel;

extern QueueHandle_t mqttSubsQueue;


uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

bool acua_gprs_init(){
    enum eGPRSStatus response;

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);


    //Time for the GPRS to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    queue_conf_send_gpio(GPIO_ON_GPRS, 1);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    queue_conf_send_gpio(GPIO_ON_GPRS, 0);

    vTaskDelay(15000 / portTICK_PERIOD_MS);


    do{
        //uart_flush(UART_NUM_1);
        response = acua_gprs_send_command(CPIN, CPIN_OK, SHORT_DELAY, false, true);
        vTaskDelay(2000 / portTICK_PERIOD_MS);    
    } while(response != GPRS_OK);

    bool ok = acua_gprs_validate_certs();

/*     ok ? printf("CERTS OK \n") : printf("CERTS ERROR\n");

    while(1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);

    } */

    if(!ok){
        ok = true;
        acua_gprs_delete_files();

        ok &= acua_gprs_write_client_cert();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ok &= acua_gprs_write_client_key();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ok &= acua_gprs_write_ca_cert();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ok &= acua_gprs_validate_certs();
    }

    ok &= acua_gprs_configure_ssl();

    return ok;
}

bool acua_gprs_config_network(){
    bool ok = acua_gprs_verify_status();
    return ok;
}

void acua_gprs_task(void * pvParameters){
    struct ATMsg msg;
    int len;
    int ctr = 0;
    bool ok;

    for(;;){

        /*
        if(ok){
            acua_gprs_publish("hola");
        }
        else{
            acua_gprs_disconnect_and_release();
        }
        */
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

bool acua_gprs_verify_status(){
    int len = -1;
    bool ok = true;
    enum eGPRSStatus response;
    
    response = acua_gprs_send_command(AT, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    //Valida estado de GPRS
    do{
        response = acua_gprs_send_command(CREG, CREG_OK, SHORT_DELAY, false, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);

    //Valida estado de GPRS
    do{
        response = acua_gprs_send_command(CGREG, CGREG_OK, SHORT_DELAY, false, true);
        vTaskDelay(3000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);
    
    response = acua_gprs_send_command(CSQ, CSQ_ERROR, SHORT_DELAY, false, false); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
     
    response = acua_gprs_send_command(CGATT_ACTIVATE, AT_OK, LONG_DELAY, false, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_gprs_send_command(CGATT, CGATT_OK, SHORT_DELAY, false, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  

    response = acua_gprs_send_command(SET_CGDCONT, AT_OK, SHORT_DELAY, false, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_gprs_send_command(CGDCONT_LIST, CGDCONT_OK, SHORT_DELAY, false, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_gprs_send_command(CGACT_LIST, CGACT_LIST_OK, SHORT_DELAY, false, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    response = acua_gprs_send_command(SET_CGACT, CGACT_OK, LONG_DELAY, false, true); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);  

    //response = acua_gprs_send_command(SOCK, AT_OK, LONG_DELAY, false);
    //vTaskDelay(2000 / portTICK_PERIOD_MS); 

    //IP address
    response = acua_gprs_send_command(CGPADDR, CGPADDR_FAIL, SHORT_DELAY, false, false);

    return response == GPRS_OK;
}

size_t acua_gprs_AT_len(const char * AT_command){
    return strlen(AT_command) + 2;
}

enum eGPRSStatus acua_gprs_write(const char * msg){
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
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return len >= 0 ? OK : GPRS_HARDWARE_ERROR;
}

bool acua_gprs_send_file(const char * msg){
    int len = uart_write_bytes(UART_NUM_1, msg, strlen(msg));
    bool ok;
    printf("%s >> written: %d\n", msg, len);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if (acua_gprs_recv(true) == GPRS_NO_RESPONSE){
        return false;
    }

    return acua_gprs_verify_ok((const char *)data_buffer, AT_OK);  
}


enum eGPRSStatus acua_gprs_recv(bool waitForResponse){
    int ctr = 0;
    int len;

    memset(data_buffer, '\0', BUF_SIZE);

    if(waitForResponse){
        while(!acua_gprs_response_available() && ++ctr < 100){
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    
    if(!acua_gprs_response_available() && waitForResponse){
        printf("NO response\n");
        return GPRS_NO_RESPONSE;
    }
    else if(!acua_gprs_response_available())
    {
        return GPRS_NO_RESPONSE;
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

enum eGPRSStatus acua_gprs_send_command(const char * command, const char * validation_response, int wait_ms, bool force_wait, bool validate_ok){
    int ctr = 0;

    if(acua_gprs_write(command) == GPRS_ERROR){
        printf("HW error\n");
        return GPRS_HARDWARE_ERROR;
    }

    if(force_wait){
        vTaskDelay(wait_ms / portTICK_PERIOD_MS);
    }
    else
    {
        while(!acua_gprs_response_available() && (ctr ++ < wait_ms)){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    

    if(!acua_gprs_response_available()){
        return GPRS_HARDWARE_ERROR;
    }

    if (acua_gprs_recv(true) == GPRS_NO_RESPONSE){
        printf("HW error2\n");
        return GPRS_HARDWARE_ERROR;
    }

    if(validate_ok){
        return acua_gprs_verify_ok((const char *)data_buffer, validation_response) ? GPRS_OK : GPRS_ERROR;
    }
    else{
        return acua_gprs_verify_ok((const char *)data_buffer, validation_response) ? GPRS_ERROR : GPRS_OK ;
    }
}

bool acua_gprs_verify_ok(const char * resp, const char * pattern_ok){
    return strstr(resp, pattern_ok) != NULL ? true : false;
}

bool acua_gprs_response_available(){
    int len = 0;
    int er;
    er = uart_get_buffered_data_len(UART_NUM_1, (size_t *)&len);
    if(er < 0){
        printf("error buf\n");
    }
    return len > 2;
}

bool acua_gprs_validate_certs(){
    bool ok = true;
    enum eGPRSStatus response;
    
    response = acua_gprs_send_command(CERT_LIST, CLIENT_CERT, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ok &= response == GPRS_OK;

    response = acua_gprs_send_command(CERT_LIST, CLIENT_KEY, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ok &= response == GPRS_OK;

    response = acua_gprs_send_command(CERT_LIST, CA_CERT, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ok &= response == GPRS_OK;    

    return ok;
}

void acua_gprs_save_certs(){

}

bool acua_gprs_write_client_cert(){
    bool ok;
    char * buff = malloc(sizeof(char) * 2000);
    if (buff == NULL){
        printf("MEM error\n");
        return GPRS_ERROR;
    }

    memset(buff, '\0', sizeof(char) * 2000);
    snprintf(buff, 2000, "%s\"%s\",%d", WRITE_CERT_TEMPLATE, CLIENT_CERT, strlen(keyCLIENT_CERTIFICATE_PEM));

    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    free(buff);

    if(!ok){
        return false;
    }

    ok = acua_gprs_send_file((const char *)keyCLIENT_CERTIFICATE_PEM);

    return ok;        
}

bool acua_gprs_write_client_key(){
    bool ok;
    char * buff = malloc(sizeof(char) * 2000);
    if (buff == NULL){
        printf("MEM error\n");
        return GPRS_ERROR;
    }

    memset(buff, '\0', sizeof(char) * 2000);
    snprintf(buff, 2000, "%s\"%s\",%d", WRITE_CERT_TEMPLATE, CLIENT_KEY, strlen(keyCLIENT_PRIVATE_KEY_PEM));

    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    free(buff);

    if(!ok){
        return false;
    }

    ok = acua_gprs_send_file((const char *)keyCLIENT_PRIVATE_KEY_PEM);

    return ok;    
}

bool acua_gprs_write_ca_cert(){
    bool ok;
    char * buff = malloc(sizeof(char) * 2000);
    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    memset(buff, '\0', sizeof(char) * 2000);
    snprintf(buff, 2000, "%s\"%s\",%d", WRITE_CERT_TEMPLATE, CA_CERT, strlen(keyCA_CERT_PEM));
    
    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    free(buff);

    if(!ok){
        return false;
    }

    ok = acua_gprs_send_file((const char *)keyCA_CERT_PEM);

    return ok;
}

bool acua_gprs_configure_ssl(){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s", SSL_VERSION);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_AUTH_MODE);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CACERT);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CLIENT_CERT);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CLIENT_KEY);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;

    free(buff);
    return ok;
}

bool acua_gprs_start_mqtt(){

    acua_gprs_disconnect_and_release();
    
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s", MQTT_START);
    ok &= acua_gprs_send_command(buff, MQTT_START_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s%s%s", _MQTT_CLIENT_ID_START, MQTT_CLIENT_ID, _MQTT_CLIENT_ID_END);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", MQTT_SSL_CONFIG);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    snprintf(buff, 200, "%s%s%s", _MQTT_ENDPOINT_START, MQTT_BROKER_ENDPOINT, _MQTT_ENDPOINT_END);
    ok &= acua_gprs_send_command(buff, AT_OK, 5000, true, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    free(buff);
    return ok;    
}

bool acua_gprs_subscribe(const char * topic){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s%d,%s", SUBSCRIBE_TEMPLATE, strlen(topic), "1");
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", topic);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    ok &= acua_gprs_send_command(SUBSCRIBE, SUBSCRIBE_OK, SHORT_DELAY, true, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    free(buff);
    return ok;     
}

bool acua_gprs_publish(const char * topic, const char * msg){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s%d", PUBLISH_TEMPLATE, strlen(topic));
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, false, true) == GPRS_OK;
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    snprintf(buff, 200, "%s", topic);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }  

    snprintf(buff, 200, "%s%d", PAYLOAD_TEMPLATE, strlen(msg));
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, false, true) == GPRS_OK;
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }  

    snprintf(buff, 200, "%s", msg);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, false, true) == GPRS_OK;
    //vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    ok &= acua_gprs_send_command(PUBLISH, PUBLISH_OK, SHORT_DELAY, false, true) == GPRS_OK;
    //vTaskDelay(500 / portTICK_PERIOD_MS);                    

    free(buff);
    return ok;
}

void acua_gprs_disconnect_and_release(){
    acua_gprs_send_command(MQTT_DISCONNECT, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    acua_gprs_send_command(MQTT_RELEASE, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(200 / portTICK_PERIOD_MS);    
    acua_gprs_send_command(MQTT_STOP, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(200 / portTICK_PERIOD_MS);    
}

/* bool 
// register new UART subroutine
	ESP_ERROR_CHECK(uart_isr_register(EX_UART_NUM,uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console));

	// enable RX interrupt
	ESP_ERROR_CHECK(uart_enable_rx_intr(EX_UART_NUM)); */

void acua_gprs_delete_files(){
    acua_gprs_send_command(DELETE_CACERT, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    acua_gprs_send_command(DELETE_CERTIFICATE, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    acua_gprs_send_command(DELETE_PRIVATE_KEY, AT_OK, SHORT_DELAY, false, true);
}

void acua_gprs_start_listening(){
    // release the pre registered UART handler/subroutine
    uart_isr_free(UART_NUM_1);

    // register new UART subroutine
	uart_isr_register(UART_NUM_1,acua_gprs_interrupt_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console);
    uart_enable_rx_intr(UART_NUM_1);

}

void IRAM_ATTR acua_gprs_interrupt_handle(void *arg){
    uint16_t rx_fifo_len, status;
    uint16_t i = 0;
    struct MqttSubsMsg m;

    status = UART1.int_st.val; // read UART interrupt Status
    rx_fifo_len = UART1.status.rxfifo_cnt; // read number of bytes in UART buffer

    while(rx_fifo_len){
    m.msg[i++] = UART1.fifo.rw_byte; // read all bytes
    rx_fifo_len--;
    }
    m.msg[i++] = '\n';

    // after reading bytes from buffer clear UART interrupt status
    xQueueSendFromISR(mqttSubsQueue, &m, NULL);
    //uart_flush_input(UART_NUM_1);
    uart_clear_intr_status(UART_NUM_1, UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
}

void acua_gprs_coppy_buffer(char * inputBuffet, int bufLen){
    snprintf(inputBuffet, bufLen, "%s", data_buffer);
}