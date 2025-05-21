#pragma once

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/tls_credentials.h>

int cert_provision(void);
// void send_http_request(char *location_str, char *api_str, size_t api_str_len);
void send_http_request(char *location_str, size_t location_str_len, char *api_str,
		       size_t api_str_len, char *auth_token);