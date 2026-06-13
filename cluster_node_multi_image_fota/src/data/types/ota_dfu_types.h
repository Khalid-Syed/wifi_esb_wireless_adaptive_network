#ifndef OTA_DFU_TYPES_H
#define OTA_DFU_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    char *uri;
    char *file;
    char *install;
    int retries;
    // optional fields for simulation and testing
    int res_qos; // specify the qos of the response
} ota_dfu_cmd_t;
typedef struct
{
    int session_id;
    const char *response;
} ota_dfu_cmd_resp_t;

typedef struct
{
    char *current_fwv;
    int session_id;
    char *status;
    int progress;
} ota_dfu_status_t;

#endif // OTA_DFU_TYPES_H
