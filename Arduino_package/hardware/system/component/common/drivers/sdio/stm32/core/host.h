#ifndef _MMC_SDIO_HOST_H
#define _MMC_SDIO_HOST_H

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

#define MMC_CAP_4_BIT_DATA	(1 << 0)	/* Can the host do 4 bit transfers */
#define MMC_CAP_MMC_HIGHSPEED	(1 << 1)	/* Can do MMC high-speed timing */
#define MMC_CAP_SD_HIGHSPEED	(1 << 2)	/* Can do SD high-speed timing */
#define MMC_CAP_SDIO_IRQ	(1 << 3)	/* Can signal pending SDIO IRQs */
#define MMC_CAP_SPI		(1 << 4)	/* Talks only SPI protocols */
#define MMC_CAP_NEEDS_POLL	(1 << 5)	/* Needs polling for card-detection */
#define MMC_CAP_8_BIT_DATA	(1 << 6)	/* Can the host do 8 bit transfers */
#define MMC_CAP_DISABLE		(1 << 7)	/* Can the host be disabled */
#define MMC_CAP_NONREMOVABLE	(1 << 8)	/* Nonremovable e.g. eMMC */
#define MMC_CAP_WAIT_WHILE_BUSY	(1 << 9)	/* Waits while card is busy */
#define MMC_CAP_ERASE		(1 << 10)	/* Allow erase/trim commands */
#define MMC_CAP_1_8V_DDR	(1 << 11)	/* can support  DDR mode at 1.8V */
#define MMC_CAP_1_2V_DDR	(1 << 12)	/* can support  DDR mode at 1.2V */
#define MMC_CAP_POWER_OFF_CARD	(1 << 13)	/* Can power off after boot */
#define MMC_CAP_BUS_WIDTH_TEST	(1 << 14)	/* CMD14/CMD19 bus width ok */
#define MMC_CAP_UHS_SDR12	(1 << 15)	/* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR25	(1 << 16)	/* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR50	(1 << 17)	/* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR104	(1 << 18)	/* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_DDR50	(1 << 19)	/* Host supports UHS DDR50 mode */
#define MMC_CAP_SET_XPC_330	(1 << 20)	/* Host supports >150mA current at 3.3V */
#define MMC_CAP_SET_XPC_300	(1 << 21)	/* Host supports >150mA current at 3.0V */
#define MMC_CAP_SET_XPC_180	(1 << 22)	/* Host supports >150mA current at 1.8V */
#define MMC_CAP_DRIVER_TYPE_A	(1 << 23)	/* Host supports Driver Type A */
#define MMC_CAP_DRIVER_TYPE_C	(1 << 24)	/* Host supports Driver Type C */
#define MMC_CAP_DRIVER_TYPE_D	(1 << 25)	/* Host supports Driver Type D */
#define MMC_CAP_MAX_CURRENT_200	(1 << 26)	/* Host max current limit is 200mA */
#define MMC_CAP_MAX_CURRENT_400	(1 << 27)	/* Host max current limit is 400mA */
#define MMC_CAP_MAX_CURRENT_600	(1 << 28)	/* Host max current limit is 600mA */
#define MMC_CAP_MAX_CURRENT_800	(1 << 29)	/* Host max current limit is 800mA */
#define MMC_CAP_CMD23		(1 << 30)	/* CMD23 supported. */
#define MMC_CAP_HW_RESET	(1 << 31)	/* Hardware reset */

struct mmc_ios {
	unsigned int	clock;			/* clock rate */
	unsigned short	vdd;
	unsigned char	bus_mode;		/* command output mode */
	unsigned char	chip_select;		/* SPI chip select */
	unsigned char	power_mode;		/* power supply mode */
	unsigned char	bus_width;		/* data bus width */
	unsigned char	timing;			/* timing specification used */
	unsigned char	signal_voltage;		/* signalling voltage (1.8V or 3.3V) *///just SD
	unsigned char	drv_type;		/* driver type (A, B, C, D) *///just SD for driver strength
};
#define MMC_BUSMODE_OPENDRAIN	1
#define MMC_BUSMODE_PUSHPULL	2

