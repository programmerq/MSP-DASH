#pragma once
#include <stdint.h>
#include "esp_wifi.h"
typedef struct { uint8_t *src_addr; uint8_t *des_addr; } esp_now_recv_info_t;
typedef struct { uint8_t peer_addr[6]; uint8_t channel; int ifidx; bool encrypt; } esp_now_peer_info_t;
typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *, const uint8_t *, int);
inline int esp_now_init(void) { return 0; }
inline int esp_now_register_recv_cb(esp_now_recv_cb_t) { return 0; }
inline int esp_now_add_peer(const esp_now_peer_info_t *) { return 0; }
typedef int esp_err_t;
#define ESP_ERR_ESPNOW_NOT_FOUND 0x306a
inline int esp_now_send(const uint8_t *, const uint8_t *, size_t) { return 0; }
