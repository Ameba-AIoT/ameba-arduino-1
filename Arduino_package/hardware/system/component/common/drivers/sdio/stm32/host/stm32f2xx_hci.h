/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for the MMC / SD / SDIO cell found in:
 *
 * STM32F2XX SDIO
 */

#ifndef STM32F2XX_MMC_H
#define STM32F2XX_MMC_H

/* @brief SD host Interface register */
#define SDSTM32_POWER			0x00/*!< SDIO power control register,    Address offset: 0x00 */
#define SDSTM32_CLKCR			0x04/*!< SDI clock control register,     Address offset: 0x04 */
#define SDSTM32_ARG			0x08/*!< SDIO argument register,         Address offset: 0x08 */
#define SDSTM32_CMD			0x0C/*!< SDIO command register,          Address offset: 0x0C */
#define SDSTM32_RESPCMD		0x10/*!< SDIO command response register, Address offset: 0x10 */
#define SDSTM32_RESP1			0x14/*!< SDIO response 1 register,       Address offset: 0x14 */
#define SDSTM32_RESP2			0x18/*!< SDIO response 2 register,       Address offset: 0x18 */
#define SDSTM32_RESP3			0x1C/*!< SDIO response 3 register,       Address offset: 0x1C */
#define SDSTM32_RESP4			0x20/*!< SDIO response 4 register,       Address offset: 0x20 */
#define SDSTM32_DTIMER			0x24/*!< SDIO data timer register,       Address offset: 0x24 */
#define SDSTM32_DLEN			0x28/*!< SDIO data length register,      Address offset: 0x28 */
#define SDSTM32_DCTRL			0x2C/*!< SDIO data control register,     Address offset: 0x2C */
#define SDSTM32_DCOUNT			0x30/*!< SDIO data counter register,     Address offset: 0x30 */
#define SDSTM32_STA			0x34/*!< SDIO status register,           Address offset: 0x34 */
#define SDSTM32_ICR				0x38/*!< SDIO interrupt clear register,  Address offset: 0x38 */
#define SDSTM32_MASK			0x3C/*!< SDIO mask register,             Address offset: 0x3C */
#define SDSTM32_RESERVED0		0x40/*!< Reserved, 0x40-0x44                                  */
#define SDSTM32_FIFOCNT		0x48/*!< SDIO FIFO counter register,     Address offset: 0x48 */
#define SDSTM32_RESERVED1		0x4C/*!< Reserved, 0x4C-0x7C                                  */
#define SDSTM32_FIFO			0x80/*!< SDIO data FIFO register,        Address offset: 0x80 */

#ifndef PERIPH_BB_BASE
/* device BASE addr */
#define PERIPH_BB_BASE		((u32)0x42000000) /*!< Peripheral base address in the bit-band region */
#define PERIPH_BASE			((u32)0x40000000) /*!< Peripheral base address in the alias region */
#define APB2PERIPH_BASE	(PERIPH_BASE + 0x00010000)
#define SDIO_BASE			(APB2PERIPH_BASE + 0x2C00)

/* ==== SDI clock control register (SDIO_CLKCR) BitMask Start ==== */
/* --- CLKCR Register ---*/
/* CLKCR register clear mask */
#define CLKCR_CLEAR_MASK         ((u32)0xFFFF8100) 

/* Bit 13 NEGEDGE:SDIO_CK dephasing selection bit */
#define SDIO_ClockEdge_Rising			((unsigned int)0x00000000)
#define SDIO_ClockEdge_Falling			((unsigned int)0x00002000)
#define IS_SDIO_CLOCK_EDGE(EDGE)		(((EDGE) == SDIO_ClockEdge_Rising) || \
										((EDGE) == SDIO_ClockEdge_Falling))

/* Bit 10 BYPASS: Clock divider bypass enable bit */
#define SDIO_ClockBypass_Disable		((unsigned int)0x00000000) /* SDIOCLK is divided according to the CLKDIV value */
#define SDIO_ClockBypass_Enable			((unsigned int)0x00000400) /* SDIOCLK directly drives the SDIO_CK output signal (48M)*/
#define IS_SDIO_CLOCK_BYPASS(BYPASS)	(((BYPASS) == SDIO_ClockBypass_Disable) || \
										((BYPASS) == SDIO_ClockBypass_Enable))
