#ifndef JITTER_H
#define JITTER_H
#include <zephyr/types.h>

uint32_t AddJitter(uint32_t aValue, uint16_t aJitter);
uint32_t GetUint32InRange(uint32_t aMin, uint32_t aMax);

#endif // JITTER_H
