#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mqtt_common.h"
#include "transport.h"
#include "id.h"
#include "ota_dfu_mqtt.h"
#include "ota_dfu_json.h" // this would be switched out for protobuf or other as needed
#include "ota_dfu_events.h"

#define MODULE ota_dfu_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_OTA_DFU_MQTT_LOG_LEVEL);

//
#define MQTT_TOPIC_GET ota_dfu_mqtt_topic_get
#define ON_MQTT_PUBLISH_CB ota_dfu_on_mqtt_publish_cb
// command topic
#define TOPIC OTA_DFU_MQTT_TOPIC_CMD_STR
#define TOPIC_CMD_LEN ((sizeof(OTA_DFU_MQTT_TOPIC_CMD_STR) + ID_LEN) + 1)
#define QOS_CMD OTA_DFU_MQTT_CMD_QOS
#define CMD_PAYLOAD_TYPE ota_dfu_cmd_t
#define LOG_CMD log_ota_dfu_cmd

static char topic_cmd_str[TOPIC_CMD_LEN];

// command response topic
#define TOPIC_ERR_RSP OTA_DFU_MQTT_TOPIC_CMD_ERR_RSP_STR
#define QOS_CMD_RSP OTA_DFU_MQTT_CMD_RSP_QOS
#define CMD_RSP_TYPE ota_dfu_cmd_resp_t
#define LOG_CMD_RSP log_ota_dfu_cmd_resp
#define RSP_PAYLOAD_SIZE_MAX OTA_DFU_RESP_PAYLOAD_STR_SIZE_MAX

// dt status topic publish
#define MQTT_PUB_TOPIC_GET ota_dfu_status_mqtt_topic_get
#define MQTT_PUBLISH ota_dfu_status_mqtt_publish
#define TOPIC_DT OTA_DFU_STATUS_MQTT_TOPIC_DT_STR
#define TOPIC_DT_LEN ((sizeof(OTA_DFU_STATUS_MQTT_TOPIC_DT_STR) + ID_LEN) + 1)
#define QOS_DT OTA_DFU_STATUS_MQTT_DT_QOS

#define DT_OBJ_TYPE ota_dfu_status_t
#define DT_OBJ_P_TYPE DT_OBJ_TYPE *

#define PAYLOAD_SIZE_MAX OTA_DFU_STATUS_PAYLOAD_STR_SIZE_MAX

static const char *RSP_OK = "ok";
static const char *RSP_PAYLOAD_DECODE_ERR = "payload decode err";
static const char *RSP_PAYLOAD_BAD_VALUES_ERR = "payload bad values";
static const char *RSP_PAYLOAD_UNKNOWN_ERR = "payload unknown err";

// private functions
static int publish_response(char *rsp_topic, int session_id, const char *status);
static int publish_response_err();

