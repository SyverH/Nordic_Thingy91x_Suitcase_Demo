#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_sta)

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

# Add http service
zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)

# Add static web resources
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource
  index.html
  main.js
  styles.css
#   color_wheel.svg
  Color_circle.svg
  Logo_Flat_RGB_Horizontal.svg
#   Duck.glb
  thingy91x.glb
    )
  generate_inc_file_for_target(
    app
    src/static_web_resources/${web_resource}
    ${gen_dir}/${web_resource}.gz.inc
    --gzip
  )
endforeach()

# Generate hex files from pem files
set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/certs)
zephyr_include_directories(${gen_dir})
generate_inc_file_for_target(
    app
    cert/StarfieldServicesCertG2.pem
    ${gen_dir}/StarfieldServicesCertG2.pem.inc
    )

target_sources(app PRIVATE
	src/main.c
    src/sensors.c
    src/http_resources.c
    src/wifi.c
)
