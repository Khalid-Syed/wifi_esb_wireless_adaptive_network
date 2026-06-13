#include "power_thread.h"
#include "helio_saadc.h"
#include "power_obj.h"

#ifndef POWER_THREAD_STACK_SIZE
#define POWER_THREAD_STACK_SIZE 4096
#endif
#ifndef POWER_THREAD_PRIORITY
#define POWER_THREAD_PRIORITY 2
#endif
#define MODULE power

LOG_MODULE_REGISTER(MODULE, CONFIG_POWER_THREAD_LOG_LEVEL);

#define BATTERY_IPC_UPDATE_MS 5000
#define BATTERY_IPC_CHANGE_THRESHOLD_MV 100

K_THREAD_STACK_DEFINE(power_thread_stack_area, POWER_THREAD_STACK_SIZE);
struct k_thread power_thread_data;

static void (*im_alive_cb)(int, uint32_t, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;                               // index to pass back to im_alive callback

static void power_thread_entry_point();

static volatile bool new_adc_data_rdy = false;

/** @brief Array of the GPPI channels. */
static uint8_t m_gppi_channels[2];

/** @brief Samples buffer to store values from a SAADC channel. */
static int16_t m_sample_buffers[DOUBLE_BUFFER_COUNT][ADC_BUFFER_SIZE];

/** @brief Current state for the power state machine*/
enum MpptPowerState mppt_power_state = OFF;

#define DATA_BUFFER_LENGTH 50 

const int BATT_MV_AVG_WIFI_OFF = 3000;
const int BATT_MV_AVG_WIFI_ON = 3100;

const int PV_MV_AVG_WIFI_OFF = 1000;
const int PV_MV_AVG_WIFI_ON = 1250;

const int MIN_MW_HARVESTING_STOP = 25;

int batt_mv_avg_wifi_off = BATT_MV_AVG_WIFI_OFF;
int batt_mv_avg_wifi_on = BATT_MV_AVG_WIFI_ON;
int pv_mv_avg_wifi_off = PV_MV_AVG_WIFI_OFF;
int pv_mv_avg_wifi_on = PV_MV_AVG_WIFI_ON;

bool is_production_build = true;

int32_t rb_new_ind, rb_wr_ind, rb_entries;

int32_t battery_mv [DATA_BUFFER_LENGTH];
int32_t pv_mv [DATA_BUFFER_LENGTH];
int32_t pv_ma [DATA_BUFFER_LENGTH];
int32_t ntc_dc [DATA_BUFFER_LENGTH];   //dc is deci C

int32_t battery_mv_avg;
int32_t pv_mv_avg;
int32_t pv_ma_avg;
int32_t ntc_dc_avg;
int32_t new_mw;

volatile int32_t ntc_dc_counts;
volatile int32_t pv_ma_counts;
volatile int32_t pv_mv_counts;
volatile int32_t battery_mv_counts;

volatile bool is_wifi_thread_on = false;

int32_t pv_mv_sum;
int32_t pv_ma_sum;
int32_t battery_mv_sum;
int32_t ntc_dc_sum;

static uint32_t tick_count = 0;
static int gate_duty_cycle = 0;

/* No longer needed as we use on-demand requests */
// static void send_battery_to_net_core(int32_t mv)
// {
//     static int32_t last_sent_mv = 0;
//     static uint32_t last_send_time = 0;
//     uint32_t now = k_uptime_get_32();

//     /* Strictly enforce the 5-second interval heartbeat */
//     bool heartbeat_due = (last_send_time == 0) || ((now - last_send_time) >= BATTERY_IPC_UPDATE_MS);
    
//     /* Also send if there is a significant change (100mV) */
//     bool significant_change = abs(mv - last_sent_mv) >= BATTERY_IPC_CHANGE_THRESHOLD_MV;

//     if (heartbeat_due || significant_change) {
//         if (ipc_send_battery(mv) == 0) {
//             last_sent_mv = mv;
//             last_send_time = now;
//         } else {
//             /* If IPC fails, retry in 1 second */
//             last_send_time = now - (BATTERY_IPC_UPDATE_MS - 1000);
//         }
//     }
// }

//Function declarations
int set_duty_cycle(int new_duty);
void update_mppt_power_state();
void calculate_adc_values();
void reset_ringbuffer();
bool check_charging_ok(int temperature, int battery_mv);
void transition_to_off();

/* Extern IPC battery update helper from main.c */
extern int ipc_send_battery(int32_t battery_mv);

/** @brief Structs used for thread signalling */
struct k_poll_signal adc_data_rdy_sig = K_POLL_SIGNAL_INITIALIZER(adc_data_rdy_sig);
struct k_poll_event adc_events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &adc_data_rdy_sig),
    };

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t power_thread_start(void(*cb), int idx)
{
    im_alive_cb = cb;
    thread_idx = idx;

    k_tid_t my_tid = k_thread_create(&power_thread_data, power_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(power_thread_stack_area),
                                     power_thread_entry_point,
                                     NULL, NULL, NULL,
                                     POWER_THREAD_PRIORITY, 0, K_NO_WAIT);
    return my_tid;
}

