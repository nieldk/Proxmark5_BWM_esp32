#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdint.h>
#include "esp_err.h"

#define MQTT_ALPN_COUNT_MAX             10                  // Maximum number of ALPN protocols allowed; must not exceed 255
#define MQTT_HOST_MAX_LEN               128                 // Maximum length of the MQTT broker host address, excluding the terminating '\0'
#define MQTT_PATH_MAX_LEN               256                 // Maximum length of the MQTT broker path, excluding the terminating '\0'
#define MQTT_SUBSCRIBE_TOPIC_MAX_LEN    128                 // Maximum length of the MQTT subscribe topic, excluding the terminating '\0'
#define MQTT_PUBLISH_TOPIC_MAX_LEN      128                 // Maximum length of the MQTT publish topic, excluding the terminating '\0'
#define MQTT_CLIENT_ID_MAX_LEN          256                 // Maximum length of the MQTT client ID, excluding the terminating '\0'
#define MQTT_USERNAME_MAX_LEN           1024                // Maximum length of the MQTT username, excluding the terminating '\0'
#define MQTT_PASSWORD_MAX_LEN           1024                // Maximum length of the MQTT password, excluding the terminating '\0'
#define MQTT_LWT_TOPIC_MAX_LEN          128                 // Maximum length of the MQTT Last Will topic, excluding the terminating '\0'
#define MQTT_LWT_MESSAGE_MAX_LEN        128                 // Maximum length of the MQTT Last Will message, excluding the terminating '\0'
#define MQTT_ALPN_STR_MAX_LEN           64                  // Maximum length of an MQTT ALPN protocol string, excluding the terminating '\0'
#define MQTT_SNI_HOST_MAX_LEN           256                 // Maximum length of the MQTT SNI hostname, excluding the terminating '\0'
#define MQTT_CACERT_MAX_LEN             MAX_PAYLOAD_LEN     // Maximum length of the MQTT CA certificate, excluding the terminating '\0'
#define MQTT_CCERT_MAX_LEN              MAX_PAYLOAD_LEN     // Maximum length of the MQTT client certificate, excluding the terminating '\0'
#define MQTT_CCKEY_MAX_LEN              MAX_PAYLOAD_LEN     // Maximum length of the MQTT client private key, excluding the terminating '\0'
#define MQTT_PSK_KEY_MAX_LEN            128                 // Maximum length of the MQTT pre-shared key
#define MQTT_PSK_HINT_MAX_LEN           64                  // Maximum length of the MQTT pre-shared key hint, excluding the terminating '\0'

typedef enum {
    /*
     *  0: MQTT over TCP
     *  1: MQTT over TLS (no server certificate verification)
     *  2: MQTT over TLS (verify server certificate)
     *  3: MQTT over TLS (provide client certificate)
     *  4: MQTT over TLS (verify server certificate and provide client certificate)
     *  5: MQTT over TLS (PSK — pre-shared key)
     *  6: MQTT over WebSocket (TCP-based)
     *  7: MQTT over WebSocket Secure (TLS, no server certificate verification)
     *  8: MQTT over WebSocket Secure (TLS, verify server certificate)
     *  9: MQTT over WebSocket Secure (TLS, provide client certificate)
     * 10: MQTT over WebSocket Secure (TLS, verify server certificate and provide client certificate)
     * 11: MQTT over WebSocket Secure (TLS, PSK — pre-shared key)
     */
    MQTT_SCHEME_TCP                                     = 0,
    MQTT_SCHEME_TLS_NO_VERIFY                           = 1,
    MQTT_SCHEME_TLS_VERIFY_SERVER                       = 2,
    MQTT_SCHEME_TLS_CLIENT_CERT                         = 3,
    MQTT_SCHEME_TLS_VERIFY_SERVER_CLIENT_CERT           = 4,
    MQTT_SCHEME_TLS_PSK                                 = 5,
    MQTT_SCHEME_WEBSOCKET                               = 6,
    MQTT_SCHEME_WEBSOCKET_TLS_NO_VERIFY                 = 7,
    MQTT_SCHEME_WEBSOCKET_TLS_VERIFY_SERVER             = 8,
    MQTT_SCHEME_WEBSOCKET_TLS_CLIENT_CERT               = 9,
    MQTT_SCHEME_WEBSOCKET_TLS_VERIFY_SERVER_CLIENT_CERT = 10,
    MQTT_SCHEME_WEBSOCKET_TLS_PSK                       = 11,
} mqtt_scheme_t;

