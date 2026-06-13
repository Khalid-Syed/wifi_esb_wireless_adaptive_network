/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "version.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/stats/stats.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>
#include <zephyr/mgmt/mcumgr/transport/smp_udp.h>

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif

#define MODULE mcu_manager

LOG_MODULE_REGISTER(MODULE, CONFIG_MCUMGR_LOG_LEVEL);

#define EVENT_MASK (NET_EVENT_L4_CONNECTED |    \
                    NET_EVENT_L4_DISCONNECTED | \
                    NET_EVENT_DNS_SERVER_ADD |  \
                    NET_EVENT_DNS_SERVER_DEL)

#define STORAGE_PARTITION_LABEL storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(uptime_s)
STATS_SECT_END;

/* Assign a name to the `uptime` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, uptime_s)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats)
smp_svr_stats;

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &cstorage,
    .storage_dev = (void *)STORAGE_PARTITION_ID,
    .mnt_point = "/lfs1"};
#endif
#define MCUMGR_SERVICE_TIMER_MS 1000

static void (*im_alive_cb)(int, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;                     // index to pass back to im_alive callback

/* ktimer func to */
void mcumgr_service_timer_func(struct k_timer *dummy)
{
    im_alive_cb(thread_idx, k_uptime_get_32(), MCUMGR_SERVICE_TIMER_MS);
    STATS_INC(smp_svr_stats, uptime_s);
}
K_TIMER_DEFINE(mcumgr_service_timer, mcumgr_service_timer_func, NULL);

/* start timer and init callbacks that start and stop smp server */
int mcumgr_service_init(void(*cb), int idx)
{
    im_alive_cb = cb;
    thread_idx = idx;

    /* initialize server stats */
    int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
                                "smp_svr_stats");
    if (rc < 0)
    {
        LOG_ERR("Error initializing stats system [%d]", rc);
        return rc;
    }

    /* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
    rc = fs_mount(&littlefs_mnt);
    if (rc < 0)
    {
        LOG_ERR("Error mounting littlefs [%d]", rc);
        return rc;
    }
#endif
    /* in example code, add callback events for network management */
    /* this can be done automatically with CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT=y*/
    //    net_mgmt_init_event_callback(&mgmt_cb, mcumgr_service_event_handler, EVENT_MASK);
    //    net_mgmt_add_event_callback(&mgmt_cb);
    //    net_conn_mgr_resend_status();

    // NOTE THE AUTOMATIC INIT IS NOT WORKING
    // putting a call  to smp_udp_open() when NET_EVENT_IPV4_DHCP_BOUND event is thrown
    // in the wifi_thread.c event handler does successfully start the smp server
    // TODO: get to bottom of this - submit ticket...

    /*start timer to run svr stats*/
    k_timer_start(&mcumgr_service_timer, K_MSEC(MCUMGR_SERVICE_TIMER_MS), K_MSEC(MCUMGR_SERVICE_TIMER_MS));
    return 0;
}
/**
 * @brief call this when service is stalled or otherwise not healthy
 *
 */
void mcumgr_service_stall_assert(void)
{
    ASSERT(0);
}

/* shell commands */
#include <zephyr/shell/shell.h>

static int cmd_mcumgr_service_status(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "mcumgr service status: TODO: query smp udp thread status");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mcumgr,
                               SHELL_CMD(status, NULL, "mcumgr status.", cmd_mcumgr_service_status),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mcumgr, &sub_mcumgr, "mcumgr service commands", NULL);
