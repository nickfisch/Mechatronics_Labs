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

#include "MEGN540_MessageHandeling.h"
#include "SerialIO.h"
#include "MotorPWM.h"
#include "Timing.h"

static inline void MSG_FLAG_Init(MSG_FLAG_t* p_flag)
{
    p_flag->active = false;
    p_flag->duration = -1;
    p_flag->last_trigger_time.millisec=0;
    p_flag->last_trigger_time.microsec=0;
}


/**
 * Function MSG_FLAG_Execute indicates if the action associated with the message flag should be executed
 * in the main loop both because its active and because its time.
 * @return [bool] True for execute action, False for skip action
 */
bool MSG_FLAG_Execute( MSG_FLAG_t* p_flag)
{
    // *** MEGN540  ***
    // THIS FUNCTION WILL BE MOST USEFUL FORM LAB 2 ON.
    // What is the logic to indicate an action should be executed?
    // For Lab 1, ignore the timing part.
    if (p_flag->active == true){
        // lower case t call
        if (p_flag->duration < 0) {
            return true;
        }
        // upercase T call
        else if (p_flag->duration <= SecondsSince(&p_flag->last_trigger_time)) {
            return true;
        }
    }
    return false;
}


/**
 * Function Message_Handling_Init initializes the message handling and all associated state flags and data to their default
 * conditions.
 */
void Message_Handling_Init()
{
    // *** MEGN540  ***
    // YOUR CODE HERE. This is where you'd initialize any
    // state machine flags to control your main-loop state machine

    MSG_FLAG_Init( &mf_restart ); // needs to be initialized to the default values.
    MSG_FLAG_Init( &mf_loop_timer );
    MSG_FLAG_Init( &mf_time_float_send );
    MSG_FLAG_Init( &mf_send_time );
    MSG_FLAG_Init( &mf_encoder_count );
    MSG_FLAG_Init( &mf_battery_voltage );
    MSG_FLAG_Init( &mf_set_PWM ); 
    MSG_FLAG_Init( &mf_stop_PWM );
    MSG_FLAG_Init( &mf_send_sys );
    MSG_FLAG_Init( &mf_distance );
    MSG_FLAG_Init( &mf_velocity ); 
    return;
}

/**
 * Function Message_Handler processes USB messages as necessary and sets status flags to control the flow of the program.
 * It returns true unless the program receives a reset message.
 * @return
 */
