
#include "modbus.h"
//#include "modbus_master.h"
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver/uart.h"
#include "gpio_handler.h"
#include "gpio_info.h"
//#include "utils.h"



#define BUF_SIZE 2000

int modbus_serial_new=0;

int modbus_serial_state = MODBUS_GETADDY;


uint8_t Modbus_RX_BUFFER[MODBUS_SERIAL_RX_BUFFER_SIZE];



int len=0;        
       // memset(data_buffer, '\0', MODBUS_SERIAL_RX_BUFFER_SIZE+10);


union{
    int8_t b[2];  //queden definits 2 bytes b0 i b1
    int16_t d;    //Defineix un Word de 16 bits
} modbus_serial_crc;

 


uint16_t wCRCTable[] = {
0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };

//#if(MODBUS_TYPE == MODBUS_TYPE_MASTER)
int32_t modbus_serial_wait=MODBUS_SERIAL_TIMEOUT;


uint16_t CRC16 (uint8_t * puchMsg, int8_t usDataLen )
{
    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;

   while (usDataLen--)
   {
      nTemp = *puchMsg++ ^ wCRCWord;
      wCRCWord >>= 8;
      wCRCWord ^= wCRCTable[nTemp];
   }
   return wCRCWord;
    //modbus_serial_crc.b[1] = wCRCWord >> 8;
    //modbus_serial_crc.b[0] = (uint8_t)wCRCWord; 
}


// Objectiu:    Posa un byte a la UART per poder enviar trames
// Entrades:    Caràcter
// Sortides:    Cap

void modbus_serial_putc(int8_t c)
{
    char * ptr = (char *)&c;
    uart_write_bytes(UART_NUM_1, ptr, 1);
    //fputc(c, MODBUS_SERIAL);   //Enviem el byte C a la UART //todo
    //modbus_calc_crc(c);        //Enviem el byte C a la funció per calcular el CRC (data)
    ets_delay_us(10000000/MODBUS_SERIAL_BAUD); //Retard perquè el receptor tingui temps de calcular el CRC
}




void modbus_serial_send_stop(void)
{
        int8_t crc_low, crc_high;
        
        crc_high = modbus_serial_crc.b[1];  //Guarda el valor del Checksum MSB
        crc_low = modbus_serial_crc.b[0];   //Guarda el valor del checksum LSB
        //Per defecte es calcula el CRC d'aquests dos valors però enviarem a la UART
        //directement el valor CRC high i low.
        modbus_serial_putc(crc_high);   // Enviem aquests valors a la UART 
        modbus_serial_putc(crc_low);
        
        gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN, 0);
        ets_delay_us(3500000/MODBUS_SERIAL_BAUD); //3.5 character delay
        modbus_serial_crc.d=0xFFFF;
}



/*
// Objectiu:
Enviar missastge pel bus RS485
// Entrades:
1) L'adreçament del receptor (to)
//
2) Nombre de bytes de dades a enviar
//
3) Un punter per les dades a enviar
//
4) La llargada de les dades
// Sortides:
MODBUS_TRUE és correcte
//
MODBUS_FALSE si hi ha fallida
// Nota:
Format: font | destinació | llargada-dades | dades | checksum
*/

void modbus_serial_send_start(int8_t to, int8_t func)
{
    modbus_serial_crc.d = 0xFFFF;//Inicialitza el CRC
    
    modbus_serial_new = MODBUS_FALSE;//re-inicializa el bit de nueva trama
    
    //RCV_OFF();//Parem la Recepció (off)
    
//#if (MODBUS_SERIAL_ENABLE_PIN!=0)
    gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN, 1);
//#endif
    ets_delay_us(3500000/MODBUS_SERIAL_BAUD); //3.5 character delay (caràcters de retràs
    modbus_serial_putc(to);//Envia les dades a la UART
    modbus_serial_putc(func);
}


uint8_t make8(uint16_t Tobytes,uint8_t Pos)
{
        uint8_t Aux = 0;

        switch(Pos)
        {
                case 0:
                        Aux=(uint8_t)(Tobytes);
                        break;

                case 1:
                        Aux=(uint8_t)(Tobytes>>8);
                        break;        
        }
        return Aux;
}
//              MODBUS MASTER FUNCTIONS      


#if(MODBUS_TYPE==MODBUS_TYPE_MASTER)

