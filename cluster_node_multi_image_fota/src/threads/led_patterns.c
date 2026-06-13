
#include <zephyr/kernel.h>
#include <errno.h>
#include "led_patterns.h"

#define MODULE led_patterns
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LED_PATTERNS_LOG_LEVEL);

//////////////////////////////////////////////////////////////////////////////////
// LED patterns
//////////////////////////////////////////////////////////////////////////////////

/* boot up pattern slow blinks */
led_pulse_t boot_pulse[] = {
    {1000, 1000},
};
led_pattern_t boot_pattern = {
    .name = "boot_pattern",
    .pulses = boot_pulse,
    .duration = sizeof(boot_pulse) / sizeof(led_pulse_t),
    .repeat = LED_PATTERN_REPEAT_INFINITE,
    .priority = 5,
};

/*network joining pattern */
led_pulse_t network_connecting_pulse[] = {
    {100, 100},
    {0, 200},
};
led_pattern_t network_connecting_pattern = {
    .name = "network_connecting_pattern",
    .pulses = network_connecting_pulse,
    .duration = sizeof(network_connecting_pulse) / sizeof(led_pulse_t),
    .repeat = LED_PATTERN_REPEAT_INFINITE,
    .priority = 5,
};
/*network unhappy pattern */
led_pulse_t network_unhappy_pulse[] = {
    {100, 100},
    {100, 100},
    {100, 100},
    {0, 200},
};
led_pattern_t network_unhappy_pattern = {
    .name = "network_unhappy_pattern",
    .pulses = network_unhappy_pulse,
    .duration = sizeof(network_unhappy_pulse) / sizeof(led_pulse_t),
    .repeat = LED_PATTERN_REPEAT_INFINITE,
    .priority = 6,
};

/* happy heartbeat pattern */
led_pulse_t heart_beat_pulse[] = {
    {100, 200},
    {100, 200},
    {0, 700},
};
led_pattern_t heart_beat = {
    .name = "heart_beat",
    .pulses = heart_beat_pulse,
    .duration = sizeof(heart_beat_pulse) / sizeof(led_pulse_t),
    .repeat = LED_PATTERN_REPEAT_INFINITE,
    .priority = 6,
};

/////////////////////////////////////////////////////////////////////////
// Array of patterns
/////////////////////////////////////////////////////////////////////////
led_pattern_t *led_patterns[] = {
    &boot_pattern,
    &network_connecting_pattern,
    &network_unhappy_pattern,
    &heart_beat,
};

led_pattern_t **led_patterns_get(void)
{
    return &led_patterns[0];
};
int led_patterns_count_get(void)
{
    return (sizeof(led_patterns) / sizeof(led_pattern_t *));
};
