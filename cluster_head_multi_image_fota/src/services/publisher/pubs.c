#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "transport_events.h"
#include "jitter.h"
#include "pubs.h"
#include "pub_table.h"
#include "wifi_thread.h"
#include "time_sync.h"
#include "start_mqtt.h"

#define MODULE pubs
LOG_MODULE_REGISTER(MODULE, CONFIG_PUBS_LOG_LEVEL);
static int pub_get_idx(struct k_work *work);
static int pub_get_idx_by_topic(char *topic);

bool ncla_started = false;

/**
 * @brief call init of each pub in the pub table
 *
 */
void pubs_init(void)
{
    LOG_DBG("");
    for (int i = 0; i < pub_table_size(); i++)
    {
        if (pub_table[i].init != NULL)
        {
            pub_table[i].init();
        }
    }
}
/**
 * @brief start scheduling of messages
 *
 */
void pubs_start(void)
{
    LOG_DBG("");

    for (int i = 0; i < pub_table_size(); i++)
    {
        int initial_interval = 0;
        if (strcmp(pub_table[i].topic, "ncla") == 0) 
        {
            (void)pub_schedule(&(pub_table[i].dwork->work), initial_interval);
        }
        // (void)pub_schedule(&(pub_table[i].dwork->work), initial_interval);
    }
}

void pubs_start_ncla(void)
{
    int initial_interval = 0;
    (void)pub_schedule(&(pub_table[0].dwork->work), initial_interval);
}

void pubs_start_cla(void)
{
    int initial_interval = 0;
    (void)pub_schedule(&(pub_table[1].dwork->work), initial_interval);
}

void pubs_stop_ncla(void)
{
    k_work_cancel_delayable(pub_table[0].dwork);
}

void pubs_stop_cla(void)
{
    k_work_cancel_delayable(pub_table[1].dwork);
}


/**
 * @brief cancel all pub work items
 *
 */
void pubs_stop(void)
{
    for (int i = 0; i < pub_table_size(); i++)
    {
        LOG_DBG("%s stop", pub_table[i].topic);
        k_work_cancel_delayable(pub_table[i].dwork);
    }
}
/**
 * @brief work handler callback for all of the pub modules
 *
 * @param work
 */
void pubs_work_handler(struct k_work *work)
{
    LOG_DBG("", __func__);

    int idx = pub_get_idx(work); // get index of pub based on work item
    if (idx < 0)
    {
        LOG_ERR("pub_get_idx failed");
        return;
    }
    LOG_DBG(" %s publish", pub_table[idx].topic);
    pub_table[idx].pub();

    int interval = pub_table[idx].interval_ms;
    if (interval < 0)
    {
        LOG_DBG("interval < 0, setting to %d ms", PUBS_MIN_INTERVAL_MS);
        interval = PUBS_MIN_INTERVAL_MS;
    }
    (void)pub_schedule(work, interval);
}

/**
 * @brief listen for transport status events
 *
 * @param aeh
 * @return true
 * @return false
 */
bool pubs_transport_event_handler(const struct app_event_header *aeh)
{
    LOG_DBG("%s", __func__);

    if (is_transport_status_event(aeh))
    {
        struct transport_status_event *le = cast_transport_status_event(aeh);
        LOG_DBG("transport status event: %s", le->status);
        if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_READY))
        {
            printk("TRANSPORT_STATUS_EVENT_READY\n");
            // Sadman's note: disabling automated start message publising on TRANSPORT_STATUS_EVENT_READY
            LOG_INF("TRANSPORT_STATUS_EVENT_READY");
            // if (start_ncla)
            // {
            //     // start publishing messages
            //     LOG_DBG("Start MQTT publishing");
            //     pubs_start_ncla();
            //     ncla_started = true;
            // }
        }
        else if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_DISCONNECTED))
        {
            printk("TRANSPORT_STATUS_EVENT_DISCONNECTED\n");
            // Sadman's note: disabling automated stop message publising on TRANSPORT_STATUS_EVENT_DISCONNECTED
            LOG_INF("TRANSPORT_STATUS_EVENT_DISCONNECTED");
            // if (!(start_ncla) && ncla_started)
            // {
            //     // stop publishing messages
            //     LOG_DBG("Stop MQTT publishing");
            //     pubs_stop_ncla();
            //     ncla_started = false;
            // }
        }
        else
        { // unhandled events
            LOG_DBG("unknown transport status event: %s", le->status);
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);
    return false;
}
int pub_schedule(struct k_work *work, int ms)
{
    // Sadman: Deactivate TCP-based Time Sync
    // if (!fetch_server_time_success) {
    //     if (fetch_time_from_server() == 0) {
    //         fetch_server_time_success = true;
    //     }
    // }
    int idx = pub_get_idx(work);
    if (idx < 0)
    {
        LOG_ERR("pub_get_idx failed");
        return -1;
    }
    LOG_DBG("%s schedule for %.1f s", pub_table[idx].topic, ((float)ms) / 1000);
    if (ms == 0)
    {
        k_work_reschedule(pub_table[idx].dwork, K_NO_WAIT);
    }
    else
    {
        if (ms < PUBS_MIN_INTERVAL_MS)
        {
            ms = PUBS_MIN_INTERVAL_MS;
        }
        k_work_reschedule(pub_table[idx].dwork, K_MSEC(ms));
    }
    return 0;
}
int pub_schedule_topic(char *topic, int ms)
{
    int idx = pub_get_idx_by_topic(topic);
    if (idx < 0)
    {
        LOG_ERR("pub_get_idx failed");
        return -1;
    }
    LOG_DBG("%s schedule for %.1f s", pub_table[idx].topic, ((float)ms) / 1000);
    if (ms == 0)
    {
        k_work_reschedule(pub_table[idx].dwork, K_NO_WAIT);
    }
    else
    {
        if (ms < PUBS_MIN_INTERVAL_MS)
        {
            ms = PUBS_MIN_INTERVAL_MS;
        }
        k_work_reschedule(pub_table[idx].dwork, K_MSEC(ms));
    }
    return 0;
}

static int pub_get_idx_by_topic(char *topic)
{
    for (int i = 0; i < pub_table_size(); i++)
    {
        if (strncmp(topic, pub_table[i].topic, strlen(topic)) == 0)
        {
            return i;
        }
    }
    LOG_ERR("pub_get_idx_by_topic failed");
    return -1;
}

/**
 * @brief get index of pub in pub table based on work item
 *
 * @param work
 * @return int
 */
static int pub_get_idx(struct k_work *work)
{
    for (int i = 0; i < pub_table_size(); i++)
    {
        if (&(pub_table[i].dwork->work) == work)
        {
            return i;
        }
    }
    LOG_ERR("pub_get_idx failed");
    return -1;
}
// subscribe to transport events
APP_EVENT_LISTENER(MODULE, pubs_transport_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, transport_status_event);