/*
read_coils    
        Input:  int8       address   Slave Address     
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
        
*/


bool modbus_read_coils(int8_t address, int16_t start_address, int16_t quantity)
{
        bool ok;
        uint8_t msg[] = {address, FUNC_READ_COILS, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)};

        modbus_serial_send_start(address, FUNC_READ_COILS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        
        uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();

        return ok;
}



/*
        read_discrete_input
        Input:  int8       address            Slave Address
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
        
*/


bool modbus_read_discrete_input(int8_t address, int16_t start_address, int16_t quantity)
{       

        bool ok;
        uint8_t msg[] = {address, FUNC_READ_DISCRETE_INPUT, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)};    

        modbus_serial_send_start(address, FUNC_READ_DISCRETE_INPUT);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));

        uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
         ok = modbus_read_hw_buffer();
        return ok;
        
}




/*

read_holding_registers
        Input:  int8       address            Slave Address
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output: exception                     0 if no error, else the exception*/
        
bool modbus_read_holding_registers(int8_t address, int16_t start_address, int16_t quantity)
{
        bool ok;
        uint8_t msg[] = {address, FUNC_READ_HOLDING_REGISTERS, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)};

        modbus_serial_send_start(address, FUNC_READ_HOLDING_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));

         uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
       // MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        ok = modbus_read_hw_buffer();
        return ok;
}



/*

        read_input_registers
        Input:     int8       address            Slave Address
        int16      start_address      Address to start reading from
        int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
*/
bool modbus_read_input_registers(int8_t address, int16_t start_address, int16_t quantity)
{
        bool ok;
        uint8_t msg[] = {address, FUNC_READ_INPUT_REGISTERS, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)}; 

        modbus_serial_send_start(address, FUNC_READ_INPUT_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));

         uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
      /*  MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;//*/
        ok = modbus_read_hw_buffer();
        return ok;
        
}




/*

        write_single_coil
        Input:     int8       address            Slave Address
        int16      output_address     Address to write into
        int1       on                 MODBUS_TRUE for on, MODBUS_FALSE for off
        Output:    exception                     0 if no error, else the exception
        
*/
uint8_t modbus_write_single_coil(int8_t address, int16_t output_address, int on)
{
        uint8_t ok;
        uint8_t msg[] = {address, FUNC_WRITE_SINGLE_COIL, 
                        make8(output_address,1), make8(output_address,0), 
                        make8(output_address,1), make8(output_address,0)};

        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_COIL);
        modbus_serial_putc(make8(output_address,1));
        modbus_serial_putc(make8(output_address,0));

        if(on)
                modbus_serial_putc(0xFF);
        else
                modbus_serial_putc(0x00);
        
        modbus_serial_putc(0x00);

         uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
       ok = modbus_read_hw_buffer();
        return ok;
}


/*
        write_single_register
        Input:  int8       address            Slave Address
                int16      reg_address        Address to write into
                int16      reg_value          Value to write
        Output:    exception                     0 if no error, else the exception
*/
uint8_t modbus_write_single_register(int8_t address, int16_t reg_address, int16_t reg_value)
{
        uint8_t ok;
        uint8_t msg[] = {address, FUNC_WRITE_SINGLE_REGISTER, 
                        make8(reg_address,1), make8(reg_address,0), 
                        make8(reg_value,1), make8(reg_value,0)}; 
                
        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_REGISTER);
        modbus_serial_putc(make8(reg_address,1));
        modbus_serial_putc(make8(reg_address,0));
        modbus_serial_putc(make8(reg_value,1));
        modbus_serial_putc(make8(reg_value,0));

         uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}



/*
        read_exception_status
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
        
*/

uint8_t modbus_read_exception_status(int8_t address)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_READ_EXCEPTION_STATUS);
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}




/*
        diagnostics
        Input:     int8       address            Slave Address
        int16      sub_func           Subfunction to send
        int16      data               Data to send, changes based on subfunction
        Output:    exception                     0 if no error, else the exception
*/

uint8_t modbus_diagnostics(int8_t address, int16_t sub_func, int16_t data)
{
        uint8_t ok;
        uint8_t msg[] = {address, FUNC_DIAGNOSTICS, 
                        make8(sub_func,1), make8(sub_func,0), 
                        make8(data,1), make8(data,0)}; 

        modbus_serial_send_start(address, FUNC_DIAGNOSTICS);
        modbus_serial_putc(make8(sub_func,1));
        modbus_serial_putc(make8(sub_func,0));
        modbus_serial_putc(make8(data,1));
        modbus_serial_putc(make8(data,0));

        uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}

