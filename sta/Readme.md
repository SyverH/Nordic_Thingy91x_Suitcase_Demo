# Thingy91x Suitcase Demo
This project demonstrates how to host a simple web page on the Thingy:91x serving sensor data. The web page is hosted on the Thingy:91x and can be accessed by connecting the device to the network and opening the IP address of the device in a web browser.

## Requirements
HW:
- Thingy:91x
- USB-C cable
- Segger debugger or similar swd debugger

SW:
- NCS v2.9.0


## Configuration 
WiFi configuration is done in `prj.conf` file. 
```Cmake
CONFIG_WIFI_CREDENTIALS_STATIC_SSID="YourSSID"
CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD="YourPassword"
```

## Building and running
Before running the project the nRF9151 must be wiped to free up the external flash and the nRF7002.
To do this, connect the power and debuggers to the Thingy:91x, ensure switch 2 is set to the nRF91 target, and erase the flash using erase all in nrf connect programmer.

Before flashing the project, ensure that switch 2 is set to the nRF5340 target.
```
west build -p -b thingy91x/nrf5340/cpuapp
west flash --erase
```

## TODO
- implement wifi provisioning
- implement secure web server
- add option to run as a softAP
- implement wifi positioning
- implement bmm350 magnetometer (waiting for driver in zephyr)