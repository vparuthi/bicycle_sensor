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
void turn_on_buzzer(uint16_t distance){
    if(distance < 40 && distance >= 20){
        turn_off_buzzer();
        param.timerPeriod = 1000;
        Timer_A_outputPWM(TIMER_A0_BASE, &param);   //Turn on PWM
    }else if(distance < 20){
        turn_off_buzzer();
        param.timerPeriod = 800;
        Timer_A_outputPWM(TIMER_A0_BASE, &param);   //Turn on PWM
    }else{
        turn_off_buzzer();
    }
}
