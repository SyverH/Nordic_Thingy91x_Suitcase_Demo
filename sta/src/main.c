/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi station sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sta, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
// #include <zephyr/drivers/gpio.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>

#include <zephyr/data/json.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>

#include <zephyr/sys/reboot.h>

// #include <zephyr/drivers/sensor.h>
// #include <zephyr/device.h>
#include "sensors.h"

#include <net/wifi_mgmt_ext.h>
#include <net/wifi_ready.h>

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) ||                                      \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>
#endif

#include "net_private.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

#define MAX_SSID_LEN      32
#define STATUS_POLLING_MS 300

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback net_shell_mgmt_cb;

#ifdef CONFIG_WIFI_READY_LIB
static K_SEM_DEFINE(wifi_ready_state_changed_sem, 0, 1);
static bool wifi_ready_status;
#endif /* CONFIG_WIFI_READY_LIB */

static struct {
	const struct shell *sh;
	union {
		struct {
			uint8_t connected: 1;
			uint8_t connect_result: 1;
			uint8_t disconnect_requested: 1;
			uint8_t _unused: 5;
		};
		uint8_t all;
	};
} context;

bool wifi_connected = false;

int counter = 0;

// Prototype for the led handler function to allow it to be used in the led_resource_detail struct
static int led_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data);
// Prototype for the ws_sensors_setup function to allow it to be used in the
// ws_netstats_resource_detail struct
int ws_sensors_setup(int ws_socket, void *user_data);

///////////////////////////////////////////////////////////////////////////////
// HTTP data

static uint16_t test_http_service_port = 80;
HTTP_SERVICE_DEFINE(test_http_service, NULL, &test_http_service_port, 1, 10, NULL);

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static const uint8_t main_js_gz[] = {
#include "main.js.gz.inc"
};

// static const uint8_t color_wheel_png_gz[] = {
// 	#include "color_wheel.svg.gz.inc"
// };

static const uint8_t logo_Nordic_svg_gz[] = {
	#include "Logo_Flat_RGB_Horizontal.svg.gz.inc"
};
	
// struct http_resource_detail_static color_wheel_png_gz_resource_detail = {
//     .common = {
//         .type = HTTP_RESOURCE_TYPE_STATIC,
//         .bitmask_of_supported_http_methods = BIT(HTTP_GET),
//         .content_encoding = "gzip",
// 		.content_type = "image/svg+xml",
//     },
//     .static_data = color_wheel_png_gz,
//     .static_data_len = sizeof(color_wheel_png_gz),
// };

// HTTP_RESOURCE_DEFINE(color_wheel_png_gz_resource, test_http_service, "/color_wheel.svg",
// 		     &color_wheel_png_gz_resource_detail);

struct http_resource_detail_static logo_Nordic_svg_gz_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "image/svg+xml",
	},
	.static_data = logo_Nordic_svg_gz,
	.static_data_len = sizeof(logo_Nordic_svg_gz),
};

HTTP_RESOURCE_DEFINE(logo_Nordic_svg_gz_resource, test_http_service, "/Logo_Flat_RGB_Horizontal.svg",
		     &logo_Nordic_svg_gz_resource_detail);

struct http_resource_detail_static index_html_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

struct http_resource_detail_static main_js_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "application/javascript",
		},
	.static_data = main_js_gz,
	.static_data_len = sizeof(main_js_gz),
};

HTTP_RESOURCE_DEFINE(main_js_gz_resource, test_http_service, "/main.js",
		     &main_js_gz_resource_detail);

struct led_command {
	int led_num;
	bool led_state;
};

static const struct json_obj_descr led_command_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct led_command, led_num, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_command, led_state, JSON_TOK_TRUE),
};

static uint8_t led_buf[256];

static struct http_resource_detail_dynamic led_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = led_handler,
	.data_buffer = led_buf,
	.data_buffer_len = sizeof(led_buf),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(led_resource, test_http_service, "/led", &led_resource_detail);

struct ws_sensors_ctx {
	int sock;
	struct k_work_delayable work;
};

static struct ws_sensors_ctx sensors_ctx[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS];

