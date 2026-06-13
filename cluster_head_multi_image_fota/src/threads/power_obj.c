#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "power_obj.h"

K_MUTEX_DEFINE(mppt_mutex);

static mppt_pub_t mppt_obj;

mppt_pub_t *heliostat_mppt_get(void)
{
    return &mppt_obj;
}

bool attempt_mppt_mtx_lock(void)
{
    if (k_mutex_lock(&mppt_mutex, K_MSEC(100)) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void mppt_mtx_unlock(void)
{
    k_mutex_unlock(&mppt_mutex);
}