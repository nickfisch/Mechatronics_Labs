#include <avr/interrupt.h>
#include "Battery_Monitor.h"

static const float BITS_TO_BATTERY_VOLTS =2;   
/**
 * Function Battery_Monitor_Init initializes the Battery Monitor to record the current battery voltages.
 */
void Battery_Monitor_Init()
{
	// *** MEGN540 LAB3 YOUR CODE HERE ***
    ADCSRA |= (1 << MUX2) | (1 << MUX1) | (1 << ADEN);	// MUX sets ADC6 as input ADEN enables analog to digital conversion
    ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
}

/**
 * Function Battery_Voltage initiates the A/D measurement and returns the result for the battery voltage.
 */
float Battery_Voltage()
{
    // A Union to assist with reading the LSB and MSB in the  16 bit register
    union { struct {uint8_t LSB; uint8_t MSB; } split; uint16_t value;} data;

    // *** MEGN540 LAB3 YOUR CODE HERE ***
    unsigned char sreg;
    sreg = SREG;
    cli();	// disable interrupts
    data.split.LSB = ADCL;
    data.split.MSB = ADCH;
    sei();	// re enable interrupts
    SREG = sreg;
    return data.value * BITS_TO_BATTERY_VOLTS;
}