static uint8_t ws_netstats_buffer[128];

struct http_resource_detail_websocket ws_netstats_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_WEBSOCKET,
			/* We need HTTP/1.1 Get method for upgrading */
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = ws_sensors_setup,
	.data_buffer = ws_netstats_buffer,
	.data_buffer_len = sizeof(ws_netstats_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(ws_netstats_resource, test_http_service, "/", &ws_netstats_resource_detail);

///////////////////////////////////////////////////////////////////////////////

static void sensor_handeler(struct k_work *work)
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
	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		sensors_ctx[i].sock = -1;
		k_work_init_delayable(&sensors_ctx[i].work, sensor_handeler);
	}

	return 0;
}
SYS_INIT(ws_sensors_init, APPLICATION, 0);

static int get_free_sensor_slot(void)
{
	for (int i = 0; i < CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS; i++) {
		if (sensors_ctx[i].sock < 0) {
			return i;
		}
	}

	return -1;
}

int ws_sensors_setup(int ws_socket, void *user_data)
{
	int slot = get_free_sensor_slot();
	if (slot < 0) {
		LOG_ERR("No free slot for sensor websocket");
		return -ENOMEM;
	}
	LOG_INF("Setting up sensor websocket on slot %d", slot);

	sensors_ctx[slot].sock = ws_socket;

	LOG_INF("Using socket %d for sensor websocket", ws_socket);

	int ret = k_work_reschedule(&sensors_ctx[slot].work, K_NO_WAIT);
	if (ret) {
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

	ret = json_obj_parse(buf, len, led_command_descr, ARRAY_SIZE(led_command_descr), &cmd);
	if (ret != expected_return_code) {
		LOG_WRN("Failed to fully parse JSON payload, ret=%d", ret);
		return;
	}

	LOG_INF("POST request setting LED %d to state %d", cmd.led_num, cmd.led_state);

	ret = dk_set_leds_state(1 << cmd.led_num, cmd.led_state);
	if (ret) {
		LOG_ERR("Failed to set LED state");
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

static int cmd_wifi_status(void)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = {0};

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		LOG_INF("Status request failed");

		return -ENOEXEC;
	}

	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		LOG_INF("Interface Mode: %s", wifi_mode_txt(status.iface_mode));
		LOG_INF("Link Mode: %s", wifi_link_mode_txt(status.link_mode));
		LOG_INF("SSID: %.32s", status.ssid);
		LOG_INF("BSSID: %s",
			net_sprint_ll_addr_buf(status.bssid, WIFI_MAC_ADDR_LEN, mac_string_buf,
					       sizeof(mac_string_buf)));
		LOG_INF("Band: %s", wifi_band_txt(status.band));
		LOG_INF("Channel: %d", status.channel);
		LOG_INF("Security: %s", wifi_security_txt(status.security));
		LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp));
		LOG_INF("RSSI: %d", status.rssi);
	}
	return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (context.connected) {
		return;
	}

	if (status->status) {
		LOG_ERR("Connection failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
		LOG_WRN("HTTP server staring");
		http_server_start();
		context.connected = true;
	}

	context.connect_result = true;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (!context.connected) {
		return;
	}

	if (context.disconnect_requested) {
		LOG_INF("Disconnection request %s (%d)", status->status ? "failed" : "done",
			status->status);
		context.disconnect_requested = false;
	} else {
		LOG_INF("Received Disconnected");
		context.connected = false;
	}

	cmd_wifi_status();
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
}

static void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

	LOG_INF("DHCP IP address: %s", dhcp_info);
}
static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				   struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

static int wifi_connect(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	context.connected = false;
	context.connect_result = false;

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0)) {
		LOG_ERR("Connection request failed");

		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

	return 0;
}

int bytes_from_str(const char *str, uint8_t *bytes, size_t bytes_len)
{
	size_t i;
	char byte_str[3];

	if (strlen(str) != bytes_len * 2) {
		LOG_ERR("Invalid string length: %zu (expected: %d)\n", strlen(str), bytes_len * 2);
		return -EINVAL;
	}

	for (i = 0; i < bytes_len; i++) {
		memcpy(byte_str, str + i * 2, 2);
		byte_str[2] = '\0';
		bytes[i] = strtol(byte_str, NULL, 16);
	}

	return 0;
}