/* Bit 9 PWRSAV: Power saving configuration bit */
#define SDIO_ClockPowerSave_Disable			((unsigned int)0x00000000)
#define SDIO_ClockPowerSave_Enable			((unsigned int)0x00000200) 
#define IS_SDIO_CLOCK_POWER_SAVE(SAVE)	(((SAVE) == SDIO_ClockPowerSave_Disable) || \
                                        ((SAVE) == SDIO_ClockPowerSave_Enable))

/* Bits 12:11 WIDBUS: Wide bus mode enable bit */
#define SDIO_BusWide_1b					((unsigned int)0x00000000)
#define SDIO_BusWide_4b					((unsigned int)0x00000800)
#define SDIO_BusWide_8b					((unsigned int)0x00001000)
#define IS_SDIO_BUS_WIDE(WIDE)			(((WIDE) == SDIO_BusWide_1b) || ((WIDE) == SDIO_BusWide_4b) || \
										((WIDE) == SDIO_BusWide_8b))

/* Bit 14 HWFC_EN: HW Flow Control enable */
#define SDIO_HardwareFlowControl_Disable			((unsigned int)0x00000000)
#define SDIO_HardwareFlowControl_Enable			((unsigned int)0x00004000)
#define IS_SDIO_HARDWARE_FLOW_CONTROL(CONTROL)	(((CONTROL) == SDIO_HardwareFlowControl_Disable) || \
													((CONTROL) == SDIO_HardwareFlowControl_Enable))
/* ==== SDI clock control register (SDIO_CLKCR) BitMask End ==== */

/* ==== SDIO power control register (SDIO_POWER) BitMask Start ==== */
#define SDIO_PowerState_OFF				((unsigned int)0x00000000)
#define SDIO_PowerState_ON				((unsigned int)0x00000003)
#define IS_SDIO_POWER_STATE(STATE)	(((STATE) == SDIO_PowerState_OFF) || ((STATE) == SDIO_PowerState_ON))
/* ==== SDIO power control register (SDIO_POWER) BitMask End ==== */


/* ==== SDIO status register (SDIO_STA ISR) BitMask Start ==== */
/* ==== SDIO interrupt clear register (SDIO_ICR) BitMask Start ==== */
/* ==== SDIO mask register (SDIO_MASK IMR) BitMask Start ==== */
#define SDIO_IT_CCRCFAIL				((unsigned int)0x00000001)/*!<Command response received (CRC check failed) */
#define SDIO_IT_DCRCFAIL				((unsigned int)0x00000002)/*!<Data block sent/received (CRC check failed) */
#define SDIO_IT_CTIMEOUT				((unsigned int)0x00000004)/*!<Command response timeout, The Command TimeOut period has a fixed value of 64 SDIO_CK clock periods */
#define SDIO_IT_DTIMEOUT				((unsigned int)0x00000008)/*!<Data timeout */
#define SDIO_IT_TXUNDERR				((unsigned int)0x00000010)/*!<Transmit FIFO underrun error */
#define SDIO_IT_RXOVERR					((unsigned int)0x00000020)/*!<Received FIFO overrun error */
#define SDIO_IT_CMDREND				((unsigned int)0x00000040)/*!<Command response received (CRC check passed) */
#define SDIO_IT_CMDSENT				((unsigned int)0x00000080)/*!<Command sent (no response required) */
#define SDIO_IT_DATAEND				((unsigned int)0x00000100)/*!<Data end (data counter, SDIDCOUNT, is zero) */
#define SDIO_IT_STBITERR				((unsigned int)0x00000200)/*!<Start bit not detected on all data signals in wide bus mode */
#define SDIO_IT_DBCKEND				((unsigned int)0x00000400)/*!<Data block sent/received (CRC check passed) */
#define SDIO_IT_CMDACT					((unsigned int)0x00000800)/*!<Command transfer in progress */
#define SDIO_IT_TXACT					((unsigned int)0x00001000)/*!<Data transmit in progress */
#define SDIO_IT_RXACT					((unsigned int)0x00002000)/*!<Data receive in progress */
#define SDIO_IT_TXFIFOHE				((unsigned int)0x00004000)/*!<Transmit FIFO Half Empty: at least 8 words can be written into the FIFO */
#define SDIO_IT_RXFIFOHF				((unsigned int)0x00008000) /*!<Receive FIFO Half Full: there are at least 8 words in the FIFO */
#define SDIO_IT_TXFIFOF					((unsigned int)0x00010000)/*!<Transmit FIFO full, When HW Flow Control is enabled, TXFIFOE signals becomes activated when the FIFO
contains 2 words.*/
#define SDIO_IT_RXFIFOF					((unsigned int)0x00020000)/*!<Receive FIFO full, When HW Flow Control is enabled, RXFIFOF signals becomes activated 2 words before the
FIFO is full. */
#define SDIO_IT_TXFIFOE					((unsigned int)0x00040000)/*!<Transmit FIFO empty */
#define SDIO_IT_RXFIFOE					((unsigned int)0x00080000)/*!<Receive FIFO empty */
#define SDIO_IT_TXDAVL					((unsigned int)0x00100000)/*!<Data available in transmit FIFO */
#define SDIO_IT_RXDAVL					((unsigned int)0x00200000)/*!<Data available in receive FIFO */
#define SDIO_IT_SDIOIT					((unsigned int)0x00400000)/*!<SDIO interrupt received */
#define SDIO_IT_CEATAEND				((unsigned int)0x00800000)/*!<CE-ATA command completion signal received for CMD61 */
#define IS_SDIO_IT(IT) 					((((IT) & (unsigned int)0xFF000000) == 0x00) && ((IT) != (unsigned int)0x00))

