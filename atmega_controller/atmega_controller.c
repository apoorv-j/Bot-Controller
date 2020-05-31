#define F_CPU 16000000UL   // Define Crystal Frequency of eYFi-Mega Board
#include <avr/io.h>        // Standard AVR IO Library
#include <util/delay.h>    // Standard AVR Delay Library
#include <avr/interrupt.h> // Standard AVR Interrupt Library
#include "uart.h"          // Third Party UART Library
#include <string.h>
#include <stdio.h>
#include "i2c.h"
#include "VL53L0X.h"
#include "millis.h"
// #include "mcp23017.h"
// #include "pid.h"

// #define MESSAGE			"Tx from ATmega 2560> Hello ESP32!\n"
#define a1 PB6 // Macro for Pin Number of motor A pin 1 right motor
#define a2 PB7 // Macro for Pin Number of motor A pin 2 left motor
#define b1 PL3 // Macro for Pin Number of motor B pin 1
#define b2 PL4 // Macro for Pin Number of motor B pin 2
#define PIN_LED_RED PH3
#define PIN_LED_BLUE PH4 // Macro for Pin Number of Blue Led
#define PIN_LED_GREEN PH5

#define PIN_USER_SW PE7 // Macro for Pin Number of User Switch

float MAX_SPEED = 0.3;

typedef struct
{
    int x;
    int y;
} tuple;

volatile unsigned int counter_switch = 0; // variable, immune to compiler optimization, to store press count
volatile unsigned int count = 0;          // Used in ISR of Timer2 to store ms elasped
unsigned int seconds = 0;                 // Stores seconds elasped
char *MESSAGE = "Tx from ATmega 2560> Hello ESP32!\n";
statInfo_t xTraStats_1, xTraStats_2, xTraStats_3; //stores the readings of VL53L0X sensors

void init_button(void)
{
    DDRE &= ~(1 << PIN_USER_SW); // Make PIN_USER_SW input
    PORTE |= (1 << PIN_USER_SW); // Turn on Internal Pull-Up resistor of PIN_USER_SW (Optional)
}
void init_switch_interrupt(void)
{

    // all interrupts have to be disabled before configuring interrupts
    cli(); // Disable Interrupts Globally

    EIMSK |= (1 << INT7); // Turn ON INT7 (alternative function of PE7 i.e Button Pin)

    EICRB |= (1 << ISC71); // Falling Edge detection on INT7
    EICRB &= ~(1 << ISC70);

    sei(); // Enable Interrupts Gloabally
}
ISR(INT7_vect)
{
    counter_switch++; // increment this when the User Switch is pressed and then released
}

void init_led()
{
    DDRH |= (1 << PIN_LED_RED) | (1 << PIN_LED_BLUE) | (1 << PIN_LED_GREEN);  // initialize the pins PIN_LED_RED,PIN_LED_BLUE,PIN_LED_GREEN of port H as output pins.
    PORTH |= (1 << PIN_LED_RED) | (1 << PIN_LED_BLUE) | (1 << PIN_LED_GREEN); // set the values that all the LEDs remains off initially
}

void led_redOn(void)
{
    PORTH &= ~(1 << PIN_LED_RED); // Make PHPIN_LED_RED Low
}

void led_redOff(void)
{
    PORTH |= (1 << PIN_LED_RED); // Make PHPIN_LED_RED High
}

void led_blueOn(void)
{
    PORTH &= ~(1 << PIN_LED_BLUE); // Make PHPIN_LED_BLUE Low
}

void led_blueOff(void)
{
    PORTH |= (1 << PIN_LED_BLUE); // Make PHPIN_LED_BLUE High
}

void led_greenOn(void)
{
    PORTH &= ~(1 << PIN_LED_GREEN); // Make PHPIN_LED_GREEN Low
}

void led_greenOff(void)
{
    PORTH |= (1 << PIN_LED_GREEN); // Make PHPIN_LED_GREEN High
}

void motion_pin_config(void) //Configure Pins as Output
{
    DDRL |= (1 << b1) | (1 << b2);  // initialize the pins of port L as output pins.
    PORTL |= (1 << b1) | (1 << b2); // set the values that all the pins remains off initially

    DDRB |= (1 << a1) | (1 << a2); // initialize the pins of port H as output pins.
    PORTB |= (1 << a1) | (1 << a2);
}

