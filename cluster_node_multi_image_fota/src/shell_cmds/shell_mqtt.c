/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include "mqtt_helper_heliogen.h"
#include <zephyr/logging/log.h>
#include "id.h"
#include "transport.h"
#include "transport_queue.h"
#include "mqtt_common.h"
#include "pubs.h"
#include "status_pub.h"

#ifdef CONFIG_WIFI

/*
mqtt shell commands
*/
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

static int cmd_mqtt_state_print(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "mqtt state %s", transport_state_str_get());
    return 0;
}

static int cmd_mqtt_pub(const struct shell *shell, size_t argc, char **argv)
{
    int qos;
    if (argc < 3)
    {
        shell_print(shell, "error - pass in [topic] [payload] strings");
        return 0;
    }
    if (argc > 3)
    {
        qos = atoi(argv[3]);
        if (qos < MQTT_QOS_0_AT_MOST_ONCE || qos > MQTT_QOS_2_EXACTLY_ONCE)
        {
            shell_print(shell, "error - qos must be 0, 1, or 2");
            return 0;
        }
    }
    else
    {
        qos = MQTT_QOS_1_AT_LEAST_ONCE;
    }
    transport_publish(argv[1], argv[2], qos);

    shell_print(shell, "mqtt publish topic %s payload %s qos %d", argv[1], argv[2], qos);

    return 0;
}

static int cmd_mqtt_sub(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2)
    {
        shell_print(shell, "error - pass in [topic] string");
        return 0;
    }
    struct mqtt_topic topics[] = {
        {
            .topic.utf8 = argv[1],
            .topic.size = strlen(argv[1]),
        },
    };
    transport_subscribe(topics, 1, k_uptime_get_32());
    shell_print(shell, "mqtt sub topic %s", argv[1]);

    return 0;
}
static int cmd_mqtt_sub_default(const struct shell *shell, size_t argc, char **argv)
{
    mqtt_common_default_topics_subscribe();
    return 0;
}

static int cmd_mqtt_pubq(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "mqtt pubq pending = %d", transport_queue_pending());
    return 0;
}

// for testing - simulate receiving a message
static int cmd_mqtt_receive(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(shell, "error - pass in [topic] [payload] strings");
        return 0;
    }
    struct mqtt_helper_buf topic = {
        .ptr = argv[1],
        .size = strlen(argv[1]),
    };
    struct mqtt_helper_buf payload = {
        .ptr = argv[2],
        .size = strlen(argv[2]),
    };
    if (mqtt_common_on_mqtt_publish(topic, payload))
    {
        shell_print(shell, "mqtt publish topic %s payload %s", argv[1], argv[2]);
    }
    else
    {
        shell_print(shell, "mqtt publish topic %s payload %s - not handled", argv[1], argv[2]);
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mqtt,
                               SHELL_CMD(state, NULL, "Print state", cmd_mqtt_state_print),
                               SHELL_CMD(pub, NULL, "publish [topic] [payload]", cmd_mqtt_pub),
                               SHELL_CMD(sub, NULL, "subscribe [topic]", cmd_mqtt_sub),
                               SHELL_CMD(sub_default, NULL, "subscribe to all default topics", cmd_mqtt_sub_default),
                               SHELL_CMD(receive, NULL, "receive [topic] [payload]", cmd_mqtt_receive),
                               SHELL_CMD(pubq, NULL, "pubq pending msgs", cmd_mqtt_pubq),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(mqtt, &sub_mqtt, "MQTT commands", NULL);

#endif // CONFIG_WIFI
       /*
       example topic and payload:
      mqtt receive cmd/hs-nrf5340/00180700ED71/goto
       {"session_id":99,"res_topic":"goto_res","axis_a":33,"axis_b":55}
       */