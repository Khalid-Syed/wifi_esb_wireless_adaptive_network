#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "fs_sub_mqtt.h"
#include "transport.h"
#include "id.h"
#include "fs_sub_json.h" // this would be switched out for protobuf or other as needed
#include "storage.h"

#define MODULE fs_sub_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_FS_SUB_MQTT_LOG_LEVEL);

//
#define MQTT_TOPIC_GET fs_mqtt_topic_get
#define ON_MQTT_PUBLISH_CB fs_on_mqtt_publish_cb
// command topic
#define TOPIC FS_MQTT_TOPIC_CMD_STR
#define TOPIC_CMD_LEN ((sizeof(FS_MQTT_TOPIC_CMD_STR) + ID_LEN) + 1)
#define QOS_CMD FS_MQTT_CMD_QOS
#define CMD_PAYLOAD_TYPE fs_cmd_t
#define LOG_CMD log_fs_cmd

static char topic_cmd_str[TOPIC_CMD_LEN];

// command response topic
#define TOPIC_ERR_RSP FS_MQTT_TOPIC_CMD_ERR_RSP_STR
#define QOS_CMD_RSP FS_MQTT_CMD_RSP_QOS
#define CMD_RSP_TYPE fs_cmd_resp_t
#define LOG_CMD_RSP log_fs_cmd_resp
#define RSP_PAYLOAD_SIZE_MAX FS_RESP_PAYLOAD_STR_SIZE_MAX

static const char *RSP_OK = "ok";
static const char *RSP_PAYLOAD_DECODE_ERR = "payload decode err";
static const char *RSP_PAYLOAD_BAD_VALUES_ERR = "payload bad values";
static const char *RSP_PAYLOAD_UNKNOWN_ERR = "payload unknown err";
static const char *RSP_ERR = "err";
static const char *RSP_ERR_UNKNOWN_CMD = "err-unknown-fs-cmd";

static int publish_response(char *rsp_topic, int session_id, const char *status, const char *res_data);
static int publish_response_err();

