#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>
#include <stdio.h>

float get_random_number(float min, float max) 
{
    uint32_t rand_val = sys_rand32_get();  // Get a random 32-bit number
    float scaled = (float)rand_val / (float)UINT32_MAX;  // Scale to [0.0, 1.0]
    return min + scaled * (max - min);
}