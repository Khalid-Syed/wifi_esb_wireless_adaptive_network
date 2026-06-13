#include "transport_events.h"
#include <zephyr/logging/log.h>
// https : // docs.zephyrproject.org/apidoc/latest/group__log__api.html

#define MODULE transport_events
LOG_MODULE_REGISTER(MODULE, CONFIG_TRANSPORT_EVENTS_LOG_LEVEL);
// -----------------------
// TRANSPORT STATUS EVENT
// -----------------------
void transport_status_event_throw(char *status)
{
    struct transport_status_event *event = new_transport_status_event();
    event->status = status;
    event->ts = k_uptime_get();
    APP_EVENT_SUBMIT(event);
}

void log_transport_status_event(const struct app_event_header *aeh)
{
    LOG_DBG("log an transport status event");
    struct transport_status_event *se = cast_transport_status_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "status = %s, %lld",
                          se->status, se->ts);
}
static void profile_transport_status_event(struct log_event_buf *buf,
                                      const struct app_event_header *aeh)
{
    struct transport_status_event *event = cast_transport_status_event(aeh);

    ARG_UNUSED(event);

    nrf_profiler_log_encode_string(buf, event->status);
    // how to encode an int64 timestamp?
    // nrf_profiler_log_encode_int64(buf, event->ts);
}
APP_EVENT_INFO_DEFINE(transport_status_event,
                      ENCODE(
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_TIMESTAMP),
                      ENCODE(
                          "status",
                          "ts"),
                      profile_transport_status_event);

APP_EVENT_TYPE_DEFINE(transport_status_event,
                      log_transport_status_event,
                      &transport_status_event_info,
                      APP_EVENT_FLAGS_CREATE());
// INIT_LOG_ENABLE flag will call log print at every event fire, or you can call manually
// APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