/*
        get_comm_event_couter
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/

uint8_t modbus_get_comm_event_counter(int8_t address)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_COUNTER);
        modbus_serial_send_stop();
       ok = modbus_read_hw_buffer();
        return ok;
}

/*
        get_comm_event_log
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/
uint8_t modbus_get_comm_event_log(int8_t address)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_LOG);
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}



/*
        write_multiple_coils
        Input:     int8       address            Slave Address
        int16      start_address      Address to start at
        int16      quantity           Amount of coils to write to
        int1*      values             A pointer to an array holding the values to write
        Output:    exception                     0 if no error, else the exception
*/
uint8_t modbus_write_multiple_coils(int8_t address, int16_t start_address, int16_t quantity,int8_t *values)
{
        uint8_t ok;
        uint8_t msg[] = {address, FUNC_WRITE_MULTIPLE_COILS, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)}; 

        int8_t i,count;count = (int8_t)((quantity/8));
        if(quantity%8)
                count++;      
        modbus_serial_send_start(address, FUNC_WRITE_MULTIPLE_COILS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_putc(count);
        for(i=0; i < count; ++i) 
                modbus_serial_putc(values[i]);

        uint16_t CRC_RTN=CRC16 (msg, 6);
        modbus_serial_crc.b[1] = CRC_RTN >> 8;
        modbus_serial_crc.b[0] = (uint8_t)CRC_RTN; 
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}


/* 
        write_multiple_registers
        Input:     int8       address            Slave Address
        int16      start_address      Address to start at
        int16      quantity           Amount of coils to write to
        int16*     values             A pointer to an array holding the data to write
        Output:    exception                     0 if no error, else the exception
*/
uint8_t modbus_write_multiple_registers(int8_t address, int16_t start_address, int16_t quantity,int16_t *values)
{
        uint8_t ok;
        uint8_t msg[] = {address, FUNC_WRITE_MULTIPLE_REGISTERS, 
                        make8(start_address,1), make8(start_address,0), 
                        make8(quantity,1), make8(quantity,0)};         
        int8_t i,count;
        count = quantity*2;
        modbus_serial_send_start(address, FUNC_WRITE_MULTIPLE_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_putc(count);
        for(i=0; i < quantity; ++i)
        {
                modbus_serial_putc(make8(values[i],1));
                modbus_serial_putc(make8(values[i],0));
        }

        CRC16(msg, 6);
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}

/*
        report_slave_id
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/
uint8_t modbus_report_slave_id(int8_t address)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_REPORT_SLAVE_ID);
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
        
}

/*
        read_file_record
        Input:  int8                address            Slave Address
                int8                byte_count         Number of bytes to read
                read_sub_request*   request      Structure holding record information
        Output:    exception                              0 if no error, else the exception
*/
uint8_t modbus_read_file_record(int8_t address, int8_t byte_count, modbus_read_sub_request *request)
{
        uint8_t ok;
        int8_t i;modbus_serial_send_start(address, FUNC_READ_FILE_RECORD);
        modbus_serial_putc(byte_count);
        for(i=0; i < (byte_count/7); i+=7)
        {
                modbus_serial_putc(request->reference_type);
                modbus_serial_putc(make8(request->file_number, 1));
                modbus_serial_putc(make8(request->file_number, 0));
                modbus_serial_putc(make8(request->record_number, 1));
                modbus_serial_putc(make8(request->record_number, 0));
                modbus_serial_putc(make8(request->record_length, 1));
                modbus_serial_putc(make8(request->record_length, 0));
                request++;
        }
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}


/*
        write_file_record
        Input:     int8                address            Slave Address
        int8                byte_count         Number of bytes to read
        read_sub_request*   request            Structure holding record/data information
        Output:    exception                              0 if no error, else the exception
*/
uint8_t modbus_write_file_record(int8_t address, int8_t byte_count, modbus_write_sub_request *request)
{
        uint8_t ok;
        int8_t i, j=0;
        modbus_serial_send_start(address, FUNC_WRITE_FILE_RECORD);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; i+=(7+(j*2)))
        {
                modbus_serial_putc(request->reference_type);
                modbus_serial_putc(make8(request->file_number, 1));
                modbus_serial_putc(make8(request->file_number, 0));
                modbus_serial_putc(make8(request->record_number, 1));
                modbus_serial_putc(make8(request->record_number, 0));
                modbus_serial_putc(make8(request->record_length, 1));
                modbus_serial_putc(make8(request->record_length, 0));
                for(j=0; (j < request->record_length) && (j < MODBUS_SERIAL_RX_BUFFER_SIZE-8); j+=2)
                {
                        modbus_serial_putc(make8(request->data[j], 1));
                        modbus_serial_putc(make8(request->data[j], 0));
                }request++;
        }
        modbus_serial_send_stop();
       ok = modbus_read_hw_buffer();
        return ok;
}

