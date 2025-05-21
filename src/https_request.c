#include "https_request.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(HTTPS_REQUEST, CONFIG_WIFI_STA_LOG_LEVEL);

// Define a buffer large enough to hold headers and body
#define SEND_BUF_SIZE 1024
#define RECV_BUF_SIZE 2048
static char recv_buf[RECV_BUF_SIZE];

#define HTTPS_PORT "443"
#define POST_URL "/v1/location/wifi"
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

void send_http_request(char *location_str, size_t location_str_len, char *api_str,
		       size_t api_str_len, char *auth_token)
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

	// Clear location string
	memset(location_str, 0, location_str_len);

	LOG_INF("Looking up %s\n", CONFIG_HTTPS_HOSTNAME);

	// NOTE: Multiple DNS attempts inorder to deal with unstable network such as mobile hotspots
	for (int i = 0; i < CONFIG_DNS_ATTEMPTS; i++) {
		err = getaddrinfo(CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, &hints, &res);
		if (err) {
			if (i == CONFIG_DNS_ATTEMPTS - 1) {
				LOG_ERR("getaddrinfo() failed, errno %d, err %d\n", errno, err);
				return;
			}
			LOG_WRN("getaddrinfo() failed, errno %d, err %d\n", errno, err);
		} else {
			break;
		}
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

	// // Remove trailing comma of the api string
	api_str[strlen(api_str) - 1] = '\0';

	// Combine the JSON body string
	char json_body[1024];
	int json_body_len =
		snprintf(json_body, sizeof(json_body), "{\"accessPoints\":[%s]}", api_str);

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
				  POST_URL, CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, auth_token,
				  json_body_len, json_body);

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

	// Get the HTTP response code
	char *response_code = strstr(recv_buf, "HTTP/1.1 ");
	int http_response_code = 0;
	if (response_code) {
		response_code += strlen("HTTP/1.1 ");
		char *end_of_code = strstr(response_code, " ");
		if (end_of_code) {
			*end_of_code = '\0'; // Temporarily terminate the string
			LOG_WRN("HTTP Response Code: %s\n", response_code);
			http_response_code = atoi(response_code);
			*end_of_code = ' '; // Restore the space
		} else {
			LOG_ERR("Could not find end of HTTP response code.\n");
		}
	} else {
		LOG_ERR("Could not find HTTP response code.\n");
	}

	char *header_end = strstr(recv_buf, "\r\n\r\n"); // Locate the end of headers
	if (header_end) {
		LOG_INF("========================================");
		LOG_INF("HTTP Headers:");
		*header_end = '\0';        // Temporarily terminate at the end of headers
		LOG_INF("\n%s\n", recv_buf); // Print headers
		LOG_INF("========================================");
		LOG_INF("HTTP Body:");
		LOG_INF("\n%s\n", header_end + 4); // Print the body after separating headers
		LOG_INF("========================================");
	} else {
		LOG_WRN("Could not find end of headers.\n");
	}

	LOG_INF("Finished, closing socket.\n");

	int ret = 0;

	switch (http_response_code) {
	case 200:
		ret = snprintf(location_str, location_str_len, "%s", header_end + 4);
		break;
	case 400:
		LOG_ERR("Bad Request: %s", header_end + 4);
		memcpy(location_str, "{\"message\": \"Bad Request\"}",
		       sizeof("{\"message\": \"Bad Request\"}"));
		break;
	case 401:
		LOG_ERR("Unauthorized: %s", header_end + 4);
		ret = snprintf(location_str, location_str_len, "%s", header_end + 4);
		break;
	case 403:
		LOG_ERR("Forbidden: %s", header_end + 4);
		memcpy(location_str, "{\"message\": \"Forbidden\"}",
		       sizeof("{\"message\": \"Forbidden\"}"));
		break;
	case 404:
		LOG_ERR("Not Found: %s", header_end + 4);
		memcpy(location_str, "{\"message\": \"Location not found\"}",
		       sizeof("{\"message\": \"Location not found\"}"));
		break;
	case 500:
		LOG_ERR("Internal Server Error: %s", header_end + 4);
		memcpy(location_str, "{\"message\": \"Internal Server Error\"}",
		       sizeof("{\"message\": \"Internal Server Error\"}"));
		break;
	case 503:
		LOG_ERR("Service Unavailable: %s", header_end + 4);
		memcpy(location_str, "{\"message\": \"Service Unavailable\"}",
		       sizeof("{\"message\": \"Service Unavailable\"}"));
		break;
	default:
		LOG_ERR("Unexpected HTTP response code: %d", http_response_code);
		memcpy(location_str, "{\"message\": \"Unexpected HTTP response code\"}",
		       sizeof("{\"message\": \"Unexpected HTTP response code\"}"));
		break;
	}
	if (ret < 0) {
		LOG_ERR("Failed to format location string\n");
		goto clean_up;
	}

clean_up:
	freeaddrinfo(res);
	(void)close(fd);
}