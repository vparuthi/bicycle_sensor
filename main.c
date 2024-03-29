#include "hal_LCD.h"
#include "ultra_sonic_sensor.h"
#include "led.h"

const char* rear_threshold_names[NUM_REAR_THRESHOLDS] = {"RED", "ORANGE", "YELLOW"};
const char* front_threshold_names[NUM_FWD_THRESHOLDS] = {"2 BEEPS", "4 BEEPS"};

volatile uint16_t time = 20;
volatile int direction = FORWARD;
volatile int* rear_thresholds;
volatile int* front_thresholds;

// 0 means look for rising edge
// 1 means look for falling edge
volatile int edge_check = 0;

#pragma vector=TIMER1_A1_VECTOR
__interrupt void TA1_ISR(void){
    if(edge_check == 0){
        edge_check = 1;
        time = Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_2);
    }else{
        edge_check = 0;
        time = Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_2) - time;
    }
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_2);
}

// called when both button held functionality is needed
int on_double_button_hold(int *count, int hold_time){
    // both buttons held
    if ((GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 0) && (GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 0)){
        *count = *count + 1;
    }else{
        *count = 0;
    }

    if(*count > hold_time){
        return 1;
    }else{
        return 0;
    }
}

// called when choosing to change forward or rear settings
void on_single_button_hold(int *count, int *button_state, int *both_pressed, int port, int pin){
    if ((GPIO_getInputPinValue(port, pin) == 0) && !(*both_pressed)){
        (*count)++;
    } else {
        *(count) = 0;
    }

    if(*count > SHORT_BTN_HOLD_TIME && *button_state){
        // reset count and button_state
        *count = 0;
        *button_state = 0;

        if(port == SW1_PORT){
            displayScrollText("REAR SETUP");
            // call set_distance_thresholds() to set all three thresholds
            set_distance_thresholds(rear_thresholds, NUM_REAR_THRESHOLDS);
        }else{
            displayScrollText("FRONT SETUP");
            // call set_distance_thresholds() to set all three thresholds
            set_distance_thresholds(front_thresholds, NUM_FWD_THRESHOLDS);

        }


        // clear the LCD prior to exiting
        clearLCD();
    }
}

// called when changing the actual distance value
void on_button_click(int *distance, int *counter, int *button_state, int min_distance, int port, int pin){
    if(GPIO_getInputPinValue(port, pin) == 0 && (*button_state)){
        (*counter)++;
    }else{
        *button_state = 1;
        *counter = 0;
    }

    if(*counter > TOGGLE_TIME){
        // if left button decrement distance value
        if(port == SW1_PORT){
            if(*distance > min_distance + 1){
                (*distance)--;
            }
        }else{
            (*distance)++;
        }

        // display distance
        display_distance(*distance);

        // reset button_counter and state so one click doesn't trigger multiple times
        *counter = 0;
        *button_state = 0;
    }
}

int adjust_distance(int min_distance, int init_distance_val){
    int both_buttons = 0;
    int left_button = 0;
    int right_button = 0;
    int button_state = 0;
    int distance = init_distance_val;


    while(1){
        // both buttons held
        if(on_double_button_hold(&both_buttons, SHORT_BTN_HOLD_TIME)){
            return distance;
        }

        /* note: the on_button_click was implemented to work only for a single button click,
         * but due to the use of the same button_state variable in the front and rear calls users
         * can hold the button to change the distance value*/

        // left button
        if(!both_buttons){
            on_button_click(&distance, &left_button, &button_state, min_distance, SW1_PORT, SW1_PIN);
        }

        // right button
        if(!both_buttons){
            on_button_click(&distance, &right_button, &button_state, min_distance, SW2_PORT, SW2_PIN);
        }
    }
}

