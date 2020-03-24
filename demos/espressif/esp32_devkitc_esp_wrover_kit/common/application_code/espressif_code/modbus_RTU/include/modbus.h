#ifndef _MODBUS_H_
#define _MODBUS_H_

#include <stdint.h>
#include <stdbool.h>
//#include "modbus_master.h"



#define byte        uint8_t
#define MODBUS_TRUE        1
#define MODBUS_FALSE       0       




#define MODBUS_TYPE_MASTER  99999
#define MODBUS_TYPE_SLAVE   88888

#define MODBUS_SERIAL       UART_NUM_1

#ifndef MODBUS_TYPE
#define MODBUS_TYPE     MODBUS_TYPE_MASTER
#endif

#ifndef MODBUS_SERIAL_INT_SOURCE
#define MODBUS_SERIAL_INT_SOURCE    MODBUS_INT_EXT  // Seleccionar entre una interrupció externa
#endif// o una interrupció sèrie assíncrona 

#ifndef MODBUS_SERIAL_BAUD
#define MODBUS_SERIAL_BAUD 57600
#endif

#ifndef MODBUS_SERIAL_RX_PIN
#define MODBUS_SERIAL_RX_PIN      GPIO_NUM_16  // Pin de rebre dades 
#endif

#ifndef MODBUS_SERIAL_TX_PIN
#define MODBUS_SERIAL_TX_PIN       GPIO_NUM_17 // Pin de transmetre dades 
#endif

#ifndef MODBUS_SERIAL_CTS_PIN
#define MODBUS_SERIAL_CTS_PIN       UART_PIN_NO_CHANGE // Pin de transmetre dades 
#endif

#ifndef MODBUS_SERIAL_RTS_PIN
#define MODBUS_SERIAL_RTS_PIN       UART_PIN_NO_CHANGE // Pin de transmetre dades 
#endif

/*#ifndef MODBUS_SERIAL_ENABLE_PIN
#define MODBUS_SERIAL_ENABLE_PIN   GPIO_NUM_7   // Pin DE de control.  RX low, TX high.
#endif //*/

#ifndef MODBUS_SERIAL_RX_ENABLE
#define MODBUS_SERIAL_RX_ENABLE    0   // Controls RE pin.  Should keep low.
#endif

#ifndef MODBUS_SERIAL_TIMEOUT
#define MODBUS_SERIAL_TIMEOUT      10000     //in us
#endif


#ifndef MODBUS_SERIAL_RX_BUFFER_SIZE
#define MODBUS_SERIAL_RX_BUFFER_SIZE    64      //tamany de enviar/rebre buffer
#endif




#define TTW 20




//***************************************************************************

/*TYPEDEF DEFINEIX UN TIPUS. DEFINEIX CADENES DE TEXT,
 CADA CADENA DE TXTQUEDA ASSOCIADA A UN NÚMERO
 ********************************************************************/


typedef enum _exception
{
    ILLEGAL_FUNCTION=1,
    ILLEGAL_DATA_ADDRESS=2,
    ILLEGAL_DATA_VALUE=3,
    SLAVE_DEVICE_FAILURE=4,
    ACKNOWLEDGE=5,
    SLAVE_DEVICE_BUSY=6, 
    MEMORY_PARITY_ERROR=8,
    GATEWAY_PATH_UNAVAILABLE=10,
    GATEWAY_TARGET_NO_RESPONSE=11,
    TIMEOUT=12
} exception;




   typedef struct _modbus_read_sub_request
    {
        int8_t reference_type;
        int16_t file_number;
        int16_t record_number;
        int16_t record_length;
        } modbus_read_sub_request;
        
    typedef struct _modbus_write_sub_request
    {
        int8_t reference_type;
        int16_t file_number;
        int16_t record_number;
        int16_t record_length;
        int16_t data[MODBUS_SERIAL_RX_BUFFER_SIZE-8];
        } modbus_write_sub_request;
  


  /*MODBUS Slave Functions*
 ********************************************************************
 * The following structs are used for read/write_sub_request_rsp.  
 * Thesefunctions take in one of these structs.  Please refer to the MODBUS
 * protocol specification if you do not understand the members of thestructure.
 * ********************************************************************/

