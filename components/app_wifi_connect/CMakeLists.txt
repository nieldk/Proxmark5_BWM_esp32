# Register component
idf_component_register(SRCS "app_wifi_connect.c" INCLUDE_DIRS "." REQUIRES nvs_flash esp_wifi app_common app_nvs_rw)