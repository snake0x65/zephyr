/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include "cirrus_bus_impl.h"

int cirrus_spi_reg_read(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t *val)
{
	const struct spi_dt_spec *bus = (const struct spi_dt_spec *)bus_io->bus;
	uint8_t ds = (bus_io->fmt.reg_bits / 8) + (bus_io->fmt.pad_bits / 8) + (bus_io->fmt.val_bits / 8);
	uint8_t frame[24];
	int ret;

	struct spi_buf tx_bufs, rx_bufs;
	struct spi_buf_set tx_set, rx_set;

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
		if (bus_io->fmt.reg_bits == 32) {
			sys_put_le32(reg, frame);
		} else {
			sys_put_le16(reg, frame);
		}
		break;
	default:
		return -ENOTSUP;
	}

	tx_bufs.buf = frame;
	tx_bufs.len = ds;
	rx_bufs.buf = frame + ds;
	rx_bufs.len = ds;
	// We only do single transaction for now
	tx_set.buffers = &tx_bufs;
	tx_set.count = 1;
	rx_set.buffers = &rx_bufs,
	rx_set.count = 1,

	// cirrus spi read flag byte 0
	*frame |= 0x80;

	ret = spi_transceive_dt(bus, &tx_set, &rx_set);
	if (ret < 0) {
		return ret;
	}

	switch (bus_io->fmt.endian) {
	case CIRRUS_BIG_ENDIAN:
		if (bus_io->fmt.reg_bits == 32) {
			*val = sys_get_be32(frame + ds);
		} else {
			*val = sys_get_be16(frame + ds);
		}
		break;
	case CIRRUS_LITTLE_ENDIAN:
		if (bus_io->fmt.reg_bits == 32) {
			*val = sys_get_le32(frame + ds);
		} else {
			*val = sys_get_le16(frame + ds);
		}
		break;
	default:
		break;
	}

	return 0;
}

int cirrus_spi_reg_write(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t val)
{
	const struct spi_dt_spec *bus = (const struct spi_dt_spec *)bus_io->bus;
	uint8_t ds = (bus_io->fmt.reg_bits / 8) + (bus_io->fmt.pad_bits / 8);
	uint8_t frame[24];

	struct spi_buf tx_bufs;
	struct spi_buf_set tx_set;

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

	tx_bufs.buf = frame;
	tx_bufs.len = (bus_io->fmt.reg_bits / 8);
	// We only do single transaction for now
	tx_set.buffers = &tx_bufs;
	tx_set.count = 1;

	return spi_write_dt(bus, &tx_set);
}

int cirrus_spi_bulk_write(struct cirrus_bus_io *bus_io,
				  const uint32_t *regs,
				  const uint32_t *vals,
				  size_t count)
{
	int ret;
	for (size_t i = 0; i < count; i++) {
		ret = cirrus_spi_reg_write(bus_io, regs[i], vals[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
