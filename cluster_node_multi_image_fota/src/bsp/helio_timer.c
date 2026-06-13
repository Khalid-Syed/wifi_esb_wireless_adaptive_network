#include "helio_timer.h"
#include "heliogen_io.h"

/**
 * @brief Init the hardware timer for stepper phase changes
 * @details 24 bit timer and prescaler of 4 to get 1us resolution
 *          the timer will be cleared on a COMPARE[0] event
 * 
 * @param block PWM block specified which corresponds to the hardware timer
 * @param interval_usec Timer period in microseconds
 */
void helio_init_stepper_timer(PwmSelect_t block, uint32_t interval_usec)
{
    // Timer 0 will use half the interval for first timeout
    // And use CC[1] as the clearing event
    if (block == PWM_0)
    {
        NRF_TIMER0->TASKS_STOP = 1;
        NRF_TIMER0->MODE        = TIMER_MODE_MODE_Timer;
        NRF_TIMER0->BITMODE     = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
        NRF_TIMER0->PRESCALER   = 4;  // 1us resolution

        NRF_TIMER0->TASKS_CLEAR = 1;            // clear the task first to be usable for later
        NRF_TIMER0->CC[0] = interval_usec / 2;  // first delay
        NRF_TIMER0->CC[1] = interval_usec;      // step frequency

        NRF_TIMER0->INTENSET    = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        NRF_TIMER0->INTENSET    = TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos;

        /* Create an Event-Task shortcut to clear TIMER0 on COMPARE[1] event. */
        NRF_TIMER0->SHORTS      = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos);
    }
    else if (block == PWM_1)
    {
        NRF_TIMER1->TASKS_STOP = 1;
        NRF_TIMER1->MODE        = TIMER_MODE_MODE_Timer;
        NRF_TIMER1->BITMODE     = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
        NRF_TIMER1->PRESCALER   = 4;  // 1us resolution

        NRF_TIMER1->TASKS_CLEAR = 1;       // clear the task first to be usable for later
        NRF_TIMER1->CC[0] = interval_usec; // first timeout

        NRF_TIMER1->INTENSET    = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        /* Create an Event-Task shortcut to clear TIMER0 on COMPARE[0] event. */
        NRF_TIMER1->SHORTS      = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
    }
}

/**
 * @brief Stop the specified hardware timer
 * 
 * @param block PWM block specified which corresponds to the hardware timer
 */
void helio_stop_stepper_timer(PwmSelect_t block)
{
    if (block == PWM_0)
    {
        NRF_TIMER0->TASKS_STOP = 1;
    }
    else if (block == PWM_1)
    {
        NRF_TIMER1->TASKS_STOP = 1;
    }
}

/**
 * @brief Restart the timer with the specified interval
 * @details This should only be called after the timer is stopped
 *          and after it was initialized
 * 
 * @param block PWM block specified which corresponds to the hardware timer
 * @param interval_usec 
 */
void helio_start_stepper_timer(PwmSelect_t block, uint32_t interval_usec)
{   
    if (block == PWM_0)
    {
        NRF_TIMER0->TASKS_CLEAR = 1;
        NRF_TIMER0->CC[0] = interval_usec / 2;
	    NRF_TIMER0->CC[1] = interval_usec;
        NRF_TIMER0->TASKS_START = 1;
    }
    else if (block == PWM_1)
    {
        NRF_TIMER1->TASKS_CLEAR = 1;
        NRF_TIMER1->CC[0] = interval_usec;
        NRF_TIMER1->TASKS_START = 1;
    }
}