/*
        mask_write_register
        Input:     int8       address            Slave Address
        int16      reference_address  Address to mask
        int16      AND_mask           A mask to AND with the data at reference_address
        int16      OR_mask            A mask to OR with the data at reference_address
        Output:    exception                              0 if no error, else the exception
*/
uint8_t modbus_mask_write_register(int8_t address, int16_t reference_address,int16_t AND_mask, int16_t OR_mask)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_MASK_WRITE_REGISTER);
        modbus_serial_putc(make8(reference_address,1));
        modbus_serial_putc(make8(reference_address,0));
        modbus_serial_putc(make8(AND_mask,1));
        modbus_serial_putc(make8(AND_mask,0));
        modbus_serial_putc(make8(OR_mask, 1));
        modbus_serial_putc(make8(OR_mask, 0));
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}



/*
        read_write_multiple_registers
        Input:     int8       address                Slave Address
        int16      read_start             Address to start reading
        int16      read_quantity          Amount of registers to read
        int16      write_start            Address to start writing
        int16      write_quantity         Amount of registers to write
        int16*     write_registers_value  Pointer to an aray us to write
        Output:    exception                         0 if no error, else the exception
*/
uint8_t modbus_read_write_multiple_registers(int8_t address, int16_t read_start,int16_t read_quantity, int16_t write_start,int16_t write_quantity,int16_t *write_registers_value)
{
        uint8_t ok;
        int8_t i;
        modbus_serial_send_start(address, FUNC_READ_WRITE_MULTIPLE_REGISTERS);
        modbus_serial_putc(make8(read_start,1));
        modbus_serial_putc(make8(read_start,0));
        modbus_serial_putc(make8(read_quantity,1));
        modbus_serial_putc(make8(read_quantity,0));
        modbus_serial_putc(make8(write_start, 1));
        modbus_serial_putc(make8(write_start, 0));
        modbus_serial_putc(make8(write_quantity, 1));
        modbus_serial_putc(make8(write_quantity, 0));
        modbus_serial_putc((int8_t)(2*write_quantity));
        for(i=0; i < write_quantity ; i+=2)
        {
                modbus_serial_putc(make8(write_registers_value[i], 1));
                modbus_serial_putc(make8(write_registers_value[i+1], 0));
        }
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}


/*
        read_FIFO_queue
        Input:     int8       address           Slave Address
        int16      FIFO_address      FIFO address
        Output:    exception                    0 if no error, else the exception
*/
uint8_t modbus_read_FIFO_queue(int8_t address, int16_t FIFO_address)
{
        uint8_t ok;
        modbus_serial_send_start(address, FUNC_READ_FIFO_QUEUE);
        modbus_serial_putc(make8(FIFO_address, 1));
        modbus_serial_putc(make8(FIFO_address, 0));
        modbus_serial_send_stop();
        ok = modbus_read_hw_buffer();
        return ok;
}
        

//END MODBUS Master Functions        
#else







/********************************************************************
 * The following slave functions are defined in the MODBUS protocol.
 * Please refer to http://www.modbus.org for the purpose of each ofthese.  
 * All functions take the slaves address as their firstparameter.
 * ********************************************************************/

//****************************CODI 1****************************************/
/*
        read_coils_rsp
        Input:  int8       address            Slave Address
                int8       byte_count         Number of bytes being sent
                int8*      coil_data          Pointer to an array of data to send
                Output:    void
        
*/

