#include "driverlib/driverlib.h"
#include <stdlib.h>
#include <msp430.h>
#include "utils.h"

//Trigger Port & Pin
#define TRIGGER_PORT GPIO_PORT_P2
#define TRIGGER_PIN GPIO_PIN5

//number of micro seconds per timer count
#define MICRO_SEC_PER_TICK 30

void send_trigger(int port, int pin, int u_delay){
    GPIO_setOutputHighOnPin(port, pin);
    uDelay(u_delay);
    GPIO_setOutputLowOnPin(port, pin);
}

void Init_Distance_Sensor(void){
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P8, GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
}

uint16_t calculate_distance(int time){
    return (time * MICRO_SEC_PER_TICK)/58;
}