/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void power_thread_stall_assert(void)
{
    __ASSERT(0, "Power thread stalled");
}

/***************************  ADC CODE  **************************/

// Number of bits to right shift to divide by the sample size
// In this case the sample size is 64
#define SAMPLE_BIT_SHIFT 6

// Left shift by 2 to multiply by 4, this will get the correct
// samples from the ADC buffer
#define BUFFER_BIT_SHIFT 2

#define NTC_IDX 0
#define PV_I_IDX 1
#define BAT_V_SENSE_IDX 2
#define PV_SENSE_IDX 3

#define PV_I_SCALE_FACTOR 1

/**
 * @brief Timer 2 and GPPI are used to setup ADC, this handler is called at that frequency
 * 
 * @param p_event SAADC event with the adc data to process
 */
static void saadc_power_handler(nrfx_saadc_evt_t const * p_event)
{
    nrfx_err_t status;
    (void)status;

	static uint16_t buffer_index = 1;

	switch (p_event->type)
	{
		case NRFX_SAADC_EVT_DONE:
            ntc_dc_counts = ((int16_t*)(p_event->data.done.p_buffer))[NTC_IDX];
            pv_mv_counts =  ((int16_t*)(p_event->data.done.p_buffer))[PV_SENSE_IDX];
            pv_ma_counts =  ((int16_t*)(p_event->data.done.p_buffer))[PV_I_IDX];
            battery_mv_counts =  ((int16_t*)(p_event->data.done.p_buffer))[BAT_V_SENSE_IDX];

            /***
            int32_t moving_avg_ntc = ntc_sum >> SAMPLE_BIT_SHIFT;
            // TODO: update to NTC conversion factor
            // float temp_ntc_C = battery_ntc_adc_to_temp(moving_avg_ntc);
            float temp_ntc = (float) moving_avg_ntc;

            int32_t moving_avg_pv_i = pv_i_sum >> SAMPLE_BIT_SHIFT;
            // TODO: update to PV_I conversion factor
            float pv_i_milliamps = (float) moving_avg_pv_i * PV_I_SCALE_FACTOR;

            int32_t moving_avg_bat_v = bat_v_sen_sum >> SAMPLE_BIT_SHIFT;
            float bat_v = (float) moving_avg_bat_v;

            int32_t moving_avg_pv_sen = pv_sen_sum >> SAMPLE_BIT_SHIFT;
            float pv_v = (float) moving_avg_pv_sen;

            // TODO: remove debug logging
            if (adc_counter % 100 == 0)
            {
                LOG_ERR("Temp NTC (mV): %f", temp_ntc);
                LOG_ERR("PV Current (mA): %f", pv_i_milliamps);
                LOG_ERR("Battery Voltage (mV): %f", bat_v);
                LOG_ERR("PV Voltage (mV): %f", pv_v);
            }
            ***/

            //TODO: I think this is where this should be raised
            // k_poll_signal_raise(&adc_data_rdy_sig, 1);
            // LOG_INF("adc_data_rdy signaled");

            new_adc_data_rdy = true;

			break;

        case NRFX_SAADC_EVT_CALIBRATEDONE:
			LOG_DBG("SAADC event: CALIBRATEDONE");

			status = nrfx_saadc_mode_trigger();
            NRFX_ASSERT(status == NRFX_SUCCESS);
            break;

        case NRFX_SAADC_EVT_READY:
			LOG_DBG("SAADC event: READY");
			nrfx_gppi_channels_enable(NRFX_BIT(m_gppi_channels[SAADC_SAMPLING]));
            break;

        case NRFX_SAADC_EVT_BUF_REQ:
			
			status = nrfx_saadc_buffer_set(m_sample_buffers[buffer_index++], ADC_BUFFER_SIZE);
			NRFX_ASSERT(status == NRFX_SUCCESS);
			buffer_index = buffer_index % DOUBLE_BUFFER_COUNT;

            break;

        case NRFX_SAADC_EVT_FINISHED:
			LOG_INF("SAADC event: FINISHED");
			nrfx_gppi_channels_disable(NRFX_BIT(m_gppi_channels[SAADC_SAMPLING]));

			helio_uninit_saadc(m_gppi_channels);

            break;

        default:
            break;
    }
}

