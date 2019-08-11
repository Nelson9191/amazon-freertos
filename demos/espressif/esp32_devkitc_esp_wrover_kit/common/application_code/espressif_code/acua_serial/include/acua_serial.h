#ifndef _ACUA_SERIAL_H_
#define _ACUA_SERIAL_H_

#include <stdio.h>
#include <stdbool.h>

enum eGPRSStatus{
    GPRS_OK,
    GPRS_ERROR,
    GPRS_HARDWARE_ERROR,
    GPRS_NO_RESPONSE
};

void acua_serial_init();

void acua_serial_task(void * pvParameters);

bool acua_serial_verify_status();


bool acua_serial_verify_ok(const char * resp, const char * pattern_ok);

size_t acua_serial_AT_len(const char * AT_command);

bool acua_serial_send(const char * AT_command, const char * AT_ok);

enum eGPRSStatus acua_serial_send_command(const char * command, const char * validation_response, int wait_ms);

bool acua_serial_response_available();

#endif