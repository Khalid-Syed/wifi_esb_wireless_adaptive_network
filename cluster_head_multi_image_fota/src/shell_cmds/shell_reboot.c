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
#include <zephyr/sys/reboot.h>
#include "reboot_timer.h"

static int cmd_reboot(const struct shell *shell, size_t argc, char **argv)
{

    shell_print(shell, "Device will now reboot in 3 seconds");
    reboot_timer_start(3);
    return 0;
}

SHELL_CMD_ARG_REGISTER(reboot, NULL, "Reboot (warm)", cmd_reboot, 1, 0);
