#ifndef _MODBUS_MASTER_H_
#define _MODBUS_MASTER_H_
#include "modbus.h"

#define MODBUS_PERIOD_S       1 * 1000   


#define modbus_timming_ms       3*1000      


#define MODBUS_SLAVE_0_ADDRESS    0x00
#define MODBUS_SLAVE_1_ADDRESS    0x01
#define MODBUS_SLAVE_2_ADDRESS    0x02
#define MODBUS_SLAVE_3_ADDRESS    0x03
#define MODBUS_SLAVE_4_ADDRESS    0x04
#define MODBUS_SLAVE_5_ADDRESS    0x05
#define MODBUS_SLAVE_6_ADDRESS    0x06
#define MODBUS_SLAVE_7_ADDRESS    0x07
#define MODBUS_SLAVE_8_ADDRESS    0x08
#define MODBUS_SLAVE_9_ADDRESS    0x09

volatile uint8_t Current=0;
//volatile uint8_t FSM_State=0;

uint8_t MODBUS_SLAVE_ADDRESS[]= {MODBUS_SLAVE_0_ADDRESS, MODBUS_SLAVE_1_ADDRESS, MODBUS_SLAVE_2_ADDRESS, MODBUS_SLAVE_3_ADDRESS,MODBUS_SLAVE_4_ADDRESS,MODBUS_SLAVE_5_ADDRESS,MODBUS_SLAVE_6_ADDRESS,MODBUS_SLAVE_7_ADDRESS,MODBUS_SLAVE_8_ADDRESS,MODBUS_SLAVE_9_ADDRESS};

#define Slave_QQTy          1


volatile modbus_rx Slaves[Slave_QQTy];


void modbus_master_init(void);
static void _modbus_task(void * pvParameters);

void FSM_CONTROL(void);
void Print_Function_Debug(void);
int8_t swap_bits(int8_t);
void read_all_coils(void);
void read_all_inputs(void);
void read_all_holding(void);
void read_all_input_reg(void);
void write_coil(void);
void write_reg(void);
void write_coils(void);
void write_regs(void);
void unknown_func(void);

void parse_read(char );
void parse_write(char );


#endif