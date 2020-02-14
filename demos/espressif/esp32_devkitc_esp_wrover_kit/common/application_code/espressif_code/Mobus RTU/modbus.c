
#include "modbus.h"
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "utils.h"



#define BUF_SIZE 2000
#define UART_NUM_1 1

//******************************************************
//        Esta Función va a reemplazar a Modbus_put
//******************************************************
void Serial_Tx(byte tx)
{

}

//*****************************************************

uint8_t make8(uint16_t Tobytes,uint8_t Pos)
{
        uint8_t Aux;
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



//*****************************************************


void RCV_ON(void)                       //ESta Funcion va a ser la que habilita el serial y las interrupciones
{
        #if(MODBUS_SERIAL_INT_SOURCE!=MODBUS_INT_EXT)
                while(kbhit(MODBUS_SERIAL)) 
                {
                        getc();
                }  //Borra RX buffer. Borra interrupció RDA. Borra el flag del overrun error flag.

        #if(MODBUS_SERIAL_INT_SOURCE==MODBUS_INT_RDA)
                clear_interrupt(INT_RDA);            //Neteja interrupció RDA
        #else
                clear_interrupt(INT_RDA2);
        #endif
        #if(MODBUS_SERIAL_RX_ENABLE!=0) 
                output_low(MODBUS_SERIAL_RX_ENABLE);
        #endif
        #if(MODBUS_SERIAL_INT_SOURCE==MODBUS_INT_RDA)
                enable_interrupts(INT_RDA);
        #else
                enable_interrupts(INT_RDA2);
        #endif
        #else
                clear_interrupt(INT_EXT);
                ext_int_edge(H_TO_L);
                #if(MODBUS_SERIAL_RX_ENABLE!=0) 
                        output_low(MODBUS_SERIAL_RX_ENABLE);
                #endif
                enable_interrupts(INT_EXT);
        #endif
}
        
        



        
// Objectiu:   Inicialitza comunicació RS-485. Cridem aquesta funció abans
//d'utilitzar qualsevol funció RS-485  
// Entrades:   Cap
// Sortides:   Cap

void modbus_init(void)   
{


        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, MODBUS_SERIAL_ENABLE_PIN, ECHO_TEST_CTS);
        uart_driver_install(UART_NUM_1, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);
        gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN,0);   //Possem a zero el Tcs del transreceptor
        Modbus_Task_Start();
       
       // RCV_ON();                               //crida aquesta funció per activar la recepció
       // setup_timer_2(T2_DIV_BY_16,249,5);      //Configuració del Timer 2 per interrupció ~4ms
        //enable_interrupts(GLOBAL);              //Permet les interrupcions globals
        
}




// Objectiu:    Inicia el Timer de Timeout alhora d'enviar o rebre trames
// Entrades:    Enable, s'utilitza per canviar l'estat on/off
// Sortides:    cap
// El Timeout s'haurà d'activar quan el següent byte tardi més de 4ms
// a 1 = càrrega Timeout 
// a 0 = Parat
// Per enviar trama activem el Timeout


void modbus_enable_timeout(int enable)
{
        disable_interrupts(INT_TIMER2);   //Desabilita la interrupció del Timer 2
        if(enable) 
        {
                set_timer2(0);                 //Posa a 0 el Timer
                clear_interrupt(INT_TIMER2);   //Neteja la interrupció
                enable_interrupts(INT_TIMER2); //Torna a habilitar la int del timer 
        }
}




// Objectiu:    Quan s'ha disparat la interrupció 2 el programa se situa a int timer2
// Entrades:    Cap
// Sortides:    Cap

//#int_timer2
void modbus_timeout_now(void)   //EL MODBUS-GETDATA és l'estat 2 definit a enum//Si el programa es troba en l'Estat 2 i el Crc ha arribat a 0 (trama correcta) i tenim nova trama:  
{
        if((modbus_serial_state == MODBUS_GETDATA) && (modbus_serial_crc.d == 0x0000) && (!modbus_serial_new))
        {
                modbus_rx.len-=2;            
                modbus_serial_new=TRUE;   //Marcador que indica que hi ha una nova trama a processar
        }
        else 
                modbus_serial_new=FALSE;//prepara microcontrolador per la següent trama
        modbus_serial_crc.d=0xFFFF;  //Inicialitza el CRC (sempre inicialitzat a 0xFFFF)
        modbus_serial_state=MODBUS_GETADDY; 
        modbus_enable_timeout(FALSE); //Para temporitzador de timeout
}

