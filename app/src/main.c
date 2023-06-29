/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "app_version.h"
#include "ubxlib.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

// This is mostly directly taken from ubxlib/example/gnss/pos_main.c

static const uDeviceCfg_t gDeviceCfg = {
    .deviceType = U_DEVICE_TYPE_GNSS,
    .deviceCfg = {
        .cfgGnss = {
            .moduleType = U_CFG_TEST_GNSS_MODULE_TYPE,
            .pinEnablePower = -1,
            .pinDataReady = -1, // Not used
            // There is an additional field here:
            // "i2cAddress", which we do NOT set,
            // we allow the compiler to set it to 0
            // and all will be fine. You may set the
            // field to the I2C address of your GNSS
            // device if you have modified the I2C
            // address of your GNSS device to something
            // other than the default value of 0x42,
            // for example:
            // .i2cAddress = 0x43
        },
    },
//In the ubxlib example they don't check defined, but that didn't work for me.
#if defined(U_CFG_APP_GNSS_I2C) &&  (U_CFG_APP_GNSS_I2C >= 0)
    .transportType = U_DEVICE_TRANSPORT_TYPE_I2C,
    .transportCfg = {
        .cfgI2c = {
            .i2c = U_CFG_APP_GNSS_I2C,
            //.pinSda = U_CFG_APP_PIN_GNSS_SDA,
            //.pinScl = U_CFG_APP_PIN_GNSS_SCL
            // There two additional fields here
            // "clockHertz" amd "alreadyOpen", which
            // we do NOT set, we allow the compiler
            // to set them to 0 and all will be fine.
            // You may set clockHertz if you want the
            // I2C bus to use a different clock frequency
            // to the default of
            // #U_PORT_I2C_CLOCK_FREQUENCY_HERTZ, for
            // example:
            // .clockHertz = 400000
            // You may set alreadyOpen to true if you
            // are already using this I2C HW block,
            // with the native platform APIs,
            // elsewhere in your application code,
            // and you would like the ubxlib code
            // to use the I2C HW block WITHOUT
            // [re]configuring it, for example:
            .alreadyOpen = true
            // if alreadyOpen is set to true then
            // pinSda, pinScl and clockHertz will
            // be ignored.
        },
    },
#endif
#if defined(U_CFG_APP_GNSS_SPI) &&  (U_CFG_APP_GNSS_SPI >= 0)
    .transportType = U_DEVICE_TRANSPORT_TYPE_SPI,
    .transportCfg = {
        .cfgSpi = {
            .spi = U_CFG_APP_GNSS_SPI,
            // pins *must* be negative, in ubxlib/port/platform/zephyr/src/u_port_spi.c uPortSpiOpen it says
            // > On Zephyr the pins are set at compile time so those passed
            // > into here must be non-valid
            .pinMosi = -1, // U_CFG_APP_PIN_GNSS_SPI_MOSI,
            .pinMiso = -1, // U_CFG_APP_PIN_GNSS_SPI_MISO,
            .pinClk = -1, // U_CFG_APP_PIN_GNSS_SPI_CLK,
            // I don't really know how this works
            .device = U_COMMON_SPI_CONTROLLER_DEVICE_INDEX_DEFAULTS(0) // U_CFG_APP_PIN_GNSS_SPI_SELECT
            //.device = U_COMMON_SPI_CONTROLLER_DEVICE_DEFAULTS(32+14)
        },
    },
#  endif
};

// Count of the number of position fixes received
static size_t gPositionCount = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