void start_adc(bool calibrate, uint32_t adc_period_ms)
{
	// initialize the 4 ADC channels
	helio_start_saadc(&saadc_power_handler, m_gppi_channels, m_sample_buffers, calibrate, adc_period_ms);
}

void calculate_adc_values()
{    
    //If the ringbuffer is full, remove oldest values from sum variables
    if (rb_entries >= DATA_BUFFER_LENGTH)
    {
        pv_mv_sum -= pv_mv[rb_wr_ind];
        pv_ma_sum -= pv_ma[rb_wr_ind];
        battery_mv_sum -= battery_mv[rb_wr_ind];
        ntc_dc_sum -= ntc_dc[rb_wr_ind];
    }

    // Convert ADC counts to relevant values
    pv_mv[rb_wr_ind] = (int) ((float) (pv_mv_counts * 600) / (float) 895); // Multiply by Vref(600 mv) and divide by 4095*Rdivider factor(0.0.218) to get 894.64
    battery_mv[rb_wr_ind] = (int) ((float) (battery_mv_counts * 600) / (float) 607);   // Multiply by Vref(600 mv) and divide by 4095*Rdivider factor(0.148) to get 607.16
    pv_ma[rb_wr_ind] = (int) ((float) (pv_ma_counts * 600) / (float) (12285));  // Vout = Ipv * (0.015 ohms) * (200 Gain)
    ntc_dc[rb_wr_ind] = (int) (helio_ntc_adc_to_temp(ntc_dc_counts) * 10);

    // Add new values to running sum
    pv_mv_sum += pv_mv[rb_wr_ind];
    battery_mv_sum += battery_mv[rb_wr_ind];
    pv_ma_sum += pv_ma[rb_wr_ind];
    ntc_dc_sum += ntc_dc[rb_wr_ind]; 

    // Handle ring buffer
    rb_new_ind = rb_wr_ind;
    rb_wr_ind++;
    if (rb_wr_ind >= DATA_BUFFER_LENGTH)
    {
        rb_wr_ind = 0;
    }
    
    if (rb_entries < DATA_BUFFER_LENGTH)
    {
        rb_entries++;
    }
   
    // Update average values
    pv_mv_avg = pv_mv_sum / rb_entries;
    pv_ma_avg = pv_ma_sum / rb_entries;
    battery_mv_avg = battery_mv_sum / rb_entries;
    ntc_dc_avg = ntc_dc_sum / rb_entries;
}

void reset_ringbuffer()
{
    rb_new_ind = 0;
    rb_wr_ind = 0;
    rb_entries = 0;
    pv_mv_sum = 0;
    pv_ma_sum = 0;
    battery_mv_sum = 0;
    ntc_dc_sum = 0;
}

static void update_mppt_obj()
{
    if (attempt_mppt_mtx_lock())
    {
        heliostat_mppt_get()->mppt_state = (int) mppt_power_state;
        heliostat_mppt_get()->gate_duty = gate_duty_cycle;
        heliostat_mppt_get()->mv = pv_mv_avg;
        heliostat_mppt_get()->ma = pv_ma_avg;
        heliostat_mppt_get()->mw = new_mw;
        heliostat_mppt_get()->bat_mv = battery_mv_avg;
        heliostat_mppt_get()->tenth_C = ntc_dc_avg;
        mppt_mtx_unlock();
    }
}