int fs_sub_process_payload(fs_cmd_t cmd, char *status, char *res_data)
{

    if (strcmp("wr", cmd.cmd) == 0)
    {
        LOG_DBG("write cmd, file = %s data = %s", cmd.file, cmd.data);
        // write to file
        // delete file if exists - ignore errors
        storage_remove(cmd.file);
        // create file and write data to file
        int rc = storage_write(cmd.file, cmd.data, strlen(cmd.data));
        sprintf(res_data, "%d", rc);
        if (rc < 0)
        {
            LOG_ERR("storage_write failed rc = %d", rc);
            sprintf(status, "%s", RSP_ERR);
        }
        else
        {
            sprintf(status, "%s", RSP_OK);
        }
    }
    else if (strcmp("rd", cmd.cmd) == 0)
    {
        LOG_DBG("read cmd %s", cmd.file);
        // read from file
        int rc = storage_read(cmd.file, 0, res_data, RSP_PAYLOAD_SIZE_MAX);
        if (rc < 0)
        {
            LOG_ERR("storage_read failed");
            sprintf(res_data, "%d", rc);
            sprintf(status, "%s", RSP_ERR);
        }
        else
        {
            LOG_DBG("storage_read success, rc = %d", rc);
            LOG_DBG("res_data %s", res_data);
            sprintf(status, "%s", RSP_OK);
        }
    }
    else if (strcmp("mv", cmd.cmd) == 0)
    {
        LOG_DBG("read cmd %s", cmd.file);
        // read from file
        int rc = storage_move(cmd.file, cmd.data);
        if (rc < 0)
        {
            LOG_ERR("storage_renamed failed");
            sprintf(res_data, "%d", rc);
            sprintf(status, "%s", RSP_ERR);
        }
        else
        {
            LOG_DBG("storage_rename success, rc = %d", rc);
            LOG_DBG("res_data %s", res_data);
            sprintf(status, "%s", RSP_OK);
        }
    }
    else if (strcmp("rm", cmd.cmd) == 0)
    {
        LOG_DBG("rm cmd %s", cmd.file);
        // remove file
        int rc = storage_remove(cmd.file);
        if (rc < 0)
        {
            LOG_ERR("storage_renamed failed");
            sprintf(res_data, "%d", rc);
            sprintf(status, "%s", RSP_ERR);
        }
        else
        {
            LOG_DBG("storage_remove success, rc = %d", rc);
            sprintf(status, "%s", RSP_OK);
        }
    }
    else if (strcmp("ls", cmd.cmd) == 0)
    {
        LOG_DBG("ls cmd");
        // read directory
        int rc = storage_list(res_data, RSP_PAYLOAD_SIZE_MAX);
        if (rc < 0)
        {
            LOG_ERR("storage_read failed");
            sprintf(res_data, "%d", rc);
            sprintf(status, "%s", RSP_ERR);
        }
        else
        {
            sprintf(status, "%s", RSP_OK);
        }
    }
    else
    {
        LOG_ERR("unknown cmd");
        sprintf(res_data, "");
        sprintf(status, "%s", RSP_ERR_UNKNOWN_CMD);
    }
    return 0;
}

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
        LOG_DBG("%s matched topic %s payload: %s", __func__, topic.ptr, payload.ptr);
        // return 1;
        CMD_PAYLOAD_TYPE cmd;
        char payload_str[payload.size + 1];
        memcpy(payload_str, payload.ptr, payload.size);
        payload_str[payload.size] = '\0';
        k_sleep(K_MSEC(500));
        int err = PAYLOAD_DECODE(payload_str, &cmd);
        if (err == 0)
        {
            LOG_CMD(&cmd);
            char res_data[RSP_PAYLOAD_SIZE_MAX] = "";
            char rc[16] = "";
            fs_sub_process_payload(cmd, rc, res_data);
            LOG_DBG("rc %s %s", rc, res_data);
            publish_response(cmd.res_topic, cmd.session_id, rc, res_data);
        }
        else if (err == -ENOEXEC)
        {
            LOG_ERR("%s err %d failed to decode fs_cmd", __func__, err);
            publish_response_err(RSP_PAYLOAD_DECODE_ERR);
        }
        else if (err == -EINVAL)
        {
            LOG_ERR("%s err %d bad values", __func__, err);
            publish_response_err(RSP_PAYLOAD_BAD_VALUES_ERR);
        }
        else
        {
            LOG_ERR("%s err %d unknown", __func__, err);
            publish_response_err(RSP_PAYLOAD_UNKNOWN_ERR);
        }
        return true;
    }
    return false;
}

static int publish_response(char *rsp_topic, int session_id, const char *status, const char *res_data)
{
    // build response topic
    LOG_DBG("%s session_id=%d status=%s res_data %s", __func__, session_id, status, res_data);
    
    char topic_cmd_rsp_str[MQTT_RESPONSE_TOPIC_SIZE_MAX + 1];
    char id[ID_LEN];
    if (!(id_get(id, sizeof(id))))
    {
        // this builds a topic like: fs/cmd/rsp/1234567890/fs_res
        snprintf(topic_cmd_rsp_str, MQTT_RESPONSE_TOPIC_SIZE_MAX, FS_MQTT_TOPIC_CMD_RSP_STR, id, rsp_topic);
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    // build response payload
    char rsp_payload_str[RSP_PAYLOAD_SIZE_MAX + 1];

    CMD_RSP_TYPE rsp;
    rsp.session_id = session_id,
    rsp.status = status;
    rsp.data = res_data;
    LOG_CMD_RSP(&rsp);
    int err = 0;
    RSP_ENCODE(&rsp, rsp_payload_str, RSP_PAYLOAD_SIZE_MAX);

    if (err == 0)
    {
        // publish response
        LOG_DBG("%s publish response %s %s", __func__, topic_cmd_rsp_str, rsp_payload_str);
        err = mqtt_common_publish(topic_cmd_rsp_str, rsp_payload_str, QOS_CMD_RSP);
    }
    LOG_DBG("err = %d", err);

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
