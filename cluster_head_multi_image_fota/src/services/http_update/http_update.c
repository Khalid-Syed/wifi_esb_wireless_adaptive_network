
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <dfu/dfu_target_mcuboot.h>
#include <net/fota_download.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/reboot.h>
#include <app_event_manager.h>
#include "http_update.h"
#include "ota_dfu_events.h"
#include "power_events.h"
// #include "ota_dfu_json.h"
#include "ota_dfu_mqtt.h"
#include "string_utils.h"
#include "app_version.h"
// #include "transport.h"
#include "storage_counter.h"

// FOTA: NET CORE
#include <pm_config.h> 
#include <zephyr/storage/flash_map.h>
#define IMAGE_MAGIC 0x96f3b83d
#define NETCORE_SHARED_BUF_ADDR ((uint8_t *)PM_RPMSG_NRF53_SRAM_ADDRESS)
#define NETCORE_SHARED_BUF_SIZE (PM_RPMSG_NRF53_SRAM_SIZE)
// FOTA: NET CORE

#define MODULE http_update
LOG_MODULE_REGISTER(MODULE, CONFIG_HTTP_UPDATE_LOG_LEVEL);

/////////////////////// NET CORE FOTA ///////////////////////
/*
 * Determine the OTA target based on the filename.
 *	-1: error
 *	 0: app-core
 *	 1: net-core
 */
static int ota_target_from_file(const char *file)
{
    if (file == NULL) {
        return -1;
    }

	/* anything starting with "app_" is for app-core */
    if (strncmp(file, "app_", 4) == 0) {
        return 0;
    }

    /* anything starting with "net_" is for net-core */
    if (strncmp(file, "net_", 4) == 0) {
        return 1;
    }

    /* fallback */
    return -1;
}

struct __packed image_version {
    uint8_t  major;
    uint8_t  minor;
    uint16_t revision;
    uint32_t build_num;
};

struct __packed image_header {
    uint32_t magic;
    uint32_t load_addr;
    uint16_t hdr_size;
    uint16_t protect_tlv_size;
    uint32_t img_size;
    uint32_t flags;
    struct image_version ver;
    uint32_t pad1;
};

static int ota_target = -1;

static int apply_netcore_update_from_mcuboot_secondary(void)
{
    const struct flash_area *fa = NULL;
    struct image_header hdr;
    int ret;
	int flash_area_erase_fail_safe = 0;

    ret = flash_area_open(PM_MCUBOOT_SECONDARY_ID, &fa);
    if (ret) {
        LOG_ERR("flash_area_open(mcuboot_secondary) failed: %d", ret);
        return ret;
    }

    ret = flash_area_read(fa, 0, &hdr, sizeof(hdr));
    if (ret) {
        LOG_ERR("flash_area_read header failed: %d", ret);
		flash_area_erase_fail_safe = 1;
        goto out_close;
    }

	if (hdr.hdr_size == 0 || hdr.img_size == 0) {
        ret = -EINVAL;
        LOG_ERR("Invalid header (hdr_size=%u img_size=%u)", hdr.hdr_size, hdr.img_size);
		flash_area_erase_fail_safe = 1;
        goto out_close;
    }

    if (hdr.img_size > NETCORE_SHARED_BUF_SIZE) {
        ret = -ENOMEM;
        LOG_ERR("NET payload too big for RPMSG SRAM (%u > %u)", hdr.img_size, NETCORE_SHARED_BUF_SIZE);
		flash_area_erase_fail_safe = 1;
        goto out_close;
    }

    uint8_t *shared_buf = NETCORE_SHARED_BUF_ADDR;

	// Read payload (skip MCUboot header)
    ret = flash_area_read(fa, hdr.hdr_size, shared_buf, hdr.img_size);
    if (ret) {
        LOG_ERR("Read payload failed: %d", ret);
		flash_area_erase_fail_safe = 1;
        goto out_close;
    }

    // Optional sanity check (vector table)
    uint32_t sp = ((uint32_t *)shared_buf)[0];
    uint32_t reset = ((uint32_t *)shared_buf)[1];
    LOG_INF("NET img: size=%u sp=0x%08x reset=0x%08x", hdr.img_size, sp, reset);

    ret = pcd_network_core_update(shared_buf, hdr.img_size);
    if (ret) {
        LOG_ERR("pcd_network_core_update failed: %d", ret);
		flash_area_erase_fail_safe = 1;
        goto out_close;
    }

    // IMPORTANT: dfu_target_mcuboot will have marked image-0 upgrade.
    // Erase secondary slot so CPUAPP MCUboot won't try to swap app core on reboot.
    ret = flash_area_erase(fa, 0, fa->fa_size);
    if (ret) {
        LOG_ERR("Erase secondary failed: %d", ret);
        goto out_close;
    }

    LOG_INF("NET update applied. Rebooting whole device...");
    sys_reboot(SYS_REBOOT_COLD);

out_close:
	if (flash_area_erase_fail_safe == 1) 
	{
		// Flash area erase failed, handle fail-safe
		LOG_ERR("Entering \"Flash area erase\" fail-safe mode");
		// IMPORTANT: dfu_target_mcuboot will have marked image-0 upgrade.
		// Erase secondary slot so CPUAPP MCUboot won't try to swap app core on reboot.
		ret = flash_area_erase(fa, 0, fa->fa_size);
		if (ret) {
			LOG_ERR("Erase secondary failed: %d", ret);
			flash_area_close(fa);
			return ret;
		}
		flash_area_close(fa);
		return 1;
	}
	else
	{
		flash_area_close(fa);
		return ret;
	}
}
/////////////////////// NET CORE FOTA ///////////////////////