#define SDIO_FLAG_CCRCFAIL				((unsigned int)0x00000001)
#define SDIO_FLAG_DCRCFAIL				((unsigned int)0x00000002)
#define SDIO_FLAG_CTIMEOUT				((unsigned int)0x00000004)
#define SDIO_FLAG_DTIMEOUT			((unsigned int)0x00000008)
#define SDIO_FLAG_TXUNDERR			((unsigned int)0x00000010)
#define SDIO_FLAG_RXOVERR				((unsigned int)0x00000020)
#define SDIO_FLAG_CMDREND				((unsigned int)0x00000040)
#define SDIO_FLAG_CMDSENT				((unsigned int)0x00000080)
#define SDIO_FLAG_DATAEND				((unsigned int)0x00000100)
#define SDIO_FLAG_STBITERR				((unsigned int)0x00000200)
#define SDIO_FLAG_DBCKEND				((unsigned int)0x00000400)
#define SDIO_FLAG_CMDACT				((unsigned int)0x00000800)
#define SDIO_FLAG_TXACT				((unsigned int)0x00001000)
#define SDIO_FLAG_RXACT				((unsigned int)0x00002000)
#define SDIO_FLAG_TXFIFOHE				((unsigned int)0x00004000)
#define SDIO_FLAG_RXFIFOHF				((unsigned int)0x00008000)
#define SDIO_FLAG_TXFIFOF				((unsigned int)0x00010000)
#define SDIO_FLAG_RXFIFOF				((unsigned int)0x00020000)
#define SDIO_FLAG_TXFIFOE				((unsigned int)0x00040000)
#define SDIO_FLAG_RXFIFOE				((unsigned int)0x00080000)
#define SDIO_FLAG_TXDAVL				((unsigned int)0x00100000)
#define SDIO_FLAG_RXDAVL				((unsigned int)0x00200000)
#define SDIO_FLAG_SDIOIT				((unsigned int)0x00400000)
#define SDIO_FLAG_CEATAEND			((unsigned int)0x00800000)
#define IS_SDIO_FLAG(FLAG) (((FLAG)  == SDIO_FLAG_CCRCFAIL) || \
                            ((FLAG)  == SDIO_FLAG_DCRCFAIL) || \
                            ((FLAG)  == SDIO_FLAG_CTIMEOUT) || \
                            ((FLAG)  == SDIO_FLAG_DTIMEOUT) || \
                            ((FLAG)  == SDIO_FLAG_TXUNDERR) || \
                            ((FLAG)  == SDIO_FLAG_RXOVERR) || \
                            ((FLAG)  == SDIO_FLAG_CMDREND) || \
                            ((FLAG)  == SDIO_FLAG_CMDSENT) || \
                            ((FLAG)  == SDIO_FLAG_DATAEND) || \
                            ((FLAG)  == SDIO_FLAG_STBITERR) || \
                            ((FLAG)  == SDIO_FLAG_DBCKEND) || \
                            ((FLAG)  == SDIO_FLAG_CMDACT) || \
                            ((FLAG)  == SDIO_FLAG_TXACT) || \
                            ((FLAG)  == SDIO_FLAG_RXACT) || \
                            ((FLAG)  == SDIO_FLAG_TXFIFOHE) || \
                            ((FLAG)  == SDIO_FLAG_RXFIFOHF) || \
                            ((FLAG)  == SDIO_FLAG_TXFIFOF) || \
                            ((FLAG)  == SDIO_FLAG_RXFIFOF) || \
                            ((FLAG)  == SDIO_FLAG_TXFIFOE) || \
                            ((FLAG)  == SDIO_FLAG_RXFIFOE) || \
                            ((FLAG)  == SDIO_FLAG_TXDAVL) || \
                            ((FLAG)  == SDIO_FLAG_RXDAVL) || \
                            ((FLAG)  == SDIO_FLAG_SDIOIT) || \
                            ((FLAG)  == SDIO_FLAG_CEATAEND))

