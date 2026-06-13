/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "string_utils.h"
#include "null_param_check.h"
#include "transport_queue.h"

/* Register log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_TRANSPORT_QUEUE_LOG_LEVEL);

/* define a zephyr message queue */
K_MSGQ_DEFINE(transport_msgq,
              sizeof(transport_msg_t),
              CONFIG_TRANSPORT_MSGQ_LEN,
              TRANSPORT_MSGQ_ITEM_SIZE_POW_2);

int transport_queue_add(transport_msg_t msg)
{
    NULL_PARAM_CHECK(msg.topic);
    NULL_PARAM_CHECK(msg.payload);

    LOG_DBG("Adding msg %s %s to transport_msgq", msg.topic, msg.payload);
    // stuff topic and payload onto the heap
    char *t = k_malloc(sizeof(char) * strlen(msg.topic) + 1); // TODO: not rely on strlen
    if (t == NULL)
    {
        LOG_ERR("Failed to allocate memory for topic");
        return -1;
    }
    safe_strlcpy(t, msg.topic, strlen(msg.topic) + 1);

    char *p = k_malloc(sizeof(char) * strlen(msg.payload) + 1);
    if (p == NULL)
    {
        k_free(t);
        LOG_ERR("Failed to allocate memory for payload");
        return -1;
    }
    safe_strlcpy(p, msg.payload, strlen(msg.payload) + 1);

    // store pointers to k_malloc'd topic and payload in new msg object
    transport_msg_t qmsg;
    qmsg.topic = t;
    qmsg.payload = p;
    qmsg.qos = msg.qos;

    // add msg into queue
    int err = k_msgq_put(&transport_msgq, &qmsg, K_NO_WAIT);
    if (err != 0)
    {
        k_free(t);
        k_free(p);
        LOG_ERR("Failed to add msg into transport_msgq");
        return -1;
    }
    LOG_DBG("Added msg %s %s qos %d to transport_msgq", t, p, qmsg.qos);
    return 0;
}
int transport_queue_pending(void)
{
    return k_msgq_num_used_get(&transport_msgq);
}
int transport_queue_peek(transport_msg_t *msg)
{
    int err = k_msgq_peek(&transport_msgq, msg);
    if (err != 0)
    {
        LOG_ERR("Failed to get msg from transport_msgq");
        return -1;
    }
    return 0;
}
int transport_queue_remove(void)
{
    transport_msg_t msg;

    int err = k_msgq_get(&transport_msgq, &msg, K_NO_WAIT);
    if (err != 0)
    {
        LOG_ERR("Failed to get msg from transport_msgq");
        return -1;
    }
    k_free(msg.topic);
    k_free(msg.payload);
    return 0;
}

int transport_queue_clear(void)
{
    transport_msg_t msg;
    while (k_msgq_get(&transport_msgq, &msg, K_NO_WAIT) == 0)
    {
        k_free(msg.topic);
        k_free(msg.payload);
    }
    LOG_DBG("Cleared transport_msgq");
    return 0;
}
