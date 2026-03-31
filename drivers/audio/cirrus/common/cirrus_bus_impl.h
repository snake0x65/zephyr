/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CIRRUS_BUS_IMPL_H_
#define ZEPHYR_INCLUDE_CIRRUS_BUS_IMPL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

enum cirrus_bus_type {
	CIRRUS_BUS_I2C,
	CIRRUS_BUS_SPI,
};

enum cirrus_endian {
	CIRRUS_BIG_ENDIAN,
	CIRRUS_LITTLE_ENDIAN,
};

struct cirrus_spi_format {
	uint8_t reg_bits;
	uint8_t pad_bits;
	uint8_t val_bits;
	enum cirrus_endian endian;
};

struct cirrus_bus_io {
	struct cirrus_spi_format fmt;
	void *bus;
	int (*reg_read)(struct cirrus_bus_io *bus, uint32_t reg, uint32_t *val);
	int (*reg_write)(struct cirrus_bus_io *bus, uint32_t reg, uint32_t val);
	int (*bulk_write)(struct cirrus_bus_io *bus,
			  const uint32_t *regs,
			  const uint32_t *vals,
			  size_t count);
};

/* i2c */
int cirrus_i2c_reg_read(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t *val);
int cirrus_i2c_reg_write(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t val);
int cirrus_i2c_bulk_write(struct cirrus_bus_io *bus_io,
			 const uint32_t *regs,
			 const uint32_t *vals,
			 size_t count);

/* spi */
int cirrus_spi_reg_read(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t *val);
int cirrus_spi_reg_write(struct cirrus_bus_io *bus_io, uint32_t reg, uint32_t val);
int cirrus_spi_bulk_write(struct cirrus_bus_io *bus_io,
				  const uint32_t *regs,
				  const uint32_t *vals,
				  size_t count);

#endif /* ZEPHYR_INCLUDE_CIRRUS_BUS_IMPL_H_ */