#define IS_SDIO_CLEAR_FLAG(FLAG) ((((FLAG) & (unsigned int)0xFF3FF800) == 0x00) && ((FLAG) != (unsigned int)0x00))

#define IS_SDIO_GET_IT(IT) (((IT)  == SDIO_IT_CCRCFAIL) || \
                            ((IT)  == SDIO_IT_DCRCFAIL) || \
                            ((IT)  == SDIO_IT_CTIMEOUT) || \
                            ((IT)  == SDIO_IT_DTIMEOUT) || \
                            ((IT)  == SDIO_IT_TXUNDERR) || \
                            ((IT)  == SDIO_IT_RXOVERR) || \
                            ((IT)  == SDIO_IT_CMDREND) || \
                            ((IT)  == SDIO_IT_CMDSENT) || \
                            ((IT)  == SDIO_IT_DATAEND) || \
                            ((IT)  == SDIO_IT_STBITERR) || \
                            ((IT)  == SDIO_IT_DBCKEND) || \
                            ((IT)  == SDIO_IT_CMDACT) || \
                            ((IT)  == SDIO_IT_TXACT) || \
                            ((IT)  == SDIO_IT_RXACT) || \
                            ((IT)  == SDIO_IT_TXFIFOHE) || \
                            ((IT)  == SDIO_IT_RXFIFOHF) || \
                            ((IT)  == SDIO_IT_TXFIFOF) || \
                            ((IT)  == SDIO_IT_RXFIFOF) || \
                            ((IT)  == SDIO_IT_TXFIFOE) || \
                            ((IT)  == SDIO_IT_RXFIFOE) || \
                            ((IT)  == SDIO_IT_TXDAVL) || \
                            ((IT)  == SDIO_IT_RXDAVL) || \
                            ((IT)  == SDIO_IT_SDIOIT) || \
                            ((IT)  == SDIO_IT_CEATAEND))

#define IS_SDIO_CLEAR_IT(IT) ((((IT) & (unsigned int)0xFF3FF800) == 0x00) && ((IT) != (unsigned int)0x00))

/* brief  SDIO Static flags  */
#define SDIO_STATIC_FLAGS               ((unsigned int)0x000005FF)

/* ==== SDIO status register (SDIO_STA ISR) BitMask End ==== */ 
/* ==== SDIO interrupt clear register (SDIO_ICR) BitMask End ====*/
/* ==== SDIO mask register (SDIO_MASK IMR) BitMask End ==== */

/* ==== SDIO command register (SDIO_CMD) BitMask Start ==== */
/* --- CMD Register ---*/
/* CMD Register clear mask */
#define CMD_CLEAR_MASK           			((unsigned int)0xFFFFF800)

/* Bits 7:6 WAITRESP: Wait for response bits */
#define SDIO_Response_No				((unsigned int)0x00000000)
#define SDIO_Response_Short				((unsigned int)0x00000040)
#define SDIO_Response_Long				((unsigned int)0x000000C0)
#define IS_SDIO_RESPONSE(RESPONSE)	(((RESPONSE) == SDIO_Response_No) || \
										((RESPONSE) == SDIO_Response_Short) || \
										((RESPONSE) == SDIO_Response_Long))
										
/*Bit 8 WAITINT: CPSM waits for interrupt request */
#define SDIO_Wait_No					((unsigned int)0x00000000) /*!< SDIO No Wait, TimeOut is enabled */
#define SDIO_Wait_IT						((unsigned int)0x00000100) /*!< SDIO Wait Interrupt Request */
/* Bit 9 WAITPEND: CPSM Waits for ends of data transfer (CmdPend internal signal). */
#define SDIO_Wait_Pend					((unsigned int)0x00000200) /*!< SDIO Wait End of transfer */
#define IS_SDIO_WAIT(WAIT)				(((WAIT) == SDIO_Wait_No) || ((WAIT) == SDIO_Wait_IT) || \
										((WAIT) == SDIO_Wait_Pend))
										
