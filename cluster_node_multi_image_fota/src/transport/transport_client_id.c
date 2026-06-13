/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "id.h"

/* Register log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

/* Forward declarations */

/* MQTT client ID buffer */
static char client_id_str[CONFIG_MQTT_TRANSPORT_CLIENT_ID_BUFFER_SIZE] = "";
static int client_id_str_len = 0;
/********************/
/* private functions*/
/********************/

char *client_id(void)
{
    return client_id_str;
}
int client_id_len(void)
{
    return client_id_str_len;
}
int client_id_init(void)
{
    char *buffer = client_id_str;
    size_t buffer_size = sizeof(client_id_str) / sizeof(char);

    if (sizeof(CONFIG_MQTT_TRANSPORT_CLIENT_ID) - 1 > 0)
    {
        client_id_str_len = snprintk(buffer, buffer_size, "%s", CONFIG_MQTT_TRANSPORT_CLIENT_ID);
        if ((client_id_str_len < 0) || (client_id_str_len >= buffer_size))
        {
            LOG_ERR("Failed to copy client ID");
            client_id_str_len = 0;
            return client_id_str_len;
        }
    }
    else
    {
        client_id_str_len = id_get(buffer, buffer_size);
        if (client_id_str_len)
        {
            return client_id_str_len;
        }
    }

    return client_id_str_len;
}
