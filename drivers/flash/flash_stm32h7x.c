/*
 * Copyright (c) 2018 Aurelien Jarno
 * Copyright (c) 2018 Yong Jin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32h7
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"

#define STM32H7X_SECTOR_MASK		((uint32_t) 0xFFFFF8FF)
#define CR_PSIZE_MASK				((uint32_t) 0xFFFFFFCF)

/* offset and len must be aligned on 32 for write
 * , positive and not beyond end of flash */
bool flash_stm32_valid_range(struct device *dev, off_t offset, uint32_t len,
			     bool write)
{
	return (!write || (offset % 32 == 0 && len % 32 == 0U)) &&
		flash_stm32_range_exists(dev, offset, len);
}

static int write_flash_word(struct device *dev, off_t offset, uint32_t *val)
{
	volatile uint32_t *flash = (uint32_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int i;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR1 & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	for (i = 0; i < 8; i++) {
		if (flash[i] != 0xFFFFFFFF) {
			LOG_ERR("Word at offs %ld not erased", offset + i);
			return -EIO;
		}
	}

	/* prepare to write a single byte */
	regs->CR1 = (regs->CR1 & (~FLASH_CR_PSIZE)) |
		   FLASH_PSIZE_WORD | FLASH_CR_PG;
	/* flush the register write */
    __ISB();
	__DSB();

	/* write the data */
	for (i = 0; i < 8; i++) {
		flash[i] = *(val + i);
	}

	/* flush the register write */
    __ISB();
	__DSB();

	rc = flash_stm32_wait_flash_idle(dev);
	regs->CR1 &= (~FLASH_CR_PG);

	return rc;
}

static int erase_sector(struct device *dev, uint32_t sector)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR1 & FLASH_CR_LOCK) {
		return -EIO;
	}

	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	regs->CR1 &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);
	regs->CR1 |= FLASH_CR_SER | FLASH_PSIZE_WORD | (sector << FLASH_CR_SNB_Pos);
	regs->CR1 |= FLASH_CR_START;


	/* flush the register write */
    __ISB();
	__DSB();

	rc = flash_stm32_wait_flash_idle(dev);
	regs->CR1 &= ~(FLASH_CR_SER | FLASH_CR_SNB);

	return rc;
}

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len)
{
	struct flash_pages_info info;
	uint32_t start_sector, end_sector;
	uint32_t i;
	int rc = 0;

	rc = flash_get_page_info_by_offs(dev, offset, &info);
	if (rc) {
		return rc;
	}
	start_sector = info.index;

	rc = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (rc) {
		return rc;
	}
	end_sector = info.index;

	for (i = start_sector; i <= end_sector; i++) {
		rc = erase_sector(dev, i);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}

int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 32, offset += 32) {
		rc = write_flash_word(dev, offset, ((uint8_t *)data) + i);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

void flash_stm32_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (flash_layout.pages_count == 0) {
		flash_layout.pages_count = (CONFIG_FLASH_SIZE * 1024) /
			FLASH_SECTOR_SIZE;
		flash_layout.pages_size = FLASH_SECTOR_SIZE;
	}

	*layout = &flash_layout;
	*layout_size = 1;
}

int flash_stm32_check_status(struct device *dev)
{
	uint32_t const bank1_error =
#if defined (FLASH_SR_OPERR)
		FLASH_FLAG_OPERR_BANK1 |
#endif
		FLASH_FLAG_WRPERR_BANK1 |
		FLASH_FLAG_PGSERR_BANK1 |
		FLASH_FLAG_STRBERR_BANK1 |
		FLASH_FLAG_INCERR_BANK1;

	if (FLASH_STM32_REGS(dev)->SR1 & bank1_error) {
		LOG_DBG("Status: 0x%08x", FLASH_STM32_REGS(dev)->SR1 & error);
		return -EIO;
	}

	return 0;
}
