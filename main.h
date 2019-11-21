#ifndef MAIN_H_
#define MAIN_H_

#include "driverlib/driverlib.h"

#define TIMER_A_PERIOD  1000 //T = 1/f = (TIMER_A_PERIOD * 1 us)
#define HIGH_COUNT      500  //Number of cycles signal is high (Duty Cycle = HIGH_COUNT / TIMER_A_PERIOD)

// button hold thresholds
#define LONG_BTN_HOLD_TIME 75
#define SHORT_BTN_HOLD_TIME 700
#define TOGGLE_TIME 450

// rear thresholds
#define NUM_REAR_THRESHOLDS 3
#define DEFAULT_RED_THRESHOLD 20
#define DEFAULT_YELLOW_THRESHOLD 40
#define DEFAULT_ORANGE_THRESHOLD 60

// front thresholds
#define NUM_FWD_THRESHOLDS 2
#define DEFAULT_2_BEEP_THRES 10
#define DEFAULT_4_BEEP_THRES 20

//Output pin to buzzer
#define PWM_PORT        GPIO_PORT_P1
#define PWM_PIN         GPIO_PIN7
//LaunchPad LED1 - note unavailable if UART is used
#define LED1_PORT       GPIO_PORT_P1
#define LED1_PIN        GPIO_PIN0
//LaunchPad LED2
#define LED2_PORT       GPIO_PORT_P4
#define LED2_PIN        GPIO_PIN0
//LaunchPad Pushbutton Switch 1
#define SW1_PORT        GPIO_PORT_P1
#define SW1_PIN         GPIO_PIN2
//LaunchPad Pushbutton Switch 2
#define SW2_PORT        GPIO_PORT_P2
#define SW2_PIN         GPIO_PIN6
//Input to ADC - in this case input A9 maps to pin P8.1
#define ADC_IN_PORT     GPIO_PORT_P8
#define ADC_IN_PIN      GPIO_PIN1
#define ADC_IN_CHANNEL  ADC_INPUT_A9

void Init_GPIO(void);
void Init_Clock(void);
void Init_UART(void);
void Init_PWM(void);
void Init_ADC(void);
void init_timer(void);
void reset_timer_a(void);
void init_thresholds(void);

extern const char* rear_threshold_names[NUM_REAR_THRESHOLDS];
extern const char* front_threshold_names[NUM_FWD_THRESHOLDS];

int on_double_button_hold(int *count, int hold_time);
void on_single_button_hold(int *count, int *button_state, int *both_pressed, int port, int pin);
void on_button_click(int *distance, int *counter, int *button_state, int min_distance, int port, int pin);
int adjust_distance(int min_distance, int init_distance_val);
void user_mode(void);
void set_distance_thresholds(int *thresholds, int len);

Timer_A_outputPWMParam param; //Timer configuration data structure for PWM

#endif /* MAIN_H_ */