typedef struct _modbus_read_sub_request_rsp
{
        int8_t record_length;
        int8_t reference_type;
        int16_t data[((MODBUS_SERIAL_RX_BUFFER_SIZE)/2)-3];
        } modbus_read_sub_request_rsp;
        
typedef struct _modbus_write_sub_request_rsp
{
        int8_t reference_type;
        int16_t file_number;
        int16_t record_number;
        int16_t record_length;
        int16_t data[((MODBUS_SERIAL_RX_BUFFER_SIZE)/2)-8];
        } modbus_write_sub_request_rsp;



//************************************************************************
/********************************************************************
  * Aquestes funcions estan definides en el protocol MODBUS. 
  * Poden ser utilitzades per l'esclau per comprobar la funció d'entrada
  * ********************************************************************/
 
 typedef enum _function
 {
    FUNC_READ_COILS=0x01,
    FUNC_READ_DISCRETE_INPUT=0x02,
    FUNC_READ_HOLDING_REGISTERS=0x03,
    FUNC_READ_INPUT_REGISTERS=0x04,
    FUNC_WRITE_SINGLE_COIL=0x05,
    FUNC_WRITE_SINGLE_REGISTER=0x06,
    FUNC_READ_EXCEPTION_STATUS=0x07,
    FUNC_DIAGNOSTICS=0x08,
    FUNC_GET_COMM_EVENT_COUNTER=0x0B,
    FUNC_GET_COMM_EVENT_LOG=0x0C,
    FUNC_WRITE_MULTIPLE_COILS=0x0F,
    FUNC_WRITE_MULTIPLE_REGISTERS=0x10,
    FUNC_REPORT_SLAVE_ID=0x11,
    FUNC_READ_FILE_RECORD=0x14,
    FUNC_WRITE_FILE_RECORD=0x15,
    FUNC_MASK_WRITE_REGISTER=0x16,
    FUNC_READ_WRITE_MULTIPLE_REGISTERS=0x17,
    FUNC_READ_FIFO_QUEUE=0x18
} function;


// Estats de la recepció de trames MODBUS
// 1r Adreçament
// 2n Codi de funció
// 2r Dades

enum{
    MODBUS_GETADDY=0,
    MODBUS_GETFUNC=1, 
    MODBUS_GETDATA=2
}; 
    


/********************************************************************
 * Aquest struct és un conjunt de bytes que modifiquem en la recepció 
 * La funció struct ens serveix per tenir variables ordenades per grups
 *  ********************************************************************/
typedef struct
{
    int8_t address;
    int8_t len;                                //number of bytes in the message received
    function func;                           //the function of the message received
    exception error;                         //error recieved, if any
    int8_t data[MODBUS_SERIAL_RX_BUFFER_SIZE]; //data of the message received
} modbus_rx_buf_struct;    
    
    
 

//SHARED Functions: 


uint8_t make8(uint16_t,uint8_t);
void RCV_ON(void);    
void modbus_init(void);
void modbus_enable_timeout(int);
void modbus_timeout_now(void); 
void incomming_modbus_serial(void); 
void modbus_serial_send_stop(void);
int modbus_kbhit(void);
void modbus_serial_send_start(uint8_t, uint8_t);
uint16_t CRC16 (uint8_t * puchMsg, int8_t usDataLen);
uint16_t crc_modbus( const unsigned char *input_str, int num_bytes );
uint8_t modbus_read_hw_buffer(uint8_t);



void incomming_modbus_serial_new(void);
//MASTER API FUNCTIONS PROTOTYPES:
//All master API functions return 0 on success. 

