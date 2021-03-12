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

#include <stdbool.h>
#include <float.h>

#include "../c_lib/SerialIO.h"
#include "../c_lib/Timing.h"
#include "../c_lib/Encoder.h"
#include "../c_lib/Battery_Monitor.h"
#include "../c_lib/Filter.h"
#include "../c_lib/MEGN540_MessageHandeling.h"
#include "../c_lib/Filter.h"

/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
    SetupTimer0(); 
    Encoders_Init();
    Battery_Monitor_Init();
    USB_SetupHardware();
    GlobalInterruptEnable();
    Message_Handling_Init(); // initialize message handling
    
    // variable needed for timing the while loop
    Time_t startTime;
    // variable needed for mf_loop_timer
    bool firstCall = true;

    // Low battery warning variable
    struct __attribute__((__packed__)) { char let[7]; float volt; } msg = {
        .let = {'B','A','T',' ','L','O','W'},
        .volt = 0
    };
    
    // voltage variables
    float raw_voltage = Battery_Voltage();
    float filteredVoltage;
    
    // timer for low warning check
    Time_t BatWarnTimeCheck = GetTime();
    
    // timer for filter
    Time_t FilterTimer = GetTime();
 
    // initiate battery filter
    Filter_Data_t Battery_Filter;
    bool first_voltage;
    int filter_order = 4;
    float numerator[] = {0, 0.999999999999999, 1.29622953831734e-18, 3.59331193656159e-36, -FLT_MIN}; // last 'actual' value: -1.15929453215541e-60  
    float denominator[] = {1, 7.4246023962657e-19, 1.35726528211796e-36, FLT_MIN, FLT_MIN}; // last 2 'actual' values: 1.41016301495335e-61, 5.6938286148406e-86

    Filter_Init(&Battery_Filter, numerator, denominator, filter_order+1);
    first_voltage = true;

    while( true ) {
        USB_Upkeep_Task();

        //USB_Echo_Task();// you'll want to remove this once you get your serial sorted
        Message_Handling_Task();
        
        // Below here you'll process state-machine flags.
        if ( MSG_FLAG_Execute( &mf_restart ) ) {
            SetupTimer0(); 
            Encoders_Init();
            Battery_Monitor_Init();
            USB_SetupHardware();
            GlobalInterruptEnable();
            Message_Handling_Init(); 
        }   
        
        // checks send time message flag
        if ( MSG_FLAG_Execute( &mf_send_time ) ) {
            // variable for current time
            float timer0 = GetTimeSec();
            // send current time
            usb_send_msg("cf", '0', &timer0, sizeof(timer0));
            //set variables for future calls
            mf_send_time.last_trigger_time = GetTime();
            if (mf_send_time.duration <= 0){
                mf_send_time.active = false;
            }
        } 
        
        // checks float timer message flag
        if ( MSG_FLAG_Execute( &mf_time_float_send ) ) {
            // time structure for calling secconds since with
            Time_t sentTime = GetTime();
            // float to send for opperation
            float value = 42.024;
            // send the float
            usb_send_msg("cf", 'N', &value, sizeof(value));
            // calculate the time to send the value
            float timer1 = SecondsSince(&sentTime);
            //send the time to send the float
            USB_Upkeep_Task();
            usb_send_msg("cf", '1', &timer1, sizeof(timer1));
            //set variables for future calls
            mf_time_float_send.last_trigger_time = GetTime();
            if (mf_time_float_send.duration <= 0){
                mf_time_float_send.active = false;                
            }
        } 
        
        // checks loop timer message flag
        if ( MSG_FLAG_Execute( &mf_loop_timer ) || !firstCall ) {
            if(firstCall){
                startTime = GetTime();
                firstCall = false;
            }
            else{
                // loop time
                float timer2 = SecondsSince(&startTime);
                //send message
                usb_send_msg("cf", '1', &timer2, sizeof(timer2));
                //set variables for future calls
                mf_loop_timer.last_trigger_time = GetTime();
                firstCall = true;
                if (mf_loop_timer.duration <= 0){
                    mf_loop_timer.active = false;
                }
            }
            
        } 
        
        // checks encoder message flag
        if ( MSG_FLAG_Execute( &mf_encoder_count ) ) {
            struct __attribute__((__packed__)) { float cleft; float cright; } data;
            data.cleft = Counts_Left();
            data.cright = Counts_Right();
            usb_send_msg("cf", 'L', &data.cleft, sizeof(data.cleft));
            usb_send_msg("cf", 'R', &data.cright, sizeof(data.cright));
            //set variables for future calls
            mf_encoder_count.last_trigger_time = GetTime();
            if (mf_encoder_count.duration <= 0){
                mf_encoder_count.active = false;
            }
        }
        
        // updates the battery voltage every 2 ms
        if( SecondsSince(&FilterTimer) >= 0.002) {
            FilterTimer = GetTime();
            raw_voltage = Battery_Voltage();
            if (first_voltage) {
                Filter_SetTo(&Battery_Filter, raw_voltage);
                first_voltage = false;
    	    }

            filteredVoltage = Filter_Value(&Battery_Filter, raw_voltage);
        }

        msg.volt = filteredVoltage;
        
        // checks battery message flag
        if ( MSG_FLAG_Execute( &mf_battery_voltage ) ) {
            usb_send_msg("cf", 'V', &filteredVoltage, sizeof(filteredVoltage));
            //set variables for future calls
            mf_battery_voltage.last_trigger_time = GetTime();
            if (mf_battery_voltage.duration <= 0){
                mf_battery_voltage.active = false;
            }
        }
        
        // Low battery check every 10 seconds
        if( SecondsSince(&BatWarnTimeCheck) >= 10){
            BatWarnTimeCheck = GetTime();
	    float voltage_check = Filter_Last_Output(&Battery_Filter); 
            if (voltage_check < 3.6 && voltage_check >= 2.1){
                // send low battery warning
                usb_send_msg("c7sf", '!', &msg, sizeof(msg));
            }
        }
   }
}
