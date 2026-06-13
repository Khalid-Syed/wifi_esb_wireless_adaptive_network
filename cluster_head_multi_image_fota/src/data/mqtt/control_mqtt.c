#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "control_mqtt.h"
#include "transport.h"
#include "id.h"
#include "control_json.h"
#include "pubs.h"
#include "start_mqtt.h"

#define MODULE control_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_CONTROL_MQTT_LOG_LEVEL);

//
#define MQTT_TOPIC_GET control_mqtt_topic_get
#define ON_MQTT_PUBLISH_CB control_on_mqtt_publish_cb
// command topic
#define TOPIC CONTROL_MQTT_TOPIC_CMD_STR
#define TOPIC_CMD_LEN ((sizeof(CONTROL_MQTT_TOPIC_CMD_STR) + ID_LEN) + 1)
#define QOS_CMD CONTROL_MQTT_CMD_QOS
#define CMD_PAYLOAD_TYPE control_cmd_t
#define LOG_CMD log_control_cmd

static char topic_cmd_str[TOPIC_CMD_LEN];

// command response topic
#define TOPIC_ERR_RSP CONTROL_MQTT_TOPIC_CMD_ERR_RSP_STR
#define QOS_CMD_RSP CONTROL_MQTT_CMD_RSP_QOS
#define CMD_RSP_TYPE control_cmd_resp_t
#define LOG_CMD_RSP log_control_cmd_resp
#define RSP_PAYLOAD_SIZE_MAX CONTROL_RESP_PAYLOAD_STR_SIZE_MAX

static const char *RSP_OK = "ok";
static const char *RSP_PAYLOAD_DECODE_ERR = "payload decode err";
static const char *RSP_PAYLOAD_BAD_VALUES_ERR = "payload bad values";
static const char *RSP_PAYLOAD_UNKNOWN_ERR = "payload unknown err";

static int publish_response(char *rsp_topic, int session_id, const char *status);
static int publish_response_err();

bool start_cla = false;

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
            if (cmd.operation == NULL)
            {
                LOG_ERR("%s err %d operation is NULL", __func__, err);
                publish_response_err(RSP_PAYLOAD_DECODE_ERR);
                return true;
            }
            else if (strcmp(cmd.operation, "CLA") == 0)
            {
                LOG_DBG("CLA operation received");
                // Handle CLA operation
                if (start_ncla)
                {
                    pubs_stop_ncla();
                    start_ncla = false;
                }
                if (!start_cla)
                {
                    pubs_start_cla();
                    start_cla = true;
                }
                publish_response(cmd.res_topic, cmd.session_id, RSP_OK);
                return true;
            }
            else if (strcmp(cmd.operation, "NCLA") == 0)
            {
                LOG_DBG("NCLA operation received");
                // Handle NCLA operation
                if (start_cla)
                {
                    pubs_stop_cla();
                    start_cla = false;
                    k_sleep(K_MSEC(CLA_NCLA_TRANSITION_SLEEP_INTERVAL_MS));
                }
                if (!start_ncla)
                {
                    pubs_start_ncla();
                    start_ncla = true;

                }
                publish_response(cmd.res_topic, cmd.session_id, RSP_OK);
                return true;
            }
            else
            {
                LOG_ERR("%s err %d unknown", __func__, err);
                publish_response_err(RSP_PAYLOAD_UNKNOWN_ERR);
                return true;
            }
        }
        else if (err == -ENOEXEC)
        {
            LOG_ERR("%s err %d failed to decode control_cmd", __func__, err);
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

static int publish_response(char *rsp_topic, int session_id, const char *status)
{
    // build response topic
    LOG_DBG("%s session_id=%d status=%s", __func__, session_id, status);
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1] = {0};
    char id[ID_LEN];
    if (!(id_get(id, sizeof(id))))
    {
        // this builds a topic like: control/cmd/rsp/1234567890/control_res
        snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, CONTROL_MQTT_TOPIC_CMD_RSP_STR, id, rsp_topic);
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
