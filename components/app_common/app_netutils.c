#include "esp_log.h"
#include "app_netutils.h"


// Log tag
static const char *TAG = "app_netutils";


/**
 * @brief Extracts the port number from a sockaddr_storage structure (IPv4 or IPv6).
 * 
 * @param port Output port number
 * @param addr Input sockaddr_storage pointer
 * @return esp_err_t 
 *  ESP_OK on success,
 *  ESP_ERR_INVALID_STATE if ss_family is not a recognised address family,
 *  ESP_ERR_INVALID_ARG if an input argument is NULL
 */
esp_err_t get_port_from_sockaddr_storage(uint16_t *port, struct sockaddr_storage *addr) {
    if (!port || !addr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (IS_IPV4(addr)) {
        *port = ntohs(SOCKIN4(addr)->sin_port);
        return ESP_OK;
    }
#if CONFIG_LWIP_IPV6
    else if (IS_IPV6(addr)) {
        *port = ntohs(SOCKIN6(addr)->sin6_port);
        return ESP_OK;
    }
#endif

    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief Sets the port number in a sockaddr_storage structure (IPv4 or IPv6).
 *        The address family is determined from addr->ss_family.
 * 
 * @param port Port number
 * @param addr In/out sockaddr_storage pointer
 * @return esp_err_t 
 *  ESP_OK on success,
 *  ESP_ERR_INVALID_STATE if ss_family is not a recognised address family,
 *  ESP_ERR_INVALID_ARG if addr is NULL
 */
esp_err_t set_port_in_sockaddr_storage(uint16_t port, struct sockaddr_storage *addr) {
    if (!addr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (IS_IPV4(addr)) {
        SOCKIN4(addr)->sin_port = htons(port);
        return ESP_OK;
    }
#if CONFIG_LWIP_IPV6
    else if (IS_IPV6(addr)) {
        SOCKIN6(addr)->sin6_port = htons(port);
        return ESP_OK;
    }
#endif
    else {
        ESP_LOGE(TAG, "Invalid address family: %d", addr->ss_family);
    }

    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief Resolves an IP string and port into a sockaddr_storage structure.
 *        Supports both IPv4 and IPv6.
 * @param ip IP address string
 * @param port Port number
 * @param out_addr Output address structure pointer
 * @param out_len Output address length pointer (may be NULL)
 * @return ESP_OK on success, other values indicate a parse failure
 */
esp_err_t get_sockaddr_storage_from_string(const char *ip, uint16_t port, struct sockaddr_storage *out_addr, socklen_t *out_len) {
    if (!ip || ip[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST; // Disable DNS; parse numeric IPs only for speed and unambiguity

    int err = getaddrinfo(ip, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "Failed to resolve %s (%d)", ip, err);
        return ESP_ERR_INVALID_ARG;
    }

    // Guard against overflow
    if (res->ai_addrlen > sizeof(struct sockaddr_storage)) {
        freeaddrinfo(res);
        return ESP_ERR_NO_MEM;
    }

    // Copy the resolved address into the output parameter
    memcpy(out_addr, res->ai_addr, res->ai_addrlen);

    // Only write the length when the caller requests it
    if (out_len) {
        *out_len = res->ai_addrlen;
    }

    // Apply the port (getaddrinfo was called with NULL port, so set it explicitly)
    if (set_port_in_sockaddr_storage(port, out_addr) != ESP_OK) {
        freeaddrinfo(res);
        return ESP_ERR_INVALID_ARG;
    }

    freeaddrinfo(res);
    return ESP_OK;
}

/**
 * @brief Gets the IP address string from a sockaddr_storage structure (IPv4 or IPv6).
 * 
 * @param addr Input sockaddr_storage pointer
 * @param ip_buf Output buffer for the IP address string
 * @param buf_len Buffer length
 * @return esp_err_t 
 *  ESP_OK on success,
 *  ESP_ERR_INVALID_STATE if ss_family is not a recognised address family,
 *  ESP_ERR_INVALID_ARG if an input argument is invalid
 */
esp_err_t get_ipstr_from_sockaddr_storage(const struct sockaddr_storage *addr, char *ip_buf, size_t buf_len) {
    if (!addr || !ip_buf || buf_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (IS_IPV4(addr)) {
        ip4addr_ntoa_r((const ip4_addr_t *)&SOCKIN4(addr)->sin_addr, ip_buf, buf_len);
        return ESP_OK;
    }
#if CONFIG_LWIP_IPV6
    else if (IS_IPV6(addr)) {
        ip6addr_ntoa_r((const ip6_addr_t *)&SOCKIN6(addr)->sin6_addr, ip_buf, buf_len);
        return ESP_OK;
    }
#endif

    ip_buf[0] = '\0'; // No valid address is set
    return ESP_ERR_INVALID_STATE;
}
