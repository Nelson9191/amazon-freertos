#include "mqtt_config.h"
#include "mqtt_config_static.h"
#include "mqtt_info.h"
#include "task_config.h"
#include "flags.h"
#include "queue_conf.h"
#include "gpio_info.h"
#include "acua_gprs.h"
#include "at_commands.h"
#include "string.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"
#include "aws_mqtt_agent.h"
#include "aws_hello_world.h"
#include "rtc_config.h"
#include "jsmn.h"
#include "utils.h"


static MQTTAgentHandle_t xMQTTHandle = NULL;
static MessageBufferHandle_t xEchoMessageBuffer = NULL;
static const int JSON_VALUE_LEN = 20;
QueueHandle_t mqtt_queue;
QueueHandle_t mqttSubsQueue;

static char cDataBuffer[ MQTT_MAX_DATA_LENGTH ];


static uint32_t timestamp_sent;
static uint32_t timestamp_received;

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

void mqtt_config_init(void * param){
    //mqtt_queue = xQueueCreate(5, sizeof(struct MqttMsg));
    mqttSubsQueue = xQueueCreate(5, sizeof(struct MqttSubsMsg));
}

void mqtt_config_task(void * pvParameters){
    bool ok = true;
    struct MqttMsg mqtt_msg;
    struct MqttSubsMsg mqttSubsMsg;
    {
        /* data */
    };
    

    ok = acua_gprs_init();

    if(!ok){
        //Deberia reiniciar el GPRS
    }

    ok = acua_gprs_config_network();

    if(!ok){
        mqtt_config_restart();
    }

    acua_gprs_get_hour();

    ok = mqtt_config_connect();

    if (!ok){
        printf("Imposible conectar al broker.\nReiniciar ESP y GPRS\n");
        mqtt_config_restart();
    }

    gpio_set_level(GPIO_STATUS_ESP, 1);

    //Envia un comando x para que el módulo de GPIO reporte su estado
    queue_conf_send_gpio_command(1);

    timestamp_received = rtc_config_get_time();
    
    for(;;){

        if(xQueueReceive(mqtt_queue, &mqtt_msg, 0 )){//Lee si hay items en la cola
            mqtt_config_report_status(mqtt_msg);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } 

        if (acua_gprs_recv(false) == GPRS_OK){
            acua_gprs_coppy_buffer(cDataBuffer, MQTT_MAX_DATA_LENGTH);
            printf("buffer: %s\n", cDataBuffer);
        
            if (strstr(cDataBuffer, MQTT_SUBSCRIBE_TOPIC) != NULL){
                //printf("OUTPUT\n");
                mqtt_config_extract_msg();
                //printf("buffer: %s\n", cDataBuffer);
                mqtt_config_process_output(cDataBuffer);
            }
            /*
            else if (strstr(cDataBuffer, MQTT_HEARTBEAT_TOPIC) != NULL){
                printf("Heartbeat Recibido\n");
                mqtt_config_extract_msg();
                //printf("buffer: %s\n", cDataBuffer);
                mqtt_config_process_heartbeat(cDataBuffer);
            }
            */
            else if (strstr(cDataBuffer, MQTT_CONN_LOST) != NULL ){
                printf("Desconectado\n");
                gpio_set_level(GPIO_STATUS_ESP, 0);
                flags_reset_mqtt_connected();
                if (acua_gprs_config_network() == false || mqtt_config_connect() == false){
                    printf("Imposible volver a conectar al broker.\nReiniciar ESP y GPRS\n");
                    mqtt_config_restart();
                }
                else{
                    gpio_set_level(GPIO_STATUS_ESP, 1);
                }
            }
        }

        mqtt_config_verify_heartbeat();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}

void mqtt_config_extract_msg(){
    char * tmp = malloc(sizeof(char) * MQTT_MAX_DATA_LENGTH);
    if(tmp == NULL){
        return;
    }

    char * p1 = strstr(cDataBuffer, "{");
    char * p2 = strstr(cDataBuffer, "}");

    if(p1 == NULL || p2 == NULL){
        free(tmp);
        return;
    }

    memset( tmp, 0x00, MQTT_MAX_DATA_LENGTH);
    memcpy( tmp, p1, ( size_t ) (p2 - p1 + 1) );

    memset( cDataBuffer, 0x00, MQTT_MAX_DATA_LENGTH);
    memcpy( cDataBuffer, tmp, strlen(tmp) );

    free(tmp);
}

void mqtt_config_report_status(struct MqttMsg mqtt_msg){
    char cBuffer[ MQTT_MAX_DATA_LENGTH ];
    (void)snprintf( cBuffer, MQTT_MAX_DATA_LENGTH, "{\"parameter\": \"%s\", \"value\": %d, \"date\": %u, \"connection\":true}", mqtt_msg.name, mqtt_msg.status , mqtt_msg.timestamp);
    //printf("send -- %s\n", cBuffer);
    acua_gprs_publish(MQTT_PUBLISH_TOPIC, cBuffer);
}

void mqtt_config_send_heartbeat(uint32_t curr_timestamp){
    char cDataBuffer[ MQTT_MAX_DATA_LENGTH ];
    struct BuffResponse buff_response = acua_gprs_interchange_message(CSQ);
    bool ok;
    char signal[15];
    memset(signal, 0, 15);

    if (buff_response.buff != NULL){
        char * ptr = strstr((const char *)buff_response.buff, "+CSQ:");
        if (ptr){
            size_t i = 0;
            while (*ptr != '\n' && *ptr != ',' && i < 14){
                signal[i++] = *ptr;
                ptr++;
            }
        }
        (void)snprintf( cDataBuffer, MQTT_MAX_DATA_LENGTH, "{\"timestamp\": %u, \"signal\": \"%s\"}", curr_timestamp, signal);
    }
    else{
        (void)snprintf( cDataBuffer, MQTT_MAX_DATA_LENGTH, "{\"timestamp\": %u}", curr_timestamp);
    }

    utils_free_ptr(&(buff_response.buff));

    printf("Heartbeat1\n");
    ok = acua_gprs_publish(MQTT_HEARTBEAT_TOPIC, cDataBuffer);

    if (!ok){
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        printf("Heartbeat2\n");
        ok = acua_gprs_publish(MQTT_HEARTBEAT_TOPIC, cDataBuffer);
    }

    if (!ok){
        mqtt_config_restart();
    }
}

void mqtt_config_process_heartbeat(const char * cBuffer){
	int r;
    jsmn_parser p;
	jsmntok_t t[20]; /* We expect no more than 128 tokens */
    char value[JSON_VALUE_LEN];
    struct MqttMsg mqtt_msg;

    printf("****** %s\n", cBuffer);
    jsmn_init(&p);
    r = jsmn_parse(&p, cBuffer, strlen(cBuffer), t, sizeof(t)/sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }


    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return 1;
    }

    for (int i = 1; i < r; i++) {
        memset( value, 0x00, JSON_VALUE_LEN);
        strncpy(value, cBuffer + t[i+1].start, t[i+1].end-t[i+1].start);
        if (jsoneq(cBuffer, &t[i], "timestamp") == 0) {
            if(sscanf(value, "%u", &timestamp_received) == 1){
            
            }
            break;
        }
    }     
}