void velocity_left(unsigned char left_motor)
{
    OCR5AL = (unsigned char)left_motor;
}
void velocity_right(unsigned char right_motor)
{
    OCR5BL = (unsigned char)right_motor;
}

void velocity(unsigned char left_motor, unsigned char right_motor)
{
    OCR5AL = (unsigned char)left_motor;
    OCR5BL = (unsigned char)right_motor;
}

void timer5_init() //Set Register Values for starting Fast 8-bit PWM
{
    TCCR5A = 0xA9;
    TCCR5B = 0x00;
    TCNT5H = 0xFF;
    TCNT5L = 0x01;
    OCR5AH = 0x00;
    OCR5AL = 0xFF;
    OCR5BH = 0x00;
    OCR5BL = 0xFF;
    OCR5CH = 0x00;
    OCR5CL = 0xFF;
    TCCR5B = 0X0B;
}

void forward_left(float dutyCycle)
{
    velocity_left(255 * dutyCycle);
    PORTB &= ~(1 << a2);
}

void backward_left(float dutyCycle)
{
    dutyCycle = 1 - dutyCycle;
    velocity_left(255 * dutyCycle);
    PORTB |= (1 << a2);
}

void forward_right(float dutyCycle)
{
    velocity_right(255 * (dutyCycle));
    PORTB &= ~(1 << a1);
}
void backward_right(float dutyCycle)
{
    dutyCycle = 1 - dutyCycle;
    velocity_right(255 * dutyCycle);
    PORTB |= (1 << a1);
}

void forward(float dutyCycle)
{
    velocity(255 * dutyCycle, 255 * dutyCycle);
    PORTB &= ~(1 << a1);
    PORTB &= ~(1 << a2);
}

void backward(float dutyCycle)
{
    dutyCycle = 1 - dutyCycle;
    velocity(255 * dutyCycle, 255 * dutyCycle);
    PORTB |= (1 << a1);
    PORTB |= (1 << a2);
}

void hard_left(float dutyCycle)
{
    velocity((1 - dutyCycle) * 255, 255 * dutyCycle);
    PORTB &= ~(1 << a1);
    PORTB |= (1 << a2);
}

void hard_right(float dutyCycle)
{
    velocity(255 * dutyCycle, 255 * (1 - dutyCycle));
    PORTB &= ~(1 << a2);
    PORTB |= (1 << a1);
}

void soft_left(float dutyCycle)
{
    stopm();
    velocity(0, 255 * dutyCycle);
    PORTB &= ~(1 << a1);
}

void soft_right(float dutyCycle)
{
    stopm();
    velocity(255 * dutyCycle, 0);
    PORTB &= ~(1 << a2);
}

void stopm()
{
    velocity(0, 0);
    PORTB &= ~(1 << a1);
    PORTB &= ~(1 << a2);
}

uint8_t uart0_readByte(void)
{

    uint16_t rx;
    uint8_t rx_status, rx_data;

    rx = uart0_getc();
    rx_status = (uint8_t)(rx >> 8);
    rx = rx << 8;
    rx_data = (uint8_t)(rx >> 8);

    if (rx_status == 0 && rx_data != 0)
    {
        return rx_data;
    }
    else
    {
        return -1;
    }
}

int main(void)
{
    uart0_init(UART_BAUD_SELECT(115200, F_CPU));
    uart0_flush();

    timer5_init();
    motion_pin_config();
    init_led();
    init_button();
    init_switch_interrupt();
    char rx_byte[100];
    while (1)
    {
        char rx_byte = uart0_readByte();
        // uart0_puts(rx_byte);
        if (rx_byte != -1)
        {
            if (rx_byte == '.')
            {
                stopm();
            }
            else if (rx_byte == 'w')
            {
                forward(0.5);
            }
            else if (rx_byte == 'a')
            {
                hard_left(0.65);
            }
            else if (rx_byte == 'd')
            {
                hard_right(0.5);
            }
            else if (rx_byte == 'z')
            {
                soft_left(0.65);
            }
            else if (rx_byte == 'c')
            {
                soft_right(0.5);
            }
            else if (rx_byte == 's')
            {
                stopm();
            }
            else if (rx_byte == 'r')
            {
                stopm();
                led_redOn();
                _delay_ms(1000);
                led_redOff();
            }
            else if (rx_byte == 'g')
            {
                stopm();
                led_greenOn();
                _delay_ms(1000);
                led_greenOff();
            }
        }
    }
    return 0;
}