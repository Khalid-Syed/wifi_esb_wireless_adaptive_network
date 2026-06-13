#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# find_package(Git)

# if(GIT_FOUND)
# 	execute_process(
# 		COMMAND git describe --tags --long
# 		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# 		OUTPUT_VARIABLE version
# 		OUTPUT_STRIP_TRAILING_WHITESPACE
# 		ERROR_STRIP_TRAILING_WHITESPACE
# 		ERROR_VARIABLE stderr
# 		RESULT_VARIABLE return_code
# 	)

# 	if(return_code)
# 		message(STATUS "git describe failed: ${stderr}; ${KERNEL_VERSION_STRING} will be used instead")
# 	elseif(CMAKE_VERBOSE_MAKEFILE)
# 		message(STATUS "git describe stderr: ${stderr}")
# 	endif()
# endif()

# if(version)
# 	string(REGEX REPLACE "^([0-9]+).*$" "\\1" version_major ${version})
# 	string(REGEX REPLACE "^[0-9]+\.([0-9]+).*$" "\\1" version_minor "${version}")
# 	string(REGEX REPLACE "^[0-9]+\.[0-9]+\.([0-9]+).*$" "\\1" version_patch "${version}")
# 	string(REGEX REPLACE "^[0-9]+\.[0-9]+\.[0-9]+-([0-9]+)-.*$" "\\1" version_commit "${version}")
# 	string(REGEX REPLACE "^[0-9]+\.[0-9]+\.[0-9]+-[0-9]+-(.*)$" "\\1" version_hash "${version}")

# 	# Create version.conf
# 	file(WRITE conf/version.conf "CONFIG_APP_VERSION_MAJOR=${version_major}\n")
# 	file(APPEND conf/version.conf "CONFIG_APP_VERSION_MINOR=${version_minor}\n")
# 	file(APPEND conf/version.conf "CONFIG_APP_VERSION_PATCH=${version_patch}\n")
# 	file(APPEND conf/version.conf "CONFIG_APP_VERSION_COMMIT=${version_commit}\n")
# 	file(APPEND conf/version.conf "CONFIG_APP_VERSION_HASH=\"${version_hash}\"\n")
# 	file(APPEND conf/version.conf "CONFIG_APP_VERSION=\"${version}\"\n")
# 	file(APPEND conf/version.conf "CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"${version_major}.${version_minor}.${version_patch}+${version_commit}\"\n")

# 	message(STATUS "Building Tagged Firmware Version: ${version_major}.${version_minor}.${version_patch}-${version_commit}-${version_hash}")
# endif()

# Define configuration files.
list(APPEND CONF_FILE
	${CMAKE_CURRENT_SOURCE_DIR}/prj.conf
	# ${CMAKE_CURRENT_SOURCE_DIR}/conf/version.conf
)

# Disabled: module approach has ordering issues with MCUboot signing
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cluster_head_multi_image_fota)

# ########################## NET CORE CHILD IMAGE ##########################
# if(BOARD STREQUAL "heliogen_g5_node_nrf5340_cpuapp" OR
#    BOARD STREQUAL "heliogen_g5_node_nrf5340_cpuapp_ns")
#   # Inject PCD config into MCUboot for network core update support
#   add_overlay_config(
#     mcuboot
#     "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf"
#   )
# 	# Optional: inject secure_boot.conf overlay for net core (B0n)
# 	# add_overlay_config(
# 	#   empty_net_core
# 	#   "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/secure_boot.conf"
# 	# )
#   # Note: secure_boot.conf is NOT injected into empty_net_core.
#   # B0n is not needed — MCUboot on the app core handles net core FOTA via PCD.
#   # Adding B0n would require PM_CPUNET_B0N_CONTAINER_SIZE which causes
#   # circular build dependencies.
#   add_child_image(
#     NAME empty_net_core
#     SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/net_ipc
#     DOMAIN CPUNET
#     BOARD heliogen_g5_node_nrf5340_cpunet
#   )
# endif()
# ########################## NET CORE CHILD IMAGE ##########################

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

