#include "mqtt_config.h"
#include "mqtt_config_static.h"
#include "mqtt_info.h"
#include "task_config.h"
#include "flags.h"
#include "queue_conf.h"
#include "gpio_info.h"
#include "acua_gprs.h"

#include "string.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"
#include "aws_mqtt_agent.h"
#include "aws_hello_world.h"
#include "rtc_config.h"
#include "jsmn.h"


static MQTTAgentHandle_t xMQTTHandle = NULL;
static MessageBufferHandle_t xEchoMessageBuffer = NULL;
static const int JSON_VALUE_LEN = 20;
QueueHandle_t mqtt_queue;
QueueHandle_t mqttSubsQueue;

static char cDataBuffer[ MQTT_MAX_DATA_LENGTH ];


static uint32_t timestamp_sent;
static uint32_t timestamp_received;

int daniel = 0;

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

void mqtt_config_init(void * param){
    //mqtt_queue = xQueueCreate(5, sizeof(struct MqttMsg));
    //mqttSubsQueue = xQueueCreate(5, sizeof(struct MqttSubsMsg));
}

void mqtt_config_task(void * pvParameters){
    bool ok = true;
    struct MqttMsg mqtt_msg;
    int ctr = 0;
    int level = 0;

    ok = acua_gprs_init();

    if(!ok){
        //Deberia reiniciar el GPRS
    }

    ok = acua_gprs_config_network();

    if(!ok){
        //Definir que deberia hacer
    }

    acua_gprs_get_hour();

    ok = acua_gprs_start_mqtt();

    if(!ok){
        //Definir que hacer
    }

    flags_set_mqtt_connected();
    ok = acua_gprs_subscribe(MQTT_SUBSCRIBE_TOPIC);

    if(!ok)
    {
        //Definir que hacer
    }

    ok = acua_gprs_subscribe(MQTT_HEARTBEAT_TOPIC);

    if(!ok)
    {
        //Definir que hacer
    }

    
    for(;;){
        //mqtt_config_verify_heartbeat();
        //ctr_disconnect = 0;

        if(xQueueReceive(mqtt_queue, &mqtt_msg, 0 )){//Lee si hay items en la cola
            mqtt_config_report_status(mqtt_msg);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } 

        if(acua_gprs_recv(false) == GPRS_OK){
            acua_gprs_coppy_buffer(cDataBuffer, MQTT_MAX_DATA_LENGTH);
            printf("buffer: %s\n", cDataBuffer);
        
            if(strstr(cDataBuffer, MQTT_SUBSCRIBE_TOPIC) != NULL){
                //printf("OUTPUT\n");
                mqtt_config_extract_msg();
                //printf("buffer: %s\n", cDataBuffer);
                mqtt_config_process_output(cDataBuffer);
            }
            else if(strstr(cDataBuffer, MQTT_HEARTBEAT_TOPIC) != NULL){
                printf("HEARTBEAT\n");
                mqtt_config_extract_msg();
                printf("buffer: %s\n", cDataBuffer);
                //mqtt_config_process_heartbeat(cDataBuffer);
            }
        }

        if (ctr++ >= 100){
            ctr = 0;
            level = level > 0 ? 0 : 1;
            gpio_set_level(GIPO_STATUS_ESP, level);
        }
        
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
    printf("send -- %s\n", cBuffer);
    acua_gprs_publish(MQTT_PUBLISH_TOPIC, cBuffer);
}

void mqtt_config_send_heartbeat(uint32_t curr_timestamp){
    char cDataBuffer[ MQTT_MAX_DATA_LENGTH ];
    (void)snprintf( cDataBuffer, MQTT_MAX_DATA_LENGTH, "{\"timestamp\": %u}", curr_timestamp);
    printf("send---%s\n", cDataBuffer); 
    acua_gprs_publish(MQTT_HEARTBEAT_TOPIC, cDataBuffer);  
}

static MQTTAgentReturnCode_t mqtt_config_create( void ){
    MQTTAgentReturnCode_t xReturned;
    BaseType_t xReturn = pdFAIL;
    MQTTAgentConnectParams_t xConnectParameters =
    {
        MQTT_BROKER_ENDPOINT, /* The URL of the MQTT broker to connect to. */
        MQTT_AGENT_CONNECT_FLAGS,             /* Connection flags. */
        pdFALSE,                              /* Deprecated. */
        MQTT_BROKER_PORT,                     /* Port number on which the MQTT broker is listening. Can be overridden by ALPN connection flag. */
        MQTT_CLIENT_ID,                       /* Client Identifier of the MQTT client. It should be unique per broker. */
        0,                                    /* The length of the client Id, filled in later as not const. */
        pdFALSE,                              /* Deprecated. */
        NULL,                                 /* User data supplied to the callback. Can be NULL. */
        mqtt_events_callback,                 /* Callback used to report various events. Can be NULL. */
        NULL,                                 /* Certificate used for secure connection. Can be NULL. */
        0                                     /* Size of certificate used for secure connection. */
    };

    /* Check this function has not already been executed. */
    configASSERT( xMQTTHandle == NULL );

    /* The MQTT client object must be created before it can be used.  The
     * maximum number of MQTT client objects that can exist simultaneously
     * is set by mqttconfigMAX_BROKERS. */
    xReturned = MQTT_AGENT_Create( &xMQTTHandle );

    return xReturned;
}

static MQTTAgentReturnCode_t mqtt_config_connect(){
    MQTTAgentReturnCode_t xReturned;

    MQTTAgentConnectParams_t xConnectParameters =
    {
        MQTT_BROKER_ENDPOINT, /* The URL of the MQTT broker to connect to. */
        MQTT_AGENT_CONNECT_FLAGS,             /* Connection flags. */
        pdFALSE,                              /* Deprecated. */
        MQTT_BROKER_PORT,                     /* Port number on which the MQTT broker is listening. Can be overridden by ALPN connection flag. */
        MQTT_CLIENT_ID,                       /* Client Identifier of the MQTT client. It should be unique per broker. */
        0,                                    /* The length of the client Id, filled in later as not const. */
        pdFALSE,                              /* Deprecated. */
        NULL,                                 /* User data supplied to the callback. Can be NULL. */
        mqtt_events_callback,                 /* Callback used to report various events. Can be NULL. */
        NULL,                                 /* Certificate used for secure connection. Can be NULL. */
        0                                     /* Size of certificate used for secure connection. */
    };

    configPRINTF( ( "MQTT echo attempting to connect to %s.\r\n", MQTT_BROKER_ENDPOINT ) );

    /* Fill in the MQTTAgentConnectParams_t member that is not const,
    * and therefore could not be set in the initializer (where
    * xConnectParameters is declared in this function). */
    
    xConnectParameters.usClientIdLength = ( uint16_t ) strlen( ( const char * ) MQTT_CLIENT_ID );

    xReturned = MQTT_AGENT_Connect( xMQTTHandle,
                                    &xConnectParameters,
                                    MQTT_ECHO_TLS_NEGOTIATION_TIMEOUT );
    

    if(xReturned == eMQTTAgentSuccess){
        flags_set_mqtt_connected();
        printf("MQTT connected\n");
    }
    else{
        printf("MQTT could not connect\n");
    }
    return xReturned;
}

static void mqtt_config_delete(){
    (void)MQTT_AGENT_Delete( xMQTTHandle );
    flags_reset_mqtt_connected();
}

static void mqtt_config_disconnect(){
    (void) MQTT_AGENT_Disconnect( xMQTTHandle, MQTT_TIMEOUT );
    flags_reset_mqtt_connected();
}


static BaseType_t mqtt_config_subscribe_to_output(void){

    if(!flags_is_mqtt_connected()){
        return -1;
    }
    MQTTAgentReturnCode_t xReturned;
    BaseType_t xReturn;
    MQTTAgentSubscribeParams_t xSubscribeParams;
    //printf("TOPIC: %s\n", MQTT_SUBSCRIBE_TOPIC);
    /* Setup subscribe parameters to subscribe to MQTT_SUBSCRIBE_TOPIC topic. */
    xSubscribeParams.pucTopic = ( const uint8_t * )MQTT_SUBSCRIBE_TOPIC;
    xSubscribeParams.pvPublishCallbackContext = NULL;
    xSubscribeParams.pxPublishCallback = mqtt_config_subs_callback;
    xSubscribeParams.usTopicLength = ( uint16_t ) strlen( ( const char * ) MQTT_SUBSCRIBE_TOPIC );
    xSubscribeParams.xQoS = eMQTTQoS1;

    /* Subscribe to the topic. */
    xReturned = MQTT_AGENT_Subscribe( xMQTTHandle,
                                      &xSubscribeParams,
                                      MQTT_TIMEOUT );

    if( xReturned == eMQTTAgentSuccess )
    {
        configPRINTF( ( "MQTT Echo demo subscribed to %s\r\n", MQTT_SUBSCRIBE_TOPIC ) );
        xReturn = pdPASS;
    }
    else
    {
        configPRINTF( ( "ERROR:  MQTT Echo demo could not subscribe to %s\r\n", MQTT_SUBSCRIBE_TOPIC ) );
        xReturn = pdFAIL;
    }

    return xReturned;
}

static BaseType_t mqtt_config_subscribe_to_heartbeat(void){

    if(!flags_is_mqtt_connected()){
        return -1;
    }
    MQTTAgentReturnCode_t xReturned;
    BaseType_t xReturn;
    MQTTAgentSubscribeParams_t xSubscribeParams;
 
    xSubscribeParams.pucTopic = ( const uint8_t * )MQTT_HEARTBEAT_TOPIC;
    xSubscribeParams.pvPublishCallbackContext = NULL;
    xSubscribeParams.pxPublishCallback = mqtt_config_subs_callback;
    xSubscribeParams.usTopicLength = ( uint16_t ) strlen( ( const char * ) MQTT_HEARTBEAT_TOPIC );
    xSubscribeParams.xQoS = eMQTTQoS1;

    /* Subscribe to the topic. */
    xReturned = MQTT_AGENT_Subscribe( xMQTTHandle,
                                      &xSubscribeParams,
                                      MQTT_TIMEOUT );

    if( xReturned == eMQTTAgentSuccess )
    {
        configPRINTF( ( "MQTT Echo demo subscribed to %s\r\n", MQTT_HEARTBEAT_TOPIC ) );
        xReturn = pdPASS;
    }
    else
    {
        configPRINTF( ( "ERROR:  MQTT Echo demo could not subscribe to %s\r\n", MQTT_SUBSCRIBE_TOPIC ) );
        xReturn = pdFAIL;
    }

    return xReturned;
}

static MQTTBool_t mqtt_config_subs_callback(void * pvUserData, const MQTTPublishData_t * const pxCallbackParams){
    char cBuffer[ MQTT_MAX_DATA_LENGTH];
    uint32_t ulBytesToCopy = ( MQTT_MAX_DATA_LENGTH - 1 );

    /* Remove warnings about the unused parameters. */
    ( void ) pvUserData;

    if(memcmp(pxCallbackParams->pucTopic, MQTT_HEARTBEAT_TOPIC, (size_t)(pxCallbackParams->usTopicLength)) == 0){
        if( pxCallbackParams->ulDataLength <= ulBytesToCopy ){
            ulBytesToCopy = pxCallbackParams->ulDataLength;

            memset( cBuffer, 0x00, sizeof( cBuffer ) );
            memcpy( cBuffer, pxCallbackParams->pvData, ( size_t ) ulBytesToCopy );

            printf("HEARTBEAT\n");
            mqtt_config_process_heartbeat(cBuffer);
        }
    }
    else if(memcmp(pxCallbackParams->pucTopic, MQTT_SUBSCRIBE_TOPIC, (size_t)(pxCallbackParams->usTopicLength)) == 0){
        if( pxCallbackParams->ulDataLength <= ulBytesToCopy ){
            ulBytesToCopy = pxCallbackParams->ulDataLength;

            memset( cBuffer, 0x00, sizeof( cBuffer ) );
            memcpy( cBuffer, pxCallbackParams->pvData, ( size_t ) ulBytesToCopy );

            printf("OUTPUT\n");
            mqtt_config_process_output(cBuffer); 
        } 
    }

    /* The data was copied into the FreeRTOS message buffer, so the buffer
     * containing the data is no longer required.  Returning eMQTTFalse tells the
     * MQTT agent that the ownership of the buffer containing the message lies with
     * the agent and it is responsible for freeing the buffer. */
    return eMQTTFalse;
}

MQTTAgentHandle_t mqtt_config_get_handler(){
    return xMQTTHandle;
}

static BaseType_t mqtt_events_callback (void * pvUserData, const MQTTAgentCallbackParams_t * const pxCallbackParams){
    MQTTAgentEvent_t xMQTTEvent = pxCallbackParams->xMQTTEvent;
    switch(xMQTTEvent){
        case eMQTTAgentPublish:
            printf("eMQTTAgentPublish\n");
            break;
        case eMQTTAgentDisconnect:
            flags_reset_mqtt_connected();
            printf("eMQTTAgentDisconnect\n");
            break;
    }
    return pdTRUE;
}

/*

Callback
You can specify an optional callback that is invoked whenever the MQTT agent is disconnected from the broker or whenever 
a publish message is received from the broker. The received publish message is stored in a buffer taken from the 
central buffer pool. This message is passed to the callback. This callback runs in the context of the MQTT task and 
therefore must be quick. If you need to do longer processing, you must take the ownership of the buffer by returning 
pdTRUE from the callback. You must then return the buffer back to the pool whenever you are done by calling 
FreeRTOS_Agent_ReturnBuffer.

typedef enum
{
    eMQTTAgentPublish,   //< A Publish message was received from the broker. 
    eMQTTAgentDisconnect //< The connection to the broker got disconnected. 
} MQTTAgentEvent_t;
*/


/**
 * @brief Signature of the callback registered by the user to get notified of various events.
 *
 * The user can register an optional callback to get notified of various events.
 *
 * @param[in] pvUserData The user data as provided in the connect parameters while connecting.
 * @param[in] pxCallbackParams The event and related data.
 *
 * @return The return value is ignored in all other cases except publish (i.e. eMQTTAgentPublish
 * event):
 * 1. If pdTRUE is returned - The ownership of the buffer passed in the callback (xBuffer
 * in MQTTPublishData_t) lies with the user.
 * 2. If pdFALSE is returned - The ownership of the buffer passed in the callback (xBuffer
 * in MQTTPublishData_t) remains with the library and it is recycled as soon as
 * the callback returns.<br>
 * The user should take the ownership of the buffer containing the received message from the
 * broker by returning pdTRUE from the callback if the user wants to use the buffer after
 * the callback is over. The user should return the buffer whenever done by calling the
 * MQTT_AGENT_ReturnBuffer API.
 *
 * @see MQTTAgentCallbackParams_t.
 */


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

    if(curr_timestamp - timestamp_sent > 60){ // Envia heartbeat cada 60 segundos
        timestamp_sent = curr_timestamp;
        mqtt_config_send_heartbeat(timestamp_sent);
    }

    if((timestamp_sent > timestamp_received) && (timestamp_sent - timestamp_received > 240)){ // Reiniciar
        printf("Deberia reiniciar ahora\n");
        esp_restart();
    }
}