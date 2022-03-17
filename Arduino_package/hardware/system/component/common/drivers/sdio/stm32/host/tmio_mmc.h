/*
 * linux/drivers/mmc/host/tmio_mmc.h
 *
 * Copyright (C) 2007 Ian Molton
 * Copyright (C) 2004 Ian Molton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for the MMC / SD / SDIO cell found in:
 *
 * TC6393XB TC6391XB TC6387XB T7L66XB ASIC3
 */

#ifndef TMIO_MMC_H
#define TMIO_MMC_H

#define CTL_SD_CMD 0x00
#define CTL_ARG_REG 0x04
#define CTL_STOP_INTERNAL_ACTION 0x08
#define CTL_XFER_BLK_COUNT 0xa
#define CTL_RESPONSE 0x0c

#define CTL_STATUS 0x1c
#define SDHI_INFO1 0x1c
#define SDHI_INFO2 0x1e

#define CTL_IRQ_MASK 0x20
#define CTL_SD_CARD_CLK_CTL 0x24
#define CTL_SD_XFER_LEN 0x26
#define CTL_SD_MEM_CARD_OPT 0x28
#define CTL_SD_ERROR_DETAIL_STATUS 0x2c
#define CTL_SD_DATA_PORT 0x30
#define CTL_TRANSACTION_CTL 0x34
#define CTL_SDIO_STATUS 0x36
#define CTL_SDIO_IRQ_MASK 0x38
#define CTL_RESET_SD 0xe0
#define CTL_SDIO_REGS 0x100
#define CTL_CLK_AND_WAIT_CTL 0x138
#define CTL_RESET_SDIO 0x1e0

/* Definitions for values the CTRL_STATUS register can take. */
#define TMIO_STAT_CMDRESPEND    0x00000001
#define TMIO_STAT_DATAEND       0x00000004
#define TMIO_STAT_CARD_REMOVE   0x00000008
#define TMIO_STAT_CARD_INSERT   0x00000010
#define TMIO_STAT_SIGSTATE      0x00000020
#define TMIO_STAT_WRPROTECT     0x00000080
#define TMIO_STAT_CARD_REMOVE_A 0x00000100
#define TMIO_STAT_CARD_INSERT_A 0x00000200
#define TMIO_STAT_SIGSTATE_A    0x00000400
#define TMIO_STAT_CMD_IDX_ERR   0x00010000
#define TMIO_STAT_CRCFAIL       0x00020000
#define TMIO_STAT_STOPBIT_ERR   0x00040000
#define TMIO_STAT_DATATIMEOUT   0x00080000
#define TMIO_STAT_RXOVERFLOW    0x00100000
#define TMIO_STAT_TXUNDERRUN    0x00200000
#define TMIO_STAT_CMDTIMEOUT    0x00400000
#define TMIO_STAT_RXRDY         0x01000000
#define TMIO_STAT_TXRQ          0x02000000
#define TMIO_STAT_ILL_FUNC      0x20000000
#define TMIO_STAT_CMD_BUSY      0x40000000
#define TMIO_STAT_ILL_ACCESS    0x80000000

#define TMIO_BBS		512		/* Boot block size */

/* Definitions for values the CTRL_SDIO_STATUS register can take. */
#define TMIO_SDIO_STAT_IOIRQ	0x0001
#define TMIO_SDIO_STAT_EXPUB52	0x4000
#define TMIO_SDIO_STAT_EXWT	0x8000
#define TMIO_SDIO_MASK_ALL	0xc007

/* Define some IRQ masks */
/* This is the mask used at reset by the chip */
#define TMIO_MASK_ALL           0x837f031d
#define TMIO_MASK_READOP  (TMIO_STAT_RXRDY | TMIO_STAT_DATAEND)
#define TMIO_MASK_WRITEOP (TMIO_STAT_TXRQ | TMIO_STAT_DATAEND)
#define TMIO_MASK_CMD     (TMIO_STAT_CMDRESPEND | TMIO_STAT_CMDTIMEOUT | \
		TMIO_STAT_CARD_REMOVE | TMIO_STAT_CARD_INSERT)
