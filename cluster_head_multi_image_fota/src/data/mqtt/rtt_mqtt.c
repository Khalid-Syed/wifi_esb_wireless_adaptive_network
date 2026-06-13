#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "mqtt_common.h"
#include "rtt_mqtt.h"
#include "transport.h"
#include "id.h"
#include "rtt_json.h"

#include "ipc_handler.h"

#define MODULE rtt_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_RTT_MQTT_LOG_LEVEL);

//
#define MQTT_TOPIC_GET rtt_mqtt_topic_get
#define ON_MQTT_PUBLISH_CB rtt_on_mqtt_publish_cb
// command topic
#define TOPIC RTT_MQTT_TOPIC_CMD_STR
#define TOPIC_CMD_LEN (sizeof(RTT_MQTT_TOPIC_CMD_STR) + 1)
#define QOS_CMD RTT_MQTT_CMD_QOS
#define CMD_PAYLOAD_TYPE rtt_cmd_t
#define LOG_CMD log_rtt_cmd

static char topic_cmd_str[TOPIC_CMD_LEN];

// command response topic
#define TOPIC_ERR_RSP RTT_MQTT_TOPIC_CMD_ERR_RSP_STR
#define QOS_CMD_RSP RTT_MQTT_CMD_RSP_QOS
#define CMD_RSP_TYPE rtt_cmd_resp_t
#define LOG_CMD_RSP log_rtt_cmd_resp
#define RSP_PAYLOAD_SIZE_MAX RTT_RESP_PAYLOAD_STR_SIZE_MAX

static const char *RSP_OK = "ok";
static const char *RSP_PAYLOAD_DECODE_ERR = "payload decode err";
static const char *RSP_PAYLOAD_BAD_VALUES_ERR = "payload bad values";
static const char *RSP_PAYLOAD_UNKNOWN_ERR = "payload unknown err";

static int publish_response(char *rsp_topic, char *node_id, char *latest_ipc_msg);
static int publish_response_err();

/* Helper to convert hex string "0x91 0x01..." to binary */
static size_t hex_string_to_binary(const char *hex_str, uint8_t *out_bin, size_t max_len)
{
    size_t count = 0;
    const char *ptr = hex_str;
    while (*ptr && count < max_len) {
        /* Skip "0x", spaces, and commas */
        if (*ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X')) {
            ptr += 2;
            out_bin[count++] = (uint8_t)strtol(ptr, (char **)&ptr, 16);
        } else if (isxdigit((int)*ptr)) {
            out_bin[count++] = (uint8_t)strtol(ptr, (char **)&ptr, 16);
        } else {
            ptr++;
        }
    }
    return count;
}

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
    LOG_DBG("RTT: %s topic %s payload %s", __func__, topic.ptr, payload.ptr);

    if (strncmp(topic.ptr, topic_cmd_str, topic.size) == 0)
    {
        LOG_DBG("RTT: %s matched topic", __func__);
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
                LOG_ERR("RTT: %s err %d res_topic is NULL", __func__, err);
                publish_response_err(RSP_PAYLOAD_DECODE_ERR);
                return true;
            }
            else if (strcmp(cmd.operation, "RTT") == 0)
            {
                LOG_DBG("RTT: Forwarding binary payload to NET core");
                
                /* Update the current response topic for immediate ESB forwarding */
                extern void set_current_res_topic(const char *topic);
                set_current_res_topic(cmd.res_topic);

                if (cmd.payload == NULL) {
                    LOG_ERR("RTT: Payload is NULL");
                    publish_response_err("Payload NULL");
                    return true;
                }

                /* Convert ASCII hex string to binary bytes */
                uint8_t binary_payload[32];
                size_t binary_len = hex_string_to_binary(cmd.payload, binary_payload, sizeof(binary_payload));

                printk("RTT: Sending %d binary bytes to NET core\n", binary_len);

                /* Send the actual BYTES via IPC, not the string */
                /* Use a 4-second timeout to ensure we finish before the next 5s interval command arrives */
                int ret = ipc_send_and_wait(binary_payload, binary_len, K_MSEC(4000));
                
                if (ret == 0) {
                    LOG_DBG("RTT: Synchronous wait finished, message(s) already forwarded via callback");
                } else {
                    LOG_ERR("RTT: IPC wait timed out (no ESB response from nodes): %d", ret);
                    publish_response_err("No nodes responded");
                }
                
                /* Flush the MQTT transport to clear any pending ACK errors or queue build-up */
                // transport_flush() or similar if available, but for now we rely on the 10s wait.
                
                return true;
            }
            else
            {
                LOG_ERR("RTT: %s err %d unknown", __func__, err);
                publish_response_err(RSP_PAYLOAD_UNKNOWN_ERR);
                return true;
            }
        }
        else if (err == -ENOEXEC)
        {
            LOG_ERR("RTT: %s err %d failed to decode rtt_cmd", __func__, err);
            publish_response_err(RSP_PAYLOAD_DECODE_ERR);
            return true;
        }
        else if (err == -EINVAL)
        {
            LOG_ERR("RTT: %s err %d bad values", __func__, err);
            publish_response_err(RSP_PAYLOAD_BAD_VALUES_ERR);
            return true;
        }
        else
        {
            LOG_ERR("RTT: %s err %d unknown", __func__, err);
            publish_response_err(RSP_PAYLOAD_UNKNOWN_ERR);
            return true;
        }
        return true;
    }
    return false;
}

static int publish_response(char *rsp_topic, char *node_id, char *latest_ipc_msg)
{
    // build response topic
    LOG_DBG("RTT: %s node_id=%s, latest_ipc_msg=%s", __func__, node_id, latest_ipc_msg);
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1] = {0};
    snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, RTT_MQTT_TOPIC_CMD_RSP_STR, rsp_topic);
    // build response payload
    char rsp_payload_str[RSP_PAYLOAD_SIZE_MAX];
    CMD_RSP_TYPE rsp;
    rsp.node_id = node_id;
    rsp.latest_ipc_msg = latest_ipc_msg;
    LOG_CMD_RSP(&rsp);
    int err = RSP_ENCODE(&rsp, rsp_payload_str, RSP_PAYLOAD_SIZE_MAX);
    if (err == 0)
    {
        // publish response
        err = mqtt_common_publish(topic_cmd_rsp_str, rsp_payload_str, QOS_CMD_RSP);
    }
    return err;
}

int publish_response_external(char *rsp_topic, char *node_id, char *latest_ipc_msg)
{
    return publish_response(rsp_topic, node_id, latest_ipc_msg);
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
