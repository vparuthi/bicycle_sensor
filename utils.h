#include "driverlib/driverlib.h"
#include <stdlib.h>
#include <msp430.h>

void uDelay(unsigned int us){
    while(us--){
        __delay_cycles(16);
    }
}
void mDelay(unsigned int ms){
    while(ms--){
        __delay_cycles(1600);
    }
}
