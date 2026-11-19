# Register component
# app_com_defs.h comes from main; app_cmd_uart.h from app_uart_cmd. Both are
# .c-only uses (not re-exported via app_zb_probe.h), so PRIV_REQUIRES.
idf_component_register(SRCS "app_zb_probe.c" INCLUDE_DIRS "." REQUIRES bt app_common
                        PRIV_REQUIRES main app_uart_cmd)
