#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include "wifi_thread.h"
#include "power_events.h"
#include "../events/wifi_events.h"
#include "cfg.h"
#include "wifi_stats_pub.h"

// Umar: Added Time sync header file
#include "time_sync.h" 

#if 1 // CONFIG_WIFI

// logging
#define MODULE wifi_thread
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_WIFI_THREAD_LOG_LEVEL);

#ifndef WIFI_THREAD_STACK_SIZE
#define WIFI_THREAD_STACK_SIZE 4096
#endif
#ifndef WIFI_THREAD_PRIORITY
#define WIFI_THREAD_PRIORITY 5
#endif
K_THREAD_STACK_DEFINE(wifi_thread_stack_area, WIFI_THREAD_STACK_SIZE);

struct k_thread wifi_thread_data;

static void enter_shutdown_mode(void);
static void exit_shutdown_mode(void);
static void wifi_thread_entry_point(void *, void *, void *);
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint32_t mgmt_event, struct net_if *iface);
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);
static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);
static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event, struct net_if *iface);
static void print_dhcp_ip(struct net_mgmt_event_callback *cb);
static int wifi_connect(void);
// static int wifi_disconnect(void);
static int log_wifi_status(void);

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
                                NET_EVENT_WIFI_DISCONNECT_RESULT)

#define MAX_SSID_LEN 32
#define DHCP_TIMEOUT 70
#define CONNECTION_TIMEOUT_MS 30000
#define STATUS_POLLING_MS 3000
#define WIFI_THREAD_TIMEOUT_SLEEP_MS 3000
#define WIFI_NO_NETWORK_SLEEP_MS 60000 // 1 minute
#define WIFI_THREAD_CONNECTED_SLEEP_MS 10000
#define WIFI_THREAD_SHUTDOWN_SLEEP_MS 60000 // 1 minute

// For TWT
#define STATUS_POLLING_TWT_MS 300
#define TWT_RESP_TIMEOUT_S 20

#define MAX_FAILED_CONNECT_COUNT 10

