#ifndef _GPIO_HANDLER_STATIC_H_
#define _GPIO_HANDLER_STATIC_H_

/*
Rutina de atención a interrupción de señal en gpios de entrada.
Cada que hay un cambio en una entrada, esta ISR informará del pin GPIO en el cual 
se registró un cambio
@Params: - arg: puntero en el cual es FreeRTOS pone el número de GPIO de la interrupción
@Return: 
*/
static void IRAM_ATTR gpio_handler_ISR(void* arg);

/*
Devuelve la mascara bits de las entradas segun se hayan definido en gpio_info.h
@Params:
@Return: mascara de entradas
*/
static uint64_t gpio_handler_get_input_mask();

/*
Devuelve la mascara bits de las salidas segun se hayan definido en gpio_info.h
@Params:
@Return: mascara de salidas
*/
static uint64_t gpio_handler_get_output_mask();

/*
Para un gpio (entrada) compara el estado actual con el que se ha reportado anteriormente
@Params: - gpio: entrada a comparar
@Return: - true si hay un cambio entre las dos muestras, false en otro caso
*/
static bool gpio_handler_compare_status(unsigned int gpio);

/*
Para un gpio (entrada/salida) envia el reporte al queue de MQTT
@Params: - gpio: entrada/salida
@Params: - gpio_name: nombre entrada/salida
@Params: - timestamp: timestamp de reporte
@Return:
*/
static void gpio_handler_report_gpio_status(uint32_t gpio, const char * gpio_name, uint32_t timestamp);


/*
Reporta el estado de todas las entradas y salidas digitales
@Params: 
@Return:
*/
static void gpio_handler_collect_gpios();

#endif