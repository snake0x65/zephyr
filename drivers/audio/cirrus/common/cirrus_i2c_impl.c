/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include "cirrus_bus_impl.h"


int cirrus_i2c_reg_read(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t *val)
{
	const struct i2c_dt_spec *bus = (const struct i2c_dt_spec *)bus_io->bus;
	uint8_t ds = (bus_io->fmt.reg_bits / 8) + (bus_io->fmt.pad_bits / 8);
	uint8_t frame[12];
	int ret;

	memset(frame, 0, sizeof(frame));

	// Limiting to 16 or 32 bits for now
	switch (bus_io->fmt.endian) {
	case CIRRUS_BIG_ENDIAN:
		if (bus_io->fmt.reg_bits == 32) {
			sys_put_be32(reg, frame);
		} else {
			sys_put_be16(reg, frame);
		}
		break;
	case CIRRUS_LITTLE_ENDIAN:
		if (bus_io->fmt.reg_bits == 32)
			sys_put_le32(reg, frame);
		else
			sys_put_le16(reg, frame);
		break;
	default:
		return -ENOTSUP;
	}

	ret = i2c_write_read_dt(bus, frame, (bus_io->fmt.reg_bits / 8),
				frame + ds, (bus_io->fmt.val_bits / 8));
	if (ret) {
		return ret;
	}

	*val = sys_get_be32(frame + ds);

	return 0;
}

int cirrus_i2c_reg_write(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t val)
{
	const struct i2c_dt_spec *bus = (const struct i2c_dt_spec *)bus_io->bus;
	uint8_t frame[12];
	uint8_t ds = (bus_io->fmt.reg_bits / 8) + (bus_io->fmt.pad_bits / 8);

	memset(frame, 0, sizeof(frame));

	// Limiting to 16 or 32 bits for now
	switch (bus_io->fmt.endian) {
	case CIRRUS_BIG_ENDIAN:
		if (bus_io->fmt.reg_bits == 32) {
			sys_put_be32(reg, frame);
			sys_put_be32(val, frame + ds);
		} else {
			sys_put_be16(reg, frame);
			sys_put_be16(val, frame + ds);
		}
		break;
	case CIRRUS_LITTLE_ENDIAN:
		if (bus_io->fmt.reg_bits == 32) {
			sys_put_le32(reg, frame);
			sys_put_le32(val, frame + ds);
		} else {
			sys_put_le16(reg, frame);
			sys_put_le16(val, frame + ds);
		}
		break;
	default:
		return -ENOTSUP;
	}

	ds += (bus_io->fmt.val_bits / 8);

	return i2c_write_dt(bus, frame, ds);
}

int cirrus_i2c_bulk_write(struct cirrus_bus_io *bus_io,
			 const uint32_t *regs,
			 const uint32_t *vals,
			 size_t count)
{
	int ret = 0;
	for (size_t i = 0; i < count; i++) {
		ret = cirrus_i2c_reg_write(bus_io, regs[i], vals[i]);
		if (ret < 0) {
			return ret;
		}
	}
	return ret;
}
