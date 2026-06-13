#ifndef __GET_RANDOM_NUMBER_H__
#define __GET_RANDOM_NUMBER_H__

// azimuth and elevation limits
#define AZ_MIN -3.14142
#define AZ_MAX 3.14145
#define EL_MIN 0.00119
#define EL_MAX 1.54528

/**
 * @brief Get a random float number between min and max
 *
 * @param min Minimum value
 * @param max Maximum value
 * @return float Random number between min and max
 */
float get_random_number(float min, float max);

#endif // __GET_RANDOM_NUMBER_H__