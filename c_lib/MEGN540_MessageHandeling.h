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

/*
 * This file defines the incomming message length and other details.
 */

#ifndef MEGN540_MESAGEHANDELING_H
#define MEGN540_MESAGEHANDELING_H

#include "SerialIO.h"
#include "Timing.h"

/** Message Driven State Machine Flags */
typedef struct MSG_FLAG { bool active; float duration; Time_t last_trigger_time; } MSG_FLAG_t;


MSG_FLAG_t mf_restart;       	///<-- This flag indicates that the device received a restart command from the hoast. Default inactive.
MSG_FLAG_t mf_loop_timer;    	///<-- Indicates if the system should report time to complete a loop.
MSG_FLAG_t mf_time_float_send;  ///<-- Indicates if the system should report the time to send a float.
MSG_FLAG_t mf_send_time;     	///<-- Indicates if the system should send the current time.
MSG_FLAG_t mf_encoder_count; 	/// Indicates if system should send the encoder counts
MSG_FLAG_t mf_battery_voltage; 	/// Indicates if the system should send Battery voltage
MSG_FLAG_t mf_set_PWM; 		/// Indicates if the system should set the PWM
MSG_FLAG_t mf_stop_PWM; 	/// Indicates if the system should stop PWM and disable the motor
MSG_FLAG_t mf_send_sys; 	/// Indicates if the system should send system identification
MSG_FLAG_t mf_distance; 	/// Indicates if the system should drive a distance
MSG_FLAG_t mf_velocity; 	/// Indicates if the system should speed up to a velocity

/**
 * Function MSG_FLAG_Execute indicates if the action associated with the message flag should be executed
 * in the main loop both because its active and because its time.
 * @return [bool] True for execute action, False for skip action
 */
bool MSG_FLAG_Execute( MSG_FLAG_t* p_flag);

/**
 * Function Message_Handling_Init initializes the message handling and all associated state flags and data to their default
 * conditions.
 */
void Message_Handling_Init();

/**
 * Function Message_Handler processes USB messages as necessary and sets status flags to control the flow of the program.
 * It returns true unless the program receives a reset message.
 * @return
 */
void Message_Handling_Task();

/**
 * Function MEGN540_Message_Len returns the number of bytes associated with a command string per the
 * class documentation;
 * @param cmd
 * @return Size of expected string. Returns 0 if unrecognized.
 */
uint8_t MEGN540_Message_Len( char cmd );

//functions to keep things clean
void lab1Case(char command);

void tCase(char command);

void TCase(char command);

void ECase(char command);

void BCase(char command);

#endif
