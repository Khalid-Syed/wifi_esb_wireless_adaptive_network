#include "ota_dfu_events.h"

// ------------------------
// OTA_DFU CMD EVENT
// ------------------------

void log_ota_dfu_cmd_event(const struct app_event_header *aeh)
{
    struct ota_dfu_cmd_event *event = cast_ota_dfu_cmd_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "session_id = %d, "
                          "uri = %s, "
                          "file = %s, "
                          "install = %s, "
                          "retries = %d, ",
                          event->session_id,
                          event->uri,
                          event->file,
                          event->install,
                          event->retries);
}

static void profile_ota_dfu_cmd_event(struct log_event_buf *buf,
                                      const struct app_event_header *aeh)
{
    struct ota_dfu_cmd_event *event = cast_ota_dfu_cmd_event(aeh);

    ARG_UNUSED(event);
    nrf_profiler_log_encode_int32(buf, event->session_id);
    nrf_profiler_log_encode_string(buf, event->uri);
    nrf_profiler_log_encode_string(buf, event->file);
    nrf_profiler_log_encode_string(buf, event->install);
    nrf_profiler_log_encode_int32(buf, event->retries);
}

APP_EVENT_INFO_DEFINE(ota_dfu_cmd_event,
                      ENCODE(
                          NRF_PROFILER_ARG_S32,
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_S32),
                      ENCODE(
                          "session_id",
                          "uri",
                          "file",
                          "install",
                          "retries"),
                      profile_ota_dfu_cmd_event);

APP_EVENT_TYPE_DEFINE(ota_dfu_cmd_event,
                      log_ota_dfu_cmd_event,
                      &ota_dfu_cmd_event_info,
                      APP_EVENT_FLAGS_CREATE());

// -----------------------
// OTA_DFU STATUS EVENT
// -----------------------
void log_ota_dfu_status_event(const struct app_event_header *aeh)
{
    struct ota_dfu_status_event *event = cast_ota_dfu_status_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "session_id = % d, "
                          "status = %s, "
                          "progress = %d%% ",
                          event->session_id,
                          event->status,
                          event->progress);
}

static void profile_ota_dfu_status_event(struct log_event_buf *buf,
                                         const struct app_event_header *aeh)
{
    struct ota_dfu_status_event *event = cast_ota_dfu_status_event(aeh);

    ARG_UNUSED(event);
    nrf_profiler_log_encode_string(buf, event->status);
    nrf_profiler_log_encode_int32(buf, event->progress);
    nrf_profiler_log_encode_int32(buf, event->session_id);
}

APP_EVENT_INFO_DEFINE(ota_dfu_status_event,
                      ENCODE(
                          NRF_PROFILER_ARG_INT32,
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_INT32),
                      ENCODE(
                          "session_id"
                          "status",
                          "progress"),
                      profile_ota_dfu_status_event);

APP_EVENT_TYPE_DEFINE(ota_dfu_status_event,
                      log_ota_dfu_status_event,
                      &ota_dfu_status_event_info,
                      APP_EVENT_FLAGS_CREATE());