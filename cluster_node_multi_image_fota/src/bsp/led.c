#include "led.h"

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


bool led0_init()
{
    bool static initialized = false;

    if(initialized)
        return initialized;

    //  led 0
    if (!device_is_ready(led0.port))
    {
        // printk("LED not ready\n");
        return initialized;
    }    
    
    if (gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE) == 0)
    {
        initialized = true;
    }

    return initialized;
}


void led0_set(bool on)
{
    // Don't invert the on signal even though the LED is visible when low.
    // The device tree specifies the LED is GPIO_ACTIVE_LOW and takes care of it
    gpio_pin_set_dt(&led0, on);
}

