# # Only add the net_ipc child image from the main application,
# # not from child images like mcuboot that also process modules.
# if(NOT IMAGE_NAME OR IMAGE_NAME STREQUAL "app")
#   if(BOARD STREQUAL "heliogen_g5_node_nrf5340_cpuapp" OR
#      BOARD STREQUAL "heliogen_g5_node_nrf5340_cpuapp_ns")

#     # Inject PCD config into MCUboot for network core update support
#     add_overlay_config(
#       mcuboot
#       "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf"
#     )
#     # Optional: inject secure_boot.conf overlay for net core (B0n)
#     # add_overlay_config(
#     #   empty_net_core
#     #   "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/secure_boot.conf"
#     # )

#     add_child_image(
#       NAME empty_net_core
#       SOURCE_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/net_ipc
#       DOMAIN CPUNET
#       BOARD heliogen_g5_node_nrf5340_cpunet
#     )
#   endif()
# endif()

# Zephyr module build hook (see zephyr/module.yml).
# This file is evaluated for every image in a multi-image build.
# Only add the CPUNET child image when building the main CPUAPP application.

get_filename_component(G5_NODE_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)

# In the main app build, APPLICATION_SOURCE_DIR == <this repo root>.
# In mcuboot / other child images it will be something else.
if (APPLICATION_SOURCE_DIR STREQUAL "${G5_NODE_MODULE_DIR}")
  if (BOARD STREQUAL "heliogen_g5_node_nrf5340_cpuapp")

    # # Inject PCD config into MCUboot for network core update support
    # add_overlay_config(
    #   mcuboot
    #   "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf"
    # )

    # # Optional: inject secure_boot.conf overlay for net core (B0n)
    # add_overlay_config(
    #   net_ipc
    #   "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
    # )

    add_child_image(
      NAME       net_ipc
      SOURCE_DIR ${G5_NODE_MODULE_DIR}/net_ipc
      DOMAIN     CPUNET
      BOARD      heliogen_g5_node_nrf5340_cpunet
    )

    # # Inject PCD config into MCUboot for network core update support
    # add_overlay_config(
    #   mcuboot
    #   "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf"
    # )

    # # Optional: inject secure_boot.conf overlay for net core (B0n)
    # add_overlay_config(
    #   net_ipc
    #   "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/secure_boot.conf"
    # )

  endif()
endif()

