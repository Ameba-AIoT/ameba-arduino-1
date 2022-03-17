#ifndef _MMC_SDIO_OPS_H
#define _MMC_SDIO_OPS_H

int sdio_reset(struct mmc_host *host);
int mmc_io_rw_direct(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, u8 in, u8 *out);
int mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz);
int mmc_send_io_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
#endif