uint8_t modbus_read_coils(uint8_t address, int16_t start_address, int16_t quantity, modbus_rx_buf_struct * rx_struct);
uint8_t modbus_read_discrete_input(int8_t, int16_t, int16_t, modbus_rx_buf_struct * );
uint8_t modbus_read_holding_registers(int8_t, int16_t, int16_t, modbus_rx_buf_struct * );
uint8_t modbus_read_input_registers(int8_t, int16_t, int16_t, modbus_rx_buf_struct * );

uint8_t modbus_write_single_coil(int8_t,int16_t, int);
uint8_t modbus_write_single_register(int8_t, int16_t, int16_t);
uint8_t modbus_read_uint8_t_status(int8_t);
uint8_t modbus_diagnostics(int8_t, int16_t, int16_t);
uint8_t modbus_get_comm_event_counter(int8_t);
uint8_t modbus_get_comm_event_log(int8_t);
uint8_t modbus_write_multiple_coils(int8_t, int16_t, int16_t,int8_t *);
uint8_t modbus_write_multiple_registers(int8_t, int16_t, int16_t,int16_t *);
uint8_t modbus_report_slave_id(int8_t);
uint8_t modbus_read_file_record(int8_t, int8_t, modbus_read_sub_request *);
uint8_t modbus_write_file_record(int8_t, int8_t, modbus_write_sub_request *);
uint8_t modbus_mask_write_register(int8_t, int16_t ,int16_t , int16_t );
uint8_t modbus_read_write_multiple_registers(int8_t , int16_t ,int16_t , int16_t ,int16_t ,int16_t *);
uint8_t modbus_read_FIFO_queue(int8_t , int16_t );
//END MASTER API FUNCTIONS PROTYPE

//*************************************************************
//          SLAVE API FUNCTIONS PROTOTYPE
//*************************************************************

void modbus_read_coils_rsp(int8_t, int8_t, int8_t*);                                //01
void modbus_read_discrete_input_rsp(int8_t, int8_t, int8_t *);                      //02
void modbus_read_holding_registers_rsp(int8_t, int8_t, int8_t *);                   //03
void modbus_read_input_registers_rsp(int8_t, int8_t, int8_t *);                     //04
void modbus_write_single_coil_rsp(int8_t, int16_t, int16_t);                        //05
void modbus_write_single_register_rsp(int8_t, int16_t, int16_t);                    //06
void modbus_read_exception_status_rsp(int8_t, int8_t);                              //07
void modbus_diagnostics_rsp(int8_t, int16_t, int16_t);  

void modbus_copy_rx_buffer(modbus_rx_buf_struct * rx_struct);


void modbus_get_comm_event_counter_rsp(int8_t,int16_t,int16_t);                     //0B
void modbus_get_comm_event_log_rsp(int8_t,int16_t,int16_t,int16_t,int8_t*,int8_t);  //0C
void modbus_write_multiple_coils_rsp(int8_t, int16_t, int16_t);                     //0F
void modbus_write_multiple_registers_rsp(int8_t, int16_t, int16_t);                 //10
void modbus_report_slave_id_rsp(int8_t, int8_t,int,int8_t *, int8_t);               //11

void modbus_read_file_record_rsp(int8_t, int8_t, modbus_read_sub_request_rsp *);    //14
void modbus_write_file_record_rsp(int8_t,int8_t,modbus_write_sub_request_rsp *);    //15
void modbus_mask_write_register_rsp(int8_t, int16_t,int16_t, int16_t);              //16
void modbus_read_write_multiple_registers_rsp(int8_t, int8_t,int16_t *);            //17
void modbus_read_FIFO_queue_rsp(int8_t, int16_t, int16_t *);                        //18



//END SLAVE API FUNCTIONS PROTYPE

//******************************EXCEPCIONS*****************************
void modbus_exception_rsp(int8_t, int16_t, exception);


 



//******************************mine **********************
void Modbus_init(void);
void Modbus_params(void);
__uint32_t Modbus_Chechsum(__uint8_t *);

bool Checksum_compare(__uint32_t ,__uint32_t );




#endif
