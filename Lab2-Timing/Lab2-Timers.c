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

#include "../c_lib/SerialIO.h"
#include "../c_lib/Timing.h"
#include "../c_lib/MEGN540_MessageHandeling.h"

/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
    SetupTimer0(); 
    USB_SetupHardware();
    GlobalInterruptEnable();
    Message_Handling_Init(); // initialize message handling
    
    // variable needed for timing the while loop
    Time_t startTime;
    // variable needed for mf_loop_timer
    bool firstCall = true;

    while( true )
    {
        USB_Upkeep_Task();

        //USB_Echo_Task();// you'll want to remove this once you get your serial sorted
        Message_Handling_Task();
        
        // Below here you'll process state-machine flags.
        if ( MSG_FLAG_Execute( &mf_restart ) ) {
            SetupTimer0(); 
            USB_SetupHardware();
            GlobalInterruptEnable();
            Message_Handling_Init(); 
        }   
        
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
   }
}