static void (*im_alive_cb)(int, uint32_t, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;                               // index to pass back to im_alive callback
static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback net_shell_mgmt_cb;
static struct
{
    const struct shell *sh;
    union
    {
        struct
        {
            uint8_t connected : 1;
            uint8_t connect_result : 1;
            uint8_t disconnect_requested : 1;
            uint8_t _unused : 5;
        };
        uint8_t all;
    };
} context;

static bool use_twt = false;
static volatile bool is_wifi_shutdown = false;

static bool twt_supported, twt_resp_received, twt_resp_accept;
static uint32_t twt_flow_id = 1;

bool fetch_server_time_success = false;

static bool wait_for_twt_resp_received(void)
{
	int i, timeout_polls = (TWT_RESP_TIMEOUT_S * 1000) / STATUS_POLLING_TWT_MS;

	for (i = 0; i < timeout_polls; i++) {
        im_alive_cb(thread_idx, k_uptime_get_32(), STATUS_POLLING_TWT_MS);
		k_sleep(K_MSEC(STATUS_POLLING_TWT_MS));
		if (twt_resp_received) {
			return twt_resp_accept;
		}
	}

	return false;
}

static int setup_twt(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int ret;

	twt_flow_id = 1;
	params.operation = WIFI_TWT_SETUP;
	params.negotiation_type = WIFI_TWT_INDIVIDUAL;
	params.setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
	params.dialog_token = 1;
	params.flow_id = twt_flow_id;
	params.setup.responder = 0;
	params.setup.trigger = IS_ENABLED(CONFIG_TWT_TRIGGER_ENABLE);
	params.setup.implicit = 1;
	params.setup.announce = IS_ENABLED(CONFIG_TWT_ANNOUNCED_MODE);
	params.setup.twt_wake_interval = 65024;
	params.setup.twt_interval = 2524000;

	ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
	if (ret) {
		LOG_INF("TWT setup failed: %d", ret);
		return ret;
	}

	LOG_INF("TWT setup requested");

	return 0;
}

static int teardown_twt(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int ret;

	params.operation = WIFI_TWT_TEARDOWN;
	params.negotiation_type = WIFI_TWT_INDIVIDUAL;
	params.setup_cmd = WIFI_TWT_TEARDOWN;
	params.dialog_token = 1;
	params.flow_id = twt_flow_id;

	ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
	if (ret) {
		LOG_ERR("%s with %s failed, reason : %s",
			wifi_twt_operation_txt(params.operation),
			wifi_twt_negotiation_type_txt(params.negotiation_type),
			wifi_twt_get_err_code_str(params.fail_reason));
		return ret;
	}

	LOG_INF("TWT teardown success");

	return 0;
}

k_tid_t wifi_thread_start(void(*cb), int idx)
{
    im_alive_cb = cb;
    thread_idx = idx;

    int boot_wait_seconds = CONFIG_HELOGEN_WIFI_BOOT_DELAY;
    k_tid_t my_tid = k_thread_create(&wifi_thread_data, wifi_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(wifi_thread_stack_area),
                                     wifi_thread_entry_point,
                                     NULL, NULL, NULL,
                                     WIFI_THREAD_PRIORITY, 0, K_SECONDS(boot_wait_seconds));
    return my_tid;
}

/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void wifi_thread_stall_assert(void)
{
    __ASSERT(0, "wifi thread stalled");
}

static void wifi_thread_entry_point(void *, void *, void *)
{
    LOG_DBG("%s ", __func__);

    LOG_DBG("Static IP address (overridable): %s/%s -> %s",
            CONFIG_NET_CONFIG_MY_IPV4_ADDR,
            CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
            CONFIG_NET_CONFIG_MY_IPV4_GW);

    memset(&context, 0, sizeof(context));

    net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
                                 wifi_mgmt_event_handler,
                                 WIFI_SHELL_MGMT_EVENTS);

    net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

    net_mgmt_init_event_callback(&net_shell_mgmt_cb,
                                 net_mgmt_event_handler,
                                 NET_EVENT_IPV4_DHCP_BOUND);

    net_mgmt_add_event_callback(&net_shell_mgmt_cb);

    bool failed_connect = false;
    int failed_connect_count = 0;

    // TODO: put this in a zephyr FSM instead of a while loop
    while (1)
    {
        if (is_wifi_shutdown)
        {
            im_alive_cb(thread_idx, k_uptime_get_32(), WIFI_THREAD_SHUTDOWN_SLEEP_MS);
            k_sleep(K_MSEC(WIFI_THREAD_SHUTDOWN_SLEEP_MS));
            LOG_DBG("Sleeping cause in shutdown");
        }
        else if (!context.connected)
        {
            context.connect_result = false;
            
            im_alive_cb(thread_idx, k_uptime_get_32(), CONNECTION_TIMEOUT_MS);
            LOG_DBG("Call wifi_connect");
            wifi_connect();

            // wait for connection result callback or timeout
            for (int i = 0; i < CONNECTION_TIMEOUT_MS; i += STATUS_POLLING_MS) // spin here while waiting for connection
            {
                im_alive_cb(thread_idx, k_uptime_get_32(), CONNECTION_TIMEOUT_MS);

                k_sleep(K_MSEC(STATUS_POLLING_MS));
                log_wifi_status();          // print status
                if (context.connect_result) // connection result received
                {
                    LOG_DBG("Connection Result Received");
                    context.connect_result = false;
                    break;
                }
            }
            // did we connect successfully?
            if (context.connected) // yay!
            {
                // Sadman: Deactivate TCP-based Time Sync
                // if (fetch_time_from_server() == 0) {
                //     fetch_server_time_success = true;
                // }

                // Allow 60 seconds for MQTT to connect
                im_alive_cb(thread_idx, k_uptime_get_32(), STATUS_POLLING_MS * 20);
                k_sleep(K_MSEC(STATUS_POLLING_MS * 20));

                im_alive_cb(thread_idx, k_uptime_get_32(), STATUS_POLLING_MS);
                k_sleep(K_MSEC(STATUS_POLLING_MS));
                failed_connect = false;
                failed_connect_count = 0;
            }
            else if (!context.connect_result) // boo! failed to connect
            {
                LOG_ERR("%s Wi-Fi Connection Timed Out", __func__);
                im_alive_cb(thread_idx, k_uptime_get_32(), WIFI_THREAD_TIMEOUT_SLEEP_MS);
                k_sleep(K_MSEC(WIFI_THREAD_TIMEOUT_SLEEP_MS));

                failed_connect_count++;

                // Multiple failed connects, sleep for 1 minute (old: 10 minutes)
                if (failed_connect_count > MAX_FAILED_CONNECT_COUNT)
                {
                    failed_connect_count = 0;
                    LOG_ERR("%s Start Long Sleep, Wi-Fi Connection Timed Out", __func__);
                    enter_shutdown_mode();
                    im_alive_cb(thread_idx, k_uptime_get_32(), WIFI_NO_NETWORK_SLEEP_MS);
                    k_sleep(K_MSEC(WIFI_NO_NETWORK_SLEEP_MS));
                    exit_shutdown_mode();
                }
            }
        }
        else
        {
            // LOG_DBG("sleeping for %d mseconds", WIFI_THREAD_CONNECTED_SLEEP_MS);
            im_alive_cb(thread_idx, k_uptime_get_32(), WIFI_THREAD_CONNECTED_SLEEP_MS);
            k_sleep(K_MSEC(WIFI_THREAD_CONNECTED_SLEEP_MS));
        }
    }
}

