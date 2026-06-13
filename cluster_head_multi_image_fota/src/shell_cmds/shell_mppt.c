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
#include "power_obj.h"

static int cmd_mppt_get(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (attempt_mppt_mtx_lock())
    { 
        shell_print(shell, "%d,%d,%d,%d", heliostat_mppt_get()->mv, heliostat_mppt_get()->ma, heliostat_mppt_get()->bat_mv, heliostat_mppt_get()->tenth_C);
        mppt_mtx_unlock();
    }

    return 0;
}

SHELL_CMD_ARG_REGISTER(mppt_get, NULL, "Get mppt values", cmd_mppt_get, 1, 0);