// Objectiu:    Calcula el CRC de l'últim byte que se li passa per la variable Data
//              (Data = C de Modbus_serial_putc) i actualitza el CRC global 
// Entrades:    Caràcter// Sortides:    Cap
void modbus_calc_crc(char data)
{
        int8_t uIndex ; // És l'índex per adreçar-nos a les taules de CRC (lookup table)
        //Càlcul del CRC
        uIndex = (modbus_serial_crc.b[1]) ^ data; 
        modbus_serial_crc.b[1] = (modbus_serial_crc.b[0]) ^ modbus_auchCRCHi[uIndex]; //byte major pes
        modbus_serial_crc.b[0] = modbus_auchCRCLo[uIndex];                            //byte menor pes
}




// Objectiu:    Posa un byte a la UART per poder enviar trames
// Entrades:    Caràcter
// Sortides:    Cap

void modbus_serial_putc(int8 c)
{
        fputc(c, MODBUS_SERIAL);   //Enviem el byte C a la UART //todo
        modbus_calc_crc(c);        //Enviem el byte C a la funció per calcular el CRC (data)
        ets_delay_us(1000000/MODBUS_SERIAL_BAUD); //Retard perquè el receptor tingui temps de calcular el CRC
}



// Objectiu:   Interrumpeix la rutina de servei per tractar dades d'entrada
// Entrades:   Cap
// Sortides:   Cap
/*
#if(MODBUS_SERIAL_INT_SOURCE==MODBUS_INT_RDA)
        #int_rda               //Quan tenim buffer sèrie ple es dispara una interrupció  
#elif(MODBUS_SERIAL_INT_SOURCE==MODBUS_INT_RDA2)
        #int_rda2
#elif(MODBUS_SERIAL_INT_SOURCE==MODBUS_INT_EXT)
        #int_ext
#else
#error Please define a correct interrupt source
#endif                          //*/
void incomming_modbus_serial(void) 
{
        char c;
        c=fgetc(MODBUS_SERIAL);    //MODBUS SERIAL és el nom que hem donat a la UART
        //Agafa sempre tota la trama encara que no sigui per nosaltres
        if(!modbus_serial_new)  //Mira en quin estat ens trobem ADR, FUN, DAD 
        {
                if(modbus_serial_state == MODBUS_GETADDY)
                {
                        modbus_serial_crc.d = 0xFFFF; //Inicialitzar CRC per rebre
                        modbus_rx.address = c;        //Guarda adreça que arriba 
                        modbus_serial_state++;        //Incrementa Estat
                        modbus_rx.len = 0;            //Inicia byte llargada
                        modbus_rx.error=0;            //Inicia byte d'error
                }
                else
                if(modbus_serial_state == MODBUS_GETFUNC)
                {
                        modbus_rx.func = c;           //Guarda codi de funció
                        modbus_serial_state++;        //Incrementa l'estat del sistema
                }
                else
                if(modbus_serial_state == MODBUS_GETDATA)//el rx buffer size és el màxim de memòria que hem donat (definit a programa Main)
                {
                        if(modbus_rx.len>=MODBUS_SERIAL_RX_BUFFER_SIZE) 
                        {
                                modbus_rx.len=MODBUS_SERIAL_RX_BUFFER_SIZE-1;
                        }
                        modbus_rx.data[modbus_rx.len]=c;  //Guarda la C a rx.data

                        modbus_rx.len++;                  //Incrementa la posició
                }
                modbus_calc_crc(c);                  // Calcula el CRC del valor agafat de la UART
                modbus_enable_timeout(TRUE);         // Activem el temps de Timeout i el següent byte
                // Té 4 ms de Timeout  per arribar
        }
}



void modbus_serial_send_stop(void)
{
        int8_t crc_low, crc_high;
        crc_high=modbus_serial_crc.b[1];  //Guardem el valor del Checksum MSB
        crc_low=modbus_serial_crc.b[0];   //Guardem el valor del checksum LSB
        //Per defecte es calcula el CRC d'aquests dos valors però enviarem a la UART
        //directement el valor CRC high i low.
        modbus_serial_putc(crc_high);   // Enviem aquests valors a la UART 
        modbus_serial_putc(crc_low);
        #if(MODBUS_SERIAL_INT_SOURCE!=MODBUS_INT_EXT)
                WAIT_FOR_HW_BUFFER();
        #endif
        ets_delay_us(3500000/MODBUS_SERIAL_BAUD); //3.5 character delay
        RCV_ON();     //Activa la recepció
        #if(MODBUS_SERIAL_ENABLE_PIN!=0) 
                gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN, 0);
        #endif
        modbus_serial_crc.d=0xFFFF;
}


