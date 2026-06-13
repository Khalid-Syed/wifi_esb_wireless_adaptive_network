#ifndef __HELIO_ADC_H__
#define __HELIO_ADC_H__

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <nrfx_saadc.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>

#include "motor_constants.h"

/**
 * @brief Symbol specifying the number of sample buffers ( @ref m_sample_buffers ).
 *        Two buffers are required for performing double-buffered conversions.
 */
#define DOUBLE_BUFFER_COUNT 2UL

/** @brief Symbol specifying the size of singular sample buffer ( @ref m_sample_buffers ). */
#define ADC_BUFFER_SIZE 8UL
#define SAMPLE_SIZE 1UL

#define NUM_CHANNELS 4
#define NUM_SIMPLE_CHANNELS 4

#define ADC_IRQ_HIGH_PRIO 1

/** @brief Enum with intended uses of GPPI channels defined as @ref m_gppi_channels. */
typedef enum
{
    SAADC_SAMPLING,     ///< Triggers SAADC sampling task on external timer event.
    SAADC_START_ON_END, ///< Triggers SAADC start task on SAADC end event.
} gppi_channels_purpose_t;

typedef struct {
    uint32_t                    num_channels;
    unsigned long               sample_freq;
    unsigned long               acq_time_us;
    nrfx_saadc_channel_t        adc_channel_config[8];
    nrf_saadc_resolution_t      resolution;
    void*                       callback;
} Adc_Config_t;

void helio_start_saadc(void (*callback)(const nrfx_saadc_evt_t *), uint8_t m_gppi_channels[2], 
                        int16_t m_sample_buffers[DOUBLE_BUFFER_COUNT][ADC_BUFFER_SIZE], bool calibrate, uint32_t timer_period_ms);

int helio_start_simple_saadc(void (*callback)(const nrfx_saadc_evt_t *), int16_t m_sample_buffers[NUM_SIMPLE_CHANNELS]);

void helio_stop_simple_saadc();

void helio_stop_saadc();

void helio_uninit_saadc(uint8_t m_gppi_channels[2]);

void helio_cancel_simple_saadc();

/** @brief semiphore to share the saadc between modules **/
extern struct k_sem saadc_free;
#endif // __HELIO_ADC_H__