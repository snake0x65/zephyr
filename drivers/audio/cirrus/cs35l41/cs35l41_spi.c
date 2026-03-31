/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT cirrus_cs35l41_spi

#include "cs35l41.h"

static int cs35l41_spi_init(const struct device *dev)
{
	const struct cs35l41_config *cfg = dev->config;

	cfg->bus_io->bus = (void *)&cfg->bus.spi;

	return cs35l41_common_init(dev);
}

__maybe_unused static struct cirrus_bus_io cs35l41_spi_bus_io = {
	.fmt = {
		.reg_bits = 32,
		.pad_bits  = 32,
		.val_bits  = 32,
		.endian = CIRRUS_BIG_ENDIAN,
	},
	.reg_read = cirrus_spi_reg_read,
	.reg_write = cirrus_spi_reg_write,
	.bulk_write = cirrus_spi_bulk_write,
};

#define cs35l41_SPI_INIT(inst) \
	static struct cs35l41_priv cs35l41_data_##inst; \
	static const struct cs35l41_config cs35l41_config_##inst = { \
		.bus_type = CIRRUS_BUS_SPI, \
		.bus_io = &cs35l41_spi_bus_io, \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, \
			   SPI_WORD_SET(8) | SPI_TRANSFER_MSB), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, \
			      cs35l41_spi_init, \
			      NULL, \
			      &cs35l41_data_##inst, \
			      &cs35l41_config_##inst, \
			      POST_KERNEL, \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
			      &cs35l41_api);

DT_INST_FOREACH_STATUS_OKAY(cs35l41_SPI_INIT)