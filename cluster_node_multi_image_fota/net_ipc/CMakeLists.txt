#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Define configuration files.
list(APPEND CONF_FILE
	${CMAKE_CURRENT_SOURCE_DIR}/prj.conf
)

# set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pm_static.yml)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(net_ipc)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boards)

target_sources(app PRIVATE
	src/main.c
)