void set_distance_thresholds(int *thresholds, int len){

    int i = 0;
    int min_distance = 0;
    char ** threshold_names;

    if(len == NUM_REAR_THRESHOLDS){
        threshold_names = rear_threshold_names;
    }else{
        threshold_names = front_threshold_names;
    }

    for(i = 0; i < len; i++){
        displayScrollText(threshold_names[i]);

        if(min_distance < thresholds[i]){
            display_distance(thresholds[i]);
            min_distance = adjust_distance(min_distance, thresholds[i]);
        }else{
            display_distance(min_distance);
            min_distance = adjust_distance(min_distance, min_distance);
        }
        thresholds[i] = min_distance;
    }
}

void setup_mode(void){
    displayScrollText("SETUP MODE");

    // counters:
    int return_count = 0;
    int rear_button_count = 0;
    int front_button_count = 0;

    // used to prevent users from skipping back twice
    int button_state = 1;

    // used to prevent both buttons being held and triggering the R or F buttons
    int both_pressed = 0;

    while(1){
        // show options
        showChar('F', pos5);
        showChar('R', pos2);

        // return back to main
        if ((GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 0) && (GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 0)){
            return_count++;
            both_pressed = 1;
        } else {
            return_count = 0;
            button_state = 1;
            both_pressed = 0;
        }

        if(return_count > SHORT_BTN_HOLD_TIME && button_state){
            return;
        }

        // rear
        on_single_button_hold(&rear_button_count, &button_state, &both_pressed, SW1_PORT, SW1_PIN);

        // front
        on_single_button_hold(&front_button_count, &button_state, &both_pressed, SW2_PORT, SW2_PIN);
    }
}

char ADCState = 0; //Busy state of the ADC
int16_t ADCResult = 0; //Storage for the ADC conversion result

void main(void){
    //Turn off interrupts during initialization
    __disable_interrupt();

    //Stop watchdog timer unless you plan on using it
    WDT_A_hold(WDT_A_BASE);

    // Initializations - see functions for more detail
    Init_GPIO();            //Sets all pins to output low as a default
    Init_PWM();             //Sets up a PWM output
    Init_ADC();             //Sets up the ADC to sample
    Init_Clock();           //Sets up the necessary system clocks
    Init_UART();            //Sets up an echo over a COM port
    Init_LCD();             //Sets up the LaunchPad LCD display
    Init_Distance_Sensor(); //Sets up the pins for the ultrasonic sensor

    // our inits
    init_timer();           //Sets up continuous and capture timers
    init_thresholds();      //Sets up the default front & rear threshold values
    volatile uint16_t rear_distance = 100;
    volatile uint16_t forward_distance = 100;

    PMM_unlockLPM5(); //Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

    //All done initializations - turn interrupts back on.
    __enable_interrupt();

    uint16_t counter = 0;
    int button_hold_count = 0;

    while(1){
        // check for setup mode
        if(on_double_button_hold(&button_hold_count, LONG_BTN_HOLD_TIME)){
            Timer_A_stop(TIMER_A0_BASE);
            button_hold_count = 0;
            setup_mode();
        }

        // Display distances
        // Start an ADC conversion (if it's not busy) in Single-Channel, Single Conversion Mode
        if (ADCState == 0){
            display_live_distance(forward_distance, rear_distance);

            ADCState = 1; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        }

        // Triggering ultrasonic sensors
        // we delay 24 ms because the worst case is a 400 cm distance reading
        // which is 400 cm * 58 = 23,200 us

        counter++;
        if(counter > 10){
            // for some reason this if statement we have to check for forward to update rear
            if(direction == FORWARD){
                direction = REAR;

                //Reset the value in the timer before sending a pulse
                reset_timer_a();

                send_trigger(TRIGGER_PORT_FWD, TRIGGER_PIN_FWD, 10);
                rear_distance = calculate_distance(time);
                if(rear_distance > 400){
                    rear_distance = 400;
                }
                turn_on_led(rear_distance, rear_thresholds);
            }else{
                direction = FORWARD;

                //Reset the value in the timer before sending a pulse
                reset_timer_a();

                send_trigger(TRIGGER_PORT_REAR, TRIGGER_PIN_REAR, 10);
                forward_distance = calculate_distance(time);

                if(forward_distance > 400){
                    forward_distance = 400;
                }
                buzzer(forward_distance, front_thresholds);
            }
            counter = 0;
        }
    }
}

