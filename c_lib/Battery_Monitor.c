#include <avr/interrupt.h>
#include "Battery_Monitor.h"
#include "Filter.h"

static const float BITS_TO_BATTERY_VOLTS = 2*2.56/1023;

static Filter_Data_t Battery_Filter;

/**
 * Function Battery_Monitor_Init initializes the Battery Monitor to record the current battery voltages.
 */
void Battery_Monitor_Init()
{
	// *** MEGN540 LAB3 YOUR CODE HERE ***
    ADCSRA |= (1 << ADEN);			// enables ADC ADMUX = 0100110
    ADMUX |= (1 << MUX2) | (1 << MUX1); 	// MUX sets ADC6 as input ADEN enables analog to digital conversion
    ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2); 	// divider value of 128
    ADMUX |= (1 << REFS0) | (1 << REFS1); 			// AVcc w/ external capacitor on AREF
    // TODO Need to determine numerator and denominator coefficients
    int filter_order = 4;
    float numerator[] = {0.09, 0.4, 0.6, 0.4, 0.09};  
    float denominator[] = {1.0, 0.0, 0.5, 0.0, 0.02};
    Filter_Init(&Battery_Filter, numerator, denominator, filter_order+1);
}

/**
 * Function Battery_Voltage initiates the A/D measurement and returns the result for the battery voltage.
 */
float Battery_Voltage()
{
    // A Union to assist with reading the LSB and MSB in the  16 bit register
    union { struct {uint8_t LSB; uint8_t MSB; } split; uint16_t value;} data;

    // *** MEGN540 LAB3 YOUR CODE HERE ***
    //uint16_t voltage = 0;
    //data.split.LSB = ADCL;
    //data.split.MSB = ADCH;
    //voltage = Filter_Value(&Battery_Filter, data.value);
    //return data.value * BITS_TO_BATTERY_VOLTS;

    unsigned char sreg;
    sreg = SREG;
    cli();	// disable interrupts
    ADCSRA |= (1 << ADSC);	// begin ADC with ADSC bit - returns to 0 when complete
    while(1) {
        if (ADCSRA & (1 << ADIF)) {
    		data.split.LSB = ADCL;
    		data.split.MSB = ADCH;
		//break;
		sei();	// re-enable interrupts
    		SREG = sreg;
    		return Filter_Value(&Battery_Filter, data.value * BITS_TO_BATTERY_VOLTS);
	}
    }
}