/******************************
        Template Code
*******************************/
int MQTT_TOPIC_GET(struct mqtt_topic *topic)
{
    char id[ID_LEN];

    if (!(id_get(id, sizeof(id))))
    {
        snprintf(topic_cmd_str, TOPIC_CMD_LEN, TOPIC, id);
        topic->topic.utf8 = topic_cmd_str;
        topic->topic.size = strlen(topic->topic.utf8);
        topic->qos = QOS_CMD;
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    return 0;
}
bool ON_MQTT_PUBLISH_CB(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
    LOG_DBG("%s topic %s payload %s", __func__, topic.ptr, payload.ptr);

    if (strncmp(topic.ptr, topic_cmd_str, topic.size) == 0)
    {
        LOG_DBG("%s matched topic", __func__);
        CMD_PAYLOAD_TYPE cmd;
        char payload_str[payload.size + 1];
        memcpy(payload_str, payload.ptr, payload.size);
        payload_str[payload.size] = '\0';
        int err = PAYLOAD_DECODE(payload_str, &cmd);
        if (err == 0)
        {
            LOG_CMD(&cmd);
            LOG_DBG("%s rx cmd, send evt!", __func__);

            /**** SEND EVENT HERE ****/
            {
                struct ota_dfu_cmd_event *event_ota_dfu = new_ota_dfu_cmd_event();
                event_ota_dfu->session_id = cmd.session_id;
                event_ota_dfu->uri = cmd.uri;
                event_ota_dfu->file = cmd.file;
                event_ota_dfu->install = cmd.install;
                event_ota_dfu->retries = cmd.retries;
                APP_EVENT_SUBMIT(event_ota_dfu);
            }
            /* send response */
            publish_response(cmd.res_topic, cmd.session_id, RSP_OK);
        }
        else if (err == -ENOEXEC)
        {
            LOG_ERR("%s err %d failed to decode ota_dfu_cmd", __func__, err);
            publish_response_err(RSP_PAYLOAD_DECODE_ERR);
            return true;
        }
        else if (err == -EINVAL)
        {
            LOG_ERR("%s err %d bad values", __func__, err);
            publish_response_err(RSP_PAYLOAD_BAD_VALUES_ERR);
            return true;
        }
        else
        {
            LOG_ERR("%s err %d unknown", __func__, err);
            publish_response_err(RSP_PAYLOAD_UNKNOWN_ERR);
            return true;
        }
        return true;
    }
    return false;
}
int MQTT_PUB_TOPIC_GET(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len)
{
    char id[ID_LEN];

    if (!(id_get(id, sizeof(id))))
    {
        snprintf(topic_str, TOPIC_DT_LEN, TOPIC_DT, id);
        topic->topic.utf8 = topic_str;
        topic->topic.size = strlen(topic->topic.utf8);
        topic->qos = QOS_DT;
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    return 0;
}
int MQTT_PUBLISH(DT_OBJ_P_TYPE obj)
{
    // build topic
    struct mqtt_topic topic;
    char topic_str[TOPIC_DT_LEN];
    MQTT_PUB_TOPIC_GET(&topic, &topic_str[0], sizeof(topic_str));

    // build payload
    char payload_str[PAYLOAD_SIZE_MAX];
    int err = OTA_DFU_STATUS_PAYLOAD_ENCODE(obj, payload_str, PAYLOAD_SIZE_MAX);
    if (err == 0)
    {
        // publish
        err = mqtt_common_publish(topic_str, payload_str, QOS_DT);
    }
    return err;
}

static int publish_response(char *rsp_topic, int session_id, const char *status)
{
    // build response topic
    LOG_DBG("%s session_id=%d status=%s", __func__, session_id, status);
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1] = {0};
    char id[ID_LEN];
    if (!(id_get(id, sizeof(id))))
    {
        // this builds a topic like: cmd/1234567890/ota_dfu_res
        snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, OTA_DFU_MQTT_TOPIC_CMD_RSP_STR, id, rsp_topic);
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    // build response payload
    char rsp_payload_str[RSP_PAYLOAD_SIZE_MAX];
    CMD_RSP_TYPE rsp;
    rsp.session_id = session_id,
    rsp.response = status;
    LOG_CMD_RSP(&rsp);
    int err = RSP_ENCODE(&rsp, rsp_payload_str, RSP_PAYLOAD_SIZE_MAX);
    if (err == 0)
    {
        // publish response
        err = mqtt_common_publish(topic_cmd_rsp_str, rsp_payload_str, QOS_CMD_RSP);
    }
    return err;
}
static int publish_response_err(char *payload_err)
{
    // build response topic
    LOG_DBG("%s", __func__);
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1] = {0};

    char id[ID_LEN];
    if (!(id_get(id, sizeof(id))))
    {
        snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, TOPIC_ERR_RSP, id);
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    // publish err response
    int err = mqtt_common_publish(topic_cmd_rsp_str, payload_err, QOS_CMD_RSP);
    return err;
}
