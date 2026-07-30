# Register component
idf_component_register(SRCS "app_ble_spp.c" INCLUDE_DIRS "." REQUIRES nvs_flash bt app_nvs_rw)