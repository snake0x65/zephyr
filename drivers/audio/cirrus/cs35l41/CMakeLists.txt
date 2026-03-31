# Copyright 2026 Cirrus Logic, Inc. and
#                  Cirrus Logic International Semiconductor Ltd.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AUDIO_CS35L41 cs35l41.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_CS35L41_I2C cs35l41_i2c.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_CS35L41_SPI cs35l41_spi.c)