void modbus_read_coils_rsp(int8_t address, int8_t byte_count, int8_t* coil_data)
{
        int8_t i;modbus_serial_send_start(address, FUNC_READ_COILS);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; ++i)
        {
                modbus_serial_putc(*coil_data);
                coil_data++;
        }
        modbus_serial_send_stop();
}


//*************************CODI 2********************************************
/*
        read_discrete_input_rsp
        Input:  int8       address            Slave Address
                int8       byte_count         Number of bytes being sent
                int8*      input_data         Pointer to an array of data to send
        Output:    void
*/

void modbus_read_discrete_input_rsp(int8_t address, int8_t byte_count, int8_t *input_data)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_READ_DISCRETE_INPUT);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; ++i)
        {
                modbus_serial_putc(*input_data);
                input_data++;
        }
        modbus_serial_send_stop();
}


// ***********************CODI 3 LLEGIR**************************************
 /*
 * read_holding_registers_rsp         //Màster vol llegir de l'esclau
 * Input:       int8       address            Adreça de l'esclau
 *              int8       byte_count         Nombre de bytes que s'envien
 *              int8*      reg_data           Punter en un vector de dades a enviar Punter de READ HOLDING REGISTERS
 * Output:    void   
 * */
void modbus_read_holding_registers_rsp(int8_t address, int8_t byte_count, int8_t *reg_data)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_READ_HOLDING_REGISTERS);//Send Start
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; ++i)
        {
                modbus_serial_putc(*reg_data);     //Amb * es refereix al contingut
                reg_data++;                        //Sense * a la posició //Incrementem posició
        }
                modbus_serial_send_stop();           //Send Stop
}



//******************************CODI 4******************************************
/*
read_input_registers_rsp
        Entrada:     int8       address          Adreça de l'esclau
        int8       byte_count         Nombre de bytes que s'envien
        int8*      input_data         Punter en un vector de dades a enviarSortida:    
        void
*/
void modbus_read_input_registers_rsp(int8_t address, int8_t byte_count, int8_t *input_data)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_READ_INPUT_REGISTERS);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; ++i)
        {
                modbus_serial_putc(*input_data);
                input_data++;
        }
        modbus_serial_send_stop();
}


//************************CODI 5****************************************
/*
write_single_coil_rsp
        Entrades:     int8    address            Slave Address
        int16      output_address     Echo of output address received
        int16      output_value       Echo of output value received
        Sortida:    void
*/
void modbus_write_single_coil_rsp(int8_t address, int16_t output_address, int16_t output_value)
{
        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_COIL);
        modbus_serial_putc(make8(output_address,1));
        modbus_serial_putc(make8(output_address,0));
        modbus_serial_putc(make8(output_value,1));
        modbus_serial_putc(make8(output_value,0));
        modbus_serial_send_stop();
}

/**************************CODI 6*****************************************
 * 
 * write_single_register_rsp           //Escriure un sol registre
 *      Entrada:        int8       address            Adreça de l'esclau
 *                      int16      reg_address        Echo of register address received
 *                      int16      reg_value          Echo of register value received
 *      Sortida:    void*/
void modbus_write_single_register_rsp(int8_t address, int16_t reg_address, int16_t reg_value)
{
        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_REGISTER);//Send Start
        modbus_serial_putc(make8(reg_address,1));     //Posició Inici d'escriptura
        modbus_serial_putc(make8(reg_address,0));     
        modbus_serial_putc(make8(reg_value,1));       //nombre de registres
        modbus_serial_putc(make8(reg_value,0));
        modbus_serial_send_stop();                    //Send Stop
}
        
//******************************CODI 7****************************************
/*
read_exception_status_rsp
Entrada:     int8_t       address            Slave Address
Sortida:    void
*/
void modbus_read_exception_status_rsp(int8_t address, int8_t data)
{
        modbus_serial_send_start(address, FUNC_READ_EXCEPTION_STATUS);
        modbus_serial_send_stop();
}

//***************************CODI 8******************************************
/*
diagnostics_rsp
Input:  int8_t       address            Slave Address
        int16      sub_func           Echo of sub function received
        int16      data               Echo of data received
        Output:    void
*/
void modbus_diagnostics_rsp(int8_t address, int16_t sub_func, int16_t data)
{
        modbus_serial_send_start(address, FUNC_DIAGNOSTICS);
        modbus_serial_putc(make8(sub_func,1));
        modbus_serial_putc(make8(sub_func,0));
        modbus_serial_putc(make8(data,1));
        modbus_serial_putc(make8(data,0));
        modbus_serial_send_stop();
}