#define MMC_CS_DONTCARE		0
#define MMC_CS_HIGH		1
#define MMC_CS_LOW		2

#define MMC_POWER_OFF		0
#define MMC_POWER_UP		1
#define MMC_POWER_ON		2

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	MMC_TIMING_LEGACY
#define MMC_TIMING_UHS_SDR25	MMC_TIMING_SD_HS
#define MMC_TIMING_UHS_SDR50	3
#define MMC_TIMING_UHS_SDR104	4
#define MMC_TIMING_UHS_DDR50	5
#define MMC_SDR_MODE		0
#define MMC_1_2V_DDR_MODE	1
#define MMC_1_8V_DDR_MODE	2

struct mmc_host {
	struct device		*parent; //linux dev
	struct mmc_card *card; /* device attached to this host *///add
	const struct mmc_host_ops *ops;
	u32			ocr;		/* the current OCR setting */

	unsigned int		f_init;
	unsigned int		f_min;
	unsigned int		f_max;

	/* private data */
	rtw_mutex		lock;		/* lock for claim and bus ops */
	rtw_sema	mrq_completion;

	/* SDIO IRQ */
	rtw_sema	sdio_irq_sema; /* used to wakeup sdio irq thread */
	unsigned int		sdio_irq_thread_abort;

	struct mmc_ios		ios;		/* current io bus settings */

	/* host specific block data *///add
	u32			ocr_avail;
	unsigned int		max_seg_size;	/* see blk_queue_max_segment_size */
	unsigned short		max_segs;	/* see blk_queue_max_segments */
	unsigned short		unused;
	unsigned int		max_req_size;	/* maximum number of bytes in one req */
	unsigned int		max_blk_size;	/* maximum size of one mmc block */
	unsigned int		max_blk_count;	/* maximum number of blocks in one req */
	unsigned int		max_discard_to;	/* max. discard timeout in ms */
	unsigned long		caps;		/* Host capabilities */

};

struct mmc_host_ops {
	int (*enable)(struct mmc_host *host);
	int (*disable)(struct mmc_host *host, int lazy);
	void	(*post_req)(struct mmc_host *host, struct mmc_request *req, int err);
	void	(*pre_req)(struct mmc_host *host, struct mmc_request *req, bool is_first_req);
	void	(*request)(struct mmc_host *host, struct mmc_request *req);
	void	(*set_ios)(struct mmc_host *host, struct mmc_ios *ios);
	int	(*get_ro)(struct mmc_host *host);
	int	(*get_cd)(struct mmc_host *host);

	void	(*enable_sdio_irq)(struct mmc_host *host, int enable);

	/* optional callback for HC quirks */
	void	(*init_card)(struct mmc_host *host, struct mmc_card *card);

	int	(*start_signal_voltage_switch)(struct mmc_host *host, struct mmc_ios *ios);
	int	(*execute_tuning)(struct mmc_host *host);
	void	(*enable_preset_value)(struct mmc_host *host, bool enable);
	int	(*select_drive_strength)(unsigned int max_dtr, int host_drv, int card_drv);
	void	(*hw_reset)(struct mmc_host *host);
};

#define mmc_dev(x)	((x)->parent)

static inline void mmc_signal_sdio_irq(struct mmc_host *host)
{
	host->ops->enable_sdio_irq(host, 0);
	rtw_sig_sem(&host->sdio_irq_sema);
}

struct mmc_card *mmc_alloc_card(struct mmc_host *host);
void mmc_release_card(struct mmc_card *card);

struct mmc_host *mmc_alloc_host(struct device *dev);
void mmc_free_host(struct mmc_host *host);
int mmc_add_host(struct mmc_host *host);
void mmc_remove_host(struct mmc_host *host);
#endif
