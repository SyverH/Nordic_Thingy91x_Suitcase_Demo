/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi station sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(http_server, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/data/json.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>

#include <zephyr/sys/reboot.h>

#include "sensors.h"
#include "http_resources.h"
#include "wifi.h"

#include "JWT_token.h"

#define HTTPS_PORT "443"

#define POST_URL "/v1/location/wifi"

// Define a buffer large enough to hold headers and body
#define SEND_BUF_SIZE 1024
#define RECV_BUF_SIZE 2048
static char recv_buf[RECV_BUF_SIZE];

#define TLS_SEC_TAG 42

static const char cert[] = {
#include "StarfieldServicesCertG2.pem.inc"

	/* Null terminate certificate if running Mbed TLS on the application core.
	 * Required by TLS credentials API.
	 */
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))};

/* Provision certificate to modem */
int cert_provision(void)
{
	int err;

	LOG_INF("Provisioning certificate\n");

	err = tls_credential_add(TLS_SEC_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, cert, sizeof(cert));
	if (err == -EEXIST) {
		LOG_WRN("CA certificate already exists, sec tag: %d\n", TLS_SEC_TAG);
	} else if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d\n", err);
		return err;
	}

	return 0;
}

/* Setup TLS options on a given socket */
int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, CONFIG_HTTPS_HOSTNAME,
			 sizeof(CONFIG_HTTPS_HOSTNAME) - 1);
	if (err) {
		LOG_ERR("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

static void send_http_request(void)
{
	int err;
	int fd;
	int bytes;
	size_t off;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV, /* Let getaddrinfo() set port */
		.ai_socktype = SOCK_STREAM,
	};
	char peer_addr[INET6_ADDRSTRLEN];

	// // Calculate the required buffer size
	// size_t required_size = HTTP_HEAD_LEN + 10 + HTTP_HDR_END_LEN + JSON_BODY_LEN + 1;

	// // Allocate the buffer
	// char send_buf[required_size];

	LOG_INF("Looking up %s\n", CONFIG_HTTPS_HOSTNAME);

	err = getaddrinfo(CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, &hints, &res);
	if (err) {
		LOG_ERR("getaddrinfo() failed, errno %d, err %d\n", errno, err);
		return;
	}

	inet_ntop(res->ai_family, &((struct sockaddr_in *)(res->ai_addr))->sin_addr, peer_addr,
		  INET6_ADDRSTRLEN);
	LOG_INF("Resolved %s (%s)\n", peer_addr, net_family2str(res->ai_family));

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(res->ai_family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	} else {
		fd = socket(res->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (fd == -1) {
		LOG_ERR("Failed to open socket!\n");
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	LOG_INF("Connecting to %s:%d\n", CONFIG_HTTPS_HOSTNAME,
		ntohs(((struct sockaddr_in *)(res->ai_addr))->sin_port));
	err = connect(fd, res->ai_addr, res->ai_addrlen);
	if (err) {
		LOG_ERR("connect() failed, err: %d\n", errno);
		goto clean_up;
	}

	char send_buf[SEND_BUF_SIZE];

    
    extern char nrfcloud_api_str[CONFIG_WIFI_SCAN_STR_MAX_MAC_ADDR * 65];


	// Remove trailing comma of the api string
	nrfcloud_api_str[strlen(nrfcloud_api_str) - 1] = '\0';

	// Combine the JSON body string
	char json_body[1024];
	int json_body_len = snprintf(json_body, sizeof(json_body), "{\"accessPoints\":[%s]}", nrfcloud_api_str);

	// LOG_WRN("JSON body length: %d", json_body_len);


	// Formatted HTTP headers and body
	int header_len = snprintf(send_buf, sizeof(send_buf),
				  "POST %s HTTP/1.1\r\n"
				  "Host: %s:%s\r\n"
				  "Authorization: Bearer %s\r\n"
				  "Content-Type: application/json\r\n"
				  "Content-Length: %zu\r\n"
				  "Connection: close\r\n\r\n"
				  "%s",
				  POST_URL, CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, AUTH_TOKEN, json_body_len, json_body);


	// Check for truncation
	if (header_len < 0 || header_len >= sizeof(send_buf)) {
		LOG_ERR("Error: HTTP request buffer too small!\n");
		return;
	}

	// Debug: Print HTTP request content
	LOG_INF("\nHTTP Request:\n%s\n", send_buf);

	off = 0;
	do {
		bytes = send(fd, &send_buf[off], header_len - off,
			     0); // Send dynamically formatted buffer
		if (bytes < 0) {
			LOG_ERR("send() failed, err %d\n", errno);
			return;
		}
		off += bytes;
	} while (off < header_len);

	LOG_INF("Sent %d bytes\n", off);

	off = 0;
	do {
		bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 0);
		if (bytes < 0) {
			LOG_ERR("recv() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;

	} while (bytes != 0 /* peer closed connection */);

	LOG_INF("Received %d bytes\n", off);

	/* Make sure recv_buf is NULL terminated (for safe use with strstr) */
	if (off < sizeof(recv_buf)) {
		recv_buf[off] = '\0';
	} else {
		recv_buf[sizeof(recv_buf) - 1] = '\0';
	}

	// /* Print HTTP response */
	// p = strstr(recv_buf, "\r\n");
	// if (p) {
	// 	off = p - recv_buf;
	// 	recv_buf[off + 1] = '\0';
	// 	printk("\n>\t %s\n\n", recv_buf);
	// }

	// /* Print the HTTP response */
	// LOG_INF("\nHTTP Response:\n");
	// LOG_INF("%s", recv_buf);  // Print the entire buffer content

	/* Optional: Parse or process the response if needed */
	char *header_end = strstr(recv_buf, "\r\n\r\n"); // Locate the end of headers
	if (header_end) {
		LOG_INF("========================================");
		LOG_INF("\nHTTP Headers:\n");
		*header_end = '\0';        // Temporarily terminate at the end of headers
		LOG_INF("%s\n", recv_buf); // Print headers
		LOG_INF("========================================");
		LOG_INF("\nHTTP Body:\n");
		LOG_INF("%s\n", header_end + 4); // Print the body after separating headers
		LOG_INF("========================================");
	} else {
		LOG_WRN("Could not find end of headers.\n");
	}

	LOG_INF("Finished, closing socket.\n");

	char tx_buf[256];
	int ret = snprintf(tx_buf, sizeof(tx_buf), "%s", header_end + 4);
	if (ret < 0) {
		LOG_ERR("Failed to format response\n");
		goto clean_up;
	}

	http_resources_set_location(tx_buf);

	// struct ws_sensors_ctx *ctx = NULL;

	// http_resources_get_ws_ctx(&ctx);

	// int ret = websocket_send_msg(ctx->sock, tx_buf, ret, WEBSOCKET_OPCODE_DATA_TEXT, false,
	// true,
	//     SYS_FOREVER_MS);
	// if (ret < 0) {
	// LOG_INF("Couldn't send websocket msg (%d), closing connection", ret);
	// }

clean_up:
	freeaddrinfo(res);
	(void)close(fd);
}

#ifdef CONFIG_SYS_HEAP_LISTENER
#include <zephyr/sys/heap_listener.h>
extern struct sys_heap _system_heap;
struct sys_memory_stats system_heap_stats;
uint32_t system_heap_free = 0;
uint32_t system_heap_used = 0;
uint32_t system_heap_max_used = 0;

void on_system_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	if (heap_id == HEAP_ID_FROM_POINTER(&_system_heap)) {
		sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
					   &system_heap_stats);
		system_heap_used = (uint32_t)system_heap_stats.allocated_bytes;
		system_heap_max_used = (uint32_t)system_heap_stats.max_allocated_bytes;
		system_heap_free = (uint32_t)system_heap_stats.free_bytes;
		LOG_INF("system_heap ALLOC %zu. Heap state: allocated %zu, free %zu, max allocated "
			"%zu, heap size %u.\n",
			bytes, system_heap_used, system_heap_free, system_heap_max_used,
			K_HEAP_MEM_POOL_SIZE);
	}
}

void on_system_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	if (heap_id == HEAP_ID_FROM_POINTER(&_system_heap)) {
		sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
					   &system_heap_stats);
		system_heap_used = (uint32_t)system_heap_stats.allocated_bytes;
		system_heap_max_used = (uint32_t)system_heap_stats.max_allocated_bytes;
		system_heap_free = (uint32_t)system_heap_stats.free_bytes;
		LOG_INF("system_heap ALLOC %zu. Heap state: allocated %zu, free %zu, max allocated "
			"%zu, heap size %u.\n",
			bytes, system_heap_used, system_heap_free, system_heap_max_used,
			K_HEAP_MEM_POOL_SIZE);
	}
}

HEAP_LISTENER_ALLOC_DEFINE(system_heap_listener_alloc, HEAP_ID_FROM_POINTER(&_system_heap),
			   on_system_heap_alloc);
HEAP_LISTENER_FREE_DEFINE(system_heap_listener_free, HEAP_ID_FROM_POINTER(&_system_heap),
			  on_system_heap_free);
#endif // CONFIG_SYS_HEAP_LISTENER

static const struct pwm_dt_spec red_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec green_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
static const struct pwm_dt_spec blue_pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));

