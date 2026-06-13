#ifndef _POWER_EVENTS_H_
#define _POWER_EVENTS_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

struct power_status_event
{
    struct app_event_header header;
    int32_t power_state;
    int64_t ts;    // timestamp
};

APP_EVENT_TYPE_DECLARE(power_status_event);

enum wifi_thread_cmd
{
    WIFI_TURN_OFF,
    WIFI_TURN_ON,
    WIFI_THREAD_START,
    WIFI_TWT_TEARDOWN_CMD,
};

void log_power_status_event(const struct app_event_header *aeh);

#endif