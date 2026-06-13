
#include <stdio.h>
#include "template_thread.h"

#ifndef TEMPLATE_THREAD_STACK_SIZE
#define TEMPLATE_THREAD_STACK_SIZE 4096
#endif
#ifndef TEMPLATE_THREAD_PRIORITY
#define TEMPLATE_THREAD_PRIORITY 5
#endif
#define MODULE template
// module logging
// https://docs.zephyrproject.org/latest/services/logging/index.html

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MODULE);

K_THREAD_STACK_DEFINE(template_thread_stack_area, TEMPLATE_THREAD_STACK_SIZE);
struct k_thread template_thread_data;

static void template_thread_do_something(void);
static void template_thread_entry_point(void *, void *, void *);
static void (*im_alive_cb)(int, uint32_t, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;                               // index to pass back to im_alive callback

static int8_t i8 = 10;
static int16_t i16 = 10;
static int32_t i32 = 10;
static uint8_t u8 = 10;
static uint16_t u16 = 10;
static uint32_t u32 = 10;
static const char *s1 = "fu";  // string
static const char *s2 = "bar"; // string
// static int64_t ts;             // kernel timestamp

void template_timer_func(struct k_timer *dummy)
{

    // printk("%s send an event\n", __func__);
    struct template_event *event = new_template_event();
    event->i8 = i8--;
    event->i16 = i16--;
    event->i32 = i32--;
    event->u8 = u8++;
    event->u16 = u16++;
    event->u32 = u32++;
    if (i8 > 0)
    {
        event->s = s1;
    }
    else
    {
        event->s = s2;
    }
    event->ts = k_uptime_get();
    APP_EVENT_SUBMIT(event);
}

K_TIMER_DEFINE(template_timer, template_timer_func, NULL);

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t template_thread_start(void(*cb), int idx)
{
    im_alive_cb = cb;
    thread_idx = idx;
    k_tid_t my_tid = k_thread_create(&template_thread_data, template_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(template_thread_stack_area),
                                     template_thread_entry_point,
                                     NULL, NULL, NULL,
                                     TEMPLATE_THREAD_PRIORITY, 0, K_NO_WAIT);
    return my_tid;
}
/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void template_thread_stall_assert(void)
{
    __ASSERT(0, "template thread stalled");
}

static void template_thread_entry_point(void *, void *, void *)
{
    k_timer_start(&template_timer, K_SECONDS(5), K_SECONDS(3));
    while (1)
    {
        template_thread_do_something();
        im_alive_cb(thread_idx, k_uptime_get_32(), 1000);
        k_msleep(1000);
    }
}

static void template_thread_do_something(void)
{
    // printk("%s\n", __func__);
    // LOG_INF("info");
    // LOG_ERR("error");
    // LOG_DBG("debug");
}

// EVENT STUFF
void log_template_event(const struct app_event_header *aeh)
{
    struct template_event *event = cast_template_event(aeh);

    APP_EVENT_MANAGER_LOG(aeh,
                          "s8 = %d, "
                          "s16 = %d, "
                          "s32 = %d, "
                          "u8 = %d, "
                          "u16 = %d, "
                          "u32 = %d, "
                          "s = %s, "
                          "t = %lld",
                          event->i8,
                          event->i16,
                          event->i32,
                          event->u8,
                          event->u16,
                          event->u32,
                          event->s,
                          event->ts);
}

static void profile_template_event(struct log_event_buf *buf,
                                   const struct app_event_header *aeh)
{
    struct template_event *event = cast_template_event(aeh);

    ARG_UNUSED(event);
    nrf_profiler_log_encode_int8(buf, event->i8);
    nrf_profiler_log_encode_int16(buf, event->i16);
    nrf_profiler_log_encode_int32(buf, event->i32);
    nrf_profiler_log_encode_uint8(buf, event->u8);
    nrf_profiler_log_encode_uint16(buf, event->u16);
    nrf_profiler_log_encode_uint32(buf, event->u32);
    nrf_profiler_log_encode_string(buf, event->s);
    // how to encode an int64 timestamp?
    // nrf_profiler_log_encode_int64(buf, event->ts);
}
APP_EVENT_INFO_DEFINE(template_event,
                      ENCODE(
                          NRF_PROFILER_ARG_S8,
                          NRF_PROFILER_ARG_S16,
                          NRF_PROFILER_ARG_S32,
                          NRF_PROFILER_ARG_U8,
                          NRF_PROFILER_ARG_U16,
                          NRF_PROFILER_ARG_U32,
                          NRF_PROFILER_ARG_STRING,
                          NRF_PROFILER_ARG_TIMESTAMP),
                      ENCODE(
                          "s8",
                          "s16",
                          "s32",
                          "u8",
                          "u16",
                          "u32",
                          "s",
                          "t"),
                      profile_template_event);

APP_EVENT_TYPE_DEFINE(template_event,
                      log_template_event,
                      &template_event_info,
                      APP_EVENT_FLAGS_CREATE());
// INIT_LOG_ENABLE flag will call log print at every event fire, or you can call manually
// APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));

// template shell commands
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

// example of a simple shell subcommand
static int cmd_template_ping(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "pong");
    shell_fprintf(shell, SHELL_INFO, "Print info message\n");
    shell_print(shell, "Print simple text.");
    shell_warn(shell, "Print warning text.");
    shell_error(shell, "Print error text.");
    return 0;
}

// example of how to get params from shell
static int cmd_template_params(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "argc = %d", argc);
    for (size_t cnt = 0; cnt < argc; cnt++)
    {
        shell_print(shell, "  argv[%d] = %s", cnt, argv[cnt]);
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_template,
                               SHELL_CMD(params, NULL, "Print params command.", cmd_template_params),
                               SHELL_CMD(ping, NULL, "Ping command.", cmd_template_ping),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(template, &sub_template, "Template commands", NULL);

