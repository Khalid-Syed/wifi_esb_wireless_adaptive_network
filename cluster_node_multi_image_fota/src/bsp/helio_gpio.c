#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <hal/nrf_gpio.h>

static int cmd_gpio_reset(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int pin = atoi(argv[1]);
    shell_print(shell, "gpio %d reset", pin);
    nrf_gpio_cfg_default(pin);

    return 0;
}

static int cmd_gpio_set(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int pin = atoi(argv[1]);
    int val = atoi(argv[2]);
    shell_print(shell, "gpio %d set", pin);
    // nrf_gpio_pin_dir_set(pin, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg_output(pin);
    nrf_gpio_pin_write(pin, val);

    return 0;
}
static int cmd_gpio_toggle(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int pin = atoi(argv[1]);
    shell_print(shell, "gpio %d toggle", pin);
    nrf_gpio_cfg_output(pin);
    nrf_gpio_pin_toggle(pin);

    return 0;
}
static int cmd_gpio_get(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int pin = atoi(argv[1]);
    nrf_gpio_pin_dir_set(pin, NRF_GPIO_PIN_DIR_INPUT);
    int io = nrf_gpio_pin_read(pin);
    shell_print(shell, "gpio get %d", io);
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(gpio_sub_cmds,
                               SHELL_CMD(reset, NULL, "reset pin to default board state", cmd_gpio_reset),
                               SHELL_CMD(get, NULL, "read pin input", cmd_gpio_get),
                               SHELL_CMD(set, NULL, "write pin output", cmd_gpio_set),
                               SHELL_CMD(toggle, NULL, "toggle pin ouput", cmd_gpio_toggle),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(gpio, &gpio_sub_cmds, "gpio", NULL);
