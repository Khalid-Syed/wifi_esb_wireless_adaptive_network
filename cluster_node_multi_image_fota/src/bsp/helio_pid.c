/**
 * @file helio_pid.c
 *
 * @brief Util functions for simple PID control
 */

#include "helio_pid.h"

// TODO: determine anti-windup clamp values
#define MIN_INTEGRAL 0
#define MAX_INTEGRAL 100
#define MAX_BEMF_INTEGRAL 16000

/**
 * @brief initialize PID controller
 * 
 * @param pthis     Pid_t struct
 * @param kp        Proportional gain
 * @param ki        Integral gain
 * @param kd        Derivative gain
 * @param t_us      Time in microseconds
 */
void pid_initialize(Pid_t * pthis, float kp, float ki, float kd, float t_us)
{
    if (pthis == 0)
    {
        printk("Invalid Pid_t");   
        return; // ERROR: invalid Pid_t
    }

    NRFX_ASSERT(t_us > 0);

    pthis->Kp = kp;
    pthis->Ki = ki;
    pthis->Kd = kd;
    pthis->T_us = t_us;
}

/**
 * @brief Set the loop interval (in seconds) for the PID loop.
 *
 * @param loop_interval The desired loop interval in seconds.
 */
void set_loop_interval(Pid_t * pthis, float loop_interval) {
    pthis->T_us = loop_interval;
}

// TODO: discuss with Davis regarding back-calc/dynamic reset
/*
int16_t calculate_pid(Pid_t * pthis, int16_t error) {
    float derivative = (error - pthis->last_error) / pthis->T_us;
    float prev_integral = pthis->integral;
    pthis->integral += error * pthis->T_us;
    float output_without_clamp = pthis->Kp * error + pthis->Ki * pthis->integral + pthis->Kd * derivative;
    float max_change = pthis->output_saturation - output_without_clamp;
    if (max_change < 0.0f) {
        max_change = 0.0f;
    }
    pthis->integral = prev_integral + max_change / pthis->Ki;
    pthis->integral = CLAMP(pthis->integral, pthis->min_integral, pthis->max_integral);
    pthis->last_error = error;
    float output = pthis->Kp * error + pthis->Ki * pthis->integral + pthis->Kd * derivative;
    return (int16_t) output;
}
*/

/**
 * @brief Calculate the PID output based on the current error.
 *
 * @param error The current error between the target and current position of the motor.
 *
 * @return The output value from the PID controller.
 */
int16_t calculate_pid(Pid_t * pthis, int16_t error) {
    float derivative = (error - pthis->last_error) / pthis->T_us;
    pthis->integral += error * pthis->T_us;
    pthis->integral = CLAMP(pthis->integral, MIN_INTEGRAL, MAX_INTEGRAL); // TODO: check negative max_integral
    pthis->last_error = error;
    float output = pthis->Kp * error + pthis->Ki * pthis->integral + pthis->Kd * derivative;
    return (int16_t) output;
}

/**
 * @brief Set the result of the PID loop
 * 
 * @param pthis     Pid struct
 * @param error     Error value
 * @param pcur_val  Pointer to current value
 * @param min_val   Minimum value to clamp
 * @param max_val   Maximum value to clamp
 */
void pid_update(Pid_t * pthis, int16_t error, int16_t * pcur_val, int16_t min_val, uint16_t max_val)
{
    *pcur_val = calculate_pid(pthis, error);
    *pcur_val = CLAMP(*pcur_val, min_val, max_val);
}

/**
 * @brief Calculate the PID output based on the current error for backemf
 *
 * @param error The current error between the target and current position of the motor.
 *
 * @return The output value from the PID controller.
 */
int32_t calculate_bemf_pid(Pid_t * pthis, int16_t error) {
    float derivative = (error - pthis->last_error) / pthis->T_us;
    pthis->integral += error * pthis->T_us;
    pthis->integral = CLAMP(pthis->integral, -MAX_BEMF_INTEGRAL, MAX_BEMF_INTEGRAL);
    pthis->last_error = error;
    float output = pthis->Kp * error + pthis->Ki * pthis->integral + pthis->Kd * derivative;
    return (int32_t) output;
}

/**
 * @brief Set the result of the PID loop for back emf
 * 
 * @param pthis     Pid struct
 * @param error     Error value
 * @param pcur_val  Pointer to current value
 * @param min_val   Minimum value to clamp
 * @param max_val   Maximum value to clamp
 */
void pid_update_bemf(Pid_t * pthis, int16_t error, int32_t * pcur_val, int32_t min_val, int32_t max_val)
{
    *pcur_val = calculate_bemf_pid(pthis, error);
    *pcur_val = CLAMP(*pcur_val, min_val, max_val);
}

/**
 * @brief Reset the PID variables.
 */
void reset_pid(Pid_t * pthis) {
    pthis->last_error = 0;
    pthis->integral = 0;
}

/**
 * @brief Set the PID constants.
 *
 * @param p The proportional gain constant.
 * @param i The integral gain constant.
 * @param d The derivative gain constant.
 */
void set_pid_constants(Pid_t * pthis, float p, float i, float d) {
    pthis->Kp = p;
    pthis->Ki = i;
    pthis->Kd = d;
}

/**
 * @brief Set the maximum current for the motor (in Amps).
 *
 * @param current The desired maximum current for the motor.
 */
void set_max_current(Pid_t * pthis, float current) {
    printk("set max gain");
}

/**
 * @brief Set the current sensor gain (in V/A).
 *
 */
void set_current_sensor_gain(Pid_t * pthis, float gain)
{
    printk("set current gain");
}

/**
 * @brief Control the stepper motor using a PID loop with current limiting.
 *
 * @param target_position The desired target position for the stepper motor.
 */
void pid_control_stepper_motor(Pid_t * pthis, float target_position)
{
    printk("pid control stepper");
}
