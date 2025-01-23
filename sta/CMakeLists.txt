#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_sta)

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource
  index.html
  main.js
    )
  generate_inc_file_for_target(
    app
    src/static_web_resources/${web_resource}
    ${gen_dir}/${web_resource}.gz.inc
    --gzip
  )
endforeach()

# set(source_file_index src/static_web_resources/index.html)
# generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)

target_sources(app PRIVATE
	src/main.c
    src/sensors.c
)
