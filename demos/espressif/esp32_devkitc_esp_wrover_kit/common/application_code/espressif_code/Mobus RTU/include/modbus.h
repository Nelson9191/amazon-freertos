

#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "stdio.h"
#include "stdint-gcc.h"

#define byte        uint8_t
#define TRUE        1
#define FALSE       0       




#define MODBUS_TYPE_MASTER  99999
#define MODBUS_TYPE_SLAVE   88888
#define MODBUS_INT_RDA      77777
#define MODBUS_INT_RDA2     66666
#define MODBUS_INT_EXT      55555
#define MODBUS_SERIAL       COM1

#ifndef MODBUS_TYPE
#define MODBUS_TYPE     MODBUS_TYPE_MASTER
#endif

#ifndef MODBUS_SERIAL_INT_SOURCE
#define MODBUS_SERIAL_INT_SOURCE    MODBUS_INT_EXT  // Seleccionar entre una interrupció externa
#endif// o una interrupció sèrie assíncrona 

#ifndef MODBUS_SERIAL_BAUD
#define MODBUS_SERIAL_BAUD 115200
#endif

#ifndef MODBUS_SERIAL_RX_PIN
#define MODBUS_SERIAL_RX_PIN      GPIO_NUM_16  // Pin de rebre dades 
#endif

#ifndef MODBUS_SERIAL_TX_PIN
#define MODBUS_SERIAL_TX_PIN       GPIO_NUM_17 // Pin de transmetre dades 
#endif

#ifndef MODBUS_SERIAL_ENABLE_PIN
#define MODBUS_SERIAL_ENABLE_PIN   GPIO_NUM_7   // Pin DE de control.  RX low, TX high.
#endif

#ifndef MODBUS_SERIAL_RX_ENABLE
#define MODBUS_SERIAL_RX_ENABLE    0   // Controls RE pin.  Should keep low.
#endif

#ifndef MODBUS_SERIAL_TIMEOUT
#define MODBUS_SERIAL_TIMEOUT      10000     //in us
#endif


/*
#if( MODBUS_SERIAL_INT_SOURCE == MODBUS_INT_RDA )
#use rs232(baud=MODBUS_SERIAL_BAUD, UART1, parity=N,stream=MODBUS_SERIAL, errors)
#define RCV_OFF() {disable_interrupts(INT_RDA);}
#elif( MODBUS_SERIAL_INT_SOURCE == MODBUS_INT_RDA2 )
#use rs232(baud=MODBUS_SERIAL_BAUD, UART2, parity=N,stream=MODBUS_SERIAL, error)
#define RCV_OFF() {disable_interrupts(INT_RDA2);}
#elif( MODBUS_SERIAL_INT_SOURCE == MODBUS_INT_EXT )
#use rs232(baud=MODBUS_SERIAL_BAUD, xmit=MODBUS_SERIAL_TX_PIN, rcv=MODBUS_SERIAL_RX_PIN, parity=N, stream=MODBUS_SERIAL, disable_ints)
#define RCV_OFF() {disable_interrupts(INT_EXT);}
#else
#error Please define a correct interrupt source
#endif

//*/

#ifndef MODBUS_SERIAL_RX_BUFFER_SIZE
#define MODBUS_SERIAL_RX_BUFFER_SIZE    64      //tamany de enviar/rebre buffer
#endif


#if(MODBUS_TYPE == MODBUS_TYPE_MASTER)
int32_t modbus_serial_wait=MODBUS_SERIAL_TIMEOUT;


#define MODBUS_SERIAL_WAIT_FOR_RESPONSE()\
{\
   if(address)\
    {\
        while(!modbus_kbhit() && --modbus_serial_wait)\
        ets_delay_us(1);\
        if(!modbus_serial_wait)\
        modbus_rx.error=TIMEOUT;\
    }\
    modbus_serial_wait = MODBUS_SERIAL_TIMEOUT;\
    }
