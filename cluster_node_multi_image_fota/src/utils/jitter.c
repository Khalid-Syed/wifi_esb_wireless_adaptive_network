#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>

// some jitter code adapted from open thread source
uint32_t GetUint32InRange(uint32_t aMin, uint32_t aMax)
{
    if (aMax <= aMin)
    {
        return aMin;
    }
    return (aMin + (sys_rand32_get() % (aMax - aMin)));
}
uint32_t AddJitter(uint32_t aValue, uint16_t aJitter)
{
    aJitter = (aJitter <= aValue) ? aJitter : (uint16_t)(aValue);
    return aValue + GetUint32InRange(0, 2 * aJitter + 1) - aJitter;
}