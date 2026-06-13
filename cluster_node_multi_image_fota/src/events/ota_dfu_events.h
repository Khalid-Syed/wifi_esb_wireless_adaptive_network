#ifndef _OTA_DFU_EVENTS_H_
#define _OTA_DFU_EVENTS_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

struct ota_dfu_cmd_event
{
    struct app_event_header header;
    int session_id;
    char *uri;
    char *file;
    char *install;
    int32_t retries;
};

APP_EVENT_TYPE_DECLARE(ota_dfu_cmd_event);

struct ota_dfu_status_event
{
    struct app_event_header header;
    int session_id;
    char *status;
    int progress;
};

APP_EVENT_TYPE_DECLARE(ota_dfu_status_event);

#endif