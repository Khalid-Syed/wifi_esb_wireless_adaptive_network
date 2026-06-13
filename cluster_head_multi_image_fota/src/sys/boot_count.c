
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "storage_counter.h"

static int boot_count; // TODO: put in statistics module somewhere

int boot_count_increment(void)
{
    // increment boot count
    storage_counter_adjust("boot_count", 1, &boot_count);
    return boot_count;
}
int boot_count_get(void)
{
    return boot_count;
}
