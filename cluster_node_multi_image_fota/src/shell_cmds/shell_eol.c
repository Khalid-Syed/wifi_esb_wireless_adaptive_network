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
#include "eol_test_result.h"
#include "heliogen_io.h"
#include <hal/nrf_gpio.h>

static int cmd_eol_get(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "eol test result: %d", eol_test_result_get());
    return 0;
}

static int cmd_eol_set(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "eol test set: %d", eol_test_result_set());
    return 0;
}

static int cmd_eol_pass(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // Set Green LED
	nrf_gpio_pin_write(LED2, 1);
    return 0;
}

static int cmd_eol_fail(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // Set red LED
    nrf_gpio_pin_write(LED1, 1);
    return 0;
}

SHELL_CMD_ARG_REGISTER(eol_get, NULL, "Get test result", cmd_eol_get, 1, 0);
SHELL_CMD_ARG_REGISTER(eol_set, NULL, "Set test result", cmd_eol_set, 1, 0);
SHELL_CMD_ARG_REGISTER(eol_pass, NULL, "EOL Pass", cmd_eol_pass, 1, 0);
SHELL_CMD_ARG_REGISTER(eol_fail, NULL, "EOL Fail", cmd_eol_fail, 1, 0);