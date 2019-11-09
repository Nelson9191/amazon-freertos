#include "acua_gprs.h"
#include "clientcredential_keys.h"
#include "at_commands.h"
#include "mqtt_info.h"
#include "queue_conf.h"
#include "gpio_info.h"
#include "ntp_info.h"
#include "ntp.h"
#include "rtc_config.h"

#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "utils.h"

#define ECHO_TEST_TXD  GPIO_NUM_17
#define ECHO_TEST_RXD  GPIO_NUM_16
#define ECHO_TEST_RTS  UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE 2000
#define TICKS_TO_WAIT   20

#define SHORT_DELAY   2000
#define LONG_DELAY    15000
#define TOO_LONG_DELAY  65000
#define IP_LEN 16

uint8_t data_buffer[BUF_SIZE];

char NTP_server_ip[20];
QueueHandle_t at_queue;

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
    uart_driver_install(UART_NUM_1, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);


    //Time for the GPRS to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    queue_conf_send_gpio(GPIO_ON_GPRS, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    queue_conf_send_gpio(GPIO_ON_GPRS, 0);

    vTaskDelay(5000 / portTICK_PERIOD_MS);


    do{
        //uart_flush(UART_NUM_1);
        printf("INIT\n");
        response = acua_gprs_send_command(CPIN, CPIN_OK, SHORT_DELAY, false, true);
        vTaskDelay(2000 / portTICK_PERIOD_MS);    
    } while(response != GPRS_OK);


    bool ok = acua_gprs_validate_certs();

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
    int len = -1;
    bool ok = false;
    enum eGPRSStatus response;
    
    response = acua_gprs_send_command(AT, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);  

