#ifndef __CFG_DEFAULTS_H__
#define __CFG_DEFAULTS_H__
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "cfg.h"

/**
 * @brief Load config struct with default values
 *
 * @param cfg
 * @return int
 */
int cfg_defaults(cfg_t *cfg_p);
void set_pid_defaults(cfg_t *cfg_p);
#endif