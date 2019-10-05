#include <msp430.h>
#include "driverlib/driverlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "main.h"

void turn_off_buzzer(void){
    if (GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 1) //Look for falling edge
    {
        Timer_A_stop(TIMER_A0_BASE);   //Turn off PWM
    }
}
void turn_on_buzzer(void){
    if (GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 1) //Look for falling edge
    {
        Timer_A_outputPWM(TIMER_A0_BASE, &param);   //Turn on PWM
    }
}
