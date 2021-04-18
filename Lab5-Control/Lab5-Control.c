#include <stdbool.h>
#include <float.h>
#include <stdlib.h>

#include "../c_lib/SerialIO.h"
#include "../c_lib/Timing.h"
#include "../c_lib/Encoder.h"
#include "../c_lib/Battery_Monitor.h"
#include "../c_lib/Filter.h"
#include "../c_lib/MEGN540_MessageHandeling.h"
#include "../c_lib/MotorPWM.h"


#define PWM_TOP 380

// timer for power off
Time_t Pwr_check;

// timer for PWM
Time_t PWM_timer;
bool PWM_timer_active;

// initiate battery filter
Filter_Data_t Battery_Filter;
bool first_voltage;
int filter_order = 4;
//int filter_order = 3;
/* MatLab code */
// [b,a] = besself(4,6*2*pi)
// [b,a] = tfdata(c2d(tf(b,a),1)
// numerator = b{1}, denominator = a{1}
float numerator[] = {0, 1.00000000002831, -6.2016614795055e-18, 7.98934716362488e-22, -7.81462568477543e-37};     
float denominator[] = {1, -3.44897255468215e-11, 3.01641584463356e-22, 6.53851093191958e-37, FLT_MIN}; // last 'actual' value: 7.13251396854483e-52

// system data that is sent with q or Q command
struct __attribute__((__packed__)) { float time; int16_t PWM_L; int16_t PWM_R; int16_t Encoder_L; int16_t Encoder_R;} sysData;
// info used to send sysData  -  t_interval in milliseconds
struct __attribute__((__packed__)) { float t_interval; Time_t start_time; Time_t last_send_time; bool active; } sys_send_info;

void Set_Send_sysData();

void Set_Motor_Directions(int16_t left, int16_t right);

void Start_PWM_Timer(bool timer); 

void Check_PWM_Timer_and_PWR();

// left side controller values
float KpLeft = 0.5468;
float leftNumerator[] = {1, -0.985319268716};     
float leftDenominator[] = {3.065964279734, -3.05128354845}; // last 'actual' value: 7.13251396854483e-52

// right side controller values
float KpRight = 0.5474;
float rightNumerator[] = {1, -0.985287739004};     
float rightDenominator[] = {3.085342484782, -3.070630223786}; // last 'actual' value: 7.13251396854483e-52


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
    Motor_PWM_Init(PWM_TOP);

    // variable needed for timing the while loop
    Time_t startTime;
    // variable needed for mf_loop_timer
    bool firstCall = true;

    // Low battery warning variable
    struct __attribute__((__packed__)) { char let[7]; float volt; } msg = {
        .let = {'B','A','T',' ','L','O','W'},
        .volt = 0
    };

    // Power off message
    struct {char let[9];} Pwr_msg = {.let={'P','O','W','E','R',' ','O','F','F'}};

    // voltage variables
    float raw_voltage = Battery_Voltage();
    float filteredVoltage;
    
    // timer for low warning check
    Time_t BatWarnTimeCheck = GetTime();
    
    // timer for power off
    Pwr_check = GetTime();

    // timer for PWM
    PWM_timer_active = false;

    // timer for filter
    Time_t FilterTimer = GetTime();
 
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
    	    Motor_PWM_Init(PWM_TOP);	// initiate the PWM top to 380, for a frequency of 21 kHz
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

	    Message_Handling_Task();

        // checks if sys_send_info is active - first time
        if (sys_send_info.active && (SecondsSince(&sys_send_info.last_send_time) >= (sys_send_info.t_interval/1000))) {
            Set_Send_sysData();
        }

	    // checks set PWM message flag
        if ( MSG_FLAG_Execute( &mf_set_PWM)) {
            //set variables for future calls
            mf_set_PWM.last_trigger_time = GetTime();
	    
	    // check for time limit (PWM_data.time_limit true if 'P' was called)
	    if (Filter_Last_Output(&Battery_Filter) < 1) {		// every 5 seconds send power warning and disable motor
	    	Motor_PWM_Enable(0);
	    	if (SecondsSince(&Pwr_check) > 5) { 		// power off warning
	    		Pwr_check = GetTime();
	    		usb_send_msg("c9s", '!', &Pwr_msg, sizeof(Pwr_msg));
			mf_set_PWM.active = false;
	    	}
	    }	    
	    else if (Filter_Last_Output(&Battery_Filter) > 4.75) 		// if voltage is high enough for Motors
	    {
	    	Motor_PWM_Enable(1);

		    Set_Motor_Directions(PWM_data.left_PWM, PWM_data.right_PWM);
	    	
	    	Motor_PWM_Left(abs(PWM_data.left_PWM));		// set the left motor pwm
	    	Motor_PWM_Right(abs(PWM_data.right_PWM));	// set the right motor PWM

		    Start_PWM_Timer(PWM_data.time_limit);		// start timer if needed (P call)

		    mf_set_PWM.active = false;
		    PWM_data_init();
	    }
	    else Motor_PWM_Enable(0);
        }

	    Check_PWM_Timer_and_PWR();

        // checks stop PWM message flag
        if ( MSG_FLAG_Execute( &mf_stop_PWM ) ) {
            //set variables for future calls
            mf_stop_PWM.last_trigger_time = GetTime();
	        mf_set_PWM.active = false;
	        Motor_PWM_Left(0);
	        Motor_PWM_Right(0);
	        Motor_PWM_Enable(0);
	        mf_stop_PWM.active = false;
        }
        
        // checks send sys message flag
        if ( MSG_FLAG_Execute( &mf_send_sys ) ) {
            sys_send_info.start_time = GetTime();
            //set variables for future calls
            if (mf_send_sys.duration > 0) {
                sys_send_info.active = true;
                sys_send_info.t_interval = mf_send_sys.duration;
                Set_Send_sysData();
            } 
            else sys_send_info.active = false;

            mf_send_sys.duration = -1;             // reset mf flag
            mf_send_sys.active = false;
        }

        // checks if sys_send_info is active - second time
        if (sys_send_info.active && (SecondsSince(&sys_send_info.last_send_time) >= (sys_send_info.t_interval/1000))) {
            Set_Send_sysData();
        }

        // check position mode
        if( MSG_FLAG_Execute( &mf_distance ) )
        {

        }

        // check velocity mode
        if( MSG_FLAG_Execute( &mf_velocity ))
        {

        }

   }
}

