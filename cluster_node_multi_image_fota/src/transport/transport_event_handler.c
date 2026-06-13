/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "wifi_events.h"
#include "transport_priv.h"

/* Register log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

/********************/
/* private functions*/
/********************/

static bool transport_wifi_event_handler(const struct app_event_header *aeh)
{
	LOG_DBG("%s", __func__);

	if (is_power_status_event(aeh))
    {
        struct power_status_event *ps = cast_power_status_event(aeh);

        if (ps->power_state == 0)
        {
            // attempt to shutdown mqtt interface
			// transport_mqtt_active_set(false);
            LOG_ERR("mqtt shutdown event recevied from power thread");
        }
        else if (ps->power_state == 1)
        {
            // attempt to bring up mqtt interface
            // transport_mqtt_active_set(true);
            LOG_ERR("mqtt turn on event recevied from power thread");
        }
        return false;
    }
	else if (is_wifi_status_event(aeh))
	{
		struct wifi_status_event *le = cast_wifi_status_event(aeh);
		// print event
		log_wifi_status_event(aeh);
		LOG_DBG("wifi status event: %s", le->status);
		// TODO: dont use strcmp
		if (!strcmp(le->status, WIFI_STATUS_EVENT_IP_ACQUIRED))
		{
			transport_obj_network_event_set(NETWORK_CONNECTED_WITH_IP);
		}
		else if (!strcmp(le->status, WIFI_STATUS_EVENT_DISCONNECTED))
		{
			transport_obj_network_event_set(NETWORK_DISCONNECTED);
		}
		else if (!strcmp(le->status, WIFI_STATUS_EVENT_CONNECTED))
		{
#if CONFIG_NET_DHCPV4
			transport_obj_network_event_set(NETWORK_DISCONNECTED);
			// do nothing, wait for IP address
#else // not using DHCP, so we must have a static ip and should be good to go
#warning "not using using DHCP"
			transport_obj_network_event_set(NETWORK_CONNECTED_WITH_IP);
#endif
		}
		else
		{
			// all of these cases should be treated as "disconnected"
			/*WIFI_STATUS_EVENT_CONNECTING "connecting"*/
			/*WIFI_STATUS_EVENT_CONNECT_FAILED "connect failed"*/
			/*WIFI_STATUS_EVENT_DISCONNECTING "disconnecting"*/
			/*WIFI_STATUS_EVENT_DISCONNECT_FAILED "disconnect failed"*/
			transport_obj_network_event_set(NETWORK_DISCONNECTED);

			LOG_ERR("unknown wifi status event: %s", le->status);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);
	return false;
}

// subscribe to wi-fi app events
APP_EVENT_LISTENER(MODULE, transport_wifi_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, wifi_status_event);
APP_EVENT_SUBSCRIBE(MODULE, power_status_event);