int pwm_set_color(int red, int green, int blue)
{
	int ret;

	LOG_DBG("PWM_Color: Red: %d, Green: %d, Blue: %d \n", red, green, blue);

	// uint32_t period = red_pwm_led.period;
	red = (red * red_pwm_led.period) / 255;
	green = (green * green_pwm_led.period) / 255;
	blue = (blue * blue_pwm_led.period) / 255;

	ret = pwm_set_pulse_dt(&red_pwm_led, red);
	if (ret != 0) {
		LOG_ERR("Error %d: red write failed\n", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&green_pwm_led, green);
	if (ret != 0) {
		LOG_ERR("Error %d: green write failed\n", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&blue_pwm_led, blue);
	if (ret != 0) {
		LOG_ERR("Error %d: blue write failed\n", ret);
		return ret;
	}

	return 0;
}

static void sensor_handler(struct k_work *work)
{
	int ret;
	static char tx_buf[1024];
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ws_sensors_ctx *ctx = CONTAINER_OF(dwork, struct ws_sensors_ctx, work);

	// ret = sensors_collect(tx_buf, sizeof(tx_buf));
	ret = sensors_get_json(tx_buf, sizeof(tx_buf));
	if (ret < 0) {
		LOG_ERR("Unable to collect sensor data, err %d", ret);
		goto unregister;
	}

	ret = websocket_send_msg(ctx->sock, tx_buf, ret, WEBSOCKET_OPCODE_DATA_TEXT, false, true,
				 SYS_FOREVER_MS);
	if (ret < 0) {
		LOG_INF("Couldn't send websocket msg (%d), closing connection", ret);
		goto unregister;
	}

	ret = k_work_reschedule(&ctx->work, K_MSEC(CONFIG_NET_SAMPLE_WEBSOCKET_SENSOR_INTERVAL));
	if (ret < 0) {
		LOG_ERR("Failed to schedule sensor work, err %d", ret);
		goto unregister;
	}

	return;

unregister:
	(void)websocket_unregister(ctx->sock);
	ctx->sock = -1;
}

int ws_sensors_init(void)
{
	struct ws_sensors_ctx *ctx = NULL;
	http_resources_get_ws_ctx(&ctx);

	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		ctx[i].sock = -1;
		k_work_init_delayable(&ctx[i].work, sensor_handler);
	}

	return 0;
}
SYS_INIT(ws_sensors_init, APPLICATION, 0);

static int get_free_sensor_slot(void)
{
	struct ws_sensors_ctx *ctx = NULL;
	http_resources_get_ws_ctx(&ctx);

	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		if (ctx[i].sock < 0) {
			return i;
		}
	}

	return -1;
}

