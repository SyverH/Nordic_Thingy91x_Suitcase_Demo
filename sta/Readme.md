# Thingy:91x Suitcase Demo
This project showcases how to host a responsive and interactive web page using the Thingy:91x.

To view the webpage, connect the device to a network and open its predefined hostname (e.g., `<Your Net Hostname>.local`) or assigned IP address in any web browser.

The website is active when the front-facing LED turns green.

## Capabilities
The demo showcases following features:
- **Sensor data visualization:** Graphs of device sensor data
- **Remote control of device:** Control of device LEDs
- **Wi-Fi SSID Locationing:** WiFi SSID Locationing with nRF Cloud REST API
- **Orientation calculation on device:** Preview of device orientation with a dynamic 3D visualization



## Requirements
HW:
- Thingy:91x
- Segger debugger or similar swd debugger

SW:
- Nordic SDK (**NCS**) v2.9.0

Optional:
- USB-C cable for logging, and net shell.

## Configuration 
### User Configurations
The settings for this demo are defined in the `prj.conf` file.
The JWT token is stored in the git-hidden file JWT_token.h to avoid accidental publication.

To necessary settings to run the demo:
- Enter your Wi-Fi credentials (SSID and Password) in `prj.conf`
- For easy connection in the browser, define the hostname in `prj.conf`
- Add your JWT (JSON Web Token) for authentication in `JWT_token.h`

Network connections can also be managed dynamically using **net shell**.

**prj.conf changes**
```Cmake
CONFIG_WIFI_CREDENTIALS_STATIC_SSID="YourSSID"
CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD="YourPassword"

CONFIG_NET_HOSTNAME="thingy91x"
```

**JWT_token.h changes**
```c
#define AUTH_TOKEN "Your Token"
```

### Generating and Updating Network Certificates
If the network certificate has expired, you can generate a new certificate the following ways:
#### With openssl
- Run `openssl s_client -connect api.nrfcloud.com:443 -servername api.nrfcloud.com -showcerts` and copy the G2 certificate from Starfield Class 2 Certification Authority.

#### With external service
- Open `ssllabs.com/ssltest` in a web browser, and enter `api.nrfcloud.com` as hostname. When generation is completed, copy the `Starfield Services Root Certificate Authority - G2`.

Once you’ve generated the updated certificate, replace the contents of the `DigiCertGlobalG2.pem` file with the newly-acquired certificate.

## Building and Running the Project
Before running the project the **nRF9151** must be wiped to free up the external flash and the nRF7002.
To do this, connect the power and debuggers to the Thingy:91x, ensure switch 2 is set to the nRF91 target, and erase the flash using erase all in nrf connect programmer.

Before flashing the project, ensure that switch 2 is set to the nRF5340 target.
```
west build -p -b thingy91x/nrf5340/cpuapp
west flash --erase
```

## TODO
- [ ] implement wifi provisioning
- [ ] implement secure web server
- [ ] add option to run as a softAP
- [x] implement wifi positioning
- [ ] implement bmm350 magnetometer (waiting for driver in zephyr)
- [x] implement mDNS
- [x] implement rgb led control
