/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#include "app_version.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define SPI_OP SPI_OP_MODE_MASTER | SPI_WORD_SET(8)
struct spi_dt_spec spi_ublox = SPI_DT_SPEC_GET(DT_NODELABEL(ublox), SPI_OP, 0);


#define READ_BUF_SIZE 0x100
static bool get_version(const struct spi_dt_spec* spec)
{
    int ret;
    uint8_t s_buf[] = { 0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34 };
    uint8_t r_buf[READ_BUF_SIZE];
    uint8_t ubx_packet[READ_BUF_SIZE];
    size_t ubx_packet_index = 0;


    struct spi_buf tx_buf = {
        .buf = s_buf,
        .len = ARRAY_SIZE(s_buf),
    };
    const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1U };
    const struct spi_buf rx_buf = {
        .buf = r_buf,
        .len = ARRAY_SIZE(r_buf),
    };
    const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1U };

    bool found_response = false;
    bool complete_response = false;
    size_t attempts = 0;
    do {
        ++attempts;
        ret = spi_transceive_dt(spec, &tx, &rx);
        tx_buf.len = 0; // don't send anything after the first time
        if(ret != 0) {
            LOG_ERR("spi_transceive_dt return nonzero");
        }
        size_t i = 0;
        if (!found_response) {
            for (i = 0; i < READ_BUF_SIZE - 1; ++i) {
                if (r_buf[i] == 0xb5 && r_buf[i + 1] == 0x62) {
                    LOG_INF("found first byte");
                    found_response = true;
                    break;
                }
            }
        }
        if (found_response) {
            LOG_INF("processing...");
            // in the middle of a packet
            if (ubx_packet_index <= 6) {
                memcpy(ubx_packet + ubx_packet_index, r_buf + i, 6 - ubx_packet_index);
                i += 6 - ubx_packet_index;
                ubx_packet_index += 6 - ubx_packet_index;
            }
            // we now have at least 5 bytes, and can get size
            // calculate full packet size (including header, size, and checksum)
            size_t len = ((ubx_packet[5] << 8) | ubx_packet[4]) + 8;

            // if we have less to go then full read buf
            if (len - ubx_packet_index < READ_BUF_SIZE - i) {
                memcpy(ubx_packet + ubx_packet_index, r_buf + i, len - ubx_packet_index);
                ubx_packet_index += len - ubx_packet_index;
                complete_response = true;
            } else {
                memcpy(ubx_packet + ubx_packet_index, r_buf + i, READ_BUF_SIZE - i);
                ubx_packet_index += READ_BUF_SIZE - i;
            }
        } else {
            LOG_INF("waiting...");
        }
    } while (attempts < 15 && !complete_response);
    // we should have a full packet now?
    LOG_HEXDUMP_INF(ubx_packet, ubx_packet_index, "ubx mon ver packet");

    return complete_response;
}

void main(void)
{
    printk("Zephyr Example Application %s\n", APP_VERSION_STR);
    while (!spi_is_ready_dt(&spi_ublox)) {
        k_msleep(5);
    }
    LOG_INF("spi is ready");

    bool found_version = get_version(&spi_ublox);
    if(found_version != true) {
        LOG_ERR("didn't find version packet");
    }

}
