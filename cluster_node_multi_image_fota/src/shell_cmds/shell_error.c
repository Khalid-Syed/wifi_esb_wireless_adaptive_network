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
#include "sys_watchdog.h"

static int cmd_assert(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Device will now assert");
    __ASSERT(0, "Asserted by user command");

    return 0;
}
static int cmd_fatal(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Device will now thow a fatal error");
    k_panic();
    // Function should not return ----------------------------------------------------------------------------------

    return 0;
}
static int cmd_null(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "null pointer dereference");
    void (*null_ptr)(void) = (void *)0xcccccccc; // cant actually use NULL, 0 is a valid pointer on ARM
    (*null_ptr)();
    // Function should not return ----------------------------------------------------------------------------------

    return 0;
}
static int cmd_watchdog(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "watchdog");
    sys_watchdog_spin();
    // Function should not return ----------------------------------------------------------------------------------

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_error,
                               SHELL_CMD(assert, NULL, "assert", cmd_assert),
                               SHELL_CMD(fatal, NULL, "fatal error", cmd_fatal),
                               SHELL_CMD(null, NULL, "null ptr dereference", cmd_null),
                               SHELL_CMD(watchdog, NULL, "watchdog trigger", cmd_watchdog),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(error, &sub_error, "Forced error cmds", NULL);