#endif


#if(MODBUS_SERIAL_INT_SOURCE != MODBUS_INT_EXT)
#byte TXSTA=getenv("sfr:TXSTA")
#bit TRMT=TXSTA.1


#define WAIT_FOR_HW_BUFFER()\
{\
    while(!TRMT);\
}   
#endif

int modbus_serial_new=0;


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
/********************************************************************
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

enum
{
    MODBUS_GETADDY=0,
    MODBUS_GETFUNC=1, 
    MODBUS_GETDATA=2
    } 
    
    modbus_serial_state = 0;
    union
    {
        int8_t b[2];  //queden definits 2 bytes b0 i b1
        int16_t d;    //Defineix un Word de 16 bits
        
        } modbus_serial_crc;



/********************************************************************
 * Aquest struct és un conjunt de bytes que modifiquem en la recepció 
 * La funció struct ens serveix per tenir variables ordenades per grups
 *  ********************************************************************/

struct
{
    int8_t address;
    int8_t len;                                //number of bytes in the message received
    function func;                           //the function of the message received
    exception error;                         //error recieved, if any
    int8_t data[MODBUS_SERIAL_RX_BUFFER_SIZE]; //data of the message received
    } modbus_rx;




/* Taula del càlcul del CRC_high-order byte*/
const unsigned char modbus_auchCRCHi[] = 
{
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
    0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
    0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
    0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
    0x80,0x41,0x00,0xC1,0x81,0x40
    };
    
    /* Taula del càlcul del CRC_low–order byte */
const char modbus_auchCRCLo[] = 
{
    0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,
    0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,
    0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
    0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,
    0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,
    0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
    0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,
    0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
    0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
    0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,
    0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,
    0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
    0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,
    0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,
    0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
    0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
    0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,
    0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
    0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,
    0x70,0xB0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
    0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
    0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,
    0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,
    0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
    0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
    0x43,0x83,0x41,0x81,0x80,0x40
    };
    
    
    
 

//SHARED Functions: 


uint8_t make8(uint16_t,uint8_t);
void RCV_ON(void);    
void modbus_init(void) ;
void modbus_enable_timeout(int);
void modbus_timeout_now(void); 
void modbus_calc_crc(char );
void incomming_modbus_serial(void); 
void modbus_serial_send_stop(void);
int modbus_kbhit(void);

//MASTER API FUNCTIONS PROTOTYPES:
//All master API functions return 0 on success. 

exception modbus_read_coils(int8_t, int16_t, int16_t);
exception modbus_read_discrete_input(int8_t, int16_t, int16_t);
exception modbus_read_holding_registers(int8_t, int16_t, int16_t);
exception modbus_read_input_registers(int8_t, int16_t, int16_t);
exception modbus_write_single_coil(int8_t,int16_t, int);
exception modbus_write_single_register(int8_t, int16_t, int16_t);
exception modbus_read_exception_status(int8_t);
exception modbus_diagnostics(int8_t, int16_t, int16_t);
exception modbus_get_comm_event_counter(int8_t);
exception modbus_get_comm_event_log(int8_t);
exception modbus_write_multiple_coils(int8_t, int16_t, int16_t,int8_t *);
exception modbus_write_multiple_registers(int8_t, int16_t, int16_t,int16_t *);
exception modbus_report_slave_id(int8_t);
exception modbus_read_file_record(int8_t, int8_t, modbus_read_sub_request *);
exception modbus_write_file_record(int8_t, int8_t, modbus_write_sub_request *);
exception modbus_mask_write_register(int8_t, int16_t ,int16_t , int16_t );
exception modbus_read_write_multiple_registers(int8_t , int16_t ,int16_t , int16_t ,int16_t ,int16_t *);
exception modbus_read_FIFO_queue(int8_t , int16_t );
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
void modbus_diagnostics_rsp(int8_t, int16_t, int16_t);                              //08


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
