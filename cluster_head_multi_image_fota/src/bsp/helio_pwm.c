#include "../hal/helio_pwm.h"
#include "../bsp/heliogen_io.h"

static uint16_t pwm_duty_block_0[4] = {OFF_DUTY_CNT, OFF_DUTY_CNT, OFF_DUTY_CNT, OFF_DUTY_CNT};

int32_t helio_pwm_init()
{
    // TODO: init boost_gate
    NRF_PWM0->PSEL.OUT[0] = (BOOST_GATE << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);

    NRF_PWM0->ENABLE = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);

    NRF_PWM0->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);

    NRF_PWM0->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);

	// 47 kHz ~ period of 21 usec, count 336
	// 91 kHz ~ period of 11 usec, count 176
    // 21 kHz ~ period of 47 usec, count 752
    NRF_PWM0->COUNTERTOP = (OFF_DUTY_CNT << PWM_COUNTERTOP_COUNTERTOP_Pos); // 800 count = 50 usec (20 kHz), 320 count = 20 usec (50 kHz)

    NRF_PWM0->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);

    NRF_PWM0->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

	// Setup pointer to pwm duty block, the values here will set the duty cycle
    NRF_PWM0->SEQ[0].PTR = ((uint32_t)(pwm_duty_block_0) << PWM_SEQ_PTR_PTR_Pos);

    NRF_PWM0->SEQ[0].CNT = ((sizeof(pwm_duty_block_0) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);

    NRF_PWM0->SEQ[0].REFRESH = 0;

    NRF_PWM0->SEQ[0].ENDDELAY = 0;

    NRF_PWM0->TASKS_SEQSTART[0] = 1;

    // TODO: check if the PWM init worked:
    return 0;
}

void update_duty_block(uint16_t new_duty)
{
    pwm_duty_block_0[0] = OFF_DUTY_CNT - new_duty;
}

void turn_off_pwm()
{
    pwm_duty_block_0[0] = OFF_DUTY_CNT;
    NRF_PWM0->TASKS_SEQSTART[0] = 1;
}

void start_pwm_sequence()
{
	NRF_PWM0->TASKS_SEQSTART[0] = 1;
}