# Declare source files
set (srcs
    "app_netutils.c"
    "app_rtos_task.c"
)

# Register component
idf_component_register(SRCS ${srcs} INCLUDE_DIRS "." REQUIRES esp_netif esp_timer)