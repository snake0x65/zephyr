/*
 * Copyright 2026 Cirrus Logic, Inc. and
 *                 Cirrus Logic International Semiconductor Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cs35l41.h"

LOG_MODULE_REGISTER(cs35l41, LOG_LEVEL_INF);

/* zephyr interfaces */

static void init_mutex(struct cs35l41_priv *cs35l41)
{
	k_mutex_init(&cs35l41->z_lock);
}

static int lock_mutex(struct cs35l41_priv *cs35l41)
{
	return k_mutex_lock(&cs35l41->z_lock, K_FOREVER);
}

static int unlock_mutex(struct cs35l41_priv *cs35l41)
{
	return k_mutex_unlock(&cs35l41->z_lock);
}

/* Ctrl bus API */
static int cs35l41_reg_read(struct cs35l41_priv *cs35l41, uint32_t reg, uint32_t *val)
{
	const struct cs35l41_config *cfg = cs35l41->cfg;
	int ret;

	lock_mutex(cs35l41);
	ret = cfg->bus_io->reg_read(cfg->bus_io, reg, val);
	unlock_mutex(cs35l41);

	return ret;
}

static int cs35l41_reg_write(struct cs35l41_priv *cs35l41, uint32_t reg, uint32_t val)
{
	const struct cs35l41_config *cfg = cs35l41->cfg;
	int ret;

	lock_mutex(cs35l41);
	ret = cfg->bus_io->reg_write(cfg->bus_io, reg, val);
	unlock_mutex(cs35l41);

	return ret;
}

__maybe_unused static int cs35l41_reg_update_bits(struct cs35l41_priv *cs35l41, uint32_t reg, uint32_t mask, uint32_t val)
{
	int ret = 0;
	uint32_t tmp, orig;

	ret = cs35l41_reg_read(cs35l41, reg, &orig);
	if (ret < 0)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;

	if (tmp != orig)
		ret = cs35l41_reg_write(cs35l41, reg, tmp);

	return ret;
}

__maybe_unused static int cs35l41_bulk_write(struct cs35l41_priv *cs35l41,
			const uint32_t *regs,
			const uint32_t *vals,
			size_t count)
{
	const struct cs35l41_config *cfg = cs35l41->cfg;
	int ret;

	lock_mutex(cs35l41);
	ret = cfg->bus_io->bulk_write(cfg->bus_io, regs, vals, count);
	unlock_mutex(cs35l41);

	return ret;
}

#define cs35l41_reg_read_poll_timeout(cs35l41, reg, val, cond, sleep_us, timeout_us)\
	({									    \
		uint32_t __timeout_tick = Z_TIMEOUT_US(timeout_us).ticks;	    \
		uint32_t __start = sys_clock_tick_get_32();			    \
		int __ret = 0;							    \
		for (;;) {							    \
			__ret = cs35l41_reg_read(cs35l41, reg, &val);		    \
			if (cond || (__ret < 0)) {				    \
				break;						    \
			}							    \
			if ((sys_clock_tick_get_32() - __start) > __timeout_tick) { \
				__ret = -ETIMEDOUT;				    \
				break;						    \
			}							    \
			if (sleep_us) {						    \
				k_usleep(sleep_us);				    \
			}							    \
		}								    \
		__ret;								    \
	})


__maybe_unused static int cs35l41_hw_reset(const struct cs35l41_priv *cs35l41)
{
	int ret = 0;

	k_msleep(5);

	return ret;
}

static int cs35l41_wait_boot_done(struct cs35l41_priv *cs35l41)
{
	unsigned int status;
	int ret;

	ret = cs35l41_reg_read_poll_timeout(cs35l41, CS35L41_IRQ1_STATUS4,
				       status, (status & CS35L41_OTP_BOOT_DONE),
				       CS35L41_GLOBAL_BOOT_DONE_SLEEP_US,
				       CS35L41_GLOBAL_BOOT_DONE_TIMEOUT_US);
	if (ret) {
		LOG_ERR("Failed waiting for GLOBAL_BOOT_DONE");
	}
	else {
		LOG_INF("(-wait-boot-done-)");
	}

	return ret;
}