void Set_Send_sysData()
{
    sys_send_info.last_send_time = GetTime();
    sysData.time = SecondsSince(&sys_send_info.start_time);
    sysData.PWM_L = Get_Motor_PWM_Left();
    sysData.PWM_R = Get_Motor_PWM_Right();
    sysData.Encoder_L = Counts_Left();
    sysData.Encoder_R = Counts_Right();
    usb_send_msg("cf4h", 'q', &sysData, sizeof(sysData));
}
/**
 * Set_Motor_direction() takes a float value for each motor and depending on their sign (+/-) sets the 
 * motor directions
 * @param left - controls the left direction
 * @param right - controls the right direction
 */
void Set_Motor_Directions(int16_t left, int16_t right)
{
	// set left motor directions - pins pb1 and pb2 control right and left directions - fwd = 0; bwd = 1
	if (left >= 0) PORTB &= ~(1 << PORTB2);
	else if (left < 0) PORTB |= (1 << PORTB2);
	
	if (right >= 0) PORTB &= ~(1 << PORTB1); 	// set right motor directions
	else if (right < 0) PORTB |= (1 << PORTB1);
}

/*
 * Start_PWM_Timer() takes a boolean value to determine if PWM_timer should be active and started
 * @param timer - true if start timer, false if timer not to start
 */
void Start_PWM_Timer(bool timer) 
{
	if( timer ) {
	    PWM_timer = GetTime();
	    PWM_data.time_limit = false;
	    PWM_timer_active = true;
	} else PWM_timer_active = false;
}

void Check_PWM_Timer_and_PWR()
{
	// if the motor is turned off while running
	if (((Get_Motor_PWM_Left() > 0) || (Get_Motor_PWM_Right() > 0)) && Filter_Last_Output(&Battery_Filter) < 4.75) {
		Motor_PWM_Enable(0);
		Motor_PWM_Left(0);
		Motor_PWM_Right(0);
		PWM_timer_active = false;
	}

	// for the PWM_timer 
	if ( PWM_timer_active && (SecondsSince(&PWM_timer) >= mf_set_PWM.duration)) {
	    Motor_PWM_Left(0);			// set the left motor pwm
	    Motor_PWM_Right(0);			// set the right motor PWM
	    PWM_timer_active = false;
	}
}

