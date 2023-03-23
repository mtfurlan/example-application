#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C VERSION 1.0.0)

# TODO: I should not have to set defines like this to say what should be specified in the devicetree
#nrf52840 dk i2c NEO-M8U
#target_compile_definitions(app PRIVATE U_CFG_APP_GNSS_I2C=0 U_CFG_TEST_GNSS_MODULE_TYPE=U_GNSS_MODULE_TYPE_M8)
#nrf5340dk spi ZED-F9R
target_compile_definitions(app PRIVATE U_CFG_APP_GNSS_SPI=4 U_CFG_TEST_GNSS_MODULE_TYPE=U_GNSS_MODULE_TYPE_M9)

configure_file(app_version.h.in ${CMAKE_BINARY_DIR}/app/include/app_version.h)
target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include src)

target_sources(app PRIVATE src/main.c)
