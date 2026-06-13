#include "helio_saadc.h"

K_SEM_DEFINE(saadc_free, 1, 1);

#define TIMER_INST_IDX 2

/** @brief Conversion time [us] (see SAADC electrical specification). */
#define ACQ_TIME_10K 3UL

/** @brief SAADC channel configuration structure for single channel use. */
static nrfx_saadc_channel_t chan0 = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN2, 0); // 0.06_A2 - NTC
static nrfx_saadc_channel_t chan1 = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN3, 1); // 0.07_A3 - PV_I
static nrfx_saadc_channel_t chan2 = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN5, 2); // 0.26_A5 - BAT_V_SENSE
static nrfx_saadc_channel_t chan3 = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN7, 3); // 0.28_A7 - PV_SENSE

nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);

/**
 * @brief ADC uses timer 2 and GPPI for ADC sampling
 *
 * @param callback event handler for adc samples
 * @param m_gppi_channels gppi channels to link adc to timer
 * @param m_sample_buffers buffers to store results
 * @param calibrate bool to determine whether to calibrate the ADC or not
 */
void helio_start_saadc(void (*callback)(const nrfx_saadc_evt_t *),
                       uint8_t m_gppi_channels[2],
                       int16_t m_sample_buffers[DOUBLE_BUFFER_COUNT][ADC_BUFFER_SIZE],
                       bool calibrate,
                       uint32_t timer_period_ms)
{
    nrfx_err_t status;
    (void)status;

    if (k_sem_take(&saadc_free, K_MSEC(1000)) != 0) // wait for saadc to be free
    {
        // LOG_ERR("saadc not available"); // should never happen, others should be quick
        //  TODO: handle this gracefully - design with @cian
        return;
    }
    k_sem_reset(&saadc_free);

    status = nrfx_saadc_init(ADC_IRQ_HIGH_PRIO);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(clk_timer.p_reg);
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_config.p_context = &timer_inst;

    status = nrfx_timer_init(&timer_inst, &timer_config, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

#if defined(__ZEPHYR__)
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SAADC), ADC_IRQ_HIGH_PRIO, nrfx_saadc_irq_handler, 0);
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE), ADC_IRQ_HIGH_PRIO, nrfx_gpiote_irq_handler,
                       0);
#endif

    nrfx_timer_clear(&timer_inst);

    /* Creating variable desired_ticks to store the output of nrfx_timer_us_to_ticks function. */
    uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer_inst, timer_period_ms * 1000);

    /*
     * Setting the timer channel NRF_TIMER_CC_CHANNEL0 in the extended compare mode to clear
     * the timer and to not trigger an interrupt if the internal counter register is equal to
     * desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_inst,
                                NRF_TIMER_CC_CHANNEL0,
                                desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                false);

    // WARNING, CHANGING ACQ TIME REQUIRES UPDATE TO KP PARAMETER
    // Using 0.6V internal reference
    chan0.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
    chan1.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
    chan2.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;
    chan3.channel_config.reference = NRF_SAADC_REFERENCE_INTERNAL;

    chan0.channel_config.gain = NRF_SAADC_GAIN1;
    chan1.channel_config.gain = NRF_SAADC_GAIN1;
    chan2.channel_config.gain = NRF_SAADC_GAIN1;
    chan3.channel_config.gain = NRF_SAADC_GAIN1;

    chan0.channel_config.acq_time = NRF_SAADC_ACQTIME_40US;
    chan1.channel_config.acq_time = NRF_SAADC_ACQTIME_40US;
    chan2.channel_config.acq_time = NRF_SAADC_ACQTIME_40US;
    chan3.channel_config.acq_time = NRF_SAADC_ACQTIME_40US;

    nrfx_saadc_channel_t saadc_channels[NUM_CHANNELS] = {chan0, chan1, chan2, chan3};

    /*
     * Setting the advanced configuration with triggering sampling by the internal timer disabled
     * (internal_timer_cc = 0) and without software start task on end event (start_on_end = false).
     */
    nrfx_saadc_adv_config_t adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    adv_config.internal_timer_cc = 0;
    adv_config.start_on_end = false; // THIS IS KEY TO GET IT TO CONTINUE SAMPLING

    status = nrfx_saadc_channels_config(saadc_channels, NUM_CHANNELS);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    uint32_t channel_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_advanced_mode_set(channel_mask,
                                          NRF_SAADC_RESOLUTION_12BIT,
                                          &adv_config,
                                          callback);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_buffer_set(m_sample_buffers[0], ADC_BUFFER_SIZE);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /*
     * Allocate a dedicated channel and configure endpoints of that channel so that the timer compare event
     * is connected with the SAADC sample task. This means that each time the timer interrupt occurs,
     * the SAADC sampling will be triggered.
     */
    status = nrfx_gppi_channel_alloc(&m_gppi_channels[SAADC_SAMPLING]);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gppi_channel_endpoints_setup(m_gppi_channels[SAADC_SAMPLING],
                                      nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));

    /*
     * Allocate a dedicated channel and configure endpoints of that so that the SAADC event end is connected
     * with the SAADC task start. This means that each time the SAADC fills up the result buffer,
     * the SAADC will be restarted and the result buffer will be prepared in RAM.
     */
    status = nrfx_gppi_channel_alloc(&m_gppi_channels[SAADC_START_ON_END]);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gppi_channel_endpoints_setup(m_gppi_channels[SAADC_START_ON_END],
                                      nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START));

    nrfx_timer_enable(&timer_inst);

    nrfx_gppi_channels_enable(NRFX_BIT(m_gppi_channels[SAADC_START_ON_END]));

    if (calibrate)
    {
        status = nrfx_saadc_offset_calibrate(callback);
    }
    else
    {
        status = nrfx_saadc_mode_trigger();
    }
    NRFX_ASSERT(status == NRFX_SUCCESS);
}