void reset_timer_a(void){
    Timer_A_stop(TIMER_A1_BASE);
    Timer_A_clear(TIMER_A1_BASE);
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
}

void init_thresholds(){
    // initializing rear_thresholds
    rear_thresholds = malloc(sizeof(int) * NUM_REAR_THRESHOLDS);
    rear_thresholds[0] = DEFAULT_RED_THRESHOLD;
    rear_thresholds[1] = DEFAULT_YELLOW_THRESHOLD;
    rear_thresholds[2] = DEFAULT_ORANGE_THRESHOLD;

    // initializing front thresholds
    front_thresholds = malloc(sizeof(int) * NUM_FWD_THRESHOLDS);
    front_thresholds[0] = DEFAULT_2_BEEP_THRES;
    front_thresholds[1] = DEFAULT_4_BEEP_THRES;
}

void init_timer(void){
    Timer_A_initContinuousModeParam params = {0};
    params.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    params.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    params.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    params.timerClear = TIMER_A_DO_CLEAR;
    params.startTimer = true;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &params);

    Timer_A_initCaptureModeParam cap_params = {0};
    cap_params.captureRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
    cap_params.captureMode = TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE;
    cap_params.captureInputSelect = TIMER_A_CAPTURE_INPUTSELECT_CCIxA;
    cap_params.synchronizeCaptureSource = TIMER_A_CAPTURE_SYNCHRONOUS;
    cap_params.captureInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    cap_params.captureOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    Timer_A_initCaptureMode(TIMER_A1_BASE, &cap_params);

}

void Init_GPIO(void)
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    //Set LaunchPad switches as inputs - they are active low, meaning '1' until pressed
    GPIO_setAsInputPinWithPullUpResistor(SW1_PORT, SW1_PIN);
    GPIO_setAsInputPinWithPullUpResistor(SW2_PORT, SW2_PIN);

    //Set LED1 and LED2 as outputs
    //GPIO_setAsOutputPin(LED1_PORT, LED1_PIN); //Comment if using UART
    GPIO_setAsOutputPin(LED2_PORT, LED2_PIN);
}

/* Clock System Initialization */
void Init_Clock(void)
{
    /*
     * The MSP430 has a number of different on-chip clocks. You can read about it in
     * the section of the Family User Guide regarding the Clock System ('cs.h' in the
     * driverlib).
     */

    /*
     * On the LaunchPad, there is a 32.768 kHz crystal oscillator used as a
     * Real Time Clock (RTC). It is a quartz crystal connected to a circuit that
     * resonates it. Since the frequency is a power of two, you can use the signal
     * to drive a counter, and you know that the bits represent binary fractions
     * of one second. You can then have the RTC module throw an interrupt based
     * on a 'real time'. E.g., you could have your system sleep until every
     * 100 ms when it wakes up and checks the status of a sensor. Or, you could
     * sample the ADC once per second.
     */
    //Set P4.1 and P4.2 as Primary Module Function Input, XT_LF
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768);
    // Set ACLK = XT1
    CS_initClockSignal(CS_ACLK, CS_XT1CLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Initializes the XT1 crystal oscillator
    CS_turnOnXT1LF(CS_XT1_DRIVE_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
}

/* UART Initialization */
void Init_UART(void)
{
    /* UART: It configures P1.0 and P1.1 to be connected internally to the
     * eSCSI module, which is a serial communications module, and places it
     * in UART mode. This let's you communicate with the PC via a software
     * COM port over the USB cable. You can use a console program, like PuTTY,
     * to type to your LaunchPad. The code in this sample just echos back
     * whatever character was received.
     */

    //Configure UART pins, which maps them to a COM port over the USB cable
    //Set P1.0 and P1.1 as Secondary Module Function Input.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

    /*
     * UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 9600 baud rate. These
     * values were calculated using the online calculator that TI provides at:
     * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
     */

    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 17, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
        param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
        param.clockPrescalar    = 6;
        param.firstModReg       = 8;
        param.secondModReg      = 17;
        param.parity            = EUSCI_A_UART_NO_PARITY;
        param.msborLsbFirst     = EUSCI_A_UART_LSB_FIRST;
        param.numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT;
        param.uartMode          = EUSCI_A_UART_MODE;
        param.overSampling      = 1;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    // Enable EUSCI_A0 RX interrupt
    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
}

/* EUSCI A0 UART ISR - Echoes data back to PC host */
#pragma vector=USCI_A0_VECTOR
__interrupt
void EUSCIA0_ISR(void)
{
    uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus);

    if (RxStatus)
    {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, EUSCI_A_UART_receiveData(EUSCI_A0_BASE));
    }
}

