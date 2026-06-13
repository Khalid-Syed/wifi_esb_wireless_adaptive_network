#ifndef _WIFI_EVENTS_H_
#define _WIFI_EVENTS_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

/*
These wi-fi and other network events should be subscribed to in order to do smart things with the heliostat behavior,
such as stopping, stowing, led behavior, etc.
*/

struct wifi_status_event
{
    struct app_event_header header;
    char *status;
    int64_t ts; // timestamp
};
APP_EVENT_TYPE_DECLARE(wifi_status_event);

#define WIFI_STATUS_EVENT_CONNECTING "connecting"
#define WIFI_STATUS_EVENT_CONNECT_FAILED "connect failed"
#define WIFI_STATUS_EVENT_CONNECTED "connected"
#define WIFI_STATUS_EVENT_DISCONNECTING "disconnecting"
#define WIFI_STATUS_EVENT_DISCONNECT_FAILED "disconnect failed"
#define WIFI_STATUS_EVENT_DISCONNECTED "disconnected"
#define WIFI_STATUS_EVENT_IP_ACQUIRED "ip acquired"

void log_wifi_status_event(const struct app_event_header *aeh);
void wifi_status_event_throw(char *status);

#endif // _WIFI_EVENTS_H_