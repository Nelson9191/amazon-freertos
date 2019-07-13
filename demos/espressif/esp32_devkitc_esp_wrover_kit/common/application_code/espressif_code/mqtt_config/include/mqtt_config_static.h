#ifndef _MQTT_CONFIG_STATIC_H_
#define _MQTT_CONFIG_STATIC_H_

#include "FreeRTOS.h"
#include "aws_mqtt_agent.h"

static BaseType_t mqtt_config_connect_to_broker( void );

static BaseType_t mqtt_config_subscribe_to_output(void);

static BaseType_t mqtt_config_subscribe_to_heartbeat(void);

/*
 * Función callback cuando el cliente recibe datos del broker     
 */
static MQTTBool_t mqtt_config_subs_callback(void * pvUserData, const MQTTPublishData_t * const pxCallbackParams);

static BaseType_t mqtt_events_callback (void * pvUserData, const MQTTAgentCallbackParams_t * const pxCallbackParams);

static MQTTAgentReturnCode_t mqtt_config_connect();

static MQTTAgentReturnCode_t mqtt_config_create( void );

static void mqtt_config_delete();

static void mqtt_config_disconnect();

static void mqtt_config_verify_heartbeat();

static void mqtt_config_process_output(const char * cBuffer);

static void mqtt_config_process_heartbeat(const char * cBuffer);


#endif