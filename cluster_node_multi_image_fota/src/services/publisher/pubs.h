#ifndef __PUBS_H__
#define __PUBS_H__
#include <zephyr/kernel.h>

// pubs module
// a scheduler for publishing mqtt messages at a regular interval with jitter
// and a minimum interval between publishes

// in your module create a delayable work item that uses the pubs_work_handler as the cb
// example:
//           K_WORK_DELAYABLE_DEFINE(your_pub_dwork, pubs_work_handler);
// and in your header file, include the extern declaration so that pubs.c can reference it
//          extern struct k_work_delayable mppt_pub_dwork;

#define PUBS_MIN_INTERVAL_MS 250 // 0.25 second minimum interval between publishes

#if (CONFIG_MQTT_HELPER_HELIOGEN)
void pubs_init(void);

// pubs_start and pubs_stop are normally called internally by the pubs module
// transport_event_handler, but can be called directly if needed
void pubs_start(void);

void pubs_start_ncla(void);
void pubs_start_cla(void);

void pubs_stop(void);

void pubs_stop_ncla(void);
void pubs_stop_cla(void);

// use this callback as the work_handler for your publishing module and it
// will call your publish callback and schedule your next publish event
void pubs_work_handler(struct k_work *work);

// optional - call this to manually schedule your next publish event
// call it with work item from your delayable work item, e.g.,  &your_dwork.work
int pub_schedule(struct k_work *work, int ms);
// use this to schedule a publish event for a topic string that is in the table
// returns -1 if topic not found
int pub_schedule_topic(char *topic, int ms);

#else // CONFIG_MQTT_HELPER_HELIOGEN

// stubs for when mqtt helper is not enabled
__attribute__((weak)) int pubs_init(void) { return 0; }
__attribute__((weak)) int pubs_start(void) { return 0; }
__attribute__((weak)) int pubs_stop(void) { return 0; }

__attribute__((weak)) int pubs_start_ncla(void) { return 0; }
__attribute__((weak)) int pubs_start_cla(void) { return 0; }
__attribute__((weak)) int pubs_stop_ncla(void) { return 0; }
__attribute__((weak)) int pubs_stop_cla(void) { return 0; }

__attribute__((weak)) void pubs_work_handler(struct k_work *work) { return 0; }
__attribute__((weak)) int pub_schedule(struct k_work *work, int ms) { return 0; }
__attribute__((weak)) int pub_schedule_topic(char *topic, int ms) { return 0; };
#endif // CONFIG_MQTT_HELPER_HELIOGEN

#endif // __PUBS_H__