#define TMIO_MASK_IRQ     (TMIO_MASK_READOP | TMIO_MASK_WRITEOP | TMIO_MASK_CMD)

struct tmio_mmc_ops {
	/* Callbacks for clock / power control */
	void (*set_pwr)(int state);
	void (*set_clk_div)(int state);
	int (*get_cd)(void);
};

struct tmio_mmc_dma {
	void *chan_priv_tx;
	void *chan_priv_rx;
	int alignment_shift;
};

/* tmio MMC platform flags */
#define TMIO_MMC_WRPROTECT_DISABLE	(1 << 0)
/*
 * Some controllers can support a 2-byte block size when the bus width
 * is configured in 4-bit mode.
 */
#define TMIO_MMC_BLKSZ_2BYTES		(1 << 1)
/*
 * Some controllers can support SDIO IRQ signalling.
 */
#define TMIO_MMC_SDIO_IRQ		(1 << 2)
/*
 * Some platforms can detect card insertion events with controller powered
 * down, in which case they have to call tmio_mmc_cd_wakeup() to power up the
 * controller and report the event to the driver.
 */
#define TMIO_MMC_HAS_COLD_CD		(1 << 3)

struct tmio_mmc_host {
	void __iomem *ctl;
	unsigned long bus_shift;
	struct mmc_command      *cmd;
	struct mmc_request      *mrq;
	struct mmc_data         *data;
	struct mmc_host         *mmc;
	unsigned int		sdio_irq_enabled;

	const struct tmio_mmc_ops *ops;	/* Low level hw interface */

	/* pio related stuff */
	struct scatterlist      *sg_ptr;
	struct scatterlist      *sg_orig;
	unsigned int            sg_len;
	unsigned int            sg_off;

	/*
 	* data for the MMC controller
 	*/
	unsigned long flags;

	/* DMA support */
	bool			force_pio;
#ifdef CONFIG_TMIO_DMA
	rtw_dma_chan		*chan_rx;
	rtw_dma_chan		*chan_tx;
	struct tasklet_struct	dma_complete;
	struct tasklet_struct	dma_issue;
	struct scatterlist	bounce_sg;
	u8			*bounce_buf;
#endif

	/* Track lost interrupts */
	//struct workqueue_struct *work;
	//struct work_struct	done;
	struct timer_list	timer;	/* Timer for timeouts */

	rtw_spinlock_t		lock;		/* protect host private data */
	unsigned long		last_req_ts;
	rtw_mutex		ios_lock;	/* protect set_ios() context */
};

int tmio_mmc_host_probe(struct tmio_mmc_host *host);
void tmio_mmc_host_remove(struct tmio_mmc_host *host);
void tmio_mmc_do_data_irq(struct tmio_mmc_host *host);

void tmio_mmc_enable_mmc_irqs(struct tmio_mmc_host *host, u32 i);
void tmio_mmc_disable_mmc_irqs(struct tmio_mmc_host *host, u32 i);
irqreturn_t tmio_mmc_irq(int irq, void *devid);

static inline char *tmio_mmc_kmap_atomic(struct scatterlist *sg,
					 unsigned long *flags)
{
#ifdef CONFIG_ARCH_SHMOBILE
	local_irq_save(*flags);
	return kmap_atomic(sg_page(sg), KM_BIO_SRC_IRQ) + sg->offset;
#else
	return (char *)sg->dma_address;
#endif
}

static inline void tmio_mmc_kunmap_atomic(struct scatterlist *sg,
					  unsigned long *flags, void *virt)
{
#ifdef CONFIG_ARCH_SHMOBILE
	kunmap_atomic(virt - sg->offset, KM_BIO_SRC_IRQ);
	local_irq_restore(*flags);
#else
#endif
}

void tmio_mmc_start_dma(struct tmio_mmc_host *host, struct mmc_data *data);
void tmio_mmc_request_dma(struct tmio_mmc_host *host);
void tmio_mmc_release_dma(struct tmio_mmc_host *host);

irqreturn_t tmio_mmc_irq(int irq, void *devid);
struct mmc_host *tmio_alloc_host(struct device *dev, void* priv);

#endif
