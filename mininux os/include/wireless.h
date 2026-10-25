#ifndef WIRELESS_H
#define WIRELESS_H

#include <stdint.h>

#define WIRELESS_SSID_MAX_LENGTH 32u
#define WIRELESS_PASSPHRASE_MAX_LENGTH 64u
#define WIRELESS_BSSID_LENGTH 6u

typedef enum {
    WIRELESS_RESULT_OK = 0,
    WIRELESS_RESULT_UNAVAILABLE,
    WIRELESS_RESULT_INVALID_ARGUMENT,
    WIRELESS_RESULT_FAILED
} wireless_result_t;

typedef enum {
    WIRELESS_STATE_UNAVAILABLE = 0,
    WIRELESS_STATE_DISCONNECTED,
    WIRELESS_STATE_SCANNING,
    WIRELESS_STATE_CONNECTING,
    WIRELESS_STATE_CONNECTED
} wireless_state_t;

typedef enum {
    WIRELESS_SECURITY_OPEN = 0,
    WIRELESS_SECURITY_WPA2,
    WIRELESS_SECURITY_WPA3,
    WIRELESS_SECURITY_UNKNOWN
} wireless_security_t;

typedef struct {
    uint8_t ssid[WIRELESS_SSID_MAX_LENGTH];
    uint8_t ssid_length;
    uint8_t bssid[WIRELESS_BSSID_LENGTH];
    int8_t signal_dbm;
    wireless_security_t security;
} wireless_network_t;

typedef void (*wireless_scan_callback_t)(
    const wireless_network_t *network,
    void *user_data
);

typedef struct {
    wireless_result_t (*initialize)(void *context);
    wireless_result_t (*scan)(
        void *context,
        wireless_scan_callback_t callback,
        void *user_data
    );
    wireless_result_t (*connect)(
        void *context,
        const uint8_t *ssid,
        uint8_t ssid_length,
        const uint8_t *passphrase,
        uint8_t passphrase_length
    );
    wireless_result_t (*disconnect)(void *context);
    wireless_state_t (*get_state)(void *context);
    void *context;
} wireless_driver_ops_t;

#endif