// Data receive callback
typedef void (*app_mqtt_client_rx_callback_t)(uint8_t *data, uint16_t length);


esp_err_t app_mqtt_init(void);
esp_err_t app_mqtt_deinit(void);
esp_err_t app_mqtt_start(void);
esp_err_t app_mqtt_stop(void);

esp_err_t app_mqtt_send(uint8_t *data, size_t length);
esp_err_t app_mqtt_set_rx_callback(app_mqtt_client_rx_callback_t callback);
esp_err_t app_mqtt_get_state(uint8_t *state);

esp_err_t app_mqtt_set_host(const char *host);
esp_err_t app_mqtt_get_host(char **host);
esp_err_t app_mqtt_set_port(uint16_t port);
esp_err_t app_mqtt_get_port(uint16_t *port);
esp_err_t app_mqtt_set_path(const char *path);
esp_err_t app_mqtt_get_path(char **path);
esp_err_t app_mqtt_set_scheme(uint8_t scheme);
esp_err_t app_mqtt_get_scheme(uint8_t *scheme);

esp_err_t app_mqtt_set_subscribe_topic(const char *subscribe_topic);
esp_err_t app_mqtt_get_subscribe_topic(char **subscribe_topic);
esp_err_t app_mqtt_set_subscribe_qos(uint8_t subscribe_qos);
esp_err_t app_mqtt_get_subscribe_qos(uint8_t *subscribe_qos);
esp_err_t app_mqtt_set_publish_topic(const char *publish_topic);
esp_err_t app_mqtt_get_publish_topic(char **publish_topic);
esp_err_t app_mqtt_set_publish_qos(uint8_t publish_qos);
esp_err_t app_mqtt_get_publish_qos(uint8_t *publish_qos);
esp_err_t app_mqtt_set_publish_retain(uint8_t publish_retain);
esp_err_t app_mqtt_get_publish_retain(uint8_t *publish_retain);

esp_err_t app_mqtt_set_client_id(const char *client_id);
esp_err_t app_mqtt_get_client_id(char **client_id);
esp_err_t app_mqtt_set_username(const char *username);
esp_err_t app_mqtt_get_username(char **username);
esp_err_t app_mqtt_set_password(const char *password);
esp_err_t app_mqtt_get_password(char **password);

esp_err_t app_mqtt_set_keepalive(int keepalive);
esp_err_t app_mqtt_get_keepalive(int *keepalive);
esp_err_t app_mqtt_set_disable_clean_session(uint8_t disable_clean_session);
esp_err_t app_mqtt_get_disable_clean_session(uint8_t *disable_clean_session);

esp_err_t app_mqtt_set_lwt_topic(const char *lwt_topic);
esp_err_t app_mqtt_get_lwt_topic(char **lwt_topic);
esp_err_t app_mqtt_set_lwt_message(const char *lwt_message);
esp_err_t app_mqtt_get_lwt_message(char **lwt_message);
esp_err_t app_mqtt_set_lwt_qos(uint8_t lwt_qos);
esp_err_t app_mqtt_get_lwt_qos(uint8_t *lwt_qos);
esp_err_t app_mqtt_set_lwt_retain(uint8_t lwt_retain);
esp_err_t app_mqtt_get_lwt_retain(uint8_t *lwt_retain);

esp_err_t app_mqtt_alpn_add(const char *alpn);
esp_err_t app_mqtt_alpn_get_count(uint8_t *alpn_count);
esp_err_t app_mqtt_alpn_get_list(char ***alpn_list, uint8_t *alpn_count);
esp_err_t app_mqtt_alpn_remove(uint8_t index);
esp_err_t app_mqtt_alpn_clear(void);

esp_err_t app_mqtt_set_sni_host(const char *sni_host);
esp_err_t app_mqtt_get_sni_host(char **sni_host);

esp_err_t app_mqtt_set_cacert(const char *cacert);
esp_err_t app_mqtt_get_cacert(char **cacert);
esp_err_t app_mqtt_set_ccert(const char *ccert);
esp_err_t app_mqtt_get_ccert(char **ccert);
esp_err_t app_mqtt_set_cckey(const char *cckey);
esp_err_t app_mqtt_get_cckey(char **cckey);

esp_err_t app_mqtt_set_psk_key(const uint8_t *key, size_t key_size);
esp_err_t app_mqtt_get_psk_key(uint8_t **key, size_t *key_size);
esp_err_t app_mqtt_set_psk_hint(const char *hint);
esp_err_t app_mqtt_get_psk_hint(char **hint);

#endif