/* Bit 10 CPSMEN: Command path state machine (CPSM) Enable bit */
#define SDIO_CPSM_Disable				((unsigned int)0x00000000)
#define SDIO_CPSM_Enable				((unsigned int)0x00000400)
#define IS_SDIO_CPSM(CPSM)				(((CPSM) == SDIO_CPSM_Enable) || ((CPSM) == SDIO_CPSM_Disable))

/* Bit 11 SDIOSuspend: SD I/O suspend command */
#define SDIO_SUSPEND_Enable			((unsigned int)0x00000800)

/* Bit 12 ENCMDcompl: Enable CMD completion */
#define SDIO_ENCMD_COMPL				((unsigned int)0x00001000)

/* Bit 13 nIEN: not Interrupt Enable */
#define SDIO_NOINT_EN					((unsigned int)0x00002000) /* if this bit is 0, interrupts in the CE-ATA device are enabled */

/* Bit 14 ATACMD: CE-ATA command */
#define SDIO_ATACMD					((unsigned int)0x00004000) /* If ATACMD is set, the CPSM transfers CMD61. */
/* ==== SDIO command register (SDIO_CMD) BitMask End ==== */

/* ==== SDIO command response register (SDIO_RESPCMD) Start ==== */
/* Bits 5:0 RESPCMD: Response command index, Read-only bit field. Contains the command index of the last command response received */
/* Bits 31:6 Reserved, always read as 0. */
/* ==== SDIO command response register (SDIO_RESPCMD) End ==== */

/* ====SDIO data control register (SDIO_DCTRL) bit Mask Start ====*/
/* --- DCTRL Register ---*/
/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK         				((unsigned int)0xFFFFFF08)

/* BIT0 Data transfer enabled bit */
#define SDIO_DTEN             					((unsigned int)0x00000001)
/* defgroup SDIO_DPSM_State  */
#define SDIO_DPSM_Disable                    		((unsigned int)0x00000000)
#define SDIO_DPSM_Enable                    		((unsigned int)0x00000001)
#define IS_SDIO_DPSM(DPSM) (((DPSM) == SDIO_DPSM_Enable) || ((DPSM) == SDIO_DPSM_Disable))

/* Bit 1 DTDIR: Data transfer direction selection */
#define SDIO_TransferDir_ToCard             		((unsigned int)0x00000000)
#define SDIO_TransferDir_ToSDIO             		((unsigned int)0x00000002)
#define IS_SDIO_TRANSFER_DIR(DIR) (((DIR) == SDIO_TransferDir_ToCard) || \
                                   ((DIR) == SDIO_TransferDir_ToSDIO))

/* Bit 2 DTMODE: Data transfer mode selection 1: Stream or SDIO multibyte data transfer */
#define SDIO_TransferMode_Block             		((unsigned int)0x00000000)
#define SDIO_TransferMode_Stream            	((unsigned int)0x00000004)
#define IS_SDIO_TRANSFER_MODE(MODE) (((MODE) == SDIO_TransferMode_Stream) || \
                                     ((MODE) == SDIO_TransferMode_Block))

/* Bit 3 DMAEN: DMA enable bit */
#define SDIO_DMAEN             					((unsigned int)0x00000008)

