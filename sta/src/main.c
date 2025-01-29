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
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/data/json.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>

#include <zephyr/sys/reboot.h>

#include "sensors.h"
#include "http_resources.h"
#include "wifi_sta.h"

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

	ret = dk_set_led(cmd.led_num, cmd.led_state);
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
	LOG_INF("HTTP server staring");
	http_server_start();
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

	http_resources_set_led_handler(led_handler);
	http_resources_set_ws_handler(ws_sensors_setup);
	wifi_sta_set_wifi_connected_cb(wifi_connected_handler);

	net_mgmt_callback_init();

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