static int cs35l41_check_device_id(struct cs35l41_priv *cs35l41)
{
	uint32_t devid, revid, chipid_match, mtl_revid;
	int ret;

	ret = cs35l41_wait_boot_done(cs35l41);
	if (ret) {
		return ret;
	}

	ret = cs35l41_reg_read(cs35l41, CS35L41_DEVID, &devid);
	if (ret) {
		LOG_ERR("Can't read device ID: %d\n", ret);
		return ret;
	}

	ret = cs35l41_reg_read(cs35l41, CS35L41_REVID, &revid);
	if (ret) {
		LOG_ERR("Can't read REV ID: %d\n", ret);
		return ret;
	}

	mtl_revid = (devid & CS35L41_MTLREVID_MASK);

	chipid_match = (mtl_revid % 2) ? CS35L41R_CHIP_ID : CS35L41_CHIP_ID;

	if (devid != chipid_match) {

		LOG_ERR("CS35L41 Device ID (%X). Expected ID %X",
			devid, chipid_match);

		return -ENODEV;
	}

	LOG_INF("Device ID 0x%x Rev %c found", devid,
		(((char)(revid & CS35L41_MTLREVID_MASK)) + 'A'));

	return 0;
}

static int cs35l41_hw_init(struct cs35l41_priv *cs35l41)
{
	int ret;

	LOG_INF("%s()", __func__);

	ret = cs35l41_check_device_id(cs35l41);
	if (ret) {
		return -ENODEV;
	}

	cs35l41->sysclk_hz = 12288000;
	cs35l41->sample_rate = 48000;

	LOG_INF("Codec initialized");

	return ret;
}

static int cs35l41_init(struct cs35l41_priv *cs35l41)
{
	const struct cs35l41_config *cfg = cs35l41->cfg;
	int ret;

	LOG_INF("%s() bus = %s", __func__,
		(cfg->bus_type==CIRRUS_BUS_I2C)?"CIRRUS_BUS_I2C":"CIRRUS_BUS_SPI");

	init_mutex(cs35l41);

	switch (cfg->bus_type) {
	case CIRRUS_BUS_I2C:
		if (!device_is_ready(cfg->bus.i2c.bus)) {
			LOG_ERR("I2C bus not ready");
			return -ENODEV;
		}
		break;

	case CIRRUS_BUS_SPI:
		if (!spi_is_ready_dt(&cfg->bus.spi)) {
			LOG_ERR("SPI bus not ready");
			return -ENODEV;
		}
		break;
	default:
		return -EINVAL;
	}

	// TODO: Reset GPIO

	LOG_INF("Soft reset");
	ret = cs35l41_reg_write(cs35l41, CS35L41_SFT_RESET, CS35L41_SOFTWARE_RESET);
	if (ret < 0) {
		LOG_ERR("Soft reset failed: %d", ret);
		return ret;
	}

	k_msleep(50);

	ret = cs35l41_hw_init(cs35l41);
	if (ret < 0) {
		LOG_ERR("Hardware init failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cs35l41_start(const struct device *dev, audio_dai_dir_t dir)
{
	__maybe_unused struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);
	return 0;
}


static int cs35l41_stop(const struct device *dev, audio_dai_dir_t dir)
{
	__maybe_unused struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);
	return 0;
}

int cs35l41_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	__maybe_unused struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);

	return 0;
}

static int cs35l41_apply_properties(const struct device *dev)
{
	__maybe_unused struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);

	return 0;
}


static int cs35l41_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	__maybe_unused struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);

	return 0;
}

const struct audio_codec_api cs35l41_api = {
	.configure = cs35l41_configure,
	.start = cs35l41_start,
	.stop = cs35l41_stop,
	.set_property = cs35l41_set_property,
	.apply_properties = cs35l41_apply_properties,
};

/*
 * The DEVICE_DT_INST_DEFINE() instances are created in bus-specific files
 * because each instance needs different config initialization.
 */
int cs35l41_common_init(const struct device *dev)
{
	struct cs35l41_priv *cs35l41 = dev->data;

	LOG_INF("%s()",__func__);

	cs35l41->dev = dev;
	cs35l41->cfg = dev->config;

	return cs35l41_init(cs35l41);
}