static int log_wifi_status(void)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status status = {0};

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                 sizeof(struct wifi_iface_status)))
    {
        LOG_ERR("%s Status request failed", __func__);
        return -ENOEXEC;
    }

    LOG_DBG("%s Wi-Fi Connection State: %s", __func__, wifi_state_txt(status.state));
    if (status.state >= WIFI_STATE_ASSOCIATED)
    {

        LOG_DBG("%s Interface Mode: %s", __func__,
                wifi_mode_txt(status.iface_mode));
        LOG_DBG("%s Link Mode: %s", __func__,
                wifi_link_mode_txt(status.link_mode));
        LOG_DBG("%s SSID: %-32s", __func__, status.ssid);
        LOG_DBG("%s Band: %s", __func__, wifi_band_txt(status.band));
        LOG_DBG("%s Channel: %d", __func__, status.channel);
        LOG_DBG("%s Security: %s", __func__, wifi_security_txt(status.security));
        LOG_DBG("%s MFP: %s", __func__, wifi_mfp_txt(status.mfp));
        LOG_DBG("%s RSSI: %d", __func__, status.rssi);

        if (status.twt_capable) {
			twt_supported = 1;
		}
    }
    return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status =
        (const struct wifi_status *)cb->info;

    if (status->status)
    {
        LOG_ERR("Connection request failed (%d)", status->status);
        context.connected = false;
        wifi_status_event_throw(WIFI_STATUS_EVENT_CONNECT_FAILED);
    }
    else
    {
        LOG_INF("Wi-Fi Connected");
        context.connected = true;
        wifi_status_event_throw(WIFI_STATUS_EVENT_CONNECTED);
    }

    log_wifi_status();
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status =
        (const struct wifi_status *)cb->info;

    if (context.disconnect_requested)
    {
        LOG_DBG("%s Disconnection request %s (%d)",
                status->status ? "failed" : "done",
                __func__, status->status);
        wifi_status_event_throw(WIFI_STATUS_EVENT_DISCONNECT_FAILED);
    }
    else
    {
        LOG_DBG("Wi-Fi Disconnected");
        wifi_status_event_throw(WIFI_STATUS_EVENT_DISCONNECTED);
        context.connected = false;
    }

    log_wifi_status();
}

