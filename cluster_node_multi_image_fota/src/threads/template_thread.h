#ifndef __TEMPLATE_THREAD_H__
#define __TEMPLATE_THREAD_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

/***********************************
This module is an example of a noridic app event producer as well as a free-standing running thread.
Call template_thread_start() to start a thread that can do something
Call log_template_event() to print event members.
In addition to defining event data structure below, see APP_EVENT_INFO_DEFINE
and APP_EVENT_TYPE_DEFINE macros at end of C module for example requirements of how to define an event.
Use these macros in your client to subscribe to event
    APP_EVENT_LISTENER(MODULE, your_callback_event_handler)
    APP_EVENT_SUBSCRIBE(MODULE, template_event);

put this in your project CMakeList.txt
# Include application application event headers
zephyr_library_include_directories(src/events)

put this in your prj.conf file:
# Configuration required by Application Event Manager
CONFIG_APP_EVENT_MANAGER=y

***********************************/

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t template_thread_start(void(*cb), int idx);
/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void template_thread_stall_assert(void);

// EVENT DEFINITION
void log_template_event(const struct app_event_header *aeh);

// data in a template event
struct template_event
{
    struct app_event_header header;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    const char *s; // string
    int64_t ts;    // timestamp
};

APP_EVENT_TYPE_DECLARE(template_event);

#endif // __TEMPLATE_THREAD_H__