int start_app(void)
{
	LOG_INF("Static IP address (overridable): %s/%s -> %s", CONFIG_NET_CONFIG_MY_IPV4_ADDR,
		CONFIG_NET_CONFIG_MY_IPV4_NETMASK, CONFIG_NET_CONFIG_MY_IPV4_GW);

	while (1) {
#ifdef CONFIG_WIFI_READY_LIB
		int ret;

		LOG_INF("Waiting for Wi-Fi to be ready");
		ret = k_sem_take(&wifi_ready_state_changed_sem, K_FOREVER);
		if (ret) {
			LOG_ERR("Failed to take semaphore: %d", ret);
			return ret;
		}

	check_wifi_ready:
		if (!wifi_ready_status) {
			LOG_INF("Wi-Fi is not ready");
			/* Perform any cleanup and stop using Wi-Fi and wait for
			 * Wi-Fi to be ready
			 */
			continue;
		}
#endif /* CONFIG_WIFI_READY_LIB */
		wifi_connect();

		while (!context.connect_result) {
			cmd_wifi_status();
			k_sleep(K_MSEC(STATUS_POLLING_MS));
		}

		if (context.connected) {
			cmd_wifi_status();
#ifdef CONFIG_WIFI_READY_LIB
			ret = k_sem_take(&wifi_ready_state_changed_sem, K_FOREVER);
			if (ret) {
				LOG_ERR("Failed to take semaphore: %d", ret);
				return ret;
			}
			goto check_wifi_ready;
#else
			k_sleep(K_FOREVER);
#endif /* CONFIG_WIFI_READY_LIB */
		}
	}

	return 0;
}

#ifdef CONFIG_WIFI_READY_LIB
void start_wifi_thread(void);
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
K_THREAD_DEFINE(start_wifi_thread_id, CONFIG_STA_SAMPLE_START_WIFI_THREAD_STACK_SIZE,
		start_wifi_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, -1);

void start_wifi_thread(void)
{
	start_app();
}

void wifi_ready_cb(bool wifi_ready)
{
	LOG_DBG("Is Wi-Fi ready?: %s", wifi_ready ? "yes" : "no");
	wifi_ready_status = wifi_ready;
	k_sem_give(&wifi_ready_state_changed_sem);
}
#endif /* CONFIG_WIFI_READY_LIB */

void net_mgmt_callback_init(void)
{
	memset(&context, 0, sizeof(context));

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb, wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&net_shell_mgmt_cb, net_mgmt_event_handler,
				     NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&net_shell_mgmt_cb);

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock / MHZ(1));
	k_sleep(K_SECONDS(1));
}

#ifdef CONFIG_WIFI_READY_LIB
static int register_wifi_ready(void)
{
	int ret = 0;
	wifi_ready_callback_t cb;
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	cb.wifi_ready_cb = wifi_ready_cb;

	LOG_DBG("Registering Wi-Fi ready callbacks");
	ret = register_wifi_ready_callback(cb, iface);
	if (ret) {
		LOG_ERR("Failed to register Wi-Fi ready callbacks %s", strerror(ret));
		return ret;
	}

	return ret;
}
#endif /* CONFIG_WIFI_READY_LIB */

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & DK_BTN1_MSK) {
        if (button_state & DK_BTN1_MSK) {
            LOG_INF("Button 1 pressed");
            sys_reboot(SYS_REBOOT_COLD); // Reset the device
        }
    }
}
        

int main(void)
{
	int ret = 0;

	ret = dk_leds_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize LEDs");
		return -1;
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

	net_mgmt_callback_init();

#ifdef CONFIG_WIFI_READY_LIB
	ret = register_wifi_ready();
	if (ret) {
		return ret;
	}
	k_thread_start(start_wifi_thread_id);
#else
	start_app();
#endif /* CONFIG_WIFI_READY_LIB */

	return ret;
}
