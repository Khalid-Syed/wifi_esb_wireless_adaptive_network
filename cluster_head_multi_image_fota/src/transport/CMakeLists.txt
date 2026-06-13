
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/
    transport.c
    transport_client_id.c
    transport_event_handler.c
    transport_fsm.c
    transport_mqtt_callbacks.c
    transport_queue.c
    transport_timer.c
    mqtt_helper_heliogen.c
    transport_stats.c
)

# Make the folder that contains the CA certificate for "test.mosquitto.org" global so that it can
# be located by the MQTT helper library / local credential provision library.
#
# The MQTT helper library provisions the certificate
# to the Mbed TLS stack for Native Posix and nRF7002 DK builds.
#
# The credential provision library provisions the certificate
# to the modem's network stack for nRF9160 builds.
#
# CONFIG_MODEM_KEY_MGMT and CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES are mutually exclusive.
#
zephyr_include_directories_ifdef(CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES credentials)
