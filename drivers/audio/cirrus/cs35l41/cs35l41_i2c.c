/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs35l41_i2c

#include "cs35l41.h"

static int cs35l41_i2c_init(const struct device *dev)
{
	const struct cs35l41_config *cfg = dev->config;

	cfg->bus_io->bus = (void *) &cfg->bus.i2c;

	return cs35l41_common_init(dev);
}

__maybe_unused struct cirrus_bus_io cs35l41_i2c_bus_io = {
	.fmt = {
		.reg_bits = 32,
		.pad_bits  = 0,
		.val_bits  = 32,
		.endian = CIRRUS_BIG_ENDIAN,
	},
	.reg_read = cirrus_i2c_reg_read,
	.reg_write = cirrus_i2c_reg_write,
	.bulk_write = cirrus_i2c_bulk_write,
};

#define cs35l41_I2C_INIT(inst) \
	static struct cs35l41_priv cs35l41_data_##inst; \
	static const struct cs35l41_config cs35l41_config_##inst = { \
		.bus_type = CIRRUS_BUS_I2C, \
		.bus_io = &cs35l41_i2c_bus_io, \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, \
			      cs35l41_i2c_init, \
			      NULL, \
			      &cs35l41_data_##inst, \
			      &cs35l41_config_##inst, \
			      POST_KERNEL, \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
			      &cs35l41_api);

DT_INST_FOREACH_STATUS_OKAY(cs35l41_I2C_INIT)