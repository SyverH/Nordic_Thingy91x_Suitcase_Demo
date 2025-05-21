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
#include "https_request.h"

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

uint8_t apply_gamma(uint8_t value, float gamma)
{
	/* (value/255)^gamma * 255 */
	return (uint8_t)(pow(value / 255.0, gamma) * 255);
}

int pwm_set_color(int red, int green, int blue)
{
	int ret;

	/* Parameters to adjust intensity of each color */
	float red_gamma = 2.2;
	float green_gamma = 2.2;
	float blue_gamma = 2.2;

	float red_intensity_scale = 1.0;
	float green_intensity_scale = 1.0;
	float blue_intensity_scale = 0.8;

	// LOG_DBG("PWM_Color: Red: %d, Green: %d, Blue: %d \n", red, green, blue);

	// Apply gamma correction
	int corrected_red = apply_gamma(red * red_intensity_scale, red_gamma);
	int corrected_green = apply_gamma(green * green_intensity_scale, green_gamma);
	int corrected_blue = apply_gamma(blue * blue_intensity_scale, blue_gamma);

	// uint32_t period = red_pwm_led.period;
	int pwm_red = (corrected_red * red_pwm_led.period) / 255;
	int pwm_green = (corrected_green * green_pwm_led.period) / 255;
	int pwm_blue = (corrected_blue * blue_pwm_led.period) / 255;

	ret = pwm_set_pulse_dt(&red_pwm_led, pwm_red);
	if (ret != 0) {
		LOG_ERR("Error %d: red write failed\n", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&green_pwm_led, pwm_green);
	if (ret != 0) {
		LOG_ERR("Error %d: green write failed\n", ret);
		return ret;
	}

	ret = pwm_set_pulse_dt(&blue_pwm_led, pwm_blue);
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
	ARG_UNUSED(user_data);

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
	ARG_UNUSED(client);
	ARG_UNUSED(user_data);

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

static void parse_jwt_post(uint8_t *buf, size_t len)
{
	ARG_UNUSED((len));

	int ret;

	ret = pwm_set_color(255, 255, 0);
	if (ret) {
		LOG_ERR("Failed to set LED color");
	}

	// Manually phrasing the JSON payload
	char temp_buff[512] = {0};
	// Remove leading {"jwt":"
	ret = snprintf(temp_buff, sizeof(temp_buff), "%s", buf + 8);
	if (ret < 0) {
		LOG_ERR("Failed to format JWT\n");
		return;
	}
	// Remove trailing "}
	temp_buff[ret - 2] = '\0';

	LOG_WRN("Temp buffer: %s", temp_buff);

	char location_str[256];
	memset(location_str, 0, sizeof(location_str));

	char api_str[CONFIG_WIFI_SCAN_STR_MAX_MAC_ADDR * 65];

	get_nrfcloud_api_str(api_str, sizeof(api_str));

	send_http_request(location_str, sizeof(location_str), api_str, sizeof(api_str), temp_buff);

	LOG_WRN("Location string: %s", location_str);

	http_resources_set_location(location_str);

	ret = pwm_set_color(0, 255, 0);
	if (ret) {
		LOG_ERR("Failed to set LED color");
	}
}

static int jwt_handler(struct http_client_ctx *client, enum http_data_status status,
		       uint8_t *buffer, size_t len, void *user_data)
{

	ARG_UNUSED(client);
	ARG_UNUSED(user_data);

	static uint8_t post_payload_buf[512];
	static size_t cursor;

	LOG_WRN("JWT handler status %d, size %zu", status, len);

	if (status == HTTP_SERVER_DATA_ABORTED) {
		cursor = 0;
		return 0;
	}

	if (len + cursor > sizeof(post_payload_buf)) {
		cursor = 0;
		LOG_ERR("Buffer overflow");
		return -ENOMEM;
	}

	/* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
	 * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
	 * the client buffer).
	 */
	memcpy(post_payload_buf + cursor, buffer, len);
	cursor += len;
	LOG_INF("JWT handler cursor %zu", cursor);

	if (status == HTTP_SERVER_DATA_FINAL) {
		parse_jwt_post(post_payload_buf, cursor);
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
	ARG_UNUSED(client);
	ARG_UNUSED(len);
	ARG_UNUSED(user_data);

	static bool response_sent;
	extern char location_buf[256];

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
	http_resources_set_jwt_handler(jwt_handler);
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
