#ifndef MOTOR_CONSTANTS_H
#define MOTOR_CONSTANTS_H

#define MOTOR_CONST_MAX_CURR_LIM 2000
#define MOTOR_CONST_MIN_STEP_PERIOD_USEC 4000
#define MOTOR_CONST_MAX_STEP_PERIOD_USEC 24000
#define MOTOR_CONST_PID_CLAMP_USEC 16000
#define MOTOR_CONST_PID_OFFSET_USEC 18000

#define BEMF_MQTT_DATA_BUFF_LEN 600

typedef enum
{
    PWM_0,
    PWM_1,
    PWM_BOTH
} PwmSelect_t;

typedef enum
{
    MOTOR_0,
    MOTOR_1,
    MOTOR_BOTH
} MotorSelect_t;

typedef enum
{
    SPIN_CW,
    SPIN_CCW
} SpinDirection_t;

#endif // MOTOR_CONSTANTS_H