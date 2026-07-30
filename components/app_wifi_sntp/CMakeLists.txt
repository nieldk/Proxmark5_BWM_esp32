# Register component
idf_component_register(SRCS "app_wifi_sntp.c" INCLUDE_DIRS "." REQUIRES lwip app_common app_nvs_rw)