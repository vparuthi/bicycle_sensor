#include <msp430.h>
#include "driverlib/driverlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "main.h"

#define BEEP_TIME 1000

void turn_off_buzzer(void){
    if (GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 1) //Look for falling edge
    {
        Timer_A_stop(TIMER_A0_BASE);   //Turn off PWM
    }
}

void beep_buzzer(int period, int num_beeps){
    param.timerPeriod = period;
    int i = 0;

    for(i = 0; i < num_beeps; i ++){
        turn_off_buzzer();
        Timer_A_outputPWM(TIMER_A0_BASE, &param);   //Turn on PWM

        int count = 0;
        while(count < BEEP_TIME/num_beeps){
            count++;
        }

        turn_off_buzzer();
        count = 0;
        while(count < (BEEP_TIME/num_beeps)){
            count++;
        }
    }
    turn_off_buzzer();
}

void buzzer(uint16_t distance, int *thresholds){
    if(distance < thresholds[1] && distance >= thresholds[0]){
        beep_buzzer(850, 2);
    }else if(distance < thresholds[0]){
        beep_buzzer(700, 4);
    }else{
        turn_off_buzzer();
    }
}
