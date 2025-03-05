#pragma once

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/data/json.h>

struct ws_sensors_ctx {
	int sock;
	struct k_work_delayable work;
};

struct led_command {
    int r;
    int g;
    int b;
};

static const struct json_obj_descr led_command_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct led_command, r, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct led_command, g, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct led_command, b, JSON_TOK_NUMBER),
};

void http_resources_set_led_handler(http_resource_dynamic_cb_t handler);
void http_resources_set_ws_handler(http_resource_websocket_cb_t handler);
void http_resources_get_ws_ctx(struct ws_sensors_ctx **ctx);