//************************+CODI 0B*******************************************
/*
get_comm_event_counter_rsp
        Input:     int8       address            Slave Address
        int16      status             Status, refer to MODBUS documentation
        int16      event_count        Count of events
        Output:    void
*/
void modbus_get_comm_event_counter_rsp(int8_t address, int16_t status, int16_t event_count)
{
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_COUNTER);
        modbus_serial_putc(make8(status, 1));
        modbus_serial_putc(make8(status, 0));
        modbus_serial_putc(make8(event_count, 1));
        modbus_serial_putc(make8(event_count, 0));
        modbus_serial_send_stop();
}



//*******************CODI 0C*************************************************
/*
get_comm_event_counter_rsp
Input:  int8       address            Slave Address
        int16      status             Status, refer to MODBUS documentation
        int16      event_count        Count of events
        int16      message_count      Count of messages
        int8*      events             Pointer to event data
        int8       events_len         Length of event data in bytes
        Output:    void
*/
void modbus_get_comm_event_log_rsp(int8_t address, int16_t status,int16_t event_count, int16_t message_count, int8_t *events, int8_t events_len)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_LOG);
        modbus_serial_putc(events_len+6);
        modbus_serial_putc(make8(status, 1));
        modbus_serial_putc(make8(status, 0));
        modbus_serial_putc(make8(event_count, 1));
        modbus_serial_putc(make8(event_count, 0));
        modbus_serial_putc(make8(message_count, 1));
        modbus_serial_putc(make8(message_count, 0));
        for(i=0; i < events_len; ++i)
        {
                modbus_serial_putc(*events);
                events++;
        }
        modbus_serial_send_stop();
}


//*************************CODI 0F******************************************
/*
write_multiple_coils_rsp
Input:  int8       address            Slave Address
        int16      start_address      Echo of address to start at
        int16      quantity           Echo of amount of coils written to
Output:    void
*/
void modbus_write_multiple_coils_rsp(int8_t address, int16_t start_address, int16_t quantity)
{
        modbus_serial_send_start(address, FUNC_WRITE_MULTIPLE_COILS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
} 

//******************************CODI 10****************************************
/*
write_multiple_registers_rsp
        Input:     int8       address            Slave Address
        int16      start_address      Echo of address to start at
        int16      quantity           Echo of amount of registers written to
        Output:    void
*/
void modbus_write_multiple_registers_rsp(int8_t address, int16_t start_address, int16_t quantity)
{
        modbus_serial_send_start(address, FUNC_WRITE_MULTIPLE_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
}




//************************CODI 11*********************************************
/*
report_slave_id_rsp
        Input:  int8       address            Slave Address
                int8       slave_id           Slave Address
                int8       run_status         Are we running?
                int8*      data               Pointer to an array holding the data
                int8       data_len           Length of data in bytes
        Output:    void
*/
void modbus_report_slave_id_rsp(int8_t address, int8_t slave_id, int run_status,int8_t *data, int8_t data_len)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_REPORT_SLAVE_ID);
        modbus_serial_putc(data_len+2);
        modbus_serial_putc(slave_id);
        if(run_status)
                modbus_serial_putc(0xFF);
        else
                modbus_serial_putc(0x00);
        for(i=0; i < data_len; ++i)
        {
                modbus_serial_putc(*data);
                data++;
        }
        modbus_serial_send_stop();
}

//********************************CODI 14******************************************************
/*
read_file_record_rsp
        Input:  int8                     address            Slave Address
                int8                     byte_count         Number of bytes to send
                read_sub_request_rsp*    request            Structure holding record/data information
                Output:    void
*/
void modbus_read_file_record_rsp(int8_t address, int8_t byte_count, modbus_read_sub_request_rsp *request)
{
        int8_t i=0,j;
        modbus_serial_send_start(address, FUNC_READ_FILE_RECORD);
        modbus_serial_putc(byte_count);
        while(i < byte_count);
        {
                modbus_serial_putc(request->record_length);
                modbus_serial_putc(request->reference_type);
                for(j=0; (j < request->record_length); j+=2)
                {
                        modbus_serial_putc(make8(request->data[j], 1));
                        modbus_serial_putc(make8(request->data[j], 0));
                }
                i += (request->record_length)+1;request++;
        }
        modbus_serial_send_stop();
}


