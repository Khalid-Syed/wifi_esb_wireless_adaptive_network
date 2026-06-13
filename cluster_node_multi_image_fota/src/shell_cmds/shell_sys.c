/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "sys_pub.h"
#include "sys_pub_mqtt.h"

#define MODULE main
LOG_MODULE_DECLARE(MODULE, CONFIG_MAIN_LOG_LEVEL);

static int cmd_sys_get(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    sys_info_t sys;
    sys_pub_get(&sys);
    shell_print(shell, "sys: uptime: %d boot_count: %d reboot_cause: %s assert_json: %s fatal_error_json : %s ",
                sys.uptime,
                sys.boot_count,
                sys.reboot_cause,
                sys.assert_json,
                sys.fatal_error_json);
    return 0;
}
static int cmd_sys_pub(const struct shell *shell, size_t argc, char **argv)
{
    int count = 1;
    if (argc > 1)
    {
        count = atoi(argv[1]);
    }
    shell_print(shell, "publish sys mqtt %d times", count);
    for (int i = 0; i < count; i++)
    {
        sys_info_t sys;
        sys_pub_get(&sys);
        sys_pub_mqtt_publish(&sys);
    }
    return 0;
}
static int cmd_sys_init(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "%s", __func__);
    sys_pub_init();
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sys,
                               SHELL_CMD(get, NULL, "get", cmd_sys_get),
                               SHELL_CMD(pub, NULL, "publish", cmd_sys_pub),
                               SHELL_CMD(init, NULL, "init", cmd_sys_init),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(sys, &sub_sys, "sys info", NULL);
