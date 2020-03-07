#include <stdbool.h>
#include <stdint.h>
#include "stdio.h"
#include <stdint.h>
//#include "modbus.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver/uart.h"
//#include "utils.h"
#include "task_config.h"
#include "gpio_info.h"

#include "modbus_master.h"
#include "gpio_handler.h"

volatile uint8_t Current=0;


volatile uint8_t FSM_State=0; 

volatile bool Valid_data_Flag=MODBUS_FALSE;

#define MODBUS_SLAVES 10

uint8_t MODBUS_SLAVE_ADDRESS[MODBUS_SLAVES]= {MODBUS_SLAVE_0_ADDRESS, MODBUS_SLAVE_1_ADDRESS, MODBUS_SLAVE_2_ADDRESS, MODBUS_SLAVE_3_ADDRESS,MODBUS_SLAVE_4_ADDRESS,MODBUS_SLAVE_5_ADDRESS,MODBUS_SLAVE_6_ADDRESS,MODBUS_SLAVE_7_ADDRESS,MODBUS_SLAVE_8_ADDRESS,MODBUS_SLAVE_9_ADDRESS};

volatile modbus_rx_buf_struct Slaves[Slave_QQTy];  



uart_config_t uart_config = {
    .baud_rate = MODBUS_SERIAL_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

void modbus_master_init(){
   uart_param_config(UART_NUM_1, &uart_config);
   uart_set_pin(UART_NUM_1, MODBUS_SERIAL_TX_PIN, MODBUS_SERIAL_RX_PIN, MODBUS_SERIAL_RTS_PIN, MODBUS_SERIAL_CTS_PIN);
   uart_driver_install(UART_NUM_1, 200*2, 200*2, 0, NULL, 0);
   gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN,0);   //Possem a zero el Tcs del transreceptor

   ( void ) xTaskCreate( _modbus_task,
                              TASK_MODBUS_NAME,
                              TASK_MODBUS_STACK_SIZE,
                              NULL,
                              TASK_MODBUS_PRIORITY,
                              NULL );    

}

static void _modbus_task(void * pvParameters)
{

    printf("Modbus created\n");

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    for(;;)
    {

        FSM_CONTROL();
      /*   printf("Apagó\n");   
          gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN, 0);
         vTaskDelay(5000 / portTICK_PERIOD_MS);
         printf("Prendió\n");
         gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN, 1);

            vTaskDelay(5000 / portTICK_PERIOD_MS);//*/
      /*  read_all_coils();
        
        read_all_inputs();
        vTaskDelay(modbus_timming_ms / portTICK_PERIOD_MS);
        read_all_holding();
        vTaskDelay(modbus_timming_ms / portTICK_PERIOD_MS);
        read_all_input_reg();
        vTaskDelay(modbus_timming_ms / portTICK_PERIOD_MS);    
        
        //write_coils();
        //vTaskDelay(3000 / portTICK_PERIOD_MS);
        //write_regs();
        //vTaskDelay(3000 / portTICK_PERIOD_MS);
       // unknown_func();

        //vTaskDelay(MODBUS_PERIOD_S / portTICK_PERIOD_MS);//*/


    }

}

//*******************************************************************

void FSM_CONTROL(void)
{
   read_all_coils();
   //parse_read(FSM_State++);
   //parse_write(FSM_State);
   vTaskDelay(modbus_timming_ms / portTICK_PERIOD_MS);
   FSM_State=(FSM_State==4)?0:FSM_State+1;

}




//*******************************************************************

void parse_read(uint8_t c)
{
   switch(c)
   {
      case 0:
         read_all_coils();
      
      
      break; 
      case 1:
         read_all_inputs();
      
      
      
      break; 
      case 2:
         read_all_holding();
      
      break; 
      case 3:
         read_all_input_reg();
      break; 

      default:          //put control restore 

      break;
   }
  
}

void parse_write(char c)
{
   switch(c)
   {
      case '5':
         write_coil();
         break;            
      case '6':
         write_reg();
         break;
      case '7':
         write_coils();
         break; 
      case '8':
         write_regs();
         break; 
      case '9':
         //unknown_func();
         break;
   }
}

/*This function may come in handy for you since MODBUS uses MSB first.*/
int8_t swap_bits(int8_t c)
{
   return ((c&1)?128:0)|((c&2)?64:0)|((c&4)?32:0)|((c&8)?16:0)|((c&16)?8:0)
          |((c&32)?4:0)|((c&64)?2:0)|((c&128)?1:0);
}




//***********************************************************************

void read_all_coils(void)
{
   printf("Coils:\r\n");
   
   for (int i = 0; i < MODBUS_SLAVES; i++)
   {
      if(!modbus_read_coils(MODBUS_SLAVE_ADDRESS[0],0,10, &Slaves[i]))
      {
         printf("Data: ");
         /*Started at 1 since 0 is quantity of coils*/
         for(int i=1; i < (Slaves[i].len); ++i)
         {
            printf("%X ", Slaves[i].data[i]);
         }
         printf("\r\n\r\n");
      }
      else
      {
         printf("<-**Exception= %X**->\r\n\r\n", Slaves[i].error);
      }
   }
}


//***********************************************************************

void read_all_inputs(void)
{
   printf("Inputs:\r\n");
   if(!(modbus_read_discrete_input(MODBUS_SLAVE_ADDRESS[Current],0,8)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}


//***********************************************************************



void read_all_holding(void)
{
   printf("Holding Registers:\r\n");
   if(!(modbus_read_holding_registers(MODBUS_SLAVE_ADDRESS[Current],0,8)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}   


//***********************************************************************

void read_all_input_reg(void)
{
   printf("Input Registers:\r\n");
   if(!(modbus_read_input_registers(MODBUS_SLAVE_ADDRESS[Current],0,8)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}


//***********************************************************************




void write_coil(void)
{
   printf("Writing Single Coil:\r\n");
   if(!(modbus_write_single_coil(MODBUS_SLAVE_ADDRESS[Current],6,MODBUS_TRUE)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}



//***********************************************************************



void write_reg(void)
{
   printf("Writing Single Register:\r\n");
   if(!(modbus_write_single_register(MODBUS_SLAVE_ADDRESS[Current],3,0x4444)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}



//***********************************************************************



void write_coils(void)
{
   int8_t coils[1] = { 0x50 };
   printf("Writing Multiple Coils:\r\n");
   if(!(modbus_write_multiple_coils(MODBUS_SLAVE_ADDRESS[Current],0,8,coils)))
  {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}



//***********************************************************************


void write_regs(void)
{
   int16_t reg_array[2] = {0x1111, 0x2222};
   printf("Writing Multiple Registers:\r\n");
   if(!(modbus_write_multiple_registers(MODBUS_SLAVE_ADDRESS[Current],0,2,reg_array)))
   {
      printf("Data: ");
      /*Started at 1 since 0 is quantity of coils*/
      for(int i=1; i < (Slaves[Current].len); ++i)
         printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   } 
}



//***********************************************************************

/*
void unknown_func(void)
{
   printf("Trying unknown function\r\n");
   printf("Diagnostic:\r\n");
   if(!(modbus_diagnostics(MODBUS_SLAVE_ADDRESS[Current],0,0)))
   {
      printf("Data: ");
      printf("%X ", Slaves[Current].data[i]);
      printf("\r\n\r\n");
   }
   else
   {
      printf("<-**Exception= %X**->\r\n\r\n", Slaves[Current].error);
   }
}
*/