#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "node_id_mqtt.h"
#include "transport.h"
#include "id.h"
#include "node_id_json.h"

#define MODULE node_id_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_NODE_ID_MQTT_LOG_LEVEL);

//
#define MQTT_TOPIC_GET node_id_mqtt_topic_get
#define ON_MQTT_PUBLISH_CB node_id_on_mqtt_publish_cb
// command topic
#define TOPIC NODE_ID_MQTT_TOPIC_CMD_STR
#define TOPIC_CMD_LEN (sizeof(NODE_ID_MQTT_TOPIC_CMD_STR) + 1)
#define QOS_CMD NODE_ID_MQTT_CMD_QOS
#define CMD_PAYLOAD_TYPE node_id_cmd_t
#define LOG_CMD log_node_id_cmd

static char topic_cmd_str[TOPIC_CMD_LEN];

// command response topic
#define TOPIC_ERR_RSP NODE_ID_MQTT_TOPIC_CMD_ERR_RSP_STR
#define QOS_CMD_RSP NODE_ID_MQTT_CMD_RSP_QOS
#define CMD_RSP_TYPE node_id_cmd_resp_t
#define LOG_CMD_RSP log_node_id_cmd_resp
#define RSP_PAYLOAD_SIZE_MAX NODE_ID_RESP_PAYLOAD_STR_SIZE_MAX

static const char *RSP_OK = "ok";
static const char *RSP_PAYLOAD_DECODE_ERR = "payload decode err";
static const char *RSP_PAYLOAD_BAD_VALUES_ERR = "payload bad values";
static const char *RSP_PAYLOAD_UNKNOWN_ERR = "payload unknown err";

static int publish_response(char *rsp_topic, char *node_id);
static int publish_response_err();

/******************************
        Template Code
*******************************/
int MQTT_TOPIC_GET(struct mqtt_topic *topic)
{
    snprintf(topic_cmd_str, TOPIC_CMD_LEN, TOPIC);
    topic->topic.utf8 = topic_cmd_str;
    topic->topic.size = strlen(topic->topic.utf8);
    topic->qos = QOS_CMD;
    
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
            if (cmd.res_topic == NULL)
            {
                LOG_ERR("%s err %d res_topic is NULL", __func__, err);
                publish_response_err(RSP_PAYLOAD_DECODE_ERR);
                return true;
            }
            else if (strcmp(cmd.operation, "GET_NODE_ID") == 0)
            {
                LOG_DBG("Node's MAC ID requested");
                char *node_id;
                char id[ID_LEN];
                if (!(id_get(id, sizeof(id))))
                {
                    node_id = id;
                }
                else
                {
                    node_id = "NA";
                    __ASSERT(0, "Failed to get id");
                }
                publish_response(cmd.res_topic, node_id);
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
            LOG_ERR("%s err %d failed to decode node_id_cmd", __func__, err);
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

static int publish_response(char *rsp_topic, char *node_id)
{
    // build response topic
    LOG_DBG("%s node_id=%s", __func__, node_id);
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1] = {0};
    snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, NODE_ID_MQTT_TOPIC_CMD_RSP_STR, rsp_topic);
    // build response payload
    char rsp_payload_str[RSP_PAYLOAD_SIZE_MAX];
    CMD_RSP_TYPE rsp;
    rsp.node_id = node_id,
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

    snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, TOPIC_ERR_RSP);

    // publish err response
    int err = mqtt_common_publish(topic_cmd_rsp_str, payload_err, QOS_CMD_RSP);
    return err;
}
