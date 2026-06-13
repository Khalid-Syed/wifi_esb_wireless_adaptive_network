#include "wifi_events.h"
#include <zephyr/logging/log.h>
// https : // docs.zephyrproject.org/apidoc/latest/group__log__api.html
// LOG_MODULE_REGISTER(MODULE, LOG_LEVEL_DBG);
#define MODULE wifi_events
LOG_MODULE_REGISTER(MODULE);
// -----------------------
// WiFi STATUS EVENT
// -----------------------
void wifi_status_event_throw(char *status)
{
    struct wifi_status_event *event = new_wifi_status_event();
    event->status = status;
    event->ts = k_uptime_get();
    APP_EVENT_SUBMIT(event);
}

void log_wifi_status_event(const struct app_event_header *aeh)
{
    LOG_DBG("log an wifi status event");
    struct wifi_status_event *se = cast_wifi_status_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "status = %s, %lld",
                          se->status, se->ts);
}
static void profile_wifi_status_event(struct log_event_buf *buf,
                                      const struct app_event_header *aeh)
{
    struct wifi_status_event *event = cast_wifi_status_event(aeh);

    ARG_UNUSED(event);

    nrf_profiler_log_encode_string(buf, event->status);
    // how to encode an int64 timestamp?
    // nrf_profiler_log_encode_int64(buf, event->ts);
}
APP_EVENT_INFO_DEFINE(wifi_status_event,
                      ENCODE(
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_TIMESTAMP),
                      ENCODE(
                          "status",
                          "ts"),
                      profile_wifi_status_event);

APP_EVENT_TYPE_DEFINE(wifi_status_event,
                      log_wifi_status_event,
                      &wifi_status_event_info,
                      APP_EVENT_FLAGS_CREATE());
// INIT_LOG_ENABLE flag will call log print at every event fire, or you can call manually
// APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
