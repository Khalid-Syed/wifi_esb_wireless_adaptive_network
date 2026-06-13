#include <stdlib.h>
#include "manager_thread.h"
#include "manager_thread_monitor.h"
#include "cfg.h"

#define MODULE manager
LOG_MODULE_DECLARE(MODULE, CONFIG_MANAGER_THREAD_LOG_LEVEL);

static int num_of_threads = 0;
static bool enable_thread_monitor = true;

typedef struct
{
    int idx;
    const char *name;
    void (*assert)(void);
    uint32_t timestamp_ms; // timestamp of last check-in
    uint32_t timeout_ms;   // timeout in ms
} thread_monitor_t;

static thread_monitor_t thread_monitor[THREAD_MONITOR_IDX_MAX] = {0};

/**
 * @brief initialize the thread monitor
 *
 */
void thread_monitor_init(void)
{
    num_of_threads = 0;
    memset(thread_monitor, 0, sizeof(thread_monitor_t) * THREAD_MONITOR_IDX_MAX);
    enable_thread_monitor = cfg_get()->manager_cfg.trd_mon_en;
    if (enable_thread_monitor == false)
    {
        LOG_ERR("warning: thread monitor disabled");
    }
}

/**
 * @brief callback that subthread should call to inticate that they are still alive and not stalled
 *
 * @param idx - index of thread
 * @param ts - timestamp of when callback was called
 * @param timeout_ms - timeout in ms when the next callback should be expected
 */
void im_alive_cb(int idx, uint32_t ts, uint32_t to)
{
    if (idx < num_of_threads)
    {
        thread_monitor[idx].timestamp_ms = ts;
        thread_monitor[idx].timeout_ms = to;
    }
    else
    {
        LOG_ERR("idx %d out of range", idx);
    }
}

/**
 * @brief add a thread to the thread monitor
 *
 * @param idx
 * @param name
 * @param assert
 */
void thread_monitor_add(int idx, const char *name, void (*assert)(void))
{
    if (idx >= THREAD_MONITOR_IDX_MAX)
    {
        LOG_ERR("idx %d out of range", idx);
        return;
    }
    thread_monitor[idx].idx = idx;
    thread_monitor[idx].name = name;
    thread_monitor[idx].assert = assert;
    thread_monitor[idx].timestamp_ms = 0;
    thread_monitor[idx].timeout_ms = 0;

    num_of_threads++;
};
/**
 * @brief loops through all thread monitor timers and checks for timeouts.
 *
 * @return int < 0 if timeout detected
 */
int thread_monitor_timeout_check(void)
{
    if (enable_thread_monitor == false)
    {
        return 0;
    }
    for (int i = 0; i < num_of_threads; i++)
    {
        if (thread_monitor[i].timestamp_ms == 0) // thread monitor disabled
        {
            continue;
        }
        if (k_uptime_get() - thread_monitor[i].timestamp_ms > (thread_monitor[i].timeout_ms + THREAD_MONITOR_TIMEOUT_TOLERANCE_MS))
        {
            LOG_ERR("TIMEOUT: %s thread idx %d timestamp %d uptime %d",
                    thread_monitor[i].name, i, thread_monitor[i].timestamp_ms, (int)k_uptime_get());

            if (thread_monitor[i].assert == NULL)
            {
                LOG_ERR("No assert function defined for thread %s", thread_monitor[i].name);
                continue;
            }
            LOG_ERR("Device will now assert in:");
            LOG_ERR("3... ");
            k_sleep(K_MSEC(1000));
            LOG_ERR("2...");
            k_sleep(K_MSEC(1000));
            LOG_ERR("1...");
            k_sleep(K_MSEC(2000));
            thread_monitor[i].assert();
            return -1;
        }
    }
    return 0;
}
/**
 * @brief artificially set a thread monitor timeout
 *
 * @param idx
 */
void thread_monitor_timeout_artificial(int idx)
{
    if (idx < num_of_threads)
    {
        thread_monitor[idx].timestamp_ms = 1;
    }
    else
    {
        LOG_ERR("idx %d out of range", idx);
    }
}