#ifndef _MMC_SDIO_H
#define _MMC_SDIO_H

/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */

/* Standard MMC commands (4.1)           type  argument     response */
   /* class 1 */
#define MMC_GO_IDLE_STATE         0   /* bc                          */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define MMC_BUS_TEST_R           14   /* adtc                    R1  */

/* class 2 */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */
#define MMC_SEND_TUNING_BLOCK    19   /* adtc                    R1  */

/* class 4 */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */

/*
 * OCR bits are mostly in host.h
 */
#define MMC_CARD_BUSY	0x80000000	/* Card Power up status bit */

/* SDIO commands                         type  argument     response */
#define SD_IO_SEND_OP_COND          5 /* bcr  [23:0] OCR         R4  */
#define SD_IO_RW_DIRECT            52 /* ac   [31:0] See below   R5  */
#define SD_IO_RW_EXTENDED          53 /* adtc [31:0] See below   R5  */

/*
  SDIO status in R5
  Type
	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R5_COM_CRC_ERROR	(1 << 15)	/* er, b */
#define R5_ILLEGAL_COMMAND	(1 << 14)	/* er, b */
#define R5_ERROR		(1 << 11)	/* erx, c */
#define R5_FUNCTION_NUMBER	(1 << 9)	/* er, c */
#define R5_OUT_OF_RANGE		(1 << 8)	/* er, c */
#define R5_STATUS(x)		(x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	((x & 0x3000) >> 12) /* s, b */


/* Card Common Control Registers (CCCR) */
#define SDIO_CCCR_CCCR		0x00

#define  SDIO_CCCR_REV_1_00	0	/* CCCR/FBR Version 1.00 */
#define  SDIO_CCCR_REV_1_10	1	/* CCCR/FBR Version 1.10 */
#define  SDIO_CCCR_REV_1_20	2	/* CCCR/FBR Version 1.20 */
#define  SDIO_CCCR_REV_3_00	3	/* CCCR/FBR Version 3.00 */

#define  SDIO_SDIO_REV_1_00	0	/* SDIO Spec Version 1.00 */
#define  SDIO_SDIO_REV_1_10	1	/* SDIO Spec Version 1.10 */
#define  SDIO_SDIO_REV_1_20	2	/* SDIO Spec Version 1.20 */
#define  SDIO_SDIO_REV_2_00	3	/* SDIO Spec Version 2.00 */
#define  SDIO_SDIO_REV_3_00	4	/* SDIO Spec Version 3.00 */

#define SDIO_CCCR_SD		0x01

#define  SDIO_SD_REV_1_01	0	/* SD Physical Spec Version 1.01 */
#define  SDIO_SD_REV_1_10	1	/* SD Physical Spec Version 1.10 */
#define  SDIO_SD_REV_2_00	2	/* SD Physical Spec Version 2.00 */

#define SDIO_CCCR_IOEx		0x02
#define SDIO_CCCR_IORx		0x03

#define SDIO_CCCR_IENx		0x04	/* Function/Master Interrupt Enable */
#define SDIO_CCCR_INTx		0x05	/* Function Interrupt Pending */

#define SDIO_CCCR_ABORT		0x06	/* function abort/card reset */

#define SDIO_CCCR_IF		0x07	/* bus interface controls */

#define  SDIO_BUS_WIDTH_1BIT	0x00
#define  SDIO_BUS_WIDTH_4BIT	0x02
#define  SDIO_BUS_ECSI		0x20	/* Enable continuous SPI interrupt */
#define  SDIO_BUS_SCSI		0x40	/* Support continuous SPI interrupt */

#define  SDIO_BUS_ASYNC_INT	0x20

#define  SDIO_BUS_CD_DISABLE     0x80	/* disable pull-up on DAT3 (pin 1) */

#define SDIO_CCCR_CAPS		0x08

#define  SDIO_CCCR_CAP_SDC	0x01	/* can do CMD52 while data transfer */
#define  SDIO_CCCR_CAP_SMB	0x02	/* can do multi-block xfers (CMD53) */
#define  SDIO_CCCR_CAP_SRW	0x04	/* supports read-wait protocol */
#define  SDIO_CCCR_CAP_SBS	0x08	/* supports suspend/resume */
#define  SDIO_CCCR_CAP_S4MI	0x10	/* interrupt during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_E4MI	0x20	/* enable ints during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_LSC	0x40	/* low speed card */
#define  SDIO_CCCR_CAP_4BLS	0x80	/* 4 bit low speed card */

#define SDIO_CCCR_CIS		0x09	/* common CIS pointer (3 bytes) */

/* Following 4 regs are valid only if SBS is set */
#define SDIO_CCCR_SUSPEND	0x0c
#define SDIO_CCCR_SELx		0x0d
#define SDIO_CCCR_EXECx		0x0e
#define SDIO_CCCR_READYx	0x0f

#define SDIO_CCCR_BLKSIZE	0x10

#define SDIO_CCCR_POWER		0x12

#define  SDIO_POWER_SMPC	0x01	/* Supports Master Power Control */
#define  SDIO_POWER_EMPC	0x02	/* Enable Master Power Control */

#define SDIO_CCCR_SPEED		0x13

#define  SDIO_SPEED_SHS		0x01	/* Supports High-Speed mode */
#define  SDIO_SPEED_EHS		0x02	/* Enable High-Speed mode */

/*
 * Function Basic Registers (FBR)
 */

#define SDIO_FBR_BASE(f)	((f) * 0x100) /* base of function f's FBRs */

#define SDIO_FBR_STD_IF		0x00

#define  SDIO_FBR_SUPPORTS_CSA	0x40	/* supports Code Storage Area */
#define  SDIO_FBR_ENABLE_CSA	0x80	/* enable Code Storage Area */

#define SDIO_FBR_STD_IF_EXT	0x01

#define SDIO_FBR_POWER		0x02

#define  SDIO_FBR_POWER_SPS	0x01	/* Supports Power Selection */
#define  SDIO_FBR_POWER_EPS	0x02	/* Enable (low) Power Selection */

#define SDIO_FBR_CIS		0x09	/* CIS pointer (3 bytes) */


#define SDIO_FBR_CSA		0x0C	/* CSA pointer (3 bytes) */

#define SDIO_FBR_CSA_DATA	0x0F

#define SDIO_FBR_BLKSIZE	0x10	/* block size (2 bytes) */

static inline bool mmc_op_multi(u32 opcode)
{
	return opcode == MMC_WRITE_MULTIPLE_BLOCK ||
	       opcode == MMC_READ_MULTIPLE_BLOCK;
}

int mmc_attach_sdio(struct mmc_host * host);
void mmc_set_ios(struct mmc_host *host);
int mmc_go_idle(struct mmc_host *host);
int mmc_send_if_cond(struct mmc_host *host, u32 ocr);
#endif
