/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>


#include "app_version.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define EEPROM_SAMPLE_OFFSET 0
#define EEPROM_SAMPLE_MAGIC  0xEE9703

struct perisistant_values {
    uint32_t magic;
    uint32_t boot_count;
};


int main(void)
{
    printk("Zephyr Example Application %s\n", APP_VERSION_STR);

    const struct device* const eeprom = DEVICE_DT_GET(DT_NODELABEL(eeprom_0));
    size_t eeprom_size;
    struct perisistant_values values;
    int ret;

    if(!device_is_ready(eeprom)) {
        printk(
                "Error: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                eeprom->name);
        return 1;
    }

    printk("Found EEPROM device \"%s\n", eeprom->name);

    if(eeprom == NULL) {
        printk("didn't find eeprom\n");
        return 1;
    }

    eeprom_size = eeprom_get_size(eeprom);
    printk("Using eeprom with size of: %zu.\n", eeprom_size);

    ret = eeprom_read(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
    if(ret != 0) {
        printk("eeprom_read failed: %d\n", ret);
        return 1;
    }

    if (values.magic != EEPROM_SAMPLE_MAGIC) {
        values.magic = EEPROM_SAMPLE_MAGIC;
        values.boot_count = 0;
    }

    printk("Device booted %d times.\n", values.boot_count);
    values.boot_count++;
    uint32_t written_boot = values.boot_count;

    ret = eeprom_write(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
    if(ret != 0) {
        printk("eeprom_write failed: %d\n", ret);
        return 1;
    }

    ret = eeprom_read(eeprom, EEPROM_SAMPLE_OFFSET, &values, sizeof(values));
    if(ret != 0) {
        printk("eeprom_read failed: %d\n", ret);
        return 1;
    }

    if(values.magic != EEPROM_SAMPLE_MAGIC) {
        printk("magic byte differs, expected %06X got %06X", EEPROM_SAMPLE_MAGIC, values.magic);
    }
    if(values.boot_count != written_boot) {
        printk("boot count differs, expected %d got %d", written_boot, values.boot_count);
    }
    return 0;
}