static void http_update_install(void);

static bool download_in_progress = false;
static int session_id = 0;
static int progress = 0;
char install_mode[32] = "now";
char status_str[32] = "idle";

/**
 * @brief send update status to broker
 *
 * @param session_id
 * @param status_str
 * @param progress
 */
void http_update_status_send(void)
{
	ota_dfu_status_t status;
	status.current_fwv = app_version_get();
	status.session_id = session_id;
	status.progress = progress;
	status.status = status_str;
	ota_dfu_status_mqtt_publish(&status);
}

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	static int progress = 0;
	switch (evt->id)
	{
	/** FOTA download error. */
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("Received error %d from fota_download", evt->cause);
		sprintf(status_str, "error: %d", evt->cause);
		progress = evt->progress;
		http_update_status_send();
		http_update_stop();
		break;
	/** FOTA download finished. */
	case FOTA_DOWNLOAD_EVT_FINISHED:
		sprintf(status_str, "complete");
		progress = evt->progress;
		// LOG_INF("Apply: NET core (PCD)");
		// int err = apply_netcore_update_from_mcuboot_secondary();
		// if (err) {
		// 	LOG_ERR("NET core apply failed: %d", err);
		// 	strcpy(status_str, "netcore apply failed");
		// } else {
		// 	strcpy(status_str, "netcore updated");
		// }
		// http_update_status_send();
		switch (ota_target) 
		{
			case 0:
				LOG_INF("Apply: APP core (MCUBoot image-0)");
				http_update_status_send();
				http_update_done();
				LOG_INF("Press 'Reset' button or enter 'reset' to apply new firmware\n");
				break;
			case 1:
				LOG_INF("Apply: NET core (PCD)");
				int err = apply_netcore_update_from_mcuboot_secondary();
				if (err) 
				{
					LOG_ERR("NET core apply failed: %d", err);
					strcpy(status_str, "netcore apply failed");
				} 
				else 
				{
					LOG_INF("NET core apply succeeded");
					strcpy(status_str, "netcore updated");
				}
				http_update_status_send();
				break;
			default:
				LOG_ERR("Unknown OTA target: %d", ota_target);
				strcpy(status_str, "unknown OTA target");
				http_update_status_send();
				break;
		}
		// if (ota_target == 0) 
		// {
		// 	LOG_INF("Apply: APP core (MCUBoot image-0)");
		// 	http_update_status_send();
		// 	http_update_done();
		// 	LOG_INF("Press 'Reset' button or enter 'reset' to apply new app core firmware\n");
		// }
		// else 
		// {
		// 	LOG_INF("Apply: NET core (PCD)");
        //     int err = apply_netcore_update_from_mcuboot_secondary();
        //     if (err) {
        //         LOG_ERR("NET core apply failed: %d", err);
        //         strcpy(status_str, "netcore apply failed");
        //     } else {
        //         strcpy(status_str, "netcore updated");
        //     }
        //     http_update_status_send();
		// 	LOG_INF("Press 'Reset' button or enter 'reset' to apply new net core firmware\n");
		// }
		break;
	/** FOTA download progress report. */
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		if (evt->progress != progress)
		{
			progress = evt->progress;
			LOG_INF("Ota progress %d%%", evt->progress);
			if (evt->progress % 10 == 0)
			{
				sprintf(status_str, "downloading %d%%", evt->progress);
				progress = evt->progress;
				http_update_status_send();
			}
		}
		break;
	/** FOTA download erase pending. */
	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		LOG_INF("OTA Erase Pending");
		break;
	/** FOTA download erase done. */
	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		LOG_INF("OTA Erase Done");
		break;
	/** FOTA download cancelled. */
	case FOTA_DOWNLOAD_EVT_CANCELLED:
		LOG_INF("OTA Cancelled");
		sprintf(status_str, "cancelled %d%%", evt->progress);
		progress = evt->progress;
		http_update_status_send();
		download_in_progress = false;
		break;
	default:
		LOG_ERR("OTA Event %d Unhandled", evt->id);
		break;
	}
}
void http_update_start(int session_id, char *host, char *filename, char *install, int retries)
{
	int err;

	if (download_in_progress == true)
	{
		/* Cancel any ongoing DFU operation */
		http_update_stop();
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0)
	{
		LOG_ERR("fota_download_init() failed, err %d\n", err);
		sprintf(status_str, "init failed %d", err);
		progress = 0;
		http_update_status_send();
		return;
	}

	err = fota_download_start(host, filename, SEC_TAG, 0, 0);
	if (err != 0)
	{
		LOG_INF("fota_download_start() failed, err %d\n", err);
		sprintf(status_str, "start failed %d", err);
		progress = 0;
		http_update_status_send();
		http_update_stop();
	}
	else
	{
		session_id = session_id;
		safe_strlcpy(install_mode, install, sizeof(install_mode));
		download_in_progress = true;
	}
}

