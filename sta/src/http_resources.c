
#include "http_resources.h"

//////////////////////////////////////// HTTP Service //////////////////////////////////////////

static uint16_t test_http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(test_http_service, NULL, &test_http_service_port, 1, 10, NULL);

//////////////////////////////////////// HTTP Resources //////////////////////////////////////////

////////////////// Index HTML //////////////////
// This is the main HTML file that is loaded by the browser.

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

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

////////////////// Main JS //////////////////
// This is the main JavaScript file that is loaded by the index.html file.

static const uint8_t main_js_gz[] = {
#include "main.js.gz.inc"
};

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

////////////////// styles CSS //////////////////
// This is the CSS file that is loaded by the index.html file.

static const uint8_t styles_css_gz[] = {
#include "styles.css.gz.inc"
};

struct http_resource_detail_static styles_css_gz_resource_detail = {
    .common =
        {
            .type = HTTP_RESOURCE_TYPE_STATIC,
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
            .content_encoding = "gzip",
            .content_type = "text/css",
        },
    .static_data = styles_css_gz,
    .static_data_len = sizeof(styles_css_gz),
};

HTTP_RESOURCE_DEFINE(styles_css_gz_resource, test_http_service, "/styles.css",
                     &styles_css_gz_resource_detail);

////////////////// Color Wheel svg //////////////////
// This is an image to be displayed in the browser.

static const uint8_t color_wheel_png_gz[] = {
	#include "color_wheel.svg.gz.inc"
};

struct http_resource_detail_static color_wheel_png_gz_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_encoding = "gzip",
		.content_type = "image/svg+xml",
    },
    .static_data = color_wheel_png_gz,
    .static_data_len = sizeof(color_wheel_png_gz),
};

HTTP_RESOURCE_DEFINE(color_wheel_png_gz_resource, test_http_service, "/color_wheel.svg",
		     &color_wheel_png_gz_resource_detail);

////////////////// Logo Nordic svg //////////////////
// This is an image to be displayed in the browser.

static const uint8_t logo_Nordic_svg_gz[] = {
#include "Logo_Flat_RGB_Horizontal.svg.gz.inc"
};

struct http_resource_detail_static logo_Nordic_svg_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "image/svg+xml",
		},
	.static_data = logo_Nordic_svg_gz,
	.static_data_len = sizeof(logo_Nordic_svg_gz),
};

HTTP_RESOURCE_DEFINE(logo_Nordic_svg_gz_resource, test_http_service,
		     "/Logo_Flat_RGB_Horizontal.svg", &logo_Nordic_svg_gz_resource_detail);

////////////////// 3D Model //////////////////
// This is an 3D model to be displayed in the browser.

static const uint8_t model_glb_gz[] = {
// #include "Duck.glb.gz.inc"
#include "thingy91x3.glb.gz.inc"
};

struct http_resource_detail_static model_glb_gz_resource_detail = {
    .common =
        {
            .type = HTTP_RESOURCE_TYPE_STATIC,
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
            .content_encoding = "gzip",
            .content_type = "model/gltf-binary",
        },
    .static_data = model_glb_gz,
    .static_data_len = sizeof(model_glb_gz),
};

HTTP_RESOURCE_DEFINE(model_glb_gz_resource, test_http_service, "/thingy91x3.glb",
                     &model_glb_gz_resource_detail);

////////////////// Recalibrate Gyro Button //////////////////
// This is a button on the webpage that allows the user to recalibrate the gyroscope.
// It is a dynamic resource that accepts POST requests with JSON payloads.

static uint8_t recalibrate_gyro_buf[256]; // Buffer to store the JSON payload

static struct http_resource_detail_dynamic recalibrate_gyro_resource_detail = {
    .common =
        {
            .type = HTTP_RESOURCE_TYPE_DYNAMIC,
            .bitmask_of_supported_http_methods = BIT(HTTP_POST),
        },
    .cb = NULL, // This is set by the http_resources_set_recalibrate_gyro_handler function to allow
            // callback function to be defined in main.c
    .data_buffer = recalibrate_gyro_buf,
    .data_buffer_len = sizeof(recalibrate_gyro_buf),
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(recalibrate_gyro_resource, test_http_service, "/recalibrate_gyro",
                     &recalibrate_gyro_resource_detail);

void http_resources_set_recalibrate_gyro_handler(http_resource_dynamic_cb_t handler)
{
    recalibrate_gyro_resource_detail.cb = handler;
}

////////////////// LED Resource //////////////////
// This is the resource that is used to control the LEDs on the Thingy:91x.
// It is a dynamic resource that accepts POST requests with JSON payloads.

static uint8_t led_buf[256]; // Buffer to store the JSON payload

static struct http_resource_detail_dynamic led_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		},
	.cb = NULL, // This is set by the http_resources_set_led_handler function to allow callback
		    // function to be defined in main.c
	.data_buffer = led_buf,
	.data_buffer_len = sizeof(led_buf),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(led_resource, test_http_service, "/led", &led_resource_detail);

void http_resources_set_led_handler(http_resource_dynamic_cb_t handler)
{
	led_resource_detail.cb = handler;
}

////////////////// WebSocket Resource //////////////////
// This is the resource that is used to send sensor data over a WebSocket connection.

static struct ws_sensors_ctx sensors_ctx[CONFIG_NET_SAMPLE_NUM_WEBSOCKET_HANDLERS];

static uint8_t ws_netstats_buffer[128];

struct http_resource_detail_websocket ws_netstats_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_WEBSOCKET,
			/* We need HTTP/1.1 Get method for upgrading */
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.cb = NULL, // This is set by the http_resources_set_ws_handler function to allow callback
		    // function to be defined in main.c
	.data_buffer = ws_netstats_buffer,
	.data_buffer_len = sizeof(ws_netstats_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(ws_netstats_resource, test_http_service, "/", &ws_netstats_resource_detail);

void http_resources_set_ws_handler(http_resource_websocket_cb_t handler)
{
	ws_netstats_resource_detail.cb = handler;
}

void http_resources_get_ws_ctx(struct ws_sensors_ctx **ctx)
{
	*ctx = sensors_ctx;
}