# Include application headers
zephyr_library_include_directories(src)
zephyr_library_include_directories(src/bsp)
zephyr_library_include_directories(src/bsp/id)
zephyr_library_include_directories(src/events)
zephyr_library_include_directories(src/hal)
zephyr_library_include_directories(src/threads)
zephyr_library_include_directories(src/transport)
zephyr_library_include_directories(src/data)
zephyr_library_include_directories(src/data/types)
zephyr_library_include_directories(src/data/json)
zephyr_library_include_directories(src/data/mqtt)
zephyr_library_include_directories(src/services)
zephyr_library_include_directories(src/cfg)
zephyr_library_include_directories(src/storage)
zephyr_library_include_directories(src/utils)
zephyr_library_include_directories(src/sys)
zephyr_library_include_directories(src/threads/manager)
zephyr_library_include_directories(src/services/http_update)
zephyr_library_include_directories(src/services/publisher)

target_sources(app PRIVATE
	src/main.c
)

# # Ensure pm_config.h is available for net core child image
# add_custom_command(
# 	TARGET app POST_BUILD
# 	COMMAND ${CMAKE_COMMAND} -E copy_if_different
# 		${CMAKE_BINARY_DIR}/include/generated/pm_config.h
# 		${CMAKE_BINARY_DIR}/empty_net_core/zephyr/include/generated/pm_config.h
# )

# threads
target_sources(app PRIVATE
	src/threads/manager/manager_thread.c
	src/threads/manager/manager_thread_monitor.c
	src/threads/manager/manager_connectivity_monitor.c
	src/threads/status_obj.c
	src/threads/power_obj.c
	# Sadman: Activate TCP-based time sync
	src/threads/time_sync.c
)

if(CONFIG_LED_THREAD)
	target_sources(app PRIVATE
		src/threads/led_thread.c
		src/threads/led_timer.c
		src/threads/led_patterns.c
		src/shell_cmds/shell_led.c
	)
endif()

if(CONFIG_WIFI)
	target_sources(app PRIVATE
		src/threads/wifi_thread.c
	)
endif()

if(CONFIG_POWER_THREAD)
	target_sources(app PRIVATE
		src/threads/power_thread.c
	)
endif()

# events
target_sources(app PRIVATE
	src/events/wifi_events.c
	src/events/ota_dfu_events.c
	src/events/transport_events.c
	src/events/power_events.c
)

# bsp
target_sources(app PRIVATE
	src/bsp/helio_spi_flash.c
	src/bsp/helio_gpio.c
	src/bsp/helio_ntc.c
	src/bsp/helio_pid.c
	src/bsp/helio_pwm.c
	src/bsp/helio_saadc.c
	src/bsp/helio_timer.c
	src/bsp/led.c
)

# Include bsp id makefile
if(CONFIG_WIFI)
	add_subdirectory(src/bsp/id)
endif()

# cfg
target_sources(app PRIVATE
	src/cfg/cfg.c
	src/cfg/cfg_defaults.c
	src/cfg/cfg_json.c
	src/cfg/cfg_shell.c
)

