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

#include "../c_lib/Timing.h"


/** These define the internal counters that will be updated in the ISR to keep track of the time
 *  The volatile keyword is because they are changing in an ISR, the static means they are not
 *  visible (not global) outside of this file.
 */
static volatile uint32_t _count_ms = 0;

/**
 * Function SetupTimer0 initializes Timer0 to have a prescalar of XX and initializes the compare
 * feature for use in an ISR.  It also enables ISR's.
 *
 *  FCPU = 16 000 000 Hz,  1 us => 1 000 000 Hz,  A prescalar of 16 would give you millisecond resolution, but you dont
 *  have enough time to do those additions every microsecond, in addition the 8-bit timer will roll over every 256 microseconds.
 *  So, you need to be counting in (effectively) 4us increments.  So use a prescalar of 64. This will give you a resolution of 4us per
 *  Timer0 Bit and enough head-room to compare at with 249 for a 1kHz update.
 *
 *  Since Timer 0 will be triggering at a kHz, we may want other things to be updated at 1kHz too.
 *
 */
void SetupTimer0()
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    // Enable timing, setup prescalers, etc.

    TCNT0 = 0;	 // set timer value to 0
    TCCR0B |= (1 << CS00);
    TCCR0B |= (1 << CS01);
    OCR0A = 249; // set the top of the compare to 249    
    TIMSK0 |= (1 << OCIE0A);
    sei(); 	// global interupt enable
    
    _count_ms= 0;
    ms_counter_1 = 0;
    ms_counter_2 = 0;
    ms_counter_3 = 0;
    ms_counter_4 = 0;
}

/**
 * This function gets the current time and returns it in a Time_t structure.
 * @return
 */
float  GetTimeSec()
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    return (float)_count_ms/1000 + ((float) GetMicro()/1000000);
}
Time_t GetTime()
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    Time_t time;
    time.millisec = _count_ms;
    // timer0 value increments every 4 microseconds, so multiply by 4
    time.microsec = TCNT0*4; 
    return time;
}

/**
 * These functions return the individual parts of the Time_t struct, useful if you only care about
 * things on second or millisecond resolution.
 * @return
 */
uint32_t GetMilli()
{
    return _count_ms;
}
uint16_t GetMicro()
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    return TCNT0*4;
}


/**
 * This function takes a start time and calculates the time since that time, it returns it in the Time struct.
 * @param p_time_start a pointer to a start time struct
 * @return (Time_t) Time since the other time.
 */
float  SecondsSince(const Time_t* time_start_p )
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    float delta_time = GetTimeSec() - ((float)time_start_p->microsec/1000000) - ((float)time_start_p->millisec/1000);
    return delta_time;
}

/** This is the Interrupt Service Routine for the Timer0 Compare A feature.
 * You'll need to set the compare flags properly for it to work.
 */
ISR(TIMER0_COMPA_vect)
{
    // *** MEGN540 Lab 2 ***
    // YOUR CODE HERE
    // YOU NEED TO RESET THE Timer0 Value to 0 again!
    TCNT0 = 0;
    
    // take care of upticks of both our internal and external variables.
    _count_ms ++;

    ms_counter_1 ++;
    ms_counter_2 ++;
    ms_counter_3 ++;
    ms_counter_4 ++;
}
