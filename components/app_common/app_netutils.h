#ifndef APP_NETUTILS_H_
#define APP_NETUTILS_H_

#include "esp_err.h"
#include <lwip/netdb.h>

#if CONFIG_LWIP_IPV6
#define IS_IPV6(addr) ((addr)->ss_family == AF_INET6)
#define SOCKIN6(addr) ((struct sockaddr_in6 *)(addr))
#endif
#define IS_IPV4(addr) ((addr)->ss_family == AF_INET)
#define SOCKIN4(addr) ((struct sockaddr_in *)(addr))

esp_err_t get_sockaddr_storage_from_string(const char *ip, uint16_t port, struct sockaddr_storage *out_addr, socklen_t *out_len);
esp_err_t get_port_from_sockaddr_storage(uint16_t *port, struct sockaddr_storage *addr);
esp_err_t set_port_in_sockaddr_storage(uint16_t port, struct sockaddr_storage *addr);
esp_err_t get_ipstr_from_sockaddr_storage(const struct sockaddr_storage *addr, char *ip_buf, size_t buf_len);

#endif /* APP_NETUTILS_H_ */
