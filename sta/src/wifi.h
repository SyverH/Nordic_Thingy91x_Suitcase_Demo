#pragma once

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <net/wifi_mgmt_ext.h>
#include <net/wifi_ready.h>
#include <stdlib.h>

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
#define STATUS_POLLING_MS      300

void wifi_sta_set_wifi_connected_cb(void *cb);
int start_app(void);
k_tid_t wifi_sta_get_start_wifi_thread_id(void);
void net_mgmt_callback_init(void);
int register_wifi_ready(void);

int wifi_scan(void);
