/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <dw3000.h>

#include <app_version.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
    int err;
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    dw3000_hw_init();
    dw3000_hw_reset();
    dw3000_hw_init_interrupt();
    dw3000_spi_speed_fast();
    k_msleep(2);


    // cast to disgard const
    if((err = dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf)) != DWT_SUCCESS) {
        LOG_ERR("failed to dwt_probe: %d", err);
        return 1;
    }

    uint32_t dev_id = dwt_readdevid();
    LOG_INF("dev id: 0x%04X", dev_id);

    /* Reads and validate device ID returns DWT_ERROR if it does not match expected else DWT_SUCCESS */
    if ((err = dwt_check_dev_id()) != DWT_SUCCESS) {
        LOG_ERR("DEV ID FAILED %d", err);
        return 1;
    }
    LOG_INF("DEV ID OK");

    // this causes
    // .../zephyr-dw3000-decadriver/platform/deca_compat.c:2571: undefined reference to `ull_getframelength'
    uint8_t foo = 5;
    dwt_getframelength(&foo);

    return 0;
}

