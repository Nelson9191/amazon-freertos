#ifndef _ACUA_GPRS_H_
#define _ACUA_GPRS_H_

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

enum eGPRSStatus{
    GPRS_OK,
    GPRS_ERROR,
    GPRS_HARDWARE_ERROR,
    GPRS_NO_RESPONSE
};

struct BuffResponse{
    uint8_t * buff;
    size_t buff_len;
};

bool acua_gprs_init();

void acua_gprs_task(void * pvParameters);

enum eGPRSStatus acua_gprs_recv(bool loop);


bool acua_gprs_verify_ok(const char * resp, const char * pattern_ok);

size_t acua_gprs_AT_len(const char * AT_command);

bool acua_gprs_send(const char * AT_command, const char * AT_ok);

enum eGPRSStatus acua_gprs_send_command(const char * command, const char * validation_response, int wait_ms, bool force_wait, bool validate_ok);

bool acua_gprs_response_available();

bool acua_gprs_validate_certs();

bool acua_gprs_validate_certs();

void acua_gprs_save_certs();
bool acua_gprs_write_client_cert();

bool acua_gprs_write_client_key();

bool acua_gprs_write_ca_cert();

bool acua_gprs_configure_ssl();

bool acua_gprs_start_mqtt();

bool acua_gprs_subscribe();

bool acua_gprs_publish(const char * topic, const char * msg);

void acua_gprs_disconnect_and_release();

void acua_gprs_delete_files();

bool acua_gprs_send_file(const char * msg);

bool acua_gprs_config_network();

void acua_gprs_coppy_buffer(char * inputBuffet, int bufLen);

struct BuffResponse acua_gprs_interchange_message(const char * msg);

bool acua_gprs_get_hour();

struct BuffResponse acua_gprs_send_buffer(const char * buff, size_t buff_len, int wait_ms, bool force_wait);
#endif