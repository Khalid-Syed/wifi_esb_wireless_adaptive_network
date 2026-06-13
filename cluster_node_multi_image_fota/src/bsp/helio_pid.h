#ifndef __HELIO_PID_H__
#define __HELIO_PID_H__

#include <zephyr/kernel.h>

typedef struct {
    float Kp;           ///< Proportional gain
    float Ki;           ///< Integral gain
    float Kd;           ///< Derivative gain
    float T_us;         ///< Time in microseconds
    float last_error;   ///< Last error value
    float integral;     ///< Integral value
    float derivative;   ///< Derivative value
} Pid_t;

/**
 * @brief initialize PID controller
 * 
 * @param pthis Pid_t struct
 * @param kp    Proportional gain
 * @param ki    Integral gain
 * @param kd    Derivative gain
 * @param t_us  Time in microseconds
 */
void pid_initialize(Pid_t * pthis, float kp, float ki, float kd, float t_us);

/**
 * @brief Set the loop interval (in seconds) for the PID loop.
 *
 * @param loop_interval The desired loop interval in seconds.
 */
void set_loop_interval(Pid_t * pthis, float loop_interval);

/**
 * @brief Calculate the PID output based on the current error.
 *
 * @param error The current error between the target and current position of the motor.
 *
 * @return The output value from the PID controller.
 */
int16_t calculate_pid(Pid_t * pthis, int16_t error);

/**
 * @brief Set the result of the PID loop
 * 
 * @param pthis     Pid struct
 * @param error     Error value
 * @param pcur_val  Pointer to current value
 * @param min_val   Minimum value to clamp
 * @param max_val   Maximum value to clamp
 */
void pid_update(Pid_t * pthis, int16_t error, int16_t * pcur_val, int16_t min_val, uint16_t max_val);

/**
 * @brief Calculate the PID output based on the current error for backemf
 *
 * @param error The current error between the target and current position of the motor.
 *
 * @return The output value from the PID controller.
 */
int32_t calculate_bemf_pid(Pid_t * pthis, int16_t error);

/**
 * @brief Set the result of the PID loop for back emf
 * 
 * @param pthis     Pid struct
 * @param error     Error value
 * @param pcur_val  Pointer to current value
 * @param min_val   Minimum value to clamp
 * @param max_val   Maximum value to clamp
 */
void pid_update_bemf(Pid_t * pthis, int16_t error, int32_t * pcur_val, int32_t min_val, int32_t max_val);

/**
 * @brief Reset the PID variables.
 */
void reset_pid(Pid_t * pthis);

/**
 * @brief Set the PID constants.
 *
 * @param p The proportional gain constant.
 * @param i The integral gain constant.
 * @param d The derivative gain constant.
 */
void set_pid_constants(Pid_t * pthis, float p, float i, float d);

/**
 * @brief Set the maximum current for the motor (in Amps).
 *
 * @param current The desired maximum current for the motor.
 */
void set_max_current(Pid_t * pthis, float current);

/**
 * @brief Set the current sensor gain (in V/A).
 *
 * @param gain The current sensor gain.
 */
void set_current_sensor_gain(Pid_t * pthis, float gain);

/**
 * @brief Control the stepper motor using a PID loop with current limiting.
 *
 * @param target_position The desired target position for the stepper motor.
 */
void pid_control_stepper_motor(Pid_t * pthis, float target_position);

#endif /* __HELIO_PID_H__ */
