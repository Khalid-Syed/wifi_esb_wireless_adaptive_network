#ifndef __LED_H__
#define __LED_H__

#include <stdio.h>
#include <zephyr/drivers/gpio.h>

bool led0_init();
void led0_set(bool on);

#endif