/* PWM Initialization */
void Init_PWM(void)
{
    /*
     * The internal timers (TIMER_A) can auto-generate a PWM signal without needing to
     * flip an output bit every cycle in software. The catch is that it limits which
     * pins you can use to output the signal, whereas manually flipping an output bit
     * means it can be on any GPIO. This function populates a data structure that tells
     * the API to use the timer as a hardware-generated PWM source.
     *
     */
    //Generate PWM - Timer runs in Up-Down mode
    param.clockSource           = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider    = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod           = TIMER_A_PERIOD; //Defined in main.h
    param.compareRegister       = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    param.compareOutputMode     = TIMER_A_OUTPUTMODE_RESET_SET;
    param.dutyCycle             = HIGH_COUNT; //Defined in main.h

    //PWM_PORT PWM_PIN (defined in main.h) as PWM output
    GPIO_setAsPeripheralModuleFunctionOutputPin(PWM_PORT, PWM_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
}

void Init_ADC(void)
{
    /*
     * To use the ADC, you need to tell a physical pin to be an analog input instead
     * of a GPIO, then you need to tell the ADC to use that analog input. Defined
     * these in main.h for A9 on P8.1.
     */

    //Set ADC_IN to input direction
    GPIO_setAsPeripheralModuleFunctionInputPin(ADC_IN_PORT, ADC_IN_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    //Initialize the ADC Module
    /*
     * Base Address for the ADC Module
     * Use internal ADC bit as sample/hold signal to start conversion
     * USE MODOSC 5MHZ Digital Oscillator as clock source
     * Use default clock divider of 1
     */
    ADC_init(ADC_BASE,
             ADC_SAMPLEHOLDSOURCE_SC,
             ADC_CLOCKSOURCE_ADCOSC,
             ADC_CLOCKDIVIDER_1);

    ADC_enable(ADC_BASE);

    /*
     * Base Address for the ADC Module
     * Sample/hold for 16 clock cycles
     * Do not enable Multiple Sampling
     */
    ADC_setupSamplingTimer(ADC_BASE,
                           ADC_CYCLEHOLD_16_CYCLES,
                           ADC_MULTIPLESAMPLESDISABLE);

    //Configure Memory Buffer
    /*
     * Base Address for the ADC Module
     * Use input ADC_IN_CHANNEL
     * Use positive reference of AVcc
     * Use negative reference of AVss
     */
    ADC_configureMemory(ADC_BASE,
                        ADC_IN_CHANNEL,
                        ADC_VREFPOS_AVCC,
                        ADC_VREFNEG_AVSS);

    ADC_clearInterrupt(ADC_BASE,
                       ADC_COMPLETED_INTERRUPT);

    //Enable Memory Buffer interrupt
    ADC_enableInterrupt(ADC_BASE,
                        ADC_COMPLETED_INTERRUPT);
}


//ADC interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt
void ADC_ISR(void)
{
    uint8_t ADCStatus = ADC_getInterruptStatus(ADC_BASE, ADC_COMPLETED_INTERRUPT_FLAG);

    ADC_clearInterrupt(ADC_BASE, ADCStatus);

    if (ADCStatus)
    {
        ADCState = 0; //Not busy anymore
        ADCResult = ADC_getResults(ADC_BASE);
    }
}