static void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
			     enum wifi_twt_negotiation_type negotiation_type,
			     bool responder, bool implicit, bool announce,
			     bool trigger, uint32_t twt_wake_interval,
			     uint64_t twt_interval)
{
	LOG_INF("TWT Dialog token: %d",
	      dialog_token);
	LOG_INF("TWT flow ID: %d",
	      flow_id);
	LOG_INF("TWT negotiation type: %s",
	      wifi_twt_negotiation_type_txt(negotiation_type));
	LOG_INF("TWT responder: %s",
	       responder ? "true" : "false");
	LOG_INF("TWT implicit: %s",
	      implicit ? "true" : "false");
	LOG_INF("TWT announce: %s",
	      announce ? "true" : "false");
	LOG_INF("TWT trigger: %s",
	      trigger ? "true" : "false");
	LOG_INF("TWT wake interval: %d us",
	      twt_wake_interval);
	LOG_INF("TWT interval: %lld us",
	      twt_interval);
	LOG_INF("========================");
}

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp =
		(const struct wifi_twt_params *)cb->info;

	if (resp->operation == WIFI_TWT_TEARDOWN) {
		LOG_INF("TWT teardown received for flow ID %d\n", resp->flow_id);
		return;
	}

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		LOG_INF("TWT response: %s",
		      wifi_twt_setup_cmd_txt(resp->setup_cmd));

		if (resp->setup_cmd == WIFI_TWT_SETUP_CMD_ACCEPT) {
			twt_resp_accept = true;
		}

		twt_resp_received = true;
		twt_flow_id = resp->flow_id;

		LOG_INF("== TWT negotiated parameters ==");
		print_twt_params(resp->dialog_token,
				 resp->flow_id,
				 resp->negotiation_type,
				 resp->setup.responder,
				 resp->setup.implicit,
				 resp->setup.announce,
				 resp->setup.trigger,
				 resp->setup.twt_wake_interval,
				 resp->setup.twt_interval);
	} else {
		LOG_INF("TWT response timed out\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        handle_wifi_connect_result(cb);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        handle_wifi_disconnect_result(cb);
        break;
    case NET_EVENT_WIFI_IFACE_STATUS:
        LOG_DBG("NET_EVENT_WIFI_IFACE_STATUS");
        break;
    case NET_EVENT_WIFI_TWT:
        handle_wifi_twt_event(cb);
        LOG_DBG("NET_EVENT_WIFI_TWT");
        break;
    case NET_EVENT_WIFI_SCAN_DONE:
        LOG_DBG("NET_EVENT_WIFI_SCAN_DONE");
        break;
    case NET_EVENT_WIFI_SCAN_RESULT:
        LOG_DBG("NET_EVENT_WIFI_SCAN_RESULT");
        break;

    default:
        break;
    }
    context.connect_result = true;
}

static void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
    /* Get DHCP info from struct net_if_dhcpv4 and print */
    const struct net_if_dhcpv4 *dhcpv4 = cb->info;
    const struct in_addr *addr = &dhcpv4->requested_ip;
    char dhcp_info[128];

    net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

    LOG_DBG("%s IP address: %s", __func__, dhcp_info);
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_IPV4_DHCP_BOUND:
        LOG_DBG("%s DHCP bound", __func__);
        print_dhcp_ip(cb);
        wifi_status_event_throw(WIFI_STATUS_EVENT_IP_ACQUIRED);
#ifdef CONFIG_MCUMGR
        if (smp_udp_open() < 0) // TODO: why does this have to be done here? - the smp auto feature should work
        {
            LOG_ERR("could not open smp udp");
        }
#endif
        break;
    case NET_EVENT_IPV4_ADDR_ADD:
        LOG_DBG("IPv4 address added");
        break;
    case NET_EVENT_IPV4_ADDR_DEL:
        LOG_DBG("IPv4 address removed");
        break;
    case NET_EVENT_IPV4_MADDR_ADD:
        LOG_DBG("IPv4 multicast address added");
        break;
    case NET_EVENT_IPV4_MADDR_DEL:
        LOG_DBG("IPv4 multicast address removed");
        break;
    case NET_EVENT_IPV4_ROUTER_ADD:
        LOG_DBG("IPv4 router added");
        break;
    case NET_EVENT_IPV4_ROUTER_DEL:
        LOG_DBG("IPv4 router removed");
        break;
    case NET_EVENT_IPV4_DHCP_START:
        LOG_DBG("IPv4 DHCP started");
        break;
    case NET_EVENT_IPV4_DHCP_STOP:
        LOG_DBG("IPv4 DHCP stopped");
        break;
    case NET_EVENT_IPV4_MCAST_JOIN:
        LOG_DBG("IPv4 multicast join");
        break;
    case NET_EVENT_IPV4_MCAST_LEAVE:
        LOG_DBG("IPv4 multicast leave");
        break;
    default:
        break;
    }
}

