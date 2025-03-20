#include "wifi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(WIFI_STA, CONFIG_WIFI_STA_LOG_LEVEL);

#include "net_private.h"

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

static void *wifi_connected_cb = NULL;

void wifi_sta_set_wifi_connected_cb(void *cb)
{
	wifi_connected_cb = cb;
}

static uint32_t scan_result;

char nrfcloud_api_str[CONFIG_WIFI_SCAN_STR_MAX_MAC_ADDR * 65] = {0};

K_SEM_DEFINE(scan_sem, 0, 1);
#define SCAN_TIMEOUT_MS 10000

int wifi_channel_to_freq(int channel)
{
    if (channel == 14) {
        return 2484;
    } else if ((channel >= 1) && (channel <= 13)) {
        return 2407 + (channel * 5);
    } else if ((channel >= 36) && (channel <= 165)) {
        return 5000 + (channel * 5);
    } else {
        return channel;
    }
}

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
	uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

	scan_result++;

	if (scan_result == 1U) {
		printk("%-4s | %-32s %-5s | %-4s | %-10s | %-4s | %-12s | %s\n",
		       "Num", "SSID", "(len)", "Chan", "Frequency", "RSSI", "Security", "BSSID");
	}

	strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
	ssid_print[sizeof(ssid_print) - 1] = '\0';

	printk("%-4d | %-32s %-5u | %-4u | %-10d | %-4d | %-12s | %s\n",
	       scan_result, ssid_print, entry->ssid_length,
	       entry->channel, wifi_channel_to_freq(entry->channel),
           entry->rssi,
	       wifi_security_txt(entry->security),
	       ((entry->mac_length) ?
			net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN, mac_string_buf,
						sizeof(mac_string_buf)) : ""));

    int ret;

    const char *sensors_json_template = "{"        // 1 char
                        "\"macAddress\":\"%s\","       // 14 chars + %s (17)
                        "\"signalStrength\":%-4d"  // 18 chars + %d (4)
                        "},";                        // 1 char
                                                    // 34 + 17 + 4 = 55

    if(scan_result < CONFIG_WIFI_SCAN_STR_MAX_MAC_ADDR) {
        ret = snprintf(nrfcloud_api_str + strlen(nrfcloud_api_str), sizeof(nrfcloud_api_str) - strlen(nrfcloud_api_str),
                    sensors_json_template, mac_string_buf, entry->rssi);
        if (ret < 0) {
            LOG_ERR("Failed to create JSON string");
        }

        // LOG_WRN("nrfcloud_api_str: %s", nrfcloud_api_str);
    }

    // remove last comma
    if(scan_result == CONFIG_WIFI_SCAN_STR_MAX_MAC_ADDR) {
        nrfcloud_api_str[strlen(nrfcloud_api_str) - 1] = '\0';
        LOG_ERR("nrfcloud_api_str: %s", nrfcloud_api_str);
    }


    

}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Scan request failed (%d)", status->status);
	} else {
		printk("Scan request done\n");
	}

	scan_result = 0U;
	k_sem_give(&scan_sem);
}

int wifi_scan(void)
{
	struct net_if *iface = net_if_get_default();
	int band_str_len;
	struct wifi_scan_params params = { 0 };

	band_str_len = sizeof(CONFIG_WIFI_SCAN_BANDS_LIST);
	if (band_str_len - 1) {
		char *buf = malloc(band_str_len);

		if (!buf) {
			LOG_ERR("Malloc Failed");
			return -EINVAL;
		}
		strcpy(buf, CONFIG_WIFI_SCAN_BANDS_LIST);
		if (wifi_utils_parse_scan_bands(buf, &params.bands)) {
			LOG_ERR("Incorrect value(s) in CONFIG_WIFI_SCAN_BANDS_LIST: %s",
					CONFIG_WIFI_SCAN_BANDS_LIST);
			free(buf);
			return -ENOEXEC;
		}
		free(buf);
	}

	if (sizeof(CONFIG_WIFI_SCAN_CHAN_LIST) - 1) {
		if (wifi_utils_parse_scan_chan(CONFIG_WIFI_SCAN_CHAN_LIST,
						params.band_chan, ARRAY_SIZE(params.band_chan))) {
			LOG_ERR("Incorrect value(s) in CONFIG_WIFI_SCAN_CHAN_LIST: %s",
					CONFIG_WIFI_SCAN_CHAN_LIST);
			return -ENOEXEC;
		}
	}

	params.dwell_time_passive = CONFIG_WIFI_SCAN_DWELL_TIME_PASSIVE;
	params.dwell_time_active = CONFIG_WIFI_SCAN_DWELL_TIME_ACTIVE;

	if (IS_ENABLED(CONFIG_WIFI_SCAN_TYPE_PASSIVE)) {
		params.scan_type = WIFI_SCAN_TYPE_PASSIVE;
	} else {
		params.scan_type = WIFI_SCAN_TYPE_ACTIVE;
	}

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params,
			sizeof(struct wifi_scan_params))) {
		LOG_ERR("Scan request failed");
		return -ENOEXEC;
	}

	printk("Scan requested\n");

	k_sem_take(&scan_sem, K_MSEC(SCAN_TIMEOUT_MS));

	return 0;
}


int cmd_wifi_status(void)
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

void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (context.connected) {
		return;
	}

	if (status->status) {
		LOG_ERR("Connection failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
		if (wifi_connected_cb) {
			((void (*)(void))wifi_connected_cb)();
		} else {
			LOG_WRN("No Wi-Fi connected callback set");
		}
		context.connected = true;
	}

	context.connect_result = true;
}

void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
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

int wifi_connect(void)
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

k_tid_t wifi_sta_get_start_wifi_thread_id(void)
{
	return start_wifi_thread_id;
}

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



void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			     struct net_if *iface)
{

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;

    case NET_EVENT_WIFI_SCAN_RESULT:
        handle_wifi_scan_result(cb);
        break;
        
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
    case NET_EVENT_WIFI_RAW_SCAN_RESULT:
        handle_raw_scan_result(cb);
        break;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
    case NET_EVENT_WIFI_SCAN_DONE:
        handle_wifi_scan_done(cb);
        break;

	default:
		break;
	}
}

void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

	LOG_INF("DHCP IP address: %s", dhcp_info);
}
void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
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
int register_wifi_ready(void)
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