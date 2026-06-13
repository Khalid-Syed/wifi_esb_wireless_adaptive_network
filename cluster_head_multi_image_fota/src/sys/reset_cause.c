
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include "reset_cause.h"
#include "string_utils.h"

#define MODULE reset_cause
LOG_MODULE_REGISTER(MODULE, CONFIG_RESET_CAUSE_LOG_LEVEL);

static inline const char *cause_to_string(uint32_t cause);

static char reset_cause_str[RESET_CAUSE_STR_MAX_LEN + 1]; // +1 is for closing array bracket
static bool initialized = false;

// reset cause code adapted from hwinfo_shell.c
char *reset_cause_get(void)
{
    int res;
    uint32_t cause;

    // in theory, the reset cause register would contain the bit for the most reset cause
    // it seems in practice, it stays set as the reset cause first encounted after a power cycle.
    // this makes it so that the reset cause string will always be the same until the next power cycle.
    // in this code, we clear the reset cause register after reading it so that the reset cause string
    // will be different after every power cycle, and only read it 1x so it the string will be consistent
    // for this boot sequence.
    // TODO: figure this out, the sample reset_cause CLI code does not clear the reset cause register automatically
    if (initialized)
    {
        return reset_cause_str;
    }
    initialized = true;
    res = hwinfo_get_reset_cause(&cause);
    if (res != 0)
    {
        return "Error reading the cause";
    }
    LOG_DBG("reset cause reg: 0x%4.4x\n", cause);
    safe_strlcpy(reset_cause_str, "[", RESET_CAUSE_STR_MAX_LEN); // start array

    int reset_cause_str_len = 0;

    for (uint32_t cause_mask = 1; cause_mask; cause_mask <<= 1)
    {
        if (cause & cause_mask)
        {
            // try to append the reset cause to the string
            char new_cause_str[32];
            int new_cause_len = snprintf(new_cause_str, sizeof(new_cause_str), "%s,", cause_to_string(cause & cause_mask));
            reset_cause_str_len = strlen(reset_cause_str);
            if (new_cause_len < RESET_CAUSE_STR_MAX_LEN - reset_cause_str_len) // do this instead of strcat to avoid buffer overflow
            {
                safe_strlcpy(&reset_cause_str[reset_cause_str_len], new_cause_str, RESET_CAUSE_STR_MAX_LEN - reset_cause_str_len);
            }
        }
    }
    // append closing array bracket
    reset_cause_str_len = strlen(reset_cause_str);
    safe_strlcpy(&reset_cause_str[reset_cause_str_len], "]", RESET_CAUSE_STR_MAX_LEN + 1 - reset_cause_str_len);

    LOG_DBG("reset cause string: %s\n", reset_cause_str);

    hwinfo_clear_reset_cause();
    return reset_cause_str;
}
// all the reset causes supported by NRF5340 platform
static inline const char *cause_to_string(uint32_t cause)
{
    switch (cause)
    {
    case RESET_PIN:
        return "pin";

    case RESET_SOFTWARE:
        return "software";

        //    case RESET_BROWNOUT:
        //        return "brownout";

        //    case RESET_POR:
        //        return "power-on reset";

    case RESET_WATCHDOG:
        return "watchdog";

    case RESET_DEBUG:
        return "debug";

        //    case RESET_SECURITY:
        //        return "security";

    case RESET_LOW_POWER_WAKE:
        return "low power wake-up";

    case RESET_CPU_LOCKUP:
        return "CPU lockup";

        //    case RESET_PARITY:
        //        return "parity error";

        //    case RESET_PLL:
        //       return "PLL error";

        //    case RESET_CLOCK:
        //        return "clock";

        //    case RESET_HARDWARE:
        //        return "hardware";

        //    case RESET_USER:
        //        return "user";

        //    case RESET_TEMPERATURE:
        //        return "temperature";

    default:
        return "unknown";
    }
}