void mqtt_config_process_output(const char * cBuffer){
	int r;
    jsmn_parser p;
	jsmntok_t t[20]; /* We expect no more than 128 tokens */
    char value[JSON_VALUE_LEN];
    struct MqttMsg mqtt_msg;

    uint32_t gpio = 0;
    uint32_t status = 0;

    jsmn_init(&p);
    r = jsmn_parse(&p, cBuffer, strlen(cBuffer), t, sizeof(t)/sizeof(t[0]));
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return 1;
    } 

    for (int i = 1; i < r; i++) {
        memset( value, 0x00, JSON_VALUE_LEN);
        strncpy(value, cBuffer + t[i+1].start, t[i+1].end-t[i+1].start);
        if (jsoneq(cBuffer, &t[i], DO01_NAME) == 0) {
            snprintf(mqtt_msg.name, 10, "%s", DO01_NAME);
            gpio = 1;
            i++;                
        } else if (jsoneq(cBuffer, &t[i], DO02_NAME) == 0) {
            snprintf(mqtt_msg.name, 10, "%s", DO02_NAME);
            gpio = 2;
            i++;
        } else if (jsoneq(cBuffer, &t[i], DO03_NAME) == 0) {            
            snprintf(mqtt_msg.name, 10, "%s", DO03_NAME);
            gpio = 3;
            i++;
        } else if (jsoneq(cBuffer, &t[i], DO04_NAME) == 0) {
            snprintf(mqtt_msg.name, 10, "%s", DO04_NAME);
            gpio = 4;
            i++;
        }else if (jsoneq(cBuffer, &t[i], "hora") == 0) {
            gpio = 0;
            i++; 
            rtc_config_set_time_(value, 0);
        }else {
            printf("Unexpected key: %.*s\n", t[i].end-t[i].start,
                    cBuffer + t[i].start);
            i++;                        
        }
    }

    if(gpio > 0){            
        queue_conf_send_gpio(gpio, value[0] == '0' ? 0 : 1);            
        mqtt_msg.status = value[0] == '0' ? 0 : 1;
        mqtt_msg.timestamp = rtc_config_get_time();
        queue_conf_send_mqtt(mqtt_msg);
    }
    else{
        printf("NAda que hacer\n");
    }
}

void mqtt_config_verify_heartbeat(){
    uint32_t curr_timestamp = rtc_config_get_time();

    if(curr_timestamp - timestamp_sent > HEARTHBEAT_SEND_INTERVAL_SECS){ // Envia heartbeat cada X segundos
        timestamp_sent = curr_timestamp;
        printf("send heartbeat -----------\n");
        mqtt_config_send_heartbeat(timestamp_sent);
        //printf("heartbeat sent: %d\n", timestamp_sent);
        //printf("last heartbeat received: %d\n", timestamp_received);
    }
}

bool mqtt_config_connect(){
    size_t ctr = 0;
    bool ok = true;

    for (int i = 0; i < 3; i++){
        ok = true;
        ok &= acua_gprs_start_mqtt();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ok &= acua_gprs_subscribe(MQTT_SUBSCRIBE_TOPIC);
        //vTaskDelay(500 / portTICK_PERIOD_MS);
        //ok &= acua_gprs_subscribe(MQTT_HEARTBEAT_TOPIC);

        if (ok){
            flags_set_mqtt_connected();
            break;
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return ok;
}

void mqtt_config_restart(){
    gpio_set_level(GPIO_RESET_GPRS, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_RESET_GPRS, 0);
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_DO01, 0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_DO02, 0);    
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_DO03, 0);    
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_DO04, 0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_STATUS_ESP, 0);  

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();    
}

void mqtt_config_turn_off_putputs(){
    gpio_set_level(GPIO_RESET_GPRS, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_RESET_GPRS, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();     
}