/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "boot_count.h"

static int cmd_boot_count(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "boot_count %d", boot_count_get());
    return 0;
}
SHELL_CMD_ARG_REGISTER(boot_count, NULL, "Show boot count", cmd_boot_count, 1, 0);