// Convert a lat/long into a whole number and a bit-after-the-decimal-point
// that can be printed by a version of printf() that does not support
// floating point operations, returning the prefix (either "+" or "-").
// The result should be printed with printf() format specifiers
// %c%d.%07d, e.g. something like:
//
// int32_t whole;
// int32_t fraction;
//
// printf("%c%d.%07d/%c%d.%07d", latLongToBits(latitudeX1e7, &whole, &fraction),
//                               whole, fraction,
//                               latLongToBits(longitudeX1e7, &whole, &fraction),
//                               whole, fraction);
static char latLongToBits(int32_t thingX1e7, int32_t* pWhole, int32_t* pFraction)
{
    char prefix = '+';

    // Deal with the sign
    if (thingX1e7 < 0) {
        thingX1e7 = -thingX1e7;
        prefix = '-';
    }
    *pWhole = thingX1e7 / 10000000;
    *pFraction = thingX1e7 % 10000000;

    return prefix;
}

// Callback for position reception.
static void callback(uDeviceHandle_t gnssHandle,
                     int32_t errorCode,
                     int32_t latitudeX1e7,
                     int32_t longitudeX1e7,
                     int32_t altitudeMillimetres,
                     int32_t radiusMillimetres,
                     int32_t speedMillimetresPerSecond,
                     int32_t svs,
                     int64_t timeUtc)
{
    char prefix[2] = { 0 };
    int32_t whole[2] = { 0 };
    int32_t fraction[2] = { 0 };

    // Not using these, just keep the compiler happy
    (void)gnssHandle;
    (void)altitudeMillimetres;
    (void)radiusMillimetres;
    (void)speedMillimetresPerSecond;
    (void)svs;
    (void)timeUtc;

    if (errorCode == 0) {
        prefix[0] = latLongToBits(longitudeX1e7, &(whole[0]), &(fraction[0]));
        prefix[1] = latLongToBits(latitudeX1e7, &(whole[1]), &(fraction[1]));
        uPortLog("I am here: https://maps.google.com/?q=%c%d.%07d,%c%d.%07d\n",
                 prefix[1],
                 whole[1],
                 fraction[1],
                 prefix[0],
                 whole[0],
                 fraction[0]);
        gPositionCount++;
    }
}

void main(void)
{
    printk("Zephyr Example Application %s\n", APP_VERSION_STR);

    uDeviceHandle_t devHandle = NULL;
    int32_t returnCode;
    int32_t guardCount = 0;

    // Initialise the APIs we will need
    uPortInit();
#ifdef U_CFG_APP_GNSS_I2C
    uPortI2cInit(); // You only need this if an I2C interface is used
#endif
#ifdef U_CFG_APP_GNSS_SPI
    uPortSpiInit(); // You only need this if an SPI interface is used
#endif
    uDeviceInit();

    // Open the device
    returnCode = uDeviceOpen(&gDeviceCfg, &devHandle);
    uPortLog("Opened device with return code %d.\n", returnCode);

    if (returnCode == 0) {
        // Since we are not using the common APIs we do not need
        // to call uNetworkInteraceUp()/uNetworkInteraceDown().

        // Start to get position
        uPortLog("Starting position stream.\n");
        returnCode = uGnssPosGetStreamedStart(devHandle,
                                              U_GNSS_POS_STREAMED_PERIOD_DEFAULT_MS,
                                              callback);
        if (returnCode == 0) {
            uPortLog("Waiting up to 60 seconds for 5 position fixes.\n");
            while ((gPositionCount < 5) && (guardCount < 60)) {
                uPortTaskBlock(1000);
                guardCount++;
            }
            // Stop getting position
            uGnssPosGetStreamedStop(devHandle);

        } else {
            uPortLog("Unable to start position stream!\n");
        }

    } else {
        uPortLog("Unable to open GNSS!\n");
    }

    // Close the device
    // Note: we don't power the device down here in order
    // to speed up testing; you may prefer to power it off
    // by setting the second parameter to true.
    uDeviceClose(devHandle, false);

    // Tidy up
    uDeviceDeinit();
#ifdef U_CFG_APP_GNSS_I2C
    uPortI2cDeinit(); // You only need this if an I2C interface is used
#endif
#ifdef U_CFG_APP_GNSS_SPI
    uPortSpiDeinit(); // You only need this if an SPI interface is used
#endif
    uPortDeinit();

    uPortLog("Done.\n");
}
