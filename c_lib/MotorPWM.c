/*
         MEGN540 Mechatronics Lab
    Copyright (C) Andrew Petruska, 2021.
       apetruska [at] mines [dot] edu
          www.mechanical.mines.edu
*/

/*
    Copyright (c) 2021 Andrew Petruska at Colorado School of Mines

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

/**
 * MotorPWM.h/c defines the functions necessary to interface with the Timer 1 PWM control of the motors on the
 * the Pololu Zumo 32U4 car. For information regarding Timer 1 PWM please consult Section 14 of the atmega32U4 datasheet
 * https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
 *
 * The Left motor pwm output is connected to the OC1B pin with the directionality controlled by PB2.
 * The Right motor pwm output is connected to the OC1A pin with the directionality controlled by PB1.
 *
 * For motor control we will want to use either of the phase-corrected PWM output types.  Because OC1A/B will be used to
 * send the PWM to the motor driver chip, we'll need to set the PWM frequency (TOP) through the ICR1 register, this
 * effectively sets the max PWM value, which can be a uint16 thing.  Thee  DRV8838 motor driver circuit is rated for
 * a PWM signal of up to 250kHz, you may want to experiment with a few different values, but start with a 20kHz base
 * frequency (Note that, humans can hear up-to about 20kHz, so a 10kHz PWM frequency might be audible).
 *
 * Note that the base frequency with no prescalar is given by:
 *          16Mhz / (2 * TOP), where 2 is because were using the phase-corrected mode.
 *
 * To Disable or enable the car's PWM functionality use table 14-3 to identify how to disconnect or connect the OC1A/B
 * output pin changes.
 *
 */

#include "MotorPWM.h"
#include "SerialIO.h"


/**
 * Function data_init() initializes left_PWM, right_PWM, and duration in the PWM_data struct to 0s
 */

void PWM_data_init()
{
	PWM_data.left_PWM = 0;
	PWM_data.right_PWM = 0;
	PWM_data.duration = 0;
}

/**
 * Function MotorPWM_Init initializes the motor PWM on Timer 1 for PWM based voltage control of the motors.
 * The Motor PWM system shall initialize in the disabled state for safety reasons. You should specifically enable
 * Motor PWM outputs only as necessary.
 * @param [uint16_t] MAX_PWM is the maximum PWM value to use. This controls the PWM frequency.
 */
void Motor_PWM_Init( uint16_t MAX_PWM )
{
	char sreg = SREG;
	cli();
	TCNT1 = 0;
	sei();
	SREG = sreg;

	TCCR1B |= (1 << WGM13) | (1 << CS10);		// phase correct mode and no prescaling
	TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);	// set OC1A/OC1B as outputs from right and left motors

	PWM_data_init();
	Motor_PWM_Enable(0);
	Set_MAX_Motor_PWM( MAX_PWM );
	Motor_PWM_Left(0);
	Motor_PWM_Right(0);
}

/**
 * Function MotorPWM_Enable enables or disables the motor PWM outputs.
 * @param [bool] enable (true set enable, false set disable)
 */
void Motor_PWM_Enable( bool enable )
{
	if (enable) DDRB |= (1 << DDB5) | (1 << DDB6);
	else DDRB |= (0 << DDB5) | (0 << DDB6);
}

/**
 * Function Is_Motor_PWM_Enabled returns if the motor PWM is enabled for output.
 * @param [bool] true if enabled, false if disabled
 */
bool Is_Motor_PWM_Enabled()
{
	if ( bit_is_set(DDRB, DDB5) && bit_is_set(DDRB, DDB6)) return true;
	else return false;
}

/*
 * Function Motor_PWM_Left sets the PWM duty cycle for the left motor.
 * @return [int32_t] The count number.
 */
void Motor_PWM_Left( int16_t pwm )
{
	unsigned char sreg = SREG;
	cli();
	OCR1B = pwm;
	sei();
	SREG = sreg;
}

/**
 * Function Motor_PWM_Right sets the PWM duty cycle for the right motor.
 * @return [int32_t] The count number.
 */
void Motor_PWM_Right( int16_t pwm )
{
	unsigned char sreg = SREG;
	cli();
	OCR1A = pwm;
	sei();
	SREG = sreg;
}

/**
 * Function Get_Motor_PWM_Left returns the current PWM duty cycle for the left motor. If disabled it returns what the
 * PWM duty cycle would be.
 * @return [int16_t] duty-cycle for the left motor's pwm
 */
int16_t Get_Motor_PWM_Left()
{
	unsigned char sreg = SREG;
	int16_t val;
	cli();
	val = OCR1B;
	sei();
	SREG = sreg;
    if (bit_is_set(PORTB, PORTB2)) val = -val;
	return val;
}

/**
 * Function Get_Motor_PWM_Right returns the current PWM duty cycle for the right motor. If disabled it returns what the
 * PWM duty cycle would be.
 * @return [int16_t] duty-cycle for the right motor's pwm
 */
int16_t Get_Motor_PWM_Right()
{
	unsigned char sreg = SREG;
	int16_t val;
	cli();
	val = OCR1A;
	sei();
	SREG = sreg;
    if (bit_is_set(PORTB, PORTB1)) val = -val;
	return val;
}

/**
 * Function Get_MAX_Motor_PWM() returns the PWM count that corresponds to 100 percent duty cycle (all on), this is the
 * same as the value written into ICR1 as (TOP).
 */
uint16_t Get_MAX_Motor_PWM()
{
	unsigned char sreg = SREG;
	int16_t val;
	cli();
	val = ICR1;
	sei();
	SREG = sreg;
	return val;
}

/**
 * Function Set_MAX_Motor_PWM sets the maximum pwm count. This function sets the timer counts to zero because
 * the ICR1 can cause undesired behaviors if change dynamically below the current counts.  See page 128 of the
 * atmega32U4 datasheat.
 */
void Set_MAX_Motor_PWM( uint16_t MAX_PWM )
{
	unsigned char sreg = SREG;
	cli();
	ICR1 = MAX_PWM;
	sei();
	SREG = sreg;
}
