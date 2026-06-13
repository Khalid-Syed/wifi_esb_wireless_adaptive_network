#ifndef __POWER_OBJ_H__
#define __POWER_OBJ_H__
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include "mppt_pub_types.h"

mppt_pub_t *heliostat_mppt_get(void);

bool attempt_mppt_mtx_lock(void);
void mppt_mtx_unlock(void);

#endif // __POWER_OBJ_H__