
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "storage_counter.h"

static int eol_test_result = 0; // TODO: put in statistics module somewhere

int eol_test_result_set(void)
{
    // increment boot count
    storage_counter_adjust("eol_test_result", 1, &eol_test_result);
    return eol_test_result;
}
int eol_test_result_get(void)
{
	storage_counter_get("eol_test_result", &eol_test_result);
    return eol_test_result;
}
