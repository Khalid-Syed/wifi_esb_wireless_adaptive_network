#ifndef _TRANSPORT_EVENTS_H_
#define _TRANSPORT_EVENTS_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

/*
These transport and other network events should be subscribed to in order to do smart things with the heliostat behavior,
such as stopping, stowing, led behavior, etc.
*/

struct transport_status_event
{
    struct app_event_header header;
    char *status;
    int64_t ts; // timestamp
};
APP_EVENT_TYPE_DECLARE(transport_status_event);

#define TRANSPORT_STATUS_EVENT_READY "ready"
#define TRANSPORT_STATUS_EVENT_DISCONNECTED "disconnected"

void log_transport_status_event(const struct app_event_header *aeh);
void transport_status_event_throw(char *status);

#endif