# Register component
idf_component_register(SRCS "app_tcp_server.c" INCLUDE_DIRS "." REQUIRES app_common app_nvs_rw)