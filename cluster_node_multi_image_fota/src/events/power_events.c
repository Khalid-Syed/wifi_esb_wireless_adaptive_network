#include "power_events.h"

// -----------------------
// POWER STATUS EVENT
// -----------------------

void log_power_status_event(const struct app_event_header *aeh)
{
    struct power_status_event *event = cast_power_status_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "power_state = %d, "
                          "ts = %lld",
                          (int32_t) event->power_state,
                          event->ts);
}

static void profile_power_status_event(struct log_event_buf *buf,
                                            const struct app_event_header *aeh)
{
    struct power_status_event *event = cast_power_status_event(aeh);

    ARG_UNUSED(event);
    nrf_profiler_log_encode_int32(buf, (int32_t) event->power_state);
    // nrf_profiler_log_encode_int64(buf, event->ts);
}

APP_EVENT_INFO_DEFINE(power_status_event,
                      ENCODE(
                            NRF_PROFILER_ARG_S32,
                            NRF_PROFILER_ARG_TIMESTAMP),
                      ENCODE(
                          "power_state",
                          "t"),
                      profile_power_status_event);

APP_EVENT_TYPE_DEFINE(power_status_event,
                      log_power_status_event,
                      &power_status_event_info,
                      APP_EVENT_FLAGS_CREATE());