# data
if(CONFIG_WIFI)
	if(CONFIG_MQTT_HELPER_HELIOGEN)

		target_sources(app PRIVATE
			src/services/publisher/pubs.c
			src/services/publisher/pub_table.c

			# src/services/publisher/template_pub.c
			src/services/publisher/mppt_pub.c
			src/services/publisher/ota_dfu_pub.c
			src/services/publisher/wifi_stats_pub.c
			src/services/publisher/status_pub.c
			src/services/publisher/sys_pub.c
			# # Sadman's edit
			src/services/publisher/ncla_pub.c
			src/services/publisher/cla_pub.c

			src/data/mqtt/mqtt_common.c
			src/data/mqtt/lwt_pub_mqtt.c
			src/data/mqtt/reboot_mqtt.c
			src/data/mqtt/status_mqtt.c
			src/data/mqtt/mppt_pub_mqtt.c
			src/data/mqtt/ota_dfu_mqtt.c
			src/data/mqtt/sys_pub_mqtt.c
			src/data/mqtt/fs_sub_mqtt.c
			src/data/mqtt/wifi_stats_pub_mqtt.c
			src/data/mqtt/remote_pub_sub_mqtt.c
			# Sadman's edit
			src/data/mqtt/rtt_mqtt.c
			src/data/mqtt/control_mqtt.c
			src/data/mqtt/start_mqtt.c
			src/data/mqtt/node_id_mqtt.c
			src/data/mqtt/ncla_pub_mqtt.c
			src/data/mqtt/cla_pub_mqtt.c

			src/data/json/reboot_json.c
			src/data/json/status_json.c
			src/data/json/ota_dfu_json.c
			src/data/json/sys_pub_json.c
			src/data/json/fs_sub_json.c
			src/data/json/mppt_pub_json.c
			src/data/json/wifi_stats_pub_json.c
			src/data/json/remote_pub_sub_json.c
			# Sadman's edit
			src/data/json/rtt_json.c
			src/data/json/control_json.c
			src/data/json/start_json.c
			src/data/json/node_id_json.c
			src/data/json/ncla_pub_json.c
			src/data/json/cla_pub_json.c
		)
	else()
		target_sources(app PRIVATE
			src/data/json/status_json.c
		)
	endif()
else()
	target_sources(app PRIVATE
		src/data/json/status_json.c
	)
endif()

# services
if(CONFIG_MCUMGR)
	target_sources(app PRIVATE
		src/services/mcumgr_service.c
	)
endif()

# file system
target_sources(app PRIVATE
	src/storage/storage.c
	src/storage/storage_counter.c
	src/storage/fs_shell.c
)

if(CONFIG_STORAGE_TEST)
	target_sources(app PRIVATE
		src/storage/storage_test.c
	)
endif()

# http OTA DFU
if(CONFIG_DEBUG)
else()
	target_sources(app PRIVATE
		src/services/http_update/http_update.c
	)
endif()

if(CONFIG_MQTT_HELPER_HELIOGEN) # Include transport folder makefile#
	add_subdirectory(src/transport)
endif()

# utils
target_sources(app PRIVATE
	src/utils/string_utils.c
	src/utils/get_nearest_element_index.c
	src/utils/jitter.c
	src/utils/get_random_number.c
)

# system utils
target_sources(app PRIVATE
	src/sys/app_version.c
	src/sys/boot_count.c
	src/sys/eol_test_result.c
	src/sys/reset_cause.c
	src/sys/sys_watchdog.c
	src/sys/retained_ram.c
	src/sys/assert_post_action.c
	src/sys/fatal_error_handler.c
	src/sys/reboot_timer.c
	src/sys/reboot_timer.c
)

# shell commands
target_sources(app PRIVATE
	src/shell_cmds/shell_error.c
	src/shell_cmds/shell_version.c
	src/shell_cmds/shell_reboot.c
	src/shell_cmds/shell_boot_count.c
	src/shell_cmds/manager_shell.c
	src/shell_cmds/shell_sys.c
	src/shell_cmds/shell_dhcp.c
	src/shell_cmds/shell_mppt.c
	src/shell_cmds/shell_mqtt.c
	src/shell_cmds/shell_retained_ram.c
	src/shell_cmds/shell_stats.c
	src/shell_cmds/shell_eol.c
)

if(CONFIG_DEBUG)
else()
	target_sources(app PRIVATE
		src/shell_cmds/http_update_shell.c
	)
endif()

# # Make sure Partition Manager generated files (pm_config.h etc.) exist before the
# # CPUNET child image starts compiling. This guards against the classic
# # "pm_config.h: No such file or directory" failure when the child image is
# # added too late in the CMake configure order.
# if (TARGET partition_manager AND TARGET net_ipc_subimage)
#   add_dependencies(net_ipc_subimage partition_manager)
# endif()