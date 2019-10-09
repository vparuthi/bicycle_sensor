#include <msp430.h>
#include "driverlib/driverlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "buzzer.h"
#include "main.h"

//LED Ports
#define GREEN_LED_PORT GPIO_PORT_P1
#define YELLOW_LED_PORT GPIO_PORT_P1
#define ORANGE_LED_PORT GPIO_PORT_P1
#define RED_LED_PORT GPIO_PORT_P5
//LED Pins
#define GREEN_LED_PIN GPIO_PIN5
#define YELLOW_LED_PIN GPIO_PIN4
#define ORANGE_LED_PIN GPIO_PIN3
#define RED_LED_PIN GPIO_PIN0

void turn_off_all_leds(void){
    GPIO_setOutputLowOnPin(GREEN_LED_PORT, GREEN_LED_PIN);
    GPIO_setOutputLowOnPin(GREEN_LED_PORT, YELLOW_LED_PIN);
    GPIO_setOutputLowOnPin(ORANGE_LED_PORT, ORANGE_LED_PIN);
    GPIO_setOutputLowOnPin(RED_LED_PORT, RED_LED_PIN);
}

void flash_led(int port, int pin, int mdelay, bool flash){
    if(flash){
        int counter = 0;
        GPIO_setOutputHighOnPin(port, pin);

        while(counter < mdelay * 1000){
            counter++;
        }

        GPIO_setOutputLowOnPin(port, pin);
        counter = 0;
        while(counter < mdelay * 1000){
            counter++;
        }
    }else{
        GPIO_setOutputHighOnPin(port, pin);
    }
}

void turn_on_led(uint16_t distance){
    if(distance >= 60){
        turn_off_all_leds();
        turn_off_buzzer();
        flash_led(GREEN_LED_PORT, GREEN_LED_PIN, 0, false);
        mDelay(1);
    } else if (distance < 60 && distance >= 40) {
        turn_off_all_leds();
        turn_off_buzzer();
        flash_led(YELLOW_LED_PORT, YELLOW_LED_PIN, 5, true);
    }else if(distance < 40 && distance >= 20){
        turn_off_all_leds();
        turn_off_buzzer();
        flash_led(ORANGE_LED_PORT, ORANGE_LED_PIN, 3, true);
    }else{
        turn_off_all_leds();
        turn_off_buzzer();
        flash_led(RED_LED_PORT, RED_LED_PIN, 1, true);
    }
}
