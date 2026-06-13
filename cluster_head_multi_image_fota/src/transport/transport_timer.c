#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>

/* Declare log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

static void transport_pub_timer_expired(struct k_timer *dummy);

void (*timer_expired_cb)();

/* Define a publishing timer in case mqtt helper does not give callback when sending qos > 0 msgs */
#define TRANSPORT_PUB_TIMER_SECONDS (10)
K_TIMER_DEFINE(transport_pub_timer, transport_pub_timer_expired, NULL);

/**
 * @brief Start the one-shot transport pub timer
 *
 */
void transport_pub_timer_start(void(*expired_cb))
{
    LOG_DBG("transport_pub_timer_start");
    timer_expired_cb = expired_cb;

    // start a one shot timer
    k_timer_start(&transport_pub_timer, K_SECONDS(TRANSPORT_PUB_TIMER_SECONDS), K_NO_WAIT);
    LOG_DBG("time remaining = %d", k_timer_remaining_get(&transport_pub_timer));
}
/**
 * @brief Get the transport pub timer status
 *
 * @return int > 0 if timer is expired or stopped, 1 if timer is running
 */
bool transport_pub_timer_running()
{
    int t = k_timer_remaining_get(&transport_pub_timer);
    // LOG_DBG("time remaining = %d", t);

    if (t != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
/**
 * @brief Stop the transport pub timer
 *
 */
void transport_pub_timer_stop()
{
    LOG_DBG("time remaining = % d", k_timer_remaining_get(&transport_pub_timer));
    k_timer_stop(&transport_pub_timer);
}

static void transport_pub_timer_expired(struct k_timer *dummy)
{
    LOG_ERR("expected pub ack callback to stop timer first, time remaining = % d ", k_timer_remaining_get(&transport_pub_timer));
    timer_expired_cb();
}
