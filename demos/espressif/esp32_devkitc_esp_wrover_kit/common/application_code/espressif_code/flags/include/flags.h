#ifndef _FLAGS_H_
#define _FLAGS_H_

#include <stdbool.h>

typedef unsigned int boolean_t;

/*
Union de flags para leer diferentes estados de control a lo largo de todo el programa
*/
typedef union {
    struct{
        boolean_t nvs :                 1;
        boolean_t spiffs :              1;
        boolean_t wifi_connected :      1;
        boolean_t mqtt_connected :      1;
        boolean_t timestamp_captured :  1;
        boolean_t timestamp_failed :    1;
        int not_used :                  26;        
    };
    int raw_flags;
} flags_t;


/*
Inicializa módulo. Crea mutex y limpia flgas de control
@Params:
@Return:
*/
void flags_init();

/*
Revisa si el módulo de NVS se inicializó.
@Params:
@Return: true si módulo inicializado, false de lo contrario
*/
bool flags_is_nvs_ok();

/*
Setea flag de NVS inicializado
@Params:
@Return: 
*/
void flags_set_nvs_ok();

/*
Revisa si módulo de WiFi se inicializó
@Params:
@Return: true si módulo inicializado, false de lo contrario
*/
bool flags_is_wifi_connected();

/*
Setea flag de WiFi inicializado
@Params:
@Return: 
*/
void flags_set_wifi_connected();

/*
Resetea flag de WiFi inicializado
@Params:
@Return: 
*/
void flags_reset_wifi_connected();


bool flags_is_mqtt_connected();

/*
Setea flag de cliente MQTT conectado
@Params:
@Return: 
*/
void flags_set_mqtt_connected();

/*
Resetea flag de MQTT
@Params:
@Return: 
*/
void flags_reset_mqtt_connected();


/*
Revisa si el timestamp fue capturado
@Params:
@Return: true si módulo inicializado, false de lo contrario
*/
bool flags_is_timestamp_captured();

/*
Setea flag de timestamp
@Params:
@Return: 
*/
void flags_set_timestamp_captured();

/*
Resetea flag de timestamp
@Params:
@Return: 
*/
void flags_reset_timestamp_captured();

/*
Revisa si el la captura del timestamp fue correcta 
@Params:
@Return: true si módulo inicializado, false de lo contrario
*/
bool flags_is_timestamp_failed();

/*
Setea flag de timestamp fail
@Params:
@Return: 
*/
void flags_set_timestamp_failed();

/*
Resetea flag de timestamp fail
@Params:
@Return: 
*/
void flags_reset_timestamp_failed();

#endif