// Objectiu:    Guarda missatge del bus en un buffer
// Entrades:    Cap
// Sortides:    TRUE si el missatge s'ha rebut
//              FALSE si no s'ha rebut
// Nota:        Les dades seran omplertes en el modbus_rx struct


int modbus_kbhit(void)
{
        if(!modbus_serial_new)                  //si no tenim nova trama Kbhit=0
                return FALSE;
        else
        if(modbus_rx.func & 0x80)           //sinó si la funció té error
        {
                modbus_rx.error = modbus_rx.data[0];  //guarda l'error
                modbus_rx.len = 1;                    //es modifica la trama a enviar
        }
        modbus_serial_new=FALSE;                 //Inicia l'indicador de trama nova
        return TRUE;                             //Kbhit=1 TENIM NOVA TRAMA
}


//              MODBUS MASTER FUNCTIONS      

#if(MODBUS_TYPE==MODBUS_TYPE_MASTER)

/*
//read_coils    
        Input:    int8       address   Slave Address     
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
        
*/


exception modbus_read_coils(int8_t address, int16_t start_address, int16_t quantity)
{
        modbus_serial_send_start(address, FUNC_READ_COILS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}



/*
        read_discrete_input
        Input:  int8       address            Slave Address
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
        
*/


exception modbus_read_discrete_input(int8_t address, int16_t start_address, int16_t quantity)
{       
        modbus_serial_send_start(address, FUNC_READ_DISCRETE_INPUT);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
        
}




/*

read_holding_registers
        Input:  int8       address            Slave Address
                int16      start_address      Address to start reading from
                int16      quantity           Amount of addresses to read
        Output: exception                     0 if no error, else the exception*/
        
exception modbus_read_holding_registers(int8_t address, int16_t start_address, int16_t quantity)
{
        modbus_serial_send_start(address, FUNC_READ_HOLDING_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}



/*

        read_input_registers
        Input:     int8       address            Slave Address
        int16      start_address      Address to start reading from
        int16      quantity           Amount of addresses to read
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_read_input_registers(int8_t address, int16_t start_address, int16_t quantity)
{
        modbus_serial_send_start(address, FUNC_READ_INPUT_REGISTERS);
        modbus_serial_putc(make8(start_address,1));
        modbus_serial_putc(make8(start_address,0));
        modbus_serial_putc(make8(quantity,1));
        modbus_serial_putc(make8(quantity,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
        
}




/*

        write_single_coil
        Input:     int8       address            Slave Address
        int16      output_address     Address to write into
        int1       on                 true for on, false for off
        Output:    exception                     0 if no error, else the exception
        
*/
exception modbus_write_single_coil(int8_t address, int16_t output_address, int on)
{
        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_COIL);
        modbus_serial_putc(make8(output_address,1));
        modbus_serial_putc(make8(output_address,0));

        if(on)
                modbus_serial_putc(0xFF);
        else
                modbus_serial_putc(0x00);
                modbus_serial_putc(0x00);
                modbus_serial_send_stop();
                MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}


/*
        write_single_register
        Input:  int8       address            Slave Address
                int16      reg_address        Address to write into
                int16      reg_value          Value to write
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_write_single_register(int8_t address, int16_t reg_address, int16_t reg_value)
{
        modbus_serial_send_start(address, FUNC_WRITE_SINGLE_REGISTER);
        modbus_serial_putc(make8(reg_address,1));
        modbus_serial_putc(make8(reg_address,0));
        modbus_serial_putc(make8(reg_value,1));
        modbus_serial_putc(make8(reg_value,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}



/*
        read_exception_status
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
        
*/

exception modbus_read_exception_status(int8_t address)
{
        modbus_serial_send_start(address, FUNC_READ_EXCEPTION_STATUS);
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}




/*
        diagnostics
        Input:     int8       address            Slave Address
        int16      sub_func           Subfunction to send
        int16      data               Data to send, changes based on subfunction
        Output:    exception                     0 if no error, else the exception
*/

exception modbus_diagnostics(int8_t address, int16_t sub_func, int16_t data)
{
        modbus_serial_send_start(address, FUNC_DIAGNOSTICS);
        modbus_serial_putc(make8(sub_func,1));
        modbus_serial_putc(make8(sub_func,0));
        modbus_serial_putc(make8(data,1));
        modbus_serial_putc(make8(data,0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}

/*
        get_comm_event_couter
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/

exception modbus_get_comm_event_counter(int8_t address)
{
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_COUNTER);
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}

/*
        get_comm_event_log
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_get_comm_event_log(int8_t address)
{
        modbus_serial_send_start(address, FUNC_GET_COMM_EVENT_LOG);
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}



/*
        write_multiple_coils
        Input:     int8       address            Slave Address
        int16      start_address      Address to start at
        int16      quantity           Amount of coils to write to
        int1*      values             A pointer to an array holding the values to write
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_write_multiple_coils(int8_t address, int16_t start_address, int16_t quantity,int8_t *values)
{
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
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}


/* 
        write_multiple_registers
        Input:     int8       address            Slave Address
        int16      start_address      Address to start at
        int16      quantity           Amount of coils to write to
        int16*     values             A pointer to an array holding the data to write
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_write_multiple_registers(int8_t address, int16_t start_address, int16_t quantity,int16_t *values)
{
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
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}

/*
        report_slave_id
        Input:     int8       address            Slave Address
        Output:    exception                     0 if no error, else the exception
*/
exception modbus_report_slave_id(int8_t address)
{
        modbus_serial_send_start(address, FUNC_REPORT_SLAVE_ID);
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
        
}

/*
        read_file_record
        Input:  int8                address            Slave Address
                int8                byte_count         Number of bytes to read
                read_sub_request*   request      Structure holding record information
        Output:    exception                              0 if no error, else the exception
*/
exception modbus_read_file_record(int8_t address, int8_t byte_count, modbus_read_sub_request *request)
{
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
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}


/*
        write_file_record
        Input:     int8                address            Slave Address
        int8                byte_count         Number of bytes to read
        read_sub_request*   request            Structure holding record/data information
        Output:    exception                              0 if no error, else the exception
*/
exception modbus_write_file_record(int8_t address, int8_t byte_count, modbus_write_sub_request *request)
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
                for(j=0; (j < request->record_length) && (j < MODBUS_SERIAL_RX_BUFFER_SIZE-8); j+=2)
                {
                        modbus_serial_putc(make8(request->data[j], 1));
                        modbus_serial_putc(make8(request->data[j], 0));
                }request++;
        }
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}

/*
        mask_write_register
        Input:     int8       address            Slave Address
        int16      reference_address  Address to mask
        int16      AND_mask           A mask to AND with the data at reference_address
        int16      OR_mask            A mask to OR with the data at reference_address
        Output:    exception                              0 if no error, else the exception
*/
exception modbus_mask_write_register(int8_t address, int16_t reference_address,int16_t AND_mask, int16_t OR_mask)
{
        modbus_serial_send_start(address, FUNC_MASK_WRITE_REGISTER);
        modbus_serial_putc(make8(reference_address,1));
        modbus_serial_putc(make8(reference_address,0));
        modbus_serial_putc(make8(AND_mask,1));
        modbus_serial_putc(make8(AND_mask,0));
        modbus_serial_putc(make8(OR_mask, 1));
        modbus_serial_putc(make8(OR_mask, 0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
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
exception modbus_read_write_multiple_registers(int8_t address, int16_t read_start,int16_t read_quantity, int16_t write_start,int16_t write_quantity,int16_t *write_registers_value)
{
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
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
}


/*
        read_FIFO_queue
        Input:     int8       address           Slave Address
        int16      FIFO_address      FIFO address
        Output:    exception                    0 if no error, else the exception
*/
exception modbus_read_FIFO_queue(int8_t address, int16_t FIFO_address)
{
        modbus_serial_send_start(address, FUNC_READ_FIFO_QUEUE);
        modbus_serial_putc(make8(FIFO_address, 1));
        modbus_serial_putc(make8(FIFO_address, 0));
        modbus_serial_send_stop();
        MODBUS_SERIAL_WAIT_FOR_RESPONSE();
        return modbus_rx.error;
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
        int8_t i;modbus_serial_send_start(address, FUNC_READ_DISCRETE_INPUT);
        modbus_serial_putc(byte_count);
        for(i=0; i < byte_count; ++i)
        {
                modbus_serial_putc(*input_data);
                input_data++;
        }
        modbus_serial_send_stop();
}


/***********************CODI 3 LLEGIR**************************************
 *  /*
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

/**************************CODI 6****************************************
 * /*
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
        int8 i;
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
        int8 i;
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





void Modbus_init(void)
{





}
void Modbus_params(void)
{



}
__uint32_t Modbus_Chechsum(__uint8_t * mdb_ptr)
{




}


bool Checksum_compare(__uint32_t CH1,__uint32_t CH2)
{


        (CH1==CH2)?return TRUE:FALSE false;
    

}