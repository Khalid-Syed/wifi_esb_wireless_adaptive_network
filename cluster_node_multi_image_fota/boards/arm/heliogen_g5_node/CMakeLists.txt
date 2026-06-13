# Copyright (c) 2022 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if ((CONFIG_BOARD_HELIOGEN_G5_NODE_CPUAPP OR CONFIG_BOARD_HELIOGEN_G5_NODE_CPUAPP_NS)
    AND CONFIG_BOARD_ENABLE_CPUNET)
zephyr_library()
zephyr_library_sources(nrf5340_cpunet_reset.c)
endif()
