# Copyright 2026 Cirrus Logic, Inc. and
#                  Cirrus Logic International Semiconductor Ltd.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AUDIO_CIRRUS_I2C cirrus_i2c_impl.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_CIRRUS_SPI cirrus_spi_impl.c)