/*
    response = acua_gprs_send_command(COPS, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(5000 / portTICK_PERIOD_MS); 

    response = acua_gprs_send_command(COPS_LIST, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);     
    */

    //Valida estado de GPRS
    for (int i = 0; i < 60; i++){
        //response = acua_gprs_send_command(CREG, CREG_OK, SHORT_DELAY, false, true);
        response = acua_gprs_send_creg(CREG, CREG_OK, SHORT_DELAY, false, true);
        if (response != GPRS_ERROR){
            ok = true;
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }

    if (!ok){
        return false;
    }

    //Valida estado de GPRS
    for (int i = 0; i < 60; i++){
        response = acua_gprs_send_command(CGREG, CGREG_OK, SHORT_DELAY, false, true);
        if (response != GPRS_ERROR){
            ok = true;
            break;
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (!ok){
        return false;
    }
 
    /*
    response = acua_gprs_send_command(CSQ, CSQ_ERROR, SHORT_DELAY, false, false); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    */
     
    response = acua_gprs_send_command(CGATT_ACTIVATE, AT_OK, LONG_DELAY, true, true); 
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
    
    //IP address
    response = acua_gprs_send_command(CGPADDR, CGPADDR_FAIL, SHORT_DELAY, true, false);
    vTaskDelay(1000 / portTICK_PERIOD_MS);  

    return response == GPRS_OK;
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
    //printf("%s >> written: %d\n", msg, len);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return len >= 0 ? OK : GPRS_HARDWARE_ERROR;
}

bool acua_gprs_send_file(const char * msg){
    int len = uart_write_bytes(UART_NUM_1, msg, strlen(msg));
    bool ok;
    //printf("%s >> written: %d\n", msg, len);

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
        //uart_flush(UART_NUM_1);
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
        //printf("CTR: %d\n", ctr);   
        while((ctr ++ < wait_ms / 2) && acua_gprs_response_available() == false ){
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
    

    if(!acua_gprs_response_available()){
        printf("GPRS_HARDWARE_ERROR\n");
        return GPRS_HARDWARE_ERROR;
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);

    enum eGPRSStatus resp = acua_gprs_recv(true);
    if (resp != GPRS_OK){
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

enum eGPRSStatus acua_gprs_send_creg(const char * command, const char * validation_response, int wait_ms, bool force_wait, bool validate_ok){
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
        //printf("CTR: %d\n", ctr);   
        while((ctr ++ < wait_ms / 2) && acua_gprs_response_available() == false ){
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
    

    if(!acua_gprs_response_available()){
        printf("GPRS_HARDWARE_ERROR\n");
        return GPRS_HARDWARE_ERROR;
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);

    enum eGPRSStatus resp = acua_gprs_recv(true);
    if (resp != GPRS_OK){
        printf("HW error2\n");
        return GPRS_HARDWARE_ERROR;
    }

    if (strstr((const char *)data_buffer, CREG_OK) != NULL || strstr((const char *)data_buffer, CREG_ROAMING) != NULL){
        return GPRS_OK;
    }
    else{
        return GPRS_ERROR;
    }
}

struct BuffResponse acua_gprs_send_buffer(const char * buff, size_t buff_len, int wait_ms, bool force_wait){
    int ctr = 0;
    size_t WAIT_STEP = 20;
    size_t MAX_WAIT_TIME = 50000;
    struct BuffResponse buff_response = {.buff = NULL, .buff_len = 0};
    size_t len = 0;

    //uart_flush(UART_NUM_1);
    uart_write_bytes(UART_NUM_1, buff, buff_len);

    if(force_wait){
        vTaskDelay(wait_ms / portTICK_PERIOD_MS);
    }
    else
    {
        while((ctr ++ < wait_ms) && acua_gprs_response_available() == false ){
            vTaskDelay(WAIT_STEP / portTICK_PERIOD_MS);
            ctr += WAIT_STEP;
        }
    }
    

    if(!acua_gprs_response_available()){
        printf("GPRS_HARDWARE_ERROR\n");
        goto exit_acua_gprs_send_buffer;
    }

    vTaskDelay(6000 / portTICK_PERIOD_MS);

    uart_get_buffered_data_len(UART_NUM_1, &len);

    printf("lenlen %d\n", len);

    if (len < 2){
        goto exit_acua_gprs_send_buffer;
    }

    buff = malloc(sizeof(uint8_t) * (len + 10));   
    if (buff == NULL){
        printf("aa\n");
        goto exit_acua_gprs_send_buffer;
    }     

    len = uart_read_bytes(UART_NUM_1, buff, len, TICKS_TO_WAIT);
    if(len <= 0){
        free(buff);
        buff = NULL;
        printf("bb\n");
    }

    buff_response.buff = buff;
    buff_response.buff_len = len;

exit_acua_gprs_send_buffer:
    
    return buff_response;
}

bool acua_gprs_verify_ok(const char * resp, const char * pattern_ok){
    return strstr(resp, pattern_ok) != NULL ? true : false;
}

inline bool acua_gprs_response_available(){
    size_t len = 0;
    if(uart_get_buffered_data_len(UART_NUM_1, &len) < 0){
        //printf("error buf\n");
        return false;
    }
    else{
        return len > 2;
    }
}

bool acua_gprs_validate_certs(){
    return acua_gprs_send_command(CERT_LIST, CERT_LIST_OK, SHORT_DELAY, false, true) == GPRS_OK;
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

void acua_gprs_coppy_buffer(char * inputBuffet, int bufLen){
    snprintf(inputBuffet, bufLen, "%s", data_buffer);
}

bool acua_gprs_get_hour(){
    enum eGPRSStatus response;
    bool ok = false;
    uint8_t * buff = NULL;
    const size_t NTP_FRAME_LEN = 48;
    char NTP_FRAME[NTP_FRAME_LEN];
    struct BuffResponse buff_response;

    char * ptr1 = NULL;
    char * ptr2 = NULL;

     if (acua_gprs_send_command(NET_OPEN, NET_OPEN_OK, SHORT_DELAY, false, true) != GPRS_OK)
    {
        //goto exit_get_hour;
    } 

    //acua_gprs_send_buffer(GET_NTP_IP, strlen(GET_NTP_IP), BUFFER_AVAILABLE, SHORT_DELAY, false, true) != GPRS_OK){
    buff_response = acua_gprs_interchange_message(GET_NTP_IP);

    if (buff_response.buff == NULL){
        printf("Sin respuesta de ntp.pool.org\n");
        goto exit_get_hour;
    }

    printf("%s\n",  buff_response.buff);

    if (strstr((const char *)buff_response.buff, AT_OK) == NULL){
        printf("Falla NTP server\n");
        goto exit_get_hour;
    }

    ptr1 = strstr((const char *)buff_response.buff, ".org\",\"");
    if (ptr1 == NULL){
        printf("Falla NTP server\n");
        goto exit_get_hour;
    }

    ptr1 += 7;

    ptr2 = strstr((const char *)ptr1, "\"");
    if (ptr2 == NULL){
        printf("Falla NTP server\n");
        goto exit_get_hour;
    }

    size_t ip_len = ptr2 - ptr1;
    if (ip_len > IP_LEN){
        printf("Falla NTP server\n");
        goto exit_get_hour;
    }

    snprintf(NTP_server_ip, ip_len + 1, "%s", ptr1);
    utils_free_ptr(&(buff_response.buff));

    acua_gprs_send_command(BUFFER_MODE, AT_OK, SHORT_DELAY, false, true);
    vTaskDelay(200   / portTICK_PERIOD_MS);
    if (acua_gprs_send_command(CIPOPEN, CIPOPEN_OK, SHORT_DELAY, false, true) != GPRS_OK){
        printf("Error CIPOPEN\n");
        goto exit_get_hour;
    }

    buff = malloc(sizeof(char) * 100);
    if (buff == NULL){
        goto exit_get_hour; 
    }

    snprintf((char *)buff, 100, "%s,%d,\"%s\",%d", CIPSEND, NTP_FRAME_LEN, NTP_server_ip, NTP_HOST_PORT);
    if (acua_gprs_send_command((const char *)buff, WRITE_SEQ, SHORT_DELAY, false, true) != GPRS_OK){
        goto exit_get_hour; 
    }

    utils_free_ptr(&buff);

    memset(NTP_FRAME, 0, NTP_FRAME_LEN);
    NTP_FRAME[0] = 0xE3;
    NTP_FRAME[1] = 0;
    NTP_FRAME[2] = 6;
    NTP_FRAME[3] = 0xEC;
    NTP_FRAME[12]  = 49;
    NTP_FRAME[13]  = 0x4E;
    NTP_FRAME[14]  = 49;
    NTP_FRAME[15]  = 52;

    buff_response = acua_gprs_send_buffer(NTP_FRAME, NTP_FRAME_LEN, SHORT_DELAY, false);
    if (buff_response.buff == NULL){
        printf("Error CIPSEND\n");
        goto exit_get_hour; 
    }

    //buff += NTP_FRAME_LEN;
    ptr1 = strstr((const char*)(buff_response.buff + NTP_FRAME_LEN), IPD_NTP_RESPONSE);
    if (ptr1 == NULL){
        printf("+IPD48 no recibido desde el servidor NTP \n");
        goto exit_get_hour; 
    }

    ptr1 = ptr1 + strlen(IPD_NTP_RESPONSE) + 2;

    int32_t tmp = 0;
    tmp |= ptr1[40] << 24;
    tmp |= ptr1[41] << 16;
    tmp |= ptr1[42] << 8;
    tmp |= ptr1[43] << 0;

    tmp = tmp - NTP_1970_TIMESTAMP;
    printf("captured timestamp: %u\n", tmp);
    rtc_config_set_time(tmp);
    utils_free_ptr(&(buff_response.buff));

    ok = true;

exit_get_hour:
    acua_gprs_send_command(CIPCLOSE, AT_OK, SHORT_DELAY, false, true);

    utils_free_ptr(&buff);
    utils_free_ptr(&(buff_response.buff));

    acua_gprs_send_command(NET_CLOSE, AT_OK, SHORT_DELAY, false, true);
    return ok;
}

struct BuffResponse acua_gprs_interchange_message(const char * msg){
    size_t ctr = 0;
    size_t WAIT_STEP = 20;
    size_t MAX_WAIT_TIME = 10000;
    size_t len = 0;
    uint8_t * buff;
    struct BuffResponse buff_response = {.buff = NULL, .buff_len = 0};
    
    if (acua_gprs_write(msg) == GPRS_ERROR){
        goto exit_acua_gprs_interchange_message;
    }

    while(!acua_gprs_response_available() && ctr < MAX_WAIT_TIME){
        vTaskDelay(WAIT_STEP / portTICK_PERIOD_MS);
        ctr += WAIT_STEP;
    }

    if (!acua_gprs_response_available()){
        goto exit_acua_gprs_interchange_message;
    }

    vTaskDelay(6000 / portTICK_PERIOD_MS);


    uart_get_buffered_data_len(UART_NUM_1, &len);

    if (len < 2){
        goto exit_acua_gprs_interchange_message;
    }

    buff = malloc(sizeof(uint8_t) * (len + 10));
    if (buff == NULL){
        goto exit_acua_gprs_interchange_message;
    }


    len = uart_read_bytes(UART_NUM_1, buff, len, TICKS_TO_WAIT);
    if(len <= 0){
        free(buff);
        buff = NULL;
    }

    buff_response.buff = buff;
    buff_response.buff_len = len;

exit_acua_gprs_interchange_message:
    
    return buff_response;
}