/* Bits 7:4 DBLOCKSIZE: Data block size */
#define SDIO_DataBlockSize_1b               		((unsigned int)0x00000000)
#define SDIO_DataBlockSize_2b               		((unsigned int)0x00000010)
#define SDIO_DataBlockSize_4b              		((unsigned int)0x00000020)
#define SDIO_DataBlockSize_8b               		((unsigned int)0x00000030)
#define SDIO_DataBlockSize_16b              		((unsigned int)0x00000040)
#define SDIO_DataBlockSize_32b              		((unsigned int)0x00000050)
#define SDIO_DataBlockSize_64b              		((unsigned int)0x00000060)
#define SDIO_DataBlockSize_128b             		((unsigned int)0x00000070)
#define SDIO_DataBlockSize_256b             		((unsigned int)0x00000080)
#define SDIO_DataBlockSize_512b             		((unsigned int)0x00000090)
#define SDIO_DataBlockSize_1024b            		((unsigned int)0x000000A0)
#define SDIO_DataBlockSize_2048b            		((unsigned int)0x000000B0)
#define SDIO_DataBlockSize_4096b            		((unsigned int)0x000000C0)
#define SDIO_DataBlockSize_8192b            		((unsigned int)0x000000D0)
#define SDIO_DataBlockSize_16384b           	((unsigned int)0x000000E0)
#define IS_SDIO_BLOCK_SIZE(SIZE) (((SIZE) == SDIO_DataBlockSize_1b) || \
				((SIZE) == SDIO_DataBlockSize_2b) || \
				((SIZE) == SDIO_DataBlockSize_4b) || \
				((SIZE) == SDIO_DataBlockSize_8b) || \
				((SIZE) == SDIO_DataBlockSize_16b) || \
				((SIZE) == SDIO_DataBlockSize_32b) || \
				((SIZE) == SDIO_DataBlockSize_64b) || \
				((SIZE) == SDIO_DataBlockSize_128b) || \
				((SIZE) == SDIO_DataBlockSize_256b) || \
				((SIZE) == SDIO_DataBlockSize_512b) || \
				((SIZE) == SDIO_DataBlockSize_1024b) || \
				((SIZE) == SDIO_DataBlockSize_2048b) || \
				((SIZE) == SDIO_DataBlockSize_4096b) || \
				((SIZE) == SDIO_DataBlockSize_8192b) || \
				((SIZE) == SDIO_DataBlockSize_16384b)) 

/* Bit 8 RWSTART: Read wait start */
#define SDIO_RWSTART           					((unsigned int)0x00000100)

/* Bit 9 RWSTOP: Read wait stop */
#define SDIO_RWSTOP           					((unsigned int)0x00000200)

/* Bit 10 RWMOD: Read wait mode */
#define SDIO_RWMOD           					((unsigned int)0x00000400)

/* Bit 11 SDIOEN: SD I/O enable functions */
#define SDIO_SDIOEN           					((unsigned int)0x00000800)
/* ====SDIO data control register (SDIO_DCTRL) bit Mask End ====*/
#else
/* CLKCR register clear mask */
#define CLKCR_CLEAR_MASK         ((u32)0xFFFF8100) 

/* CMD Register clear mask */
#define CMD_CLEAR_MASK           			((unsigned int)0xFFFFF800)

/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK         				((unsigned int)0xFFFFFF08)

/* brief  SDIO Static flags  */
#define SDIO_STATIC_FLAGS               ((unsigned int)0x000005FF)

/* Bit 3 DMAEN: DMA enable bit */
#define SDIO_DMAEN             					((unsigned int)0x00000008)
/* Bit 11 SDIOEN: SD I/O enable functions */
#define SDIO_SDIOEN           					((unsigned int)0x00000800)
#endif

/* brief  TimeOut  */
#define SDIO_CMD0TIMEOUT                ((unsigned int)0x00010000)

struct stm32_pltf_ops {
	void (*stm32_clockcmd)(u32 newstate);
	void (*stm32_dmacmd)(u32 newstate);
};

struct stm32_mmc_host {
	void __iomem *ctl;
	unsigned long bus_shift;
	struct mmc_command      *cmd;
	struct mmc_request      *mrq;
	struct mmc_data         *data;
	struct mmc_host         *mmc;
	unsigned int		sdio_irq_enabled;

	const struct stm32_pltf_ops *ops;	/* Low level hw interface */

	/* pio related stuff */
	struct scatterlist      *sg_ptr;
	struct scatterlist      *sg_orig;
	unsigned int            sg_len;
	unsigned int            sg_off;

	/* dma */
	unsigned int            dma_endof_transfer;
	unsigned int            transfer_end;

	/* Track lost interrupts */
	//struct workqueue_struct *work;
	//struct work_struct	done;
	struct timer_list	timer;	/* Timer for timeouts */

	rtw_spinlock_t		lock;		/* protect host private data */
	unsigned long		last_req_ts;
	rtw_mutex		ios_lock;	/* protect set_ios() context */
};

int stm32_mmc_host_probe(struct stm32_mmc_host *host);
void stm32_mmc_host_remove(struct stm32_mmc_host *host);
irqreturn_t stm32_mmc_irq(int irq, void *devid);
static inline char *stm32_mmc_kmap_atomic(struct scatterlist *sg,
					 unsigned long *flags)
{
	return (char *)sg->dma_address;
}

static inline void stm32_mmc_kunmap_atomic(struct scatterlist *sg,
					  unsigned long *flags, void *virt)
{
}

irqreturn_t stm32_mmc_irq(int irq, void *devid);
struct mmc_host *stm32_alloc_host(struct device *dev, void* priv);

#endif
