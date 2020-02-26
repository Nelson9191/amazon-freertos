
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

volatile bool Valid_data_Flag=MODBUS_FALSE;

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


//*****************************************************

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



//*****************************************************


void RCV_ON(void)                       //ESta Funcion va a ser la que habilita el serial y las interrupciones
{
        return;
        /*
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
        */
}
        
        



        
// Objectiu:   Inicialitza comunicació RS-485. Cridem aquesta funció abans
//d'utilitzar qualsevol funció RS-485  
// Entrades:   Cap
// Sortides:   Cap

void modbus_init(void)   
{


        /* uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, MODBUS_SERIAL_TX_PIN, MODBUS_SERIAL_RX_PIN, MODBUS_SERIAL_ENABLE_PIN, MODBUS_SERIAL_CTS_PIN);
        uart_driver_install(UART_NUM_1, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);
        gpio_handler_write(MODBUS_SERIAL_ENABLE_PIN,0);   //Possem a zero el Tcs del transreceptor */
        //Modbus_Task_Start();
       
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
        //disable_interrupts(INT_TIMER2);   //Desabilita la interrupció del Timer 2
        if(enable) 
        {
                //set_timer2(0);                 //Posa a 0 el Timer
                //clear_interrupt(INT_TIMER2);   //Neteja la interrupció
                //enable_interrupts(INT_TIMER2); //Torna a habilitar la int del timer 
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
                modbus_serial_new=MODBUS_TRUE;   //Marcador que indica que hi ha una nova trama a processar
        }
        else 
                modbus_serial_new=MODBUS_FALSE;//prepara microcontrolador per la següent trama
        modbus_serial_crc.d=0xFFFF;  //Inicialitza el CRC (sempre inicialitzat a 0xFFFF)
        modbus_serial_state=MODBUS_GETADDY; 
        modbus_enable_timeout(MODBUS_FALSE); //Para temporitzador de timeout
}

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
/*
void incomming_modbus_serial_new(void) 
{

        int len = 0;
        uint8_t index=0;
        data_buffer = malloc(sizeof(uint8_t) * (MODBUS_SERIAL_RX_BUFFER_SIZE+10));  
        len = uart_read_bytes(UART_NUM_1, (uint8_t *)data_buffer, len, 3500000/MODBUS_SERIAL_BAUD);
        if (len > 0){
            modbus_rx.address= data_buffer[0];
            modbus_rx.func = data_buffer[1]; 
            while((index++)<=len-4)
                modbus_rx.data[index] = data_buffer[index+2]; 
            modbus_rx.crc = ((data_buffer[len-2])<<8 )|| (data_buffer[len-1]);
        }


        uint16_t CRC_RTN=CRC16 (data_buffer, len);
        Valid_data_Flag=(CRC_RTN== modbus_rx.crc)?MODBUS_TRUE:MODBUS_FALSE;
                
        


}//*/

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
/*void incomming_modbus_serial(void) 
{
        char c = '\0';
        //char data_buffer[1];
        int len = uart_read_bytes(UART_NUM_1, (uint8_t *)Modbus_RX_BUFFER, 1, 3500000/MODBUS_SERIAL_BAUD);

        if (len > 0){
            c = Modbus_RX_BUFFER[0];
        }
        //c=fgetc(MODBUS_SERIAL);    //MODBUS SERIAL és el nom que hem donat a la UART
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
                //modbus_calc_crc(c);                  // Calcula el CRC del valor agafat de la UART
                modbus_enable_timeout(MODBUS_TRUE);         // Activem el temps de Timeout i el següent byte
                // Té 4 ms de Timeout  per arribar
        }
}//*/

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
                modbus_rx.error = TIMEOUT; //Crear código para malloc
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
                modbus_rx.error = TIMEOUT;
                printf("Modbus Timed out \n");
                ok = false;
        }
        else
        {
                modbus_rx.error = 0;
                modbus_rx.address = Mod_buffer[0];
                modbus_rx.func = Mod_buffer[1];


               
                for (int i = 0; i < len - 4; i++){
                        modbus_rx.data[i] = Mod_buffer[i + 2];
                        printf("-%x", Mod_buffer[i + 2]);
                }
                 //****************************** leer exepciones
                if(modbus_rx.func & 0x80) 
                {
                        modbus_rx.error = modbus_rx.data[0];
                        modbus_rx.len=1;
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
                        modbus_rx.len=len-4;
                      //  printf("Recv checksum: %2x %2x\n", Mod_buffer[len-2], Mod_buffer[len-1]);
                      //  printf("Calc checksum: %2x %2x\n", (uint8_t)(CRC_RTN>>8), (uint8_t)(CRC_RTN));
                        //printf("Data REceived: \n");  
                        //printf("--  %.*s [%d]\n", (len > 2 ? len - 2 : len), buffer, len);
                        ok = CRC_RTN == CRC_RCV;   
                }
                else
                {
                        printf("No valid Data flag \n");
                       memset( modbus_rx.data,'\0',len-4); 
                       modbus_rx.len=0;   
                }
        }

        free(Mod_buffer);
        return ok;
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


// Objectiu:    Guarda missatge del bus en un buffer
// Entrades:    Cap
// Sortides:    MODBUS_TRUE si el missatge s'ha rebut
//              MODBUS_FALSE si no s'ha rebut
// Nota:        Les dades seran omplertes en el modbus_rx struct


int modbus_kbhit(void)
{
        if(!modbus_serial_new)                  //si no tenim nova trama Kbhit=0
                return MODBUS_FALSE;
        else
        if(modbus_rx.func & 0x80)           //sinó si la funció té error
        {
                modbus_rx.error = modbus_rx.data[0];  //guarda l'error
                modbus_rx.len = 1;                    //es modifica la trama a enviar
        }
        modbus_serial_new=MODBUS_FALSE;                 //Inicia l'indicador de trama nova
        return MODBUS_TRUE;                             //Kbhit=1 TENIM NOVA TRAMA
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
exception modbus_write_single_coil(int8_t address, int16_t output_address, int on)
{
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