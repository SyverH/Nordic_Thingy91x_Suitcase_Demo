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
- FW is available as a .hex

## Configuration 
### User Configurations
Necessary settings to run the demo:
- Provision your network credentials with **net shell**
- For easy connection in the browser, define the hostname in `prj.conf`
- Add your JWT token on the website after its set up.

#### Net shell credentials example
```
wifi_cred add -s <SSID> -k 1 -p <PASSWORD>
```
-k 1 is equal to WPA2-PSK
```
wifi_cred auto_connect
```


**prj.conf changes**
```Cmake
CONFIG_NET_HOSTNAME="thingy91x"
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

The application can also be uploaded as a .hex.
