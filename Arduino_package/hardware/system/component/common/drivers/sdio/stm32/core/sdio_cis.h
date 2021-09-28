#ifndef _MMC_SDIO_CIS_H
#define _MMC_SDIO_CIS_H

int sdio_read_common_cis(struct mmc_card *card);
void sdio_free_common_cis(struct mmc_card *card);

int sdio_read_func_cis(struct sdio_func *func);
void sdio_free_func_cis(struct sdio_func *func);
#endif
