# Register component
idf_component_register(SRCS "app_udp_client.c" INCLUDE_DIRS "." REQUIRES app_common app_nvs_rw)