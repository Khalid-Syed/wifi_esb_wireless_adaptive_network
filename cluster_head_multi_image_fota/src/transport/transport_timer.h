#ifndef __TRANSPORT_TIMER_H__
#define __TRANSPORT_TIMER_H__

#include <zephyr/kernel.h>

/**
 * @brief Start the transport pub timer
 *
 */
void transport_pub_timer_start();
/**
 * @brief Get the transport pub timer status
 *
 * @return bool True if the timer is running, false otherwise
 */
bool transport_pub_timer_running();
/**
 * @brief Stop the transport pub timer
 *
 */
void transport_pub_timer_stop();

#endif // __TRANSPORT_TIMER_H__
