#include "acua_gprs.h"
#include "clientcredential_keys.h"
#include "at_commands.h"
#include "mqtt_info.h"
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

#define BUF_SIZE 2000
#define TICKS_TO_WAIT   20

#define SHORT_DELAY   2000
#define LONG_DELAY    20000
#define TOO_LONG_DELAY  65000

uint8_t data_buffer[BUF_SIZE];
QueueHandle_t at_queue;

uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

void acua_gprs_init(){
    
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

    ( void ) xTaskCreate( acua_gprs_task,
                        "serial",
                        2000,
                        NULL,
                        2,
                        NULL );           
}

void acua_gprs_task(void * pvParameters){
    struct ATMsg msg;
    int len;
    int ctr = 0;
    bool ok;

    ok = acua_gprs_verify_status();

    ok = acua_gprs_validate_certs();

    //ok = false;
    if(!ok){
        acua_gprs_delete_files();

        ok = true;
        ok &= acua_gprs_write_client_cert();
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        ok &= acua_gprs_write_client_key();
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        ok &= acua_gprs_write_ca_cert();
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        ok &= acua_gprs_validate_certs();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if(ok)
    {
        ok &= acua_gprs_configure_ssl();
    }

    //acua_gprs_disconnect_and_release();

    if(ok){
        ok &= acua_gprs_start_mqtt();
    }

    vTaskDelay(15000 / portTICK_PERIOD_MS);

    if(ok){
        ok &= acua_gprs_subscribe();
    }

    for(;;){
        if(ok){
            acua_gprs_publish("hola");
        }
        else{
            acua_gprs_disconnect_and_release();
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

bool acua_gprs_verify_status(){
    int len = -1;
    bool ok = true;
    enum eGPRSStatus response;
    
    response = acua_gprs_send_command(AT, AT_OK, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    //Valida estado de GPRS
    do{
        response = acua_gprs_send_command(CREG, CREG_OK, SHORT_DELAY, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);

    //Valida estado de GPRS
    do{
        response = acua_gprs_send_command(CGREG, CGREG_OK, SHORT_DELAY, true);
        vTaskDelay(3000 / portTICK_PERIOD_MS);    
    }while(response == GPRS_ERROR);


/*
    response = acua_gprs_send_command(COPS, AT_OK, LONG_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_gprs_send_command(COPS_LIST, AT_OK, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS); */
    
    response = acua_gprs_send_command(CSQ, CSQ_ERROR, SHORT_DELAY, false); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    response = acua_gprs_send_command(CGATT, CGATT_OK, LONG_DELAY, true); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);      
    response = acua_gprs_send_command(CGATT_ACTIVATE, AT_OK, LONG_DELAY, true); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);    
    response = acua_gprs_send_command(CGATT, CGATT_OK, LONG_DELAY, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
    response = acua_gprs_send_command(SET_CGDCONT, AT_OK, SHORT_DELAY, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    
    response = acua_gprs_send_command(CGDCONT_LIST, CGDCONT_OK, SHORT_DELAY, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

    response = acua_gprs_send_command(CGACT_LIST, CGACT_LIST_OK, SHORT_DELAY, true); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    response = acua_gprs_send_command(SET_CGACT, CGACT_OK, LONG_DELAY, true); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);  

    response = acua_gprs_send_command(SOCK, AT_OK, LONG_DELAY, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS); 

    //IP address
    response = acua_gprs_send_command(CGPADDR, CGPADDR_FAIL, SHORT_DELAY, false);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    

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
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    return len >= 0 ? OK : GPRS_HARDWARE_ERROR;
}

bool acua_gprs_send_file(const char * msg){
    int len = uart_write_bytes(UART_NUM_1, msg, strlen(msg));
    bool ok;
    printf("%s >> written: %d\n", msg, len);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if (acua_gprs_recv(false) == GPRS_NO_RESPONSE){
        return false;
    }

    return acua_gprs_verify_ok((const char *)data_buffer, AT_OK);  
}


enum eGPRSStatus acua_gprs_recv(bool loop){
    int ctr = 0;
    int len;

    memset(data_buffer, '\0', BUF_SIZE);
    while(!acua_gprs_response_available()){
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

enum eGPRSStatus acua_gprs_send_command(const char * command, const char * validation_response, int wait_ms, bool validate_ok){
    if(acua_gprs_write(command) == GPRS_ERROR){
        return GPRS_HARDWARE_ERROR;
    }

    vTaskDelay(wait_ms / portTICK_PERIOD_MS);

    if (acua_gprs_recv(false) == GPRS_NO_RESPONSE){
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
    return len > 0;
}

bool acua_gprs_validate_certs(){
    bool ok = true;
    enum eGPRSStatus response;
    
    response = acua_gprs_send_command(CERT_LIST, CLIENT_CERT, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ok &= response == GPRS_OK;

    response = acua_gprs_send_command(CERT_LIST, CLIENT_KEY, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ok &= response == GPRS_OK;

    response = acua_gprs_send_command(CERT_LIST, CA_CERT, SHORT_DELAY, true);
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

    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    free(buff);

    if(!ok){
        printf("****ERRORRRR\n");
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

    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
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
    
    ok = acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
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
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_AUTH_MODE);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CACERT);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CLIENT_CERT);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", SSL_CLIENT_KEY);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;

    free(buff);
    return ok;
}

bool acua_gprs_start_mqtt(){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s", MQTT_START);
    ok &= acua_gprs_send_command(buff, MQTT_START_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s%s%s", _MQTT_CLIENT_ID_START, MQTT_CLIENT_ID, _MQTT_CLIENT_ID_END);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", MQTT_SSL_CONFIG);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", CFG_WILL_TOPIC);
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", WILL_TOPIC);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", CFG_WILL_SMG);
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", WILL_MSG);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);    
    
    snprintf(buff, 200, "%s%s%s", _MQTT_ENDPOINT_START, MQTT_BROKER_ENDPOINT, _MQTT_ENDPOINT_END);
    ok &= acua_gprs_send_command(buff, AT_OK, TOO_LONG_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    free(buff);
    return ok;    
}

bool acua_gprs_subscribe(){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s%d,%s", SUBSCRIBE_TEMPLATE, strlen(MQTT_SUBSCRIBE_TOPIC), "1");
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    snprintf(buff, 200, "%s", MQTT_SUBSCRIBE_TOPIC);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    free(buff);
    return ok;     
}

bool acua_gprs_publish(const char * msg){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 200);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s%d", PUBLISH_TEMPLATE, strlen(MQTT_PUBLISH_TOPIC));
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    snprintf(buff, 200, "%s", MQTT_PUBLISH_TOPIC);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }  

    snprintf(buff, 200, "%s%d", PAYLOAD_TEMPLATE, strlen(msg));
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }  

    snprintf(buff, 200, "%s", msg);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    snprintf(buff, 200, "%s", msg);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    snprintf(buff, 200, "%s", PUBLISH);
    ok &= acua_gprs_send_command(buff, PUBLISH_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);                    

    free(buff);
    return ok;
}

void acua_gprs_disconnect_and_release(){
    acua_gprs_send_command(MQTT_DISCONNECT, AT_OK, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    acua_gprs_send_command(MQTT_RELEASE, AT_OK, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    
    acua_gprs_send_command(MQTT_STOP, AT_OK, SHORT_DELAY, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);    
}

bool acua_gprs_config_will_topic(){
    bool ok = true;
    char * buff = malloc(sizeof(char) * 100);

    if (buff == NULL){
        printf("MEM error\n");
        return false;
    }

    snprintf(buff, 200, "%s", CFG_WILL_TOPIC);
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }

    snprintf(buff, 200, "%s", WILL_TOPIC);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }  

    snprintf(buff, 200, "%s", CFG_WILL_SMG);
    ok &= acua_gprs_send_command(buff, WRITE_SEQ, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    } 

    snprintf(buff, 200, "%s", WILL_MSG);
    ok &= acua_gprs_send_command(buff, AT_OK, SHORT_DELAY, true) == GPRS_OK;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if(!ok){
        free(buff);
        return false;
    }           

    free(buff);
    return true;
}

void acua_gprs_delete_files(){
    acua_gprs_send_command(DELETE_CACERT, AT_OK, SHORT_DELAY, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    acua_gprs_send_command(DELETE_CERTIFICATE, AT_OK, SHORT_DELAY, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    acua_gprs_send_command(DELETE_PRIVATE_KEY, AT_OK, SHORT_DELAY, true);
}