static void load_configs()
{
    is_production_build = cfg_get()->pwr_cfg.is_prod_unit;

    batt_mv_avg_wifi_off = cfg_get_batt_mv_wifi_off();
    batt_mv_avg_wifi_on = cfg_get_batt_mv_wifi_on();
    pv_mv_avg_wifi_off = cfg_get_pv_mv_wifi_off();
    pv_mv_avg_wifi_on = cfg_get_pv_mv_wifi_on();
}

static void power_thread_entry_point()
{
    load_configs();

    //Initialize data ring buffer
    reset_ringbuffer();

	// start ADC with calibration, 2ms period is 500 Hz
	start_adc(true, 2);

    // Init PWM
    helio_pwm_init();

    nrf_gpio_pin_write(THERM_EN, 1);

    // TODO: start/stop saadc depending on the power state
	// helio_stop_saadc();
    // bool led_state = 0;

    bool min_power_wifi = false;

    int32_t prev_batt_mv_avg = 3000;
    int32_t prev_pv_mv_avg = 1000;

    // Use this to determine if the EOL test has been executed
    int32_t eol_test_result = 1;
    storage_counter_get("eol_test_result", &eol_test_result);

	while (1)
    {
        while (!new_adc_data_rdy)
        {
            k_usleep(100);
        }

        new_adc_data_rdy = false;

        calculate_adc_values();
        update_mppt_power_state();
        
        im_alive_cb(thread_idx, k_uptime_get_32(), 10000);

        // 2ms * 800 = 1.6 seconds
        if (tick_count % 800 == 0)
        {
            /*
            nrf_gpio_pin_write(LED2, led_state);
            led_state = !led_state;
            */

            update_mppt_obj();
            /* Removed periodic IPC send - Net Core now requests on-demand */
            // send_battery_to_net_core(battery_mv_avg);

            if (!min_power_wifi)
            {
                if ((battery_mv_avg > BATT_MV_AVG_WIFI_ON) && (!is_production_build || pv_mv_avg > PV_MV_AVG_WIFI_ON))
                {
                    struct power_status_event *pwr_e = new_power_status_event();
                    pwr_e->power_state = WIFI_THREAD_START;
                    APP_EVENT_SUBMIT(pwr_e);
                    min_power_wifi = true;
                }
            }

        }

        tick_count++;
	}
}

const int MAX_BATTERY_MV = 3600;
const int BATTERY_CHARGE_TEMP_MIN_DC = 0;
const int BATTERY_CHARGE_TEMP_MAX_DC = 4500;

bool check_charging_ok(int temperature, int battery_mv)
{
    if (battery_mv > MAX_BATTERY_MV)
        return false;

    if (temperature < BATTERY_CHARGE_TEMP_MIN_DC)
        return false;

    if (temperature > BATTERY_CHARGE_TEMP_MAX_DC)
        return false;

    return true;
}

void transition_to_off()
{  
    // Disable (turn off) PWM
    turn_off_pwm();
    // Change ADC to OFF frequency
    // Disable PV_I
    nrf_gpio_pin_write(PV_I_EN, 0);
    mppt_power_state = OFF;
}

const int MIN_DUTY_CYCLE = 1;
const int MAX_DUTY_CYCLE = 254; // Check if this is correct of OBOE

/**
 * @brief Safely set the boost converter duty cycle
 * 
 * @param new_duty 
 * @return int 
 */
int set_duty_cycle(int new_duty)
{
    if (new_duty < MIN_DUTY_CYCLE)
        new_duty = MIN_DUTY_CYCLE;
    if (new_duty > MAX_DUTY_CYCLE)
        new_duty = MAX_DUTY_CYCLE;
    
    // Write new duty cycle value
    update_duty_block(new_duty);
	start_pwm_sequence();

    return new_duty;
}

/**
 * @brief call this when there is new adc data to figure out what the
 * boost converter PWM should be
 * 
 */
const int MIN_PV_TURN_ON_MV = 1200;
const int SCAN_DUTY_INCREMENT = 5;

