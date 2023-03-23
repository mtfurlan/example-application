# ubxlib spi issue demonstrator
I'm trying to use ubxlib with two boards:
* nRF52840 dk
  * i2c connected NEO-M8U
    * SDA: P0.04
    * SCL: P0.03
* nRF5340 dk
  * spi connected ZED-F9R
    * MOSI: P0.20
    * MISO: P0.31
    * CLK:  P0.19

I configured the interfaces with overlays as shown by the ubxlib runner examples.

I2C on the nRF52840 works, but SPI on the nRF5340 just gives me
```
U_GNSS: initialising with ENABLE_POWER pin not connected, transport type SPI.
U_GNSS: sent command b5 62 0a 06 00 00 10 3a.
Opened device with return code -8.
Unable to open GNSS!
```

The `app-spi` application demonstrates that you can use the the ublox as spi device `compatible = "vnd,spi-device"`,
so the spi bus definitely works.

I *think* this is the correct way to do the config for spi?
```
    .transportCfg = {
        .cfgSpi = {
            .spi = U_CFG_APP_GNSS_SPI,
            // pins *must* be negative, in ubxlib/port/platform/zephyr/src/u_port_spi.c uPortSpiOpen it says
            // > On Zephyr the pins are set at compile time so those passed
            // > into here must be non-valid
            .pinMosi = -1,
            .pinMiso = -1,
            .pinClk = -1,
            // not really sure about the controller though
            .device = U_COMMON_SPI_CONTROLLER_DEVICE_DEFAULTS(-1)
        },
    },
```

## Building
setup example as described below in original readme
### nRF52840
Ensure that `app/CMakeLists.txt` has the right defines for i2c/NEO-M8U
```sh
west build --pristine -b nrf52840dk_nrf52840 app
west flash
```
### nRF5340
Ensure that `app/CMakeLists.txt` has the right defines for spi/ZED-F9R
```sh
west build --pristine -b nrf5340dk_nrf5340_cpuapp app
west flash
```


## Current Questions
* How do I make ubxlib spi work?
* Having to set defines like `U_CFG_APP_GNSS_I2C=0 U_CFG_TEST_GNSS_MODULE_TYPE=U_GNSS_MODULE_TYPE_M8` instead of
  configuring in the devicetree is very confusing, and makes targeting multiple boards very difficult.
  * Am I just doing this wrong?
* In `ubxlib/port/platform/zephyr/src/u_port_spi.c` `uPortSpiOpen` it calls `DEVICE_DT_GET_OR_NULL(DT_NODELABEL(spiX))`
  for spi0 to spi4, regardless of if those are defined.
  * This causes build issues when not using spi.


---
unmodified zephyr example application follows

# Zephyr Example Application

This repository contains a Zephyr example application. The main purpose of this
repository is to serve as a reference on how to structure Zephyr-based
applications. Some of the features demonstrated in this example are:

- Basic [Zephyr application][app_dev] skeleton
- [Zephyr workspace applications][workspace_app]
- [West T2 topology][west_t2]
- [Custom boards][board_porting]
- Custom [devicetree bindings][bindings]
- Out-of-tree [drivers][drivers]
- Out-of-tree libraries
- Example CI configuration (using Github Actions)
- Custom [west extension][west_ext]

This repository is versioned together with the [Zephyr main tree][zephyr]. This
means that every time that Zephyr is tagged, this repository is tagged as well
with the same version number, and the [manifest](west.yml) entry for `zephyr`
will point to the corresponding Zephyr tag. For example, the `example-application`
v2.6.0 will point to Zephyr v2.6.0. Note that the `main` branch always
points to the development branch of Zephyr, also `main`.

[app_dev]: https://docs.zephyrproject.org/latest/develop/application/index.html
[workspace_app]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app
[west_t2]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html#west-t2
[board_porting]: https://docs.zephyrproject.org/latest/guides/porting/board_porting.html
[bindings]: https://docs.zephyrproject.org/latest/guides/dts/bindings.html
[drivers]: https://docs.zephyrproject.org/latest/reference/drivers/index.html
[zephyr]: https://github.com/zephyrproject-rtos/zephyr
[west_ext]: https://docs.zephyrproject.org/latest/develop/west/extensions.html

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (``my-workspace``) where
the ``example-application`` and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for the example-application (main branch)
west init -m https://github.com/zephyrproject-rtos/example-application --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

### Building and running

To build the application, run the following command:

```shell
west build -b $BOARD app
```

where `$BOARD` is the target board.

You can use the `custom_plank` board found in this
repository. Note that Zephyr sample boards may be used if an
appropriate overlay is provided (see `app/boards`).

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD app -- -DOVERLAY_CONFIG=debug.conf
```

You can also use it together with the `rtt.conf` file if using Segger RTT. Once
you have built the application, run the following command to flash it:

```shell
west flash
```