void Message_Handling_Task()
{
    // *** MEGN540  ***
    // YOUR CODE HERE. I suggest you use your peak function and a switch interface
    // Either do the simple stuff strait up, set flags to have it done later.
    // If it just is a USB thing, do it here, if it requires other hardware, do it in the main and
    // set a flag to have it done here.

    // Check to see if there is data in waiting
    if( !usb_msg_length() )
        return; // nothing to process...

    // Get Your command designator without removal so if their are not enough bytes yet, the command persists
    char command = usb_msg_peek();
    
    // check if mesasage is fully in buffer
    if (usb_msg_length() < MEGN540_Message_Len(command))
        return;
        
    // send for testing as an echo function
    //usb_send_byte(usb_msg_get());
    //return;

    // process command
    switch( command )
    {
        case '*':
            if( usb_msg_length() >= MEGN540_Message_Len('*') )
            {
                lab1Case(command);
            }
            break;
        case '/':
            if( usb_msg_length() >= MEGN540_Message_Len('/') )
            {
                lab1Case(command);
            }
            break;
        case '+':
            if( usb_msg_length() >= MEGN540_Message_Len('+') )
            {
                lab1Case(command);
            }
            break;
        case '-':
            if( usb_msg_length() >= MEGN540_Message_Len('-') )
            {
                lab1Case(command);
            }
            break;
        case '~':
            if( usb_msg_length() >= MEGN540_Message_Len('~') )
            {
                //then process your reset by setting the mf_restart flag
                mf_restart.active = true;
            }
            break;
        case 't':
            if( usb_msg_length() >= MEGN540_Message_Len('t') )
            {
                tCase(command);
            }
            break;
        case 'T':
            if( usb_msg_length() >= MEGN540_Message_Len('T') )
            {
                TCase(command);
            }
            break;
        case 'e':
            if( usb_msg_length() >= MEGN540_Message_Len('e') )
            {
                usb_msg_get();
                mf_encoder_count.active = true;
            }
            break;
        case 'E':
            if( usb_msg_length() >= MEGN540_Message_Len('E') )
            {
                ECase(command);
            }
            break;
        case 'b':
            if( usb_msg_length() >= MEGN540_Message_Len('b') )
            {
                usb_msg_get();
                mf_battery_voltage.active = true;
            }
            break;
        case 'B':
            if( usb_msg_length() >= MEGN540_Message_Len('B') )
            {
                BCase(command);
            }
            break;
        case 'p':
            if( usb_msg_length() >= MEGN540_Message_Len('p') )
            {
                usb_msg_get();
		        // read left and right PWM values into PWM_data 
		        usb_msg_read_into(&PWM_data.left_PWM, sizeof(PWM_data.left_PWM));
		        usb_msg_read_into(&PWM_data.right_PWM, sizeof(PWM_data.right_PWM));
		        PWM_data.time_limit = false;

		        mf_set_PWM.active = true;	
	        }
	        break;
        case 'P':
            if( usb_msg_length() >= MEGN540_Message_Len('P') )
            {
                usb_msg_get();
		        // read left and right PWM values into volatile data 
		        usb_msg_read_into(&PWM_data.left_PWM, sizeof(PWM_data.left_PWM));
		        usb_msg_read_into(&PWM_data.right_PWM, sizeof(PWM_data.right_PWM));
		        usb_msg_read_into(&PWM_data.duration, sizeof(PWM_data.duration));
		        PWM_data.time_limit = true;

		        mf_set_PWM.active = true;
		        mf_set_PWM.duration = PWM_data.duration/1000;    // in seconds
            }
            break;
        case 's':
            if( usb_msg_length() >= MEGN540_Message_Len('s') )
            {
                usb_msg_get();
		        mf_stop_PWM.active = true;
            }
            break;
        case 'S':
            if( usb_msg_length() >= MEGN540_Message_Len('S') )
            {
                usb_msg_get();
                mf_stop_PWM.active = true;
            }
            break;
        case 'q':
            if( usb_msg_length() >= MEGN540_Message_Len('q') )
            {
                usb_msg_get();
		        mf_send_sys.active = true;
		        mf_send_sys.duration = -1;
            }
            break;
        case 'Q':
            if( usb_msg_length() >= MEGN540_Message_Len('Q') )
            {
                usb_msg_get();
   		        mf_send_sys.active = true;
		        // float for calculating duration
   		        float dur;
   		        usb_msg_read_into(&dur, sizeof(dur));
   		        // switch to see witch type of call it is
   		        if (dur <= 0) {
   		            mf_send_sys.duration = -1;
   		            return;
   		        }
   		        mf_send_sys.duration = dur;
            }
            break;
        case 'd':
            if( usb_msg_length() >= MEGN540_Message_Len('d') )
            {
                usb_msg_get();
		        mf_distance.active = true;
            }
            break;
        case 'D':
            if( usb_msg_length() >= MEGN540_Message_Len('D') )
            {
                usb_msg_get();
		        mf_distance.active = true;
            }
            break;
        case 'v':
            if( usb_msg_length() >= MEGN540_Message_Len('v') )
            {
                usb_msg_get();
		        mf_velocity.active = true;
            }
            break;
        case 'V':
            if( usb_msg_length() >= MEGN540_Message_Len('V') )
            {
                usb_msg_get();
		        mf_velocity.active = true;
            }
            break;
        default:
            // What to do if you dont recognize the command character
            usb_send_msg("cc", '?', &command, sizeof(command));
            usb_flush_input_buffer();
            break;
    }
    
}

