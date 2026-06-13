#ifndef _TIME_SYNC_H_
#define _TIME_SYNC_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <net/mqtt_helper.h>
#include <math.h>
#include <time.h>
#include <zephyr/net/sntp.h>

// Payload to receive time from server
typedef struct {
	uint8_t type;
	uint8_t board_id;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
	uint8_t  minute;
    uint8_t  second;
    uint32_t usec;
} TimePacket;

// Macros for getting time via internet
#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123
#define NTP_UNIX_EPOCH_DIFF 2208988800UL

// Macros for Central Server that hosts the time server
#define TIME_SERVER_IP  "10.30.224.10"
#define TIME_SERVER_PORT 12530

// Extern declarations of global variables
extern struct timespec ts;
extern struct tm tm_val;
extern TimePacket pck;

// Function declarations
int fetch_time_from_server(void);
TimePacket get_current_time(void);
char* get_current_time_string();

#endif // _TIME_SYNC_H_
