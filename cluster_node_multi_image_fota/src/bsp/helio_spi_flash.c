#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#define SPI_FLASH_TEST_REGION_OFFSET 0xff000
#define SPI_FLASH_SECTOR_SIZE 4096

static int cmd_spi_flash_test(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "spi_flash_test	");
    const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
    const size_t len = sizeof(expected);
    uint8_t buf[sizeof(expected)];
    const struct device *flash_dev;
    int rc;
    // const struct device *foo = DEVICE_DT_GET(DT_ALIAS(accelerometer));
    flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash0));

    if (!device_is_ready(flash_dev))
    {
        shell_print(shell, "%s: device not ready.", flash_dev->name);
        return 0;
    }

    shell_print(shell, "%s SPI flash testing", flash_dev->name);
    shell_print(shell, "==========================");

    /* Write protection needs to be disabled before each write or
     * erase, since the flash component turns on write protection
     * automatically after completion of write and erase
     * operations.
     */
    shell_print(shell, "Test 1: Flash erase");

    /* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
     * SPI_FLASH_SECTOR_SIZE = flash size
     */
    rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET,
                     SPI_FLASH_SECTOR_SIZE);
    if (rc != 0)
    {
        shell_print(shell, "Flash erase failed! %d", rc);
    }
    else
    {
        shell_print(shell, "Flash erase succeeded!");
    }

    shell_print(shell, "Test 2: Flash write");

    shell_print(shell, "Attempting to write %zu bytes", len);
    rc = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
    if (rc != 0)
    {
        shell_print(shell, "Flash write failed! %d", rc);
        return 0;
    }

    memset(buf, 0, len);
    rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
    if (rc != 0)
    {
        shell_print(shell, "Flash read failed! %d", rc);
        return 0;
    }

    if (memcmp(expected, buf, len) == 0)
    {
        shell_print(shell, "Data read matches data written. Good!!");
    }
    else
    {
        const uint8_t *wp = expected;
        const uint8_t *rp = buf;
        const uint8_t *rpe = rp + len;

        shell_print(shell, "Data read does not match data written!!");
        while (rp < rpe)
        {
            shell_print(shell, "%08x wrote %02x read %02x %s",
                        (uint32_t)(SPI_FLASH_TEST_REGION_OFFSET + (rp - buf)),
                        *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
            ++rp;
            ++wp;
        }
    }
    return 0;
}
// shell commands
SHELL_STATIC_SUBCMD_SET_CREATE(spi_flash_sub_cmds,
                               SHELL_CMD(test, NULL, "test spi flash", cmd_spi_flash_test),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(spi_flash, &spi_flash_sub_cmds, "spi_flash", NULL);