void helio_stop_saadc()
{
    // TODO: dont uninit or re-init if is already in progress
    // potential bug/crash here
    nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_STOP);
}

void helio_uninit_saadc(uint8_t m_gppi_channels[2])
{
    nrfx_timer_uninit(&timer_inst);

    NRFX_IRQ_DISABLE(NRFX_IRQ_NUMBER_GET(NRF_SAADC));

    // TODO: G5FW-115 investigate why this cuts radio and breaks MQTT
    // NRFX_IRQ_DISABLE(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE));

    // freeing channel disables it
    nrfx_gppi_channel_free(m_gppi_channels[SAADC_SAMPLING]);
    nrfx_gppi_channel_free(m_gppi_channels[SAADC_START_ON_END]);

    nrfx_saadc_uninit();
    k_sem_give(&saadc_free); // let other threads know that SAADC is free to use
}

/*
int helio_start_simple_saadc(void (*callback)(const nrfx_saadc_evt_t *), nrf_saadc_value_t m_sample_buffers[NUM_SIMPLE_CHANNELS])
{
    nrfx_err_t status;
    (void)status;

    if (k_sem_take(&saadc_free, K_NO_WAIT) != 0) // check saadc to be free
    {
        return -1;
    }
    k_sem_reset(&saadc_free);

    status = nrfx_saadc_init(ADC_IRQ_HIGH_PRIO);
    NRFX_ASSERT(status == NRFX_SUCCESS);

#if defined(__ZEPHYR__)
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SAADC), ADC_IRQ_HIGH_PRIO, nrfx_saadc_irq_handler, 0);
#endif

    chan4.channel_config.gain = NRF_SAADC_REFERENCE_VDD4;
    chan5.channel_config.gain = NRF_SAADC_REFERENCE_VDD4;
    chan6.channel_config.gain = NRF_SAADC_REFERENCE_VDD4;
    chan7.channel_config.gain = NRF_SAADC_REFERENCE_VDD4;

    chan4.channel_config.acq_time = SAADC_CH_CONFIG_TACQ_40us;
    chan5.channel_config.acq_time = SAADC_CH_CONFIG_TACQ_40us;
    chan6.channel_config.acq_time = SAADC_CH_CONFIG_TACQ_40us;
    chan7.channel_config.acq_time = SAADC_CH_CONFIG_TACQ_40us;

    nrfx_saadc_channel_t saadc_simple_channels[NUM_SIMPLE_CHANNELS] = {chan4, chan5, chan6, chan7};
    status = nrfx_saadc_channels_config(saadc_simple_channels, NUM_SIMPLE_CHANNELS);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    uint32_t channel_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_simple_mode_set(channel_mask,
                                        NRF_SAADC_RESOLUTION_12BIT,
                                        NRF_SAADC_OVERSAMPLE_DISABLED,
                                        callback);

    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_buffer_set(m_sample_buffers, NUM_SIMPLE_CHANNELS);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_offset_calibrate(callback);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    return 0;
}

void helio_stop_simple_saadc()
{
    nrfx_saadc_uninit();
    k_sem_give(&saadc_free); // let other threads know that SAADC is free to use
}

void helio_cancel_simple_saadc()
{
    // TODO: how to safely cancel saadc if its already running?
    // nrfx_saadc_uninit();  ???
    k_sem_give(&saadc_free); // give back so that other threads can use SAADC
}
*/