int ws_sensors_setup(int ws_socket, void *user_data)
{
	struct ws_sensors_ctx *ctx = NULL;
	http_resources_get_ws_ctx(&ctx);

	int slot = get_free_sensor_slot();
	if (slot < 0) {
		LOG_ERR("No free slot for sensor websocket");
		return -ENOMEM;
	}
	LOG_INF("Setting up sensor websocket on slot %d", slot);

	ctx[slot].sock = ws_socket;

	LOG_INF("Using socket %d for sensor websocket", ws_socket);

	int ret = k_work_reschedule(&ctx[slot].work, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to reschedule sensor work");
		return ret;
	}

	LOG_INF("Sensor websocket setup on slot %d", slot);

	return 0;
}

static void parse_led_post(uint8_t *buf, size_t len)
{
	int ret;
	struct led_command cmd;
	const int expected_return_code = BIT_MASK(ARRAY_SIZE(led_command_descr));

	LOG_DBG("Got POST request with payload: %s", buf);

	ret = json_obj_parse(buf, len, led_command_descr, ARRAY_SIZE(led_command_descr), &cmd);
	if (ret != expected_return_code) {
		LOG_WRN("Failed to fully parse JSON payload, ret=%d", ret);
		return;
	}

	ret = pwm_set_color(cmd.r, cmd.g, cmd.b);
	if (ret) {
		LOG_ERR("Failed to set LED color");
	}
}

static int led_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data)
{
	static uint8_t post_payload_buf[32];
	static size_t cursor;

	LOG_DBG("LED handler status %d, size %zu", status, len);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		cursor = 0;
		return 0;
	}

	if (len + cursor > sizeof(post_payload_buf)) {
		cursor = 0;
		return -ENOMEM;
	}

	/* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
	 * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
	 * the client buffer).
	 */
	memcpy(post_payload_buf + cursor, buffer, len);
	cursor += len;

	if (status == HTTP_SERVER_DATA_FINAL) {
		parse_led_post(post_payload_buf, cursor);
		cursor = 0;
	}

	return 0;
}

/**
 * @brief Function called when buttons are pressed.
 */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & DK_BTN1_MSK) {
		if (button_state & DK_BTN1_MSK) {
			LOG_INF("Button 1 pressed");
			sys_reboot(SYS_REBOOT_COLD); // Reset the device
		}
	}
}

/**
 * @brief Function called when Wi-Fi is connected.
 */
static void wifi_connected_handler(void)
{
	LOG_INF("Wi-Fi connected");

	// Print Thread name
	LOG_WRN("Thread name: %s\n", k_thread_name_get(k_current_get()));

	// LOG_INF("Sending HTTP request");
	send_http_request();

	LOG_INF("HTTP server staring");
	http_server_start();

	int ret = pwm_set_color(0, 255, 0);
	if (ret) {
		LOG_ERR("Failed to set LED color");
	}
}

static int location_handler(struct http_client_ctx *client, enum http_data_status status,
			     uint8_t *buffer, size_t len, void *user_data)
{
	static bool response_sent;

    extern char location_buf[256];

	LOG_WRN("Location handler status %d, response_sent %d", status, response_sent);

	switch (status) {
	case HTTP_SERVER_DATA_ABORTED: {
		response_sent = false;
		return 0;
	}

	case HTTP_SERVER_DATA_MORE: {
		/* A payload is not expected with the GET request. Ignore any data and wait until
		 * final callback before sending response
		 */
		return 0;
	}

	case HTTP_SERVER_DATA_FINAL: {
		if (response_sent) {
			/* Response already sent, return 0 to indicate to server that the callback
			 * does not need to be called again.
			 */
			response_sent = false;
			return 0;
		}

		response_sent = true;
		return snprintf(buffer, sizeof(location_buf), "%s", location_buf);
	}
	default: {
		LOG_WRN("Unexpected status %d", status);
		return -1;
	}
	}
}

int main(void)
{
	int ret = 0;

	// ret = dk_leds_init();
	// if (ret != 0) {
	// 	LOG_ERR("Failed to initialize LEDs");
	// 	return -1;
	// }

	if (!pwm_is_ready_dt(&red_pwm_led) || !pwm_is_ready_dt(&green_pwm_led) ||
	    !pwm_is_ready_dt(&blue_pwm_led)) {
		LOG_ERR("Error: one or more PWM devices not ready\n");
		return 0;
	}

	ret = pwm_set_color(255, 0, 0);
	if (ret) {
		LOG_ERR("Failed to set LED color");
	}

	ret = dk_buttons_init(button_handler);
	if (ret != 0) {
		LOG_ERR("Failed to initialize buttons");
		return -1;
	}

	ret = sensors_init();
	if (ret) {
		LOG_ERR("Failed to initialize sensors");
		return ret;
	}

	/* Provision certificates before connecting to the network */
	ret = cert_provision();
	if (ret) {
		return 0;
	}

	http_resources_set_led_handler(led_handler);
	http_resources_set_ws_handler(ws_sensors_setup);
	wifi_sta_set_wifi_connected_cb(wifi_connected_handler);
	http_resources_set_location_handler(location_handler);

#ifdef CONFIG_SYS_HEAP_LISTENER
		heap_listener_register(&system_heap_listener_alloc);
	heap_listener_register(&system_heap_listener_free);
#endif // CONFIG_SYS_HEAP_LISTENER

	net_mgmt_callback_init();

	ret = wifi_scan();
	if (ret) {
		LOG_ERR("Failed to scan for Wi-Fi networks, err %d", ret);
		return ret;
	}

#ifdef CONFIG_WIFI_READY_LIB
	ret = register_wifi_ready();
	if (ret) {
		return ret;
	}
	k_thread_start(wifi_sta_get_start_wifi_thread_id());
#else
	start_app();
#endif /* CONFIG_WIFI_READY_LIB */

	return ret;
}