void lab1Case(char command){
    //then process your times...

    // remove the command from the usb recieved buffer using the usb_msg_get() function
    usb_msg_get(); // removes the first character from the received buffer, we already know it was a * so no need to save it as a variable

    // Build a meaningful structure to put your data in. Here we want two floats.
    struct __attribute__((__packed__)) { float v1; float v2; } data;

    // Copy the bytes from the usb receive buffer into our structure so we can use the information
    usb_msg_read_into( &data, sizeof(data) );
    
    //switch to do the */-+
    float ret_val;
    switch ( command ) {
        case '*':
            ret_val = data.v1 * data.v2;
            break;
        case '/':
            ret_val = data.v1 / data.v2;
            break;
        case '+':
            ret_val = data.v1 + data.v2;
            break;
        case '-':
            ret_val = data.v1 - data.v2;
            break;
        default:
            break;
    }

    // send response right here if appropriate.
    usb_send_msg("cf", command, &ret_val, sizeof(ret_val));
}

void tCase(char command){
    usb_msg_get();
                
    char num = usb_msg_get();
    // switch to see witch type of call it is
    switch(num){
        //Time Now
        case 0: ;
            mf_send_time.active = true;
            return;
        //Time to send float
        case 1: ;
            mf_time_float_send.active = true;
            return;
        //Time to complete a full loop iteration
        case 2: ;
            mf_loop_timer.active = true;
            return;
        default:
            usb_send_msg("c", command, "?", sizeof("?"));
            return;
    }
}

void TCase(char command){
    usb_msg_get();
    // number for switch statement
    char num = usb_msg_get();
    // float for calculating duration
    float dur;
    usb_msg_read_into(&dur, sizeof(dur));
    // switch to see witch type of call it is
    if (dur <= 0) {
        mf_send_time.active = false;
        mf_time_float_send.active = false;
        mf_loop_timer.active = false;
        mf_send_time.duration = -1;
        mf_time_float_send.duration = -1;
        mf_loop_timer.duration = -1;
        return;
    }
        
    switch(num){
        //Time Now
        case 0: ;
            mf_send_time.active = true;
            mf_send_time.duration = dur/1000;
            return;
        //Time to send float
        case 1: ;
            mf_time_float_send.active = true;
            mf_time_float_send.duration = dur/1000;
            return;
        //Time to complete a full loop iteration
        case 2: ;
            mf_loop_timer.active = true;
            mf_loop_timer.duration = dur/1000;
            return;
        default:
            usb_send_msg("c", command, "?", sizeof("?"));
            return;
    }
}

void ECase(char command){
    usb_msg_get();
    // float for calculating duration
    float dur;
    usb_msg_read_into(&dur, sizeof(dur));
    // switch to see witch type of call it is
    if (dur <= 0) {
        mf_encoder_count.active = false;
        mf_encoder_count.duration = -1;
        return;
    }
    mf_encoder_count.active = true;
    mf_encoder_count.duration = dur/1000;
}

void BCase(char command){
    usb_msg_get();
    // float for calculating duration
    float dur;
    usb_msg_read_into(&dur, sizeof(dur));
    // switch to see witch type of call it is
    if (dur <= 0) {
        mf_battery_voltage.active = false;
        mf_battery_voltage.duration = -1;
        return;
    }
    mf_battery_voltage.active = true;
    mf_battery_voltage.duration = dur/1000;
}

/**
 * Function MEGN540_Message_Len returns the number of bytes associated with a command string per the
 * class documentation;
 * @param cmd
 * @return Size of expected string. Returns 0 if unreconized.
 */
uint8_t MEGN540_Message_Len( char cmd )
{
    switch(cmd)
    {
        case '~': return	1; break;
        case '*': return	9; break;
        case '/': return	9; break;
        case '+': return	9; break;
        case '-': return	9; break;
        case 't': return	2; break;
        case 'T': return	6; break;
        case 'e': return	1; break;
        case 'E': return	5; break;
        case 'b': return	1; break;
        case 'B': return	5; break;
//        case 'a': return	1; break;
//        case 'A': return 	5; break;
//        case 'w': return	1; break;
//        case 'W': return 	5; break;
//        case 'm': return	1; break;
//        case 'M': return	5; break;
        case 'p': return	5; break;
        case 'P': return	9; break;
        case 's': return 	1; break;
        case 'S': return 	1; break;
        case 'q': return	1; break;
        case 'Q': return 	5; break;
        case 'd': return 	9; break;
        case 'D': return	13; break;
        case 'v': return	9; break;
        case 'V': return	13; break;
        default:  return	0; break;
    }
}