void http_update_stop(void)
{
	LOG_DBG("Firmware update aborted");
	fota_download_cancel();
	download_in_progress = false;
	k_sleep(K_MSEC(1000));
}
void http_update_done(void)
{
	LOG_INF("Firmware download done");
	download_in_progress = false;
	if (strcmp(install_mode, "now") == 0)
	{
		LOG_INF("Installing firmware now");
		http_update_install();
	}
	else
	{
		LOG_INF("Firmware will be installed on next reboot");
	}
}
/**
 * @brief reboot device to install new firmware
 *
 */
static void http_update_install(void)
{
	strcpy(status_str, "installing");
	progress = 0;
	http_update_status_send();

	LOG_INF("Device will now reboot in:");

	LOG_INF("3... ");
	k_sleep(K_MSEC(1000));
	LOG_INF("2...");
	k_sleep(K_MSEC(1000));
	LOG_INF("1...");
	k_sleep(K_MSEC(2000));
	sys_reboot(SYS_REBOOT_WARM);
}
/**
 * @brief called when first connected to mqtt broker, often after a reboot
 *
 */
// void http_update_connected(void)
//{
//	// publish current fwv and http ota_dfu status
//	int session_id = 0;
//	char *status_str = "idle";
//	int progress = 0;
//	http_update_status_send(status_str, progress);
// }

static bool http_update_event_handler(const struct app_event_header *aeh)
{
	if (is_ota_dfu_cmd_event(aeh))
	{
		struct ota_dfu_cmd_event *se = cast_ota_dfu_cmd_event(aeh);
		LOG_INF("RX ota_dfu_cmd evt: session_id: %d uri: %s file: %s install: %s retries: %d",
				se->session_id, se->uri, se->file, se->install, se->retries);
		if (strcmp(se->install, "stop") == 0)
		{
			http_update_stop();
			return false;
		}

		/////////// NET CORE FOTA //////////
		ota_target = ota_target_from_file(se->file);
		/////////// NET CORE FOTA //////////

		// check to make sure you do not re-download the same file if session id is the same as last time

		int prev_session_id = 0;
		int result;
		int rc = storage_counter_get(HTTP_UPDATE_SESSION_ID_FILENAME, &prev_session_id);
		if (rc < 0)
		{
			LOG_ERR("http update session id storage get failed: %d", rc);
		}
		// Sadman's note: disabling duplicate session id check
		// else if (prev_session_id == se->session_id)
		// {
		// 	LOG_INF("Dupe http update session_id, ignoring request");
		// 	return false;
		// }

		if (download_in_progress)
		{
			LOG_INF("Download in progress, stop it first");
			http_update_stop();
		}

		// start new download
		http_update_start(se->session_id, se->uri, se->file, se->install, se->retries);

		// save session id to storage
		rc = storage_counter_set(HTTP_UPDATE_SESSION_ID_FILENAME, se->session_id, &result);
		if (rc < 0)
		{
			LOG_ERR("FAIL: set http update session counter %d", rc);
		}
		else
		{
			LOG_INF("set http update session counter %d", result);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, http_update_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ota_dfu_cmd_event);
