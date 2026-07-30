# Register component
idf_component_register(SRCS "app_mqtt_client.c" INCLUDE_DIRS "." REQUIRES mqtt app_nvs_rw)