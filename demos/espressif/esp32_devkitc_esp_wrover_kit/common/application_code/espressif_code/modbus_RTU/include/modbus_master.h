#ifndef _MODBUS_MASTER_H_
#define _MODBUS_MASTER_H_
//#include "modbus.h"

#define MODBUS_PERIOD_S       1 * 1000   
#define MODBUS_SLAVE_ADDRESS 0x0A


void modbus_master_init(void);
static void _modbus_task(void * pvParameters);


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


#endif