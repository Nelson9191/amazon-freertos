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

#define BUF_SIZE 2000


volatile uint8_t FSM_State=0; 

volatile bool Valid_data_Flag=MODBUS_FALSE;


bool modbus_read_hw_buffer()
{
        uint8_t index = 0;
        uint16_t broke = 0;
        int len = 0;
        bool ok = false;
        Valid_data_Flag=0;

        uint8_t * Mod_buffer = malloc(sizeof(uint8_t) * MODBUS_SERIAL_RX_BUFFER_SIZE);

        if (!Mod_buffer)
        {
                Slaves[Current].error = TIMEOUT; //Crear código para malloc
                return false;
        }
        memset(Mod_buffer, '\0', sizeof(uint8_t) * MODBUS_SERIAL_RX_BUFFER_SIZE);

        while ((++broke<15) && (len<=0))
        {
                vTaskDelay(100 / portTICK_PERIOD_MS);
                uart_get_buffered_data_len(UART_NUM_1, (size_t*)&len);
                if(len){
                        len = uart_read_bytes(UART_NUM_1,Mod_buffer, len, 100);
                }
        }
        
        printf("Len= %d \n str: %*.s\n",len, len, Mod_buffer);

        if(len <= 0)
        {
                Slaves[Current].error = TIMEOUT;
                printf("Modbus Timed out \n");
                ok = false;
        }
        else
        {
                Slaves[Current].error = 0;
                Slaves[Current].address = Mod_buffer[0];
                Slaves[Current].func = Mod_buffer[1];


               
                for (int i = 0; i < len - 4; i++){
                        Slaves[Current].data[i] = Mod_buffer[i + 2];
                        printf("-%x", Mod_buffer[i + 2]);
                }
                 //****************************** leer exepciones
                if( Slaves[Current].func & 0x80) 
                {
                        Slaves[Current].error = Slaves[Current].data[0];
                        Slaves[Current].len=1;
                }
                //***********************************************
                
                printf("\n");
                uint16_t CRC_RCV = (uint16_t)((Mod_buffer[len-2])<<8 )| (Mod_buffer[len-1]);
                uint16_t CRC_RTN = CRC16 (Mod_buffer, len - 2);
                Valid_data_Flag = (CRC_RTN == CRC_RCV)? MODBUS_TRUE:MODBUS_FALSE;
                printf("Recv checksum: %2x %2x\n", Mod_buffer[len-2], Mod_buffer[len-1]);
                printf("Calc checksum: %2x %2x\n", (uint8_t)(CRC_RTN>>8), (uint8_t)(CRC_RTN));
                       
                if(Valid_data_Flag==1)
                {
                         Slaves[Current].len=len-4;
                      //  printf("Recv checksum: %2x %2x\n", Mod_buffer[len-2], Mod_buffer[len-1]);
                      //  printf("Calc checksum: %2x %2x\n", (uint8_t)(CRC_RTN>>8), (uint8_t)(CRC_RTN));
                        //printf("Data REceived: \n");  
                        //printf("--  %.*s [%d]\n", (len > 2 ? len - 2 : len), buffer, len);
                        ok = CRC_RTN == CRC_RCV;   
                }
                else
                {
                        printf("No valid Data flag \n");
                       memset(  Slaves[Current].data,'\0',len-4); 
                        Slaves[Current].len=0;   
                }
        }

        free(Mod_buffer);
        return ok;
}   



uart_config_t uart_config = {
    .baud_rate = 9600,                  //9600
    .data_bits = UART_DATA_8_BITS,     
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_2,        //2
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

void modbus_master_init(){
   uart_param_config(UART_NUM_1, &uart_config);
   uart_set_pin(UART_NUM_1, MODBUS_SERIAL_TX_PIN, MODBUS_SERIAL_RX_PIN, MODBUS_SERIAL_RTS_PIN, MODBUS_SERIAL_CTS_PIN);
   uart_driver_install(UART_NUM_1, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);
   gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN,0);   //Possem a zero el Tcs del transreceptor

   // RCV_ON();                               //crida aquesta funció per activar la recepció
   // setup_timer_2(T2_DIV_BY_16,249,5);      //Configuració del Timer 2 per interrupció ~4ms
   //enable_interrupts(GLOBAL);              //Permet les interrupcions globals

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
    Print_Function_Debug();
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



   parse_read(FSM_State);
   parse_write(FSM_State);
   vTaskDelay(modbus_timming_ms / portTICK_PERIOD_MS);

}




//*******************************************************************

void parse_read(char c)
{
   switch(c)
   {
      case '1':
         read_all_coils();
      
      
      break; 
      case '2':
         read_all_inputs();
      
      
      
      break; 
      case '3':
         read_all_holding();
      
      break; 
      case '4':
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
         unknown_func();
         break;
   }
}



void Print_Function_Debug(void)
{
    printf("\r\nPick command to send\r\n1. Read all coils.\r\n");
    printf("2. Read all inputs.\r\n3. Read all holding registers.\r\n");
    printf("4. Read all input registers.\r\n5. Turn coil 6 on.\r\n6. ");
    printf("Write 0x4444 to register 0x03\r\n7. Set 8 coils using 0x50 as mask\r\n");
    printf("8. Set 2 registers to 0x1111, 0x2222\r\n9. Send unknown command\r\n");
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
   if(!modbus_read_coils(MODBUS_SLAVE_ADDRESS[Current],0,8))
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
}//*/
