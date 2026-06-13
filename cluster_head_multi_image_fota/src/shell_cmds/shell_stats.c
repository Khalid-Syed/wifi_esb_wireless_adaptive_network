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
#include "wifi_stats_pub.h"
#include "jitter.h"

#define MODULE main
LOG_MODULE_DECLARE(MODULE, CONFIG_MAIN_LOG_LEVEL);
// static int cmd_wifi_stats_pub(const struct shell *shell, size_t argc, char **argv)
//{
//     shell_print(shell, "publish wifi stats");
//     wifi_stats_pub();
//     return 0;
// }
static int cmd_jitter(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "jitter");
    int a = atoi(argv[1]);
    int j = atoi(argv[2]);
    shell_print(shell, "a = %d, j = %d", a, j);
    uint32_t jitter = AddJitter(a, j);
    shell_print(shell, "jitter = %d", jitter);
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_stats,
                               //                               SHELL_CMD(wifi_pub, NULL, "wifi stats publish", cmd_wifi_stats_pub),
                               SHELL_CMD(jitter, NULL, "get a jittered number", cmd_jitter),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(stats, &sub_stats, "stats", NULL);