void update_mppt_power_state()
{
    static int delta_duty_cycle = 0;
    static int prev_mw = 0;
    static int prev_duty = 0;
    
    switch(mppt_power_state)
    {
    case OFF:
        // Fill buffer first
        if (rb_entries < DATA_BUFFER_LENGTH)
            break;
        
        if (pv_mv_avg > MIN_PV_TURN_ON_MV && check_charging_ok(ntc_dc_avg, battery_mv[rb_new_ind]))
        {
            prev_mw = 0;
            prev_duty = 0;
            gate_duty_cycle = set_duty_cycle(MIN_DUTY_CYCLE);
            delta_duty_cycle = -1;  // This is used during TRACK
            mppt_power_state = SCAN;
            LOG_INF("Power state set to SCAN from OFF");
            LOG_INF("PV mv average: %d", pv_mv_avg);
            LOG_INF("RB_entries: %d", rb_entries);

            // Enable PV_I_EN Pin
            nrf_gpio_pin_write(PV_I_EN, 1);
            reset_ringbuffer();
        }
        break;
    
    case SCAN:

        if (!check_charging_ok(ntc_dc_avg, battery_mv[rb_new_ind]))
        {
            transition_to_off();
            LOG_INF("Power state set to OFF from SCAN");
            break;
        }

        // wait for some number of samples, could be less than buffer size 
        if (rb_entries < DATA_BUFFER_LENGTH)
            break;

        new_mw = pv_mv_avg * pv_ma_avg / 1000;

        // LOG_ERR("SCAN ~ PV_V: %d, PV_i: %d, Bat_V: %d, New_mw: %d, Prev mw: %d, Duty_cycle: %d", pv_mv_avg, pv_ma_avg, battery_mv_avg, new_mw, prev_mw, gate_duty_cycle);

        reset_ringbuffer();

        gate_duty_cycle += SCAN_DUTY_INCREMENT;

        if (new_mw < prev_mw)
        {
            gate_duty_cycle -= 2*SCAN_DUTY_INCREMENT;   //2x because one was already added
            LOG_INF("Power state set to TRACK from SCAN, new_mw less than prev");
            mppt_power_state = TRACK;
        }

        gate_duty_cycle = set_duty_cycle(gate_duty_cycle);

        if (gate_duty_cycle >= MAX_DUTY_CYCLE)
        {
            mppt_power_state = TRACK;
            LOG_INF("Power state set to TRACK from SCAN, greater than max duty");
        }

        prev_mw = new_mw;
        break;

    case TRACK:
        if (!check_charging_ok(ntc_dc_avg, battery_mv[rb_new_ind]))
        {
            transition_to_off();
            LOG_INF("Power state set to OFF from TRACK");
            break;
        }

        // wait for some number of samples, could be less than buffer size 
        if (rb_entries < DATA_BUFFER_LENGTH)
            break;

        new_mw = pv_mv_avg * pv_ma_avg / 1000;

        if (new_mw < MIN_MW_HARVESTING_STOP)
        {
            transition_to_off();
            LOG_INF("Power state set to OFF from TRACK due to Captured Power of %d", new_mw);
            break;
        }

        // LOG_ERR("TRACK ~ PV_V: %d, PV_i: %d, Bat_V: %d, New_mw: %d, Duty_cycle: %d", pv_mv_avg, pv_ma_avg, battery_mv_avg, new_mw, gate_duty_cycle);

        reset_ringbuffer();

        // Don't want the case that we get "stuck" at max or min, so always try
        // to move away from edges
        switch (gate_duty_cycle)
        {
        case MIN_DUTY_CYCLE:
            gate_duty_cycle += 1;
            delta_duty_cycle = 1;
            break;
        
        case MAX_DUTY_CYCLE:
            gate_duty_cycle -= 1;
            delta_duty_cycle = -1;
            break;

        default:
            if(new_mw < prev_mw)
            {
                delta_duty_cycle *= -1;
            }
            gate_duty_cycle += delta_duty_cycle;
            break;
        }
        prev_mw = new_mw;
        gate_duty_cycle = set_duty_cycle(gate_duty_cycle);
        break;

    default:
        //should never enter this
        break;
    }
}