static int __wifi_args_to_params(struct wifi_connect_req_params *params)
{
    params->timeout = CONNECTION_TIMEOUT_MS; // SYS_FOREVER_MS;

    /* SSID */
    // params->ssid = CONFIG_HELIOGEN_SSID_24GHZ;
    // params->ssid = CONFIG_HELIOGEN_SSID_5GHZ;
    params->ssid = cfg_get_wifi_ssid();
    params->ssid_length = strlen(params->ssid);
#if !defined(CONFIG_STA_KEY_MGMT_NONE)
    // params->psk = CONFIG_HELIOGEN_PASSWORD;
    params->psk = cfg_get_wifi_psk();
    params->psk_length = strlen(params->psk);
#endif
    if (params->psk_length != 0)
    {
        params->security = WIFI_SECURITY_TYPE_SAE; // WPA3
        // params->security = WIFI_SECURITY_TYPE_PSK; // WPA2
    }
    else
    {
        params->security = WIFI_SECURITY_TYPE_NONE;
    }
    params->channel = WIFI_CHANNEL_ANY;

    /* MFP (optional) */
    params->mfp = WIFI_MFP_OPTIONAL;

    return 0;
}

static int wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();
    static struct wifi_connect_req_params cnx_params = {0};

    context.connected = false;
    context.connect_result = false;

    __wifi_args_to_params(&cnx_params);
    LOG_INF("%s", __func__);
    LOG_INF("Connecting to %s", cnx_params.ssid);
    LOG_INF("Security: %s", wifi_security_txt(cnx_params.security));
    LOG_INF("Channel: %d", cnx_params.channel);
    LOG_INF("MFP: %s", wifi_mfp_txt(cnx_params.mfp));
    LOG_INF("Timeout: %d", cnx_params.timeout);
    LOG_INF("PSK: %s", cnx_params.psk);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                 &cnx_params, sizeof(struct wifi_connect_req_params)))
    {
        LOG_ERR("%s Connection request failed", __func__);
        context.connect_result = true; // there won't be a callback
        return -ENOEXEC;
    }

    LOG_DBG("%s Connection requested", __func__);

    return 0;
}

static int wifi_disconnect(void)
{
    struct net_if *iface = net_if_get_default();
    int status;

    status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

    LOG_ERR("Wifi disconnect status: %d", status);

    return 0;
}

int shutdown_wifi(struct net_if *iface)
{
	int ret;

	if (!net_if_is_admin_up(iface)) {
		return 0;
	}

	ret = net_if_down(iface);
	if (ret) {
		LOG_ERR("Cannot bring down iface (%d)", ret);
		return ret;
	}

	LOG_ERR("Interface down");

	return 0;
}

int startup_wifi(struct net_if *iface)
{
	int ret;

    ret = net_if_up(iface);
    if (ret) {
        LOG_ERR("Cannot bring up iface (%d)", ret);
        return ret;
    }

    LOG_ERR("Interface up");

    // Sadman: Deactivate TCP-based Time Sync
    // if (!fetch_server_time_success) {
    //     if (fetch_time_from_server() == 0) {
    //         fetch_server_time_success = true;
    //     }
    // }

	return 0;
}

void enter_shutdown_mode(void)
{

    wifi_disconnect();

    k_sleep(K_MSEC(STATUS_POLLING_MS));

	struct net_if *iface = net_if_get_default();

	shutdown_wifi(iface);

    context.connected = false;
}

void exit_shutdown_mode(void)
{
	struct net_if *iface = net_if_get_default();

	startup_wifi(iface);
}

/**
 * @brief wifi thread event handler
 *
 * @param aeh
 * @return true
 * @return false
 */
static bool wifi_thread_event_handler(const struct app_event_header *aeh)
{
    if (is_power_status_event(aeh))
    {
        struct power_status_event *ps = cast_power_status_event(aeh);

        if (ps->power_state == WIFI_TURN_OFF)
        {
            // attempt to shutdown wifi interface
            is_wifi_shutdown = true;
            LOG_ERR("Wifi shutdown event recevied from power thread");
            enter_shutdown_mode();
        }
        else if (ps->power_state == WIFI_TURN_ON)
        {
            // attempt to bring up wifi interface
            is_wifi_shutdown = false;
            LOG_ERR("Wifi turn on event recevied from power thread");
            exit_shutdown_mode();
        }
        else if (ps->power_state == WIFI_TWT_TEARDOWN_CMD)
        {
            LOG_ERR("Wifi TWT teardown received from HTTP update thread");
            teardown_twt();
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, wifi_thread_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, power_status_event);

#endif // CONFIG_WIFI