//*********************************CODI 15*********************************************************
/*
write_file_record_rsp
        Input:  int8                     address            Slave Address
                int8                     byte_count         Echo of number of bytes sent
                write_sub_request_rsp*   request            Echo of Structure holding recordinformation
                Output:    void
*/
void modbus_write_file_record_rsp(int8_t address, int8_t byte_count, modbus_write_sub_request_rsp *request)
{
        int8_t i, j=0;
        modbus_serial_send_start(address, FUNC_WRITE_FILE_RECORD);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; i+=(7+(j*2)))
        {
                modbus_serial_putc(request->reference_type);
                modbus_serial_putc(make8(request->file_number, 1));
                modbus_serial_putc(make8(request->file_number, 0));
                modbus_serial_putc(make8(request->record_number, 1));
                modbus_serial_putc(make8(request->record_number, 0));
                modbus_serial_putc(make8(request->record_length, 1));
                modbus_serial_putc(make8(request->record_length, 0));
                for(j=0; (j < request->record_length); j+=2)
                {
                        modbus_serial_putc(make8(request->data[j], 1));
                        modbus_serial_putc(make8(request->data[j], 0));
                }
        }
        modbus_serial_send_stop();
}


//*******************************CODI 16***********************************
/*
mask_write_register_rsp
        Input:     int8        address            Slave Address
        int16       reference_address  Echo of reference address
        int16       AND_mask           Echo of AND mask
        int16       OR_mask            Echo or OR mask
        Output:    void
*/
void modbus_mask_write_register_rsp(int8_t address, int16_t reference_address,int16_t AND_mask, int16_t OR_mask)
{
        modbus_serial_send_start(address, FUNC_MASK_WRITE_REGISTER);
        modbus_serial_putc(make8(reference_address,1));
        modbus_serial_putc(make8(reference_address,0));
        modbus_serial_putc(make8(AND_mask,1));
        modbus_serial_putc(make8(AND_mask,0));
        modbus_serial_putc(make8(OR_mask, 1));
        modbus_serial_putc(make8(OR_mask, 0));
        modbus_serial_send_stop();
}


//***********************CODI 17*****************************************
/*
read_write_multiple_registers_rsp
        Input:     int8        address            Slave Address
        int16*      data               Pointer to an array of data
        int8        data_len           Length of data in bytes
        Output:    void
*/
void modbus_read_write_multiple_registers_rsp(int8_t address, int8_t data_len, int16_t *data)
{
        int8_t i;
        modbus_serial_send_start(address, FUNC_READ_WRITE_MULTIPLE_REGISTERS);
        modbus_serial_putc(data_len*2);
        for(i=0; i < data_len*2; i+=2)
        {
                modbus_serial_putc(make8(data[i], 1));
                modbus_serial_putc(make8(data[i], 0));
        }
        modbus_serial_send_stop();
}




//****************************CODI 18************************************/
/*read_FIFO_queue_rsp
        Input:     int8        address            Slave Address
        int16       FIFO_len           Length of FIFO in bytes
        int16*      data               Pointer to an array of data
        Output:    void
*/
void modbus_read_FIFO_queue_rsp(int8_t address, int16_t FIFO_len, int16_t *data)
{
        int8_t i;
        int16 byte_count;
        byte_count = ((FIFO_len*2)+2);
        modbus_serial_send_start(address, FUNC_READ_FIFO_QUEUE);
        modbus_serial_putc(make8(byte_count, 1));
        modbus_serial_putc(make8(byte_count, 0));
        modbus_serial_putc(make8(FIFO_len, 1));
        modbus_serial_putc(make8(FIFO_len, 0));
        for(i=0; i < FIFO_len; i+=2)
        {
                modbus_serial_putc(make8(data[i], 1));
                modbus_serial_putc(make8(data[i], 0));
        }
        modbus_serial_send_stop();
}




//******************************EXCEPCIONS*****************************
void modbus_exception_rsp(int8_t address, int16_t func, exception error)
{
        modbus_serial_send_start(address, func|0x80);
        modbus_serial_putc(error);
        modbus_serial_send_stop();
}


#endif