/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */


#include "rtl8195a.h"
#include "rtl8195a_sdio_host.h"
#include "hal_sdio_host.h"



extern HAL_TIMER_OP HalTimerOp;
static HAL_Status SdioHostErrIntRecovery(IN VOID *Data);



static HAL_Status
SdioHostSdClkCtrl(
	IN VOID *Data,
	IN u8 En,
	IN u8 Divisor
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u32 preState = HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE);
    HAL_Status ret = HAL_OK;


	/* check if SD bus is busy */
	if((!(preState & PRES_STATE_CMD_INHIBIT_DAT)) && (!(preState & PRES_STATE_CMD_INHIBIT_CMD)))
	{
		if(En)
		{
			/* Disable sd clock */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & (~CLK_CTRL_SD_CLK_EN));
			
			/* SDCLK/divisor (if needed) */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, (HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & 0xFF) | (Divisor << 8));
			
			/* Enable sd clock */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) | CLK_CTRL_SD_CLK_EN);

            switch(Divisor)
            {
                case BASE_CLK:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_41_6MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_2:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_20_8MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_4:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_10_4MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_8:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_5_2MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_16:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_2_6MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_32:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_1_3MHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_64:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_650KHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_128:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_325KHZ;
                    break;
                case BASE_CLK_DIVIDED_BY_256:
                    pSdioHostAdapter->CurrSdClk = SD_CLK_162KHZ;
                    break;
                default:
                    DBG_SDIO_ERR("Unsupported SDCLK divisor !!\n");
                    break;
            }
		}
		else
		{
			/* Disable sd clock */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & (~CLK_CTRL_SD_CLK_EN));
		}
	}
	else
		ret = HAL_BUSY;


	return ret;
}


static VOID
SdioHostSdBusPwrCtrl(
	IN u8 En
)
{
	if(En)
	{
		/* Power off sd bus */
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_PWR_CTRL) & (~PWR_CTRL_SD_BUS_PWR));

		/* Get supported voltage and set it */
		if(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_CAPABILITIES) & CAPA_VOLT_SUPPORT_33V)
		{
			DBG_SDIO_INFO("Supply SD bus voltage: 3.3V\n");
			HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, VOLT_33V << 1);
		}
		else if(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_CAPABILITIES) & CAPA_VOLT_SUPPORT_30V)
		{
			DBG_SDIO_INFO("Supply SD bus voltage: 3.0V\n");
			HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, VOLT_30V << 1);
		}
		else if(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_CAPABILITIES) & CAPA_VOLT_SUPPORT_18V)
		{
			DBG_SDIO_INFO("Supply SD bus voltage: 1.8V\n");
			HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, VOLT_18V << 1);
		}
		else
		{
			DBG_SDIO_ERR("No supported voltage\n");
		}

		/* Power on sd bus */
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_PWR_CTRL) | PWR_CTRL_SD_BUS_PWR);
	}
	else
	{
		/* Power off sd bus */
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_PWR_CTRL, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_PWR_CTRL) & (~PWR_CTRL_SD_BUS_PWR));
	}
}


static HAL_Status
SdioHostIsTimeout (
    u32 StartCount,
    u32 TimeoutCnt
)
{
    HAL_Status ret;
    u32 CurrentCount, ExpireCount;


    CurrentCount = HalTimerOp.HalTimerReadCount(1);
    if (StartCount < CurrentCount)
	{
		ExpireCount = (0xFFFFFFFF - CurrentCount) + StartCount;
    }
    else
	{
		ExpireCount = StartCount - CurrentCount;
    }

    if (TimeoutCnt < ExpireCount)
	{
        ret = HAL_TIMEOUT;
    }
    else
	{
        ret = HAL_OK;
    }

    return ret;
}

static HAL_Status 
SdioHostChkDAT0SingleLevel(
	IN u32 Timeout
)
{
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;


    if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
        TimeoutCount = (Timeout*1000/TIMER_TICK_US);
        StartCount = HalTimerOp.HalTimerReadCount(1);
    }

	while(!(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_DAT0_SIGNAL_LEVEL))
	{
		// check timeout
		if (TimeoutCount > 0)
		{
			if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
			{
				ret = HAL_TIMEOUT;
				break;
			}
		}
		else
		{
			if (Timeout == 0)
			{
				ret = HAL_BUSY;
				break;
			}
		}
	}

	return ret;
}


static HAL_Status 
SdioHostChkCmdInhibitCMD(
	IN u32 Timeout
)
{
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;


    if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
        TimeoutCount = (Timeout*1000/TIMER_TICK_US);
        StartCount = HalTimerOp.HalTimerReadCount(1);
    }

	while(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_CMD_INHIBIT_CMD)
	{
		// check timeout
		if (TimeoutCount > 0)
		{
			if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
			{
				ret = HAL_TIMEOUT;
				break;
			}
		}
		else
		{
			if (Timeout == 0)
			{
				ret = HAL_BUSY;
				break;
			}
		}
	}

	return ret;
}


static HAL_Status 
SdioHostChkCmdInhibitDAT(
	IN u32 Timeout
)
{
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;


    if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
        TimeoutCount = (Timeout*1000/TIMER_TICK_US);
        StartCount = HalTimerOp.HalTimerReadCount(1);
    }

	while(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_CMD_INHIBIT_DAT)
	{
		// check timeout
		if (TimeoutCount > 0)
		{
			if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
			{
				ret = HAL_TIMEOUT;
				break;
			}
		}
		else
		{
			if (Timeout == 0)
			{
				ret = HAL_BUSY;
				break;
			}
		}
	}

	return ret;
}


static HAL_Status 
SdioHostChkDataLineActive(
	IN u32 Timeout
)
{
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;


    if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
        TimeoutCount = (Timeout*1000/TIMER_TICK_US);
        StartCount = HalTimerOp.HalTimerReadCount(1);
    }

	while(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_DAT_LINE_ACTIVE)
	{
		// check timeout
		if (TimeoutCount > 0)
		{
			if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
			{
				ret = HAL_TIMEOUT;
				break;
			}
		}
		else
		{
			if (Timeout == 0)
			{
				ret = HAL_BUSY;
				break;
			}
		}

        asm volatile (
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
        );
	}

	return ret;
}


static HAL_Status 
SdioHostChkCmdComplete(
	IN VOID *Data,
	IN u32 Timeout
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
		TimeoutCount = (Timeout*1000/TIMER_TICK_US);
		StartCount = HalTimerOp.HalTimerReadCount(1);
	}

	while(!(pSdioHostAdapter->CmdCompleteFlg))
	{
        if(pSdioHostAdapter->ErrIntFlg)
        {
            ret = (SdioHostErrIntRecovery(pSdioHostAdapter));
            break;
        }
        else
        {
            // check timeout
            if (TimeoutCount > 0)
            {
                if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
                {
                    ret = HAL_TIMEOUT;
                    break;
                }
            }
            else
            {
                if (Timeout == 0)
                {
                    ret = HAL_BUSY;
                    break;
                }
            }
        }
#if 0
        asm volatile (
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
        );
#endif
	}

	return ret;
}


static HAL_Status 
SdioHostChkXferComplete(
	IN VOID *Data,
	IN u32 Timeout
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u32 TimeoutCount = 0;
	u32 StartCount = 0;
    HAL_Status ret = HAL_OK;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	if((Timeout != 0) && (Timeout != SDIO_HOST_WAIT_FOREVER))
	{
		TimeoutCount = (Timeout*1000/TIMER_TICK_US);
		StartCount = HalTimerOp.HalTimerReadCount(1);
	}

	// SW workaround for transfer complete interrupt issue: Host always generats interrupt after a fixed time starting from CRC status's end bit
	// Refer to https://jira.realtek.com/browse/IOTI-31
	while((!(pSdioHostAdapter->XferCompleteFlg)) || (!(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_DAT0_SIGNAL_LEVEL)))
	{
		if(pSdioHostAdapter->ErrIntFlg)
		{
            ret = (SdioHostErrIntRecovery(pSdioHostAdapter));
            break;
		}
		else
		{
			// check timeout
			if (TimeoutCount > 0)
			{
				if (HAL_TIMEOUT == SdioHostIsTimeout(StartCount, TimeoutCount))
				{
					ret = HAL_TIMEOUT;
					break;
				}
			}
			else
			{
				if (Timeout == 0)
				{
					ret = HAL_BUSY;
					break;
				}
			}
		}
#if 0
        asm volatile (
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
        );
#endif
	}

	return ret;
}


static VOID 
SdioHostSendCmd(
	IN SDIO_HOST_CMD *Cmd
)
{
	u16 tmpVal;

	tmpVal = (Cmd->CmdFmt.CmdIdx << 8) | (Cmd->CmdFmt.CmdType << 6) | (Cmd->CmdFmt.DataPresent << 5) |
			(Cmd->CmdFmt.CmdIdxChkEn << 4) | (Cmd->CmdFmt.CmdCrcChkEn << 3) | (Cmd->CmdFmt.RespType);

	/* Set related registers and then send command to the SD card */
	HAL_SDIO_HOST_WRITE32(REG_SDIO_HOST_ARG, Cmd->Arg);
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CMD, tmpVal);
}


static HAL_Status
SdioHostGetResponse(
	IN VOID *Data,
	IN u8 RspType
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;	
	
	pSdioHostAdapter->Response[0] = HAL_SDIO_HOST_READ32(REG_SDIO_HOST_RSP0);
	pSdioHostAdapter->Response[1] = HAL_SDIO_HOST_READ32(REG_SDIO_HOST_RSP2);
	
	if(RspType == RSP_LEN_136)
	{
		pSdioHostAdapter->Response[2] = HAL_SDIO_HOST_READ32(REG_SDIO_HOST_RSP4);
		pSdioHostAdapter->Response[3] = HAL_SDIO_HOST_READ32(REG_SDIO_HOST_RSP6);
	}

	return HAL_OK;
}


static VOID
SdioHostDataTransferConfig(
	IN u32 AdmaAddr,
	IN u8 Operation,
	IN u32 BlockCount,
	IN u16 BlockSize
)
{
	/* Set start address of the descriptor table */
	HAL_SDIO_HOST_WRITE32(REG_SDIO_HOST_ADMA_SYS_ADDR, AdmaAddr);

	/* Set block size */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_BLK_SIZE, BlockSize);

	if(Operation == READ_OP)
	{
		if(BlockCount == 1)
		{
			/* Set transfer mode register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_XFER_MODE, XFER_MODE_DATA_XFER_DIR | XFER_MODE_DMA_EN);
		}
		else
		{
			/* Set block count register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_BLK_CNT, BlockCount);
			/* Set transfer mode register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_XFER_MODE, XFER_MODE_MULT_SINGLE_BLK |
															XFER_MODE_DATA_XFER_DIR |
															XFER_MODE_AUTO_CMD12_EN |
															XFER_MODE_BLK_CNT_EN |
															XFER_MODE_DMA_EN);
		}
	}
	else if(Operation == WRITE_OP)
	{
		if(BlockCount == 1)
		{
			/* Set transfer mode register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_XFER_MODE, XFER_MODE_DMA_EN);
		}
		else
		{
			/* Set block count register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_BLK_CNT, BlockCount);
			/* Set transfer mode register */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_XFER_MODE, XFER_MODE_MULT_SINGLE_BLK |
															XFER_MODE_AUTO_CMD12_EN |
															XFER_MODE_BLK_CNT_EN |
															XFER_MODE_DMA_EN);
		}
	}
}


static HAL_Status
SdioHostResetCard(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD0 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;
	
	Cmd.CmdFmt.RespType = NO_RSP;
	Cmd.CmdFmt.CmdCrcChkEn = DISABLE;
	Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_GO_IDLE_STATE;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);

	return ret;
}


static HAL_Status
SdioHostVoltageCheck(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	const u8 CheckPattern = 0xAA;
	u8 Vhs = 0x1;  // 2.7-3.6V


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD8 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_IF_COND;
	Cmd.Arg = (Vhs << 8) | CheckPattern;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_SEND_IF_COND)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}
	if((pSdioHostAdapter->Response[0] & 0xFF) != CheckPattern)
	{
		DBG_SDIO_ERR("Echo-back of check pattern: %X\n", pSdioHostAdapter->Response[0] & 0xFF);
		return HAL_ERR_UNKNOWN;
	}
	if(pSdioHostAdapter->Response[0] & BIT8)
	{
		ret = HAL_OK;
	}
	else
	{
		DBG_SDIO_ERR("Voltage accepted error!\n");
		return HAL_ERR_UNKNOWN;
	}

	return ret;
}


static HAL_Status
SdioHostGetOCR(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	u32 Retry = (ACMD41_INIT_TIMEOUT/ACMD41_POLL_INTERVAL);
	u32 HostOCR = VDD_30_31 | VDD_31_32 | VDD_32_33 | VDD_33_34;
	u8 Hcs = SDHC_SUPPORT;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD55 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_APP_CMD;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_APP_CMD)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}
	if(!((pSdioHostAdapter->Response[0]) & R1_APP_CMD))
	{
		DBG_SDIO_ERR("ACMD isn't expected!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* Inquiry ACMD41 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = DISABLE;
	Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SD_SEND_OP_COND;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	pSdioHostAdapter->CardOCR = (pSdioHostAdapter->Response[0]) & 0xFFFFFF;
//	DBG_SDIO_INFO("Card's OCR register: 0x%06X\n", pSdioHostAdapter->CardOCR);

	while(Retry--)
	{
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			break;
		
		/* CMD55 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_NOR;
		
		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = NO_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_APP_CMD;
		Cmd.Arg = 0;
		
		SdioHostSendCmd(&Cmd);
		
		ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
		if(ret != HAL_OK)
			break;
		
		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

		if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_APP_CMD)
		{
			DBG_SDIO_ERR("Command index error !!\n");
			ret = HAL_ERR_UNKNOWN;
			break;
		}
		if(!((pSdioHostAdapter->Response[0]) & R1_APP_CMD))
		{
			DBG_SDIO_ERR("ACMD isn't expected!\n");
			ret = HAL_ERR_UNKNOWN;
			break;
		}

		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;

		/* First ACMD41 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_NOR;

		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = DISABLE;
		Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
		Cmd.CmdFmt.DataPresent = NO_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_SD_SEND_OP_COND;
		Cmd.Arg = (Hcs << 30) | HostOCR;

		SdioHostSendCmd(&Cmd);

		ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
		if(ret != HAL_OK)
			break;

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
		// check card if busy
		if((pSdioHostAdapter->Response[0]) & CARD_PWR_UP_STATUS)
		{
			break;
		}

		HalDelayUs(ACMD41_POLL_INTERVAL);
	}

	// TODO: need to change sd bus voltage if fail ?
	if(ret != HAL_OK)
		return ret;
	if(!Retry)
		return HAL_TIMEOUT;

	/* Check CCS bit */
	if((pSdioHostAdapter->Response[0]) & CARD_CAPA_STATUS)
	{
		pSdioHostAdapter->IsSdhc = 1;
		DBG_SDIO_INFO("This is a SDHC card\n");
	}
	else
	{
		pSdioHostAdapter->IsSdhc = 0;
		DBG_SDIO_INFO("This is a SDSC card\n");
	}

	return ret;
}


static HAL_Status
SdioHostGetCID(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD2 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_136;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_ALL_SEND_CID;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
#if 0
	/*	Get CID */
	DBG_SDIO_INFO("Manufacturer ID: %d\n",(pSdioHostAdapter->Response[3] >> 16) & 0xFF);
	DBG_SDIO_INFO("OEM/Application ID: %c%c\n", (pSdioHostAdapter->Response[3] >> 8) & 0xFF, (pSdioHostAdapter->Response[3]) & 0xFF);
	DBG_SDIO_INFO("Product name: %c%c%c%c%c\n", pSdioHostAdapter->Response[2] >> 24, (pSdioHostAdapter->Response[2] >> 16) & 0xFF, (pSdioHostAdapter->Response[2] >> 8) & 0xFF, pSdioHostAdapter->Response[2] & 0xFF, pSdioHostAdapter->Response[1] >> 24);
	DBG_SDIO_INFO("Serial number: 0x%2X%2X\n", pSdioHostAdapter->Response[1] & 0xFFFF, (pSdioHostAdapter->Response[0] >> 16) & 0xFFFF);
	DBG_SDIO_INFO("Manufacturing date: %d/%d (Year/Month)\n", ((pSdioHostAdapter->Response[0] >> 4) & 0xF) + 2000, (pSdioHostAdapter->Response[0]) & 0xF);
#endif
	return ret;
}


static HAL_Status
SdioHostGetRCA(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD3 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_RELATIVE_ADDR;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_SEND_RELATIVE_ADDR)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	/* Get RCA */
	pSdioHostAdapter->RCA = (pSdioHostAdapter->Response[0]) >> 16;
//	DBG_SDIO_INFO("RCA = 0x%04X\n", pSdioHostAdapter->RCA);

	return ret;
}


static HAL_Status
SdioHostGetCSD(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	u8 i;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD9 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_136;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_CSD;
	Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	/* Csd[0] <-> CSD register bit[127:120], ..., Csd[14] <-> CSD register bit[15:8] */
	pSdioHostAdapter->Csd[0] = ((pSdioHostAdapter->Response[3]) >> 16) & 0xFF;
	pSdioHostAdapter->Csd[1] = ((pSdioHostAdapter->Response[3]) >> 8) & 0xFF;
	pSdioHostAdapter->Csd[2] = ((pSdioHostAdapter->Response[3]) & 0xFF);
	for(i = 0; i < 3; i++)
	{
		pSdioHostAdapter->Csd[3+(i*4)]  = ((pSdioHostAdapter->Response[2-i]) >> 24) & 0xFF;
		pSdioHostAdapter->Csd[3+(i*4)+1] = ((pSdioHostAdapter->Response[2-i]) >> 16) & 0xFF;
		pSdioHostAdapter->Csd[3+(i*4)+2] = ((pSdioHostAdapter->Response[2-i]) >> 8) & 0xFF;
		pSdioHostAdapter->Csd[3+(i*4)+3] = ((pSdioHostAdapter->Response[2-i]) & 0xFF);
	}
	pSdioHostAdapter->Csd[15] = 0x01;  // bit[0] is always '1'

	#if 0
	if((pSdioHostAdapter->Response[3] >> 22) & 0x1)
	{
		/* CSD Version 2.0 */
		DBG_SDIO_INFO("Card Capacity: %d GB\n", (((pSdioHostAdapter->Response[1] >> 8) & 0x3FFFFF) + 1)*512/1024/1024);
	}
	else
	{
		/* CSD Version 1.0 */
		u16 c_size;
		u8 read_bl_len, c_size_mult;
		u32 tmp, pwr;

		c_size = ((pSdioHostAdapter->Response[2] & 0x3) << 10) + ((pSdioHostAdapter->Response[1] & 0xFFC00000) >> 22);
		read_bl_len = (pSdioHostAdapter->Response[2] >> 8) & 0xF;
		c_size_mult = (pSdioHostAdapter->Response[1] >> 7) & 0x7;
		tmp = 1;
		pwr = c_size_mult + 2 + read_bl_len;

		while(pwr > 0)
		{
			tmp *= 2;
			pwr--;
		}

		DBG_SDIO_INFO("Card Capacity: %d MB\n", ((c_size + 1)* tmp)/1024/1024);
	}
	#endif

	return ret;
}


static HAL_Status
SdioHostCardSelection(
	IN VOID *Data,
	IN u8 Select
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	if(Select == SEL_CARD)
	{
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkCmdInhibitDAT(100);
		if(ret != HAL_OK)
			return ret;

		/* CMD7 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_NOR;

		Cmd.CmdFmt.RespType = RSP_LEN_48_CHK_BUSY;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = NO_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_SELECT_DESELECT_CARD;
		Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

		SdioHostSendCmd(&Cmd);

		ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkXferComplete(pSdioHostAdapter, 5000);
		if(ret != HAL_OK)
			return ret;

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

		if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_SELECT_DESELECT_CARD)
		{
			DBG_SDIO_ERR("Command index error !!\n");
			return HAL_ERR_UNKNOWN;
		}
	}
	else
	{
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		
		/* CMD7 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_NOR;
		
		Cmd.CmdFmt.RespType = NO_RSP;
		Cmd.CmdFmt.CmdCrcChkEn = DISABLE;
		Cmd.CmdFmt.CmdIdxChkEn = DISABLE;
		Cmd.CmdFmt.DataPresent = NO_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_SELECT_DESELECT_CARD;
		Cmd.Arg = 0;
		
		SdioHostSendCmd(&Cmd);
		
		ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
		if(ret != HAL_OK)
			return ret;
	}

	return ret;
}


static HAL_Status
SdioHostSetBusWidth(
	IN VOID *Data,
	IN u32 BusWidth
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;
	if((BusWidth != BUS_1_BIT) && (BusWidth != BUS_4_BIT))
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD55 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_APP_CMD;
	Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_APP_CMD)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}
	if(!(pSdioHostAdapter->Response[0] & BIT5))
	{
		DBG_SDIO_ERR("ACMD isn't expected!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* ACMD6 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SWITCH_FUNC;
	Cmd.Arg = BusWidth;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_SWITCH_FUNC)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	/* Set the data width of the host */
	if(BusWidth == BUS_4_BIT)
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_HOST_CTRL, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_HOST_CTRL) | BIT1);
	else
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_HOST_CTRL, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_HOST_CTRL) & (~BIT1));

	return ret;
}


static HAL_Status
SdioHostGetSCR(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	ADMA2_DESC_FMT *tmpAdmaDesc = pSdioHostAdapter->AdmaDescTbl;
	u8 ScrVal[8] = {0};


	if((pSdioHostAdapter == NULL) || (tmpAdmaDesc == NULL))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)tmpAdmaDesc) & 0x3) || (((u32)ScrVal) & 0x3))
		return HAL_ERR_PARA;

	SdioHostDataTransferConfig((u32)tmpAdmaDesc, READ_OP, 1, SCR_REG_LEN);
	tmpAdmaDesc->Attrib1.Valid = 1;
	tmpAdmaDesc->Attrib1.End = 1;
	tmpAdmaDesc->Attrib1.Int = 0;
	tmpAdmaDesc->Attrib1.Act1 = 0;
	tmpAdmaDesc->Attrib1.Act2 = 1;
	tmpAdmaDesc->Len1 = SCR_REG_LEN;
	tmpAdmaDesc->Addr1 = (u32)ScrVal;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD55 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_APP_CMD;
	Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_APP_CMD)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}
	if(!(pSdioHostAdapter->Response[0] & BIT5))
	{
		DBG_SDIO_ERR("ACMD isn't expected!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkDataLineActive(100);
	if(ret != HAL_OK)
		return ret;

	/* ACMD51 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = WITH_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_SCR;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
			
	ret = SdioHostChkXferComplete(Data, 5000);

	if(ret != HAL_OK)
	{
        if(ret != HAL_SDH_RECOVERED)
        {
            if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
            {
                HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
            
                ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                if(ret != HAL_OK)
                    DBG_SDIO_ERR("Stop transmission error!\n");
            }
        }

        return HAL_ERR_UNKNOWN;
	}

	/* SCR bit[59:56] */
	pSdioHostAdapter->SdSpecVer = ScrVal[0] & 0xF;


	return ret;
}


static HAL_Status
SdioHostSwitchFunction(
	IN VOID *Data,
	IN u8 Mode,
	IN u8 Fn2Sel,
	IN u8 Fn1Sel,
	IN u8 *StatusBuf
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	ADMA2_DESC_FMT *tmpAdmaDesc = pSdioHostAdapter->AdmaDescTbl;


	if((pSdioHostAdapter == NULL) || (tmpAdmaDesc == NULL))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)tmpAdmaDesc) & 0x3) || (((u32)StatusBuf) & 0x3))
		return HAL_ERR_PARA;

	SdioHostDataTransferConfig((u32)tmpAdmaDesc, READ_OP, 1, SWITCH_FN_STATUS_LEN);
	tmpAdmaDesc->Attrib1.Valid = 1;
	tmpAdmaDesc->Attrib1.End = 1;
	tmpAdmaDesc->Attrib1.Int = 0;
	tmpAdmaDesc->Attrib1.Act1 = 0;
	tmpAdmaDesc->Attrib1.Act2 = 1;
	tmpAdmaDesc->Len1 = SWITCH_FN_STATUS_LEN;
	tmpAdmaDesc->Addr1 = (u32)StatusBuf;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkDataLineActive(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD6 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = WITH_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SWITCH_FUNC;
	Cmd.Arg = (Mode << 31) | 0xFFFF00 | (Fn2Sel << 4) | Fn1Sel;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	ret = SdioHostChkXferComplete(Data, 5000);

	if(ret != HAL_OK)
	{
        if(ret != HAL_SDH_RECOVERED)
        {
            if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
            {
                HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
            
                ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                if(ret != HAL_OK)
                    DBG_SDIO_ERR("Stop transmission error!\n");
            }
        }

		return HAL_ERR_UNKNOWN;
	}

	return ret;
}


static HAL_Status
SdioHostInitHostController(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u16 NorIntMask, ErrIntMask;
	u32 i, isTimeOut;
	HAL_Status ret = HAL_OK;


	/* Disable SDIO Device */
	ACTCK_SDIOD_CCTRL(OFF);
	SDIOD_ON_FCTRL(OFF);
	SDIOD_OFF_FCTRL(OFF);
	SDIOD_PIN_FCTRL(OFF);

	/* Enable SDIO Host */		
	ACTCK_SDIOH_CCTRL(ON);
	SLPCK_SDIOH_CCTRL(ON);
	PinCtrl(SDIOH, 0, ON);
	SDIOH_FCTRL(ON);

	/* Software reset for all */
	i = 0;
	isTimeOut = 0;
	HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_SW_RESET, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) | SW_RESET_FOR_ALL);
	do
	{
		i++;
		if(i == 1000)
			isTimeOut = 1;
	}while(HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) & SW_RESET_FOR_ALL);
	if(isTimeOut)
		return HAL_TIMEOUT;

	HalSdioHostIrqInitRtl8195a((VOID*)pSdioHostAdapter);	
	NorIntMask = BIT7 | BIT6 | BIT1 | BIT0;
	ErrIntMask = BIT8 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0;
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_STATUS_EN, NorIntMask);
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_SIG_EN, NorIntMask);
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS_EN, ErrIntMask);
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_SIG_EN, ErrIntMask);

	/* Enable internal clock & Wait for stable */
	i = 0;
	isTimeOut = 0;
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) | CLK_CTRL_INTERAL_CLK_EN);
	do
	{
		i++;
		if(i == 1000)
			isTimeOut = 1;
	}while(!(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & CLK_CTRL_INTERAL_CLK_STABLE));
	if(isTimeOut)
		return HAL_TIMEOUT;

	/* Enable SDIO host card detect circuit */
	HAL_WRITE32(0x40050000, 0x9000, HAL_READ32(0x40050000, 0x9000) | BIT10);

	/* Select 32-bit ADMA2 */
	if(HAL_SDIO_HOST_READ32(REG_SDIO_HOST_CAPABILITIES) & CAPA_ADMA2_SUPPORT)
	{
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_HOST_CTRL, (ADMA2_32BIT << 3));
	}

	/* Set data timeout to Max. */				
	HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_TIMEOUT_CTRL, 0xE);


	return ret;
}


static HAL_Status
SdioHostInitCard(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	/* Reset card */
	ret = SdioHostResetCard(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Reset sd card fail !!\n");
		return ret;
	}
	/* Voltage check */
	ret = SdioHostVoltageCheck(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Voltage check fail !!\n");
		return ret;
	}
	/* Get OCR */
	ret = SdioHostGetOCR(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Get OCR fail !!\n");
		return ret;
	}

	/* Delay for NID clock cycles (Spec. 4.12.1) */
	HalDelayUs(20);

	/* Get CID */
	ret = SdioHostGetCID(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Get CID fail !!\n");
		return ret;
	}
	/* Get RCA */
	ret = SdioHostGetRCA(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Get RCA fail !!\n");
		return ret;
	}

	/* Change to Normal Speed */
	SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK_DIVIDED_BY_2);

	/* Get CSD */
	ret = SdioHostGetCSD(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Get CSD fail !!\n");
		return ret;
	}
	/* Select Card */
	ret = SdioHostCardSelection(pSdioHostAdapter, SEL_CARD);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Select sd card fail !!\n");
		return ret;
	}
	/* Set to 4-bit mode */
	ret = SdioHostSetBusWidth(pSdioHostAdapter, BUS_4_BIT);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Set bus width fail !!\n");
		return ret;
	}
	/* Get SD card current state */
	ret = HalSdioHostGetCardStatusRtl8195a(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Get sd card current state fail !!\n");
		return ret;
	}

	if(pSdioHostAdapter->CardCurState != TRANSFER)
	{
		DBG_SDIO_ERR("The card isn't in TRANSFER state !! (Current state: %d)\n", pSdioHostAdapter->CardCurState);
		ret = HAL_ERR_UNKNOWN;
	}


	return ret;
}

u32
SdioHostIsrHandle(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u16 NorIntVal;
	u16 NorIntStatusVal;
	u16 ErrIntVal;

	NorIntVal = HAL_SDIO_HOST_READ16(REG_SDIO_HOST_NORMAL_INT_STATUS);

	/* Clear all normal interrupt status */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_STATUS, NorIntVal&0xFF);
	/* Disable all normal interrupt signal */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_SIG_EN, 0x0);

	if(NorIntVal)
	{
		if(NorIntVal & NOR_INT_STAT_CMD_COMP)
		{
			pSdioHostAdapter->CmdCompleteFlg = 1;
			//DBG_SDIO_ERR("comamnd done\n");
		}

		if(NorIntVal & NOR_INT_STAT_XFER_COMP)
		{
			pSdioHostAdapter->XferCompleteFlg = 1;
			/*check DAT error*/
			if(NorIntVal & NOR_INT_STAT_ERR_INT){
				ErrIntVal = HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS);
				if(ErrIntVal & (ERR_INT_STAT_DATA_CRC|ERR_INT_STAT_DATA_TIMEOUT|ERR_INT_STAT_DATA_END_BIT)){
					DBG_SDIO_ERR("XFER CP with ErrIntVal: 0x%04X /0x%04X -- TYPE 0x%02X\n",NorIntVal, ErrIntVal, pSdioHostAdapter->XferType);
					pSdioHostAdapter->errType = SDIO_ERR_DAT_CRC;
					if(pSdioHostAdapter->ErrorCallback)
						pSdioHostAdapter->ErrorCallback(pSdioHostAdapter->ErrorCbPara);
				}
			}else{
				//evoke uper layer if this is a block write or read transfer
				if(pSdioHostAdapter->XferType == SDIO_XFER_R || pSdioHostAdapter->XferType == SDIO_XFER_W){
					//DBG_SDIO_ERR("xfer status: (%d/%d)\n",pSdioHostAdapter->XferType, pSdioHostAdapter->XferStage);
					if(pSdioHostAdapter->XferCompCallback != NULL)
						pSdioHostAdapter->XferCompCallback(pSdioHostAdapter->XferCompCbPara);
				}
			}
		}

		if(NorIntVal & NOR_INT_STAT_CARD_INSERT)
		{
			SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK_DIVIDED_BY_128);
			SdioHostSdBusPwrCtrl(ENABLE);
			if(pSdioHostAdapter->CardInsertCallBack != NULL)
				pSdioHostAdapter->CardInsertCallBack(pSdioHostAdapter->CardInsertCbPara);
		}

		if(NorIntVal & NOR_INT_STAT_CARD_REMOVAL)
		{
			SdioHostSdBusPwrCtrl(DISABLE);
			SdioHostSdClkCtrl(pSdioHostAdapter, DISABLE, BASE_CLK);
			if(pSdioHostAdapter->CardRemoveCallBack != NULL)
				pSdioHostAdapter->CardRemoveCallBack(pSdioHostAdapter->CardRemoveCbPara);
		}

		if(NorIntVal & NOR_INT_STAT_CARD_INT){
			NorIntStatusVal = HAL_SDIO_HOST_READ16(REG_SDIO_HOST_NORMAL_INT_STATUS_EN);
			/* Clear Card Interrupt Status Enable*/
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_STATUS_EN, NorIntStatusVal&(~BIT8));
			DBG_SDIO_ERR("CARD INT: 0x%04X\n", NorIntVal);
			/* Set Card Interrupt Status Enable*/
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_STATUS_EN, NorIntStatusVal);
		}
		
		if(NorIntVal & NOR_INT_STAT_ERR_INT)
		{
			ErrIntVal = HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS);
			/* Disable all error interrupt signal */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_SIG_EN, 0);
			
			if(pSdioHostAdapter->XferType == SDIO_XFER_R || pSdioHostAdapter->XferType == SDIO_XFER_W){
				// handle blocks write or read xfer here
				DBG_SDIO_ERR("SDIO_XFER_WR ErrIntVal:0x%04X /0x%04X -- TYPE 0x%02X(%d)\n",NorIntVal, ErrIntVal,pSdioHostAdapter->XferType, pSdioHostAdapter->CmdCompleteFlg);

				if(pSdioHostAdapter->CmdCompleteFlg){
					// data stage error, handle here 
					SdioHostErrIntRecovery(pSdioHostAdapter);
				}
				else // command stage error, handle in SdioHostChkCmdComplete
					{
					DBG_SDIO_ERR("Read/Write command Error\n");
					pSdioHostAdapter->ErrIntFlg = 1; // will auto retry the command if recover done
				}
			}	
			else{
				// handle normal xfer error interrupt in pooling process
				pSdioHostAdapter->ErrIntFlg = 1;
			}
		}
	}

	/* Enable normal interrupt */
	//HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_STATUS_EN, BIT8| BIT7 | BIT6 | BIT1 | BIT0);
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_NORMAL_INT_SIG_EN, BIT7 | BIT6 | BIT1 | BIT0);

	return 0;
}


static HAL_Status
SdioHostIrqDeInit(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	PIRQ_HANDLE SdHostIrqHandle;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	SdHostIrqHandle = &(pSdioHostAdapter->IrqHandle);
	InterruptDis(SdHostIrqHandle);
	InterruptUnRegister(SdHostIrqHandle);

	return HAL_OK;
}


static HAL_Status
SdioHostErrIntRecovery(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	u16 tmpErrIntStatus;
	u32 i = 0;
    HAL_Status ret = HAL_ERR_UNKNOWN;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;	

    DBG_SDIO_ERR("Recovering error interrupt...\n");
	tmpErrIntStatus = HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS);
	
	/* Check if CMD Line Error */
	if(tmpErrIntStatus & 0xF)
	{
		/* Software reset for CMD line */
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_SW_RESET, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) | SW_RESET_FOR_CMD);
		do
		{
			i++;
		}while((HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) & SW_RESET_FOR_CMD) && (i <= 1000));
		if(i == 1000)
        {      
            DBG_SDIO_ERR("CMD line reset timeout !!\n");
			return HAL_TIMEOUT;
        }
	}
	/* Check if DAT Line Error */
	i = 0;
	if(tmpErrIntStatus & 0x70)
	{
		/* Software reset for DAT line */
		HAL_SDIO_HOST_WRITE8(REG_SDIO_HOST_SW_RESET, HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) | SW_RESET_FOR_DAT);
		do
		{
			i++;
		}while((HAL_SDIO_HOST_READ8(REG_SDIO_HOST_SW_RESET) & SW_RESET_FOR_DAT) && (i <= 1000));
		if(i == 1000)
        {      
            DBG_SDIO_ERR("DAT line reset timeout !!\n");
			return HAL_TIMEOUT;
        }
	}
	
	DBG_SDIO_ERR("Error interrupt status: 0x%04X\n", tmpErrIntStatus);
	/* Clear previous error status */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, tmpErrIntStatus);

	pSdioHostAdapter->ErrIntFlg = 0;

	
	/* Issue abort command (CMD12) */
	ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
	if(ret != HAL_OK)
	{
		DBG_SDIO_ERR("Stop transmission error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	/* Check Command Inhibit (DAT) & Command Inhibit (CMD) */
	i = 0;
	do
	{
		i++;
	}while((HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & (PRES_STATE_CMD_INHIBIT_CMD|PRES_STATE_CMD_INHIBIT_DAT)) && (i <= 1000));
	if(i == 1000)
		return HAL_TIMEOUT;

	/* check D03~D00 in error interrupt status register */
	if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS)&0x0F){
		DBG_SDIO_ERR("Non-recoverable error(1) !!\n");
		HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS));
		return HAL_ERR_UNKNOWN;
	}else{
		/* check DAT TIMEOUT error */
		if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS)&ERR_INT_STAT_DATA_TIMEOUT){
			DBG_SDIO_ERR("Non-recoverable error(2) !!\n");
			return HAL_ERR_UNKNOWN;
		}else{
			// delay at least 50us
			HalDelayUs(50);
			/* Check DAT[3:0] signal */
			if(((HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE)) & 0xF00000)!= 0xF00000)
			{
				DBG_SDIO_ERR("Non-recoverable error(3) !!\n");
				return HAL_ERR_UNKNOWN;
			}
			else
			{		  
				DBG_SDIO_ERR("Recoverable error...\n");
				ret = HAL_SDH_RECOVERED;
			}
		}
	}	
	
	/* Enable error interrupt signal */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_SIG_EN,BIT8| BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);

	return ret;
}


HAL_Status
HalSdioHostInitHostRtl8195a(
	IN VOID *Data
)
{
	HAL_Status ret = HAL_OK;


	/* Initialize host controller */
	ret = SdioHostInitHostController(Data);
	if(ret != HAL_OK)
		DBG_SDIO_ERR("SD host initialization FAIL !!\n");


	return ret;
}


HAL_Status
HalSdioHostInitCardRtl8195a(
	IN VOID *Data
)
{
	HAL_Status ret = HAL_OK;


		/* Initialize SD card */
		ret = SdioHostInitCard(Data);
		if(ret != HAL_OK)
			DBG_SDIO_ERR("SD card initialization FAIL !!\n");


	return ret;
}


HAL_Status
HalSdioHostDeInitRtl8195a(
	IN VOID *Data
)
{
	HAL_Status ret = HAL_OK;


	SdioHostSdBusPwrCtrl(DISABLE);
	ret = SdioHostSdClkCtrl(Data, DISABLE, BASE_CLK);
	if(ret == HAL_OK)
	{
		ret = SdioHostIrqDeInit(Data);
		if(ret == HAL_OK)
		{
			/* Disable internal clock */
			HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & (~CLK_CTRL_INTERAL_CLK_EN));

			/* Disable SDIO host card detect circuit */
			HAL_WRITE32(0x40050000, 0x9000, HAL_READ32(0x40050000, 0x9000) & (~BIT10));

			SDIOH_FCTRL(OFF);
			PinCtrl(SDIOH, 0, OFF);
			SLPCK_SDIOH_CCTRL(OFF);
			ACTCK_SDIOH_CCTRL(OFF);
		}
	}

	return ret;
}


HAL_Status
HalSdioHostEnableRtl8195a(
	IN VOID *Data
)
{
	HAL_Status ret = HAL_OK;


	ACTCK_SDIOH_CCTRL(ON);
	SLPCK_SDIOH_CCTRL(ON);

	/* Enable internal clock */
	HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) | CLK_CTRL_INTERAL_CLK_EN);

	/* Wait for internal clock is stable */
	do
	{
	}while(!(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & CLK_CTRL_INTERAL_CLK_STABLE));

	/* Supply SD clock to a SD card */
	ret = SdioHostSdClkCtrl(Data, ENABLE, BASE_CLK_DIVIDED_BY_2);


	return ret;
}


HAL_Status
HalSdioHostDisableRtl8195a(
	IN VOID *Data
)
{
	HAL_Status ret = HAL_OK;


	ret = SdioHostSdClkCtrl(Data, DISABLE, BASE_CLK);
	if(ret == HAL_OK)
	{
		/* Disable internal clock */
		HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_CLK_CTRL, HAL_SDIO_HOST_READ16(REG_SDIO_HOST_CLK_CTRL) & (~CLK_CTRL_INTERAL_CLK_EN));

		SLPCK_SDIOH_CCTRL(OFF);
		ACTCK_SDIOH_CCTRL(OFF);
	}

	return ret;
}


HAL_Status
HalSdioHostIrqInitRtl8195a(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	PIRQ_HANDLE	SdHostIrqHandle;


    if (pSdioHostAdapter == NULL)
        return HAL_ERR_PARA;

	SdHostIrqHandle = &(pSdioHostAdapter->IrqHandle);

	SdHostIrqHandle->Data = (u32) (pSdioHostAdapter);
	SdHostIrqHandle->IrqNum = SDIO_HOST_IRQ;
	SdHostIrqHandle->IrqFun = (IRQ_FUN) SdioHostIsrHandle;
	SdHostIrqHandle->Priority = 6;
	InterruptRegister(SdHostIrqHandle);
	InterruptEn(SdHostIrqHandle);

	return HAL_OK;
}


HAL_Status
HalSdioHostReadBlocksDmaRtl8195a(
	IN VOID *Data,
	IN u64 ReadAddr,
	IN u32 BlockCnt
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	u32 AdmaDescAddr;

check_prg:
	// read commands are not allowed while card is programing
	HalSdioHostGetCardStatusRtl8195a(Data);
	if(pSdioHostAdapter->CardCurState == PROGRAMMING){
		DBG_SDIO_WARN("CARD STATUS:PROGRAMMING\n");
		HalDelayUs(500);
		goto check_prg;
	}
	
	if((pSdioHostAdapter == NULL) || (BlockCnt > BLK_CNT_REG_MAX))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)(pSdioHostAdapter->AdmaDescTbl)) & 0x3) || (((u32)(pSdioHostAdapter->AdmaDescTbl->Addr1)) & 0x3))
		return HAL_ERR_PARA;

	AdmaDescAddr = (u32)(pSdioHostAdapter->AdmaDescTbl);

	if(pSdioHostAdapter->IsSdhc)
	{
		/* Unit: block */
		ReadAddr /= 512;
	}

retry:
	SdioHostDataTransferConfig(AdmaDescAddr, READ_OP, BlockCnt, DATA_BLK_LEN);

	if(BlockCnt > 1)
	{
		/* Multiple block */
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkDataLineActive(100);
		if(ret != HAL_OK)
			return ret;

		/* CMD18 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferCompleteFlg = 0;
		pSdioHostAdapter->ErrIntFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_R; // write or read blocks

		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = WITH_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_READ_MULTIPLE_BLOCK;
		Cmd.Arg = (u32)ReadAddr;

		SdioHostSendCmd(&Cmd);

		ret = SdioHostChkCmdComplete(Data, 500);
		if(ret != HAL_OK)
        {
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
			DBG_SDIO_ERR("read %d blocks command fail:0x%02X\n",BlockCnt, ret);
            return ret;
        }

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

#if 0
		/* check xfer timeout */
		ret = SdioHostChkXferComplete(Data, 5000);
		if(ret != HAL_OK)
		{
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }

            return ret;
		}

		/* check CRC */
		if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT5)
		{
			DBG_SDIO_ERR("Data CRC error!\n");
			return HAL_ERR_UNKNOWN;
		}
#endif		
	}
	else
	{
		/* Single block */
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkDataLineActive(100);
		if(ret != HAL_OK)
			return ret;

		/* CMD17 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_R; // write or read blocks

		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = WITH_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_READ_SINGLE_BLOCK;
		Cmd.Arg = (u32)ReadAddr;

		SdioHostSendCmd(&Cmd);

		ret = SdioHostChkCmdComplete(Data, 500);
		if(ret != HAL_OK)
        {
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
			DBG_SDIO_ERR("read %d blocks command fail:0x%02X\n",BlockCnt, ret);

            return ret;
        }

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

#if 0
		ret = SdioHostChkXferComplete(Data, 5000);
		if(ret != HAL_OK)
		{
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
            else
            {
                if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
                {
                    HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
                
                    ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                    if(ret != HAL_OK)
                        DBG_SDIO_ERR("Stop transmission error!\n");
                }
            }

			return HAL_ERR_UNKNOWN;
		}
#endif
	}
	//DBG_SDIO_ERR("read done, isr count %d\n", isrcnt);
	return ret;
}


HAL_Status
HalSdioHostWriteBlocksDmaRtl8195a(
	IN VOID *Data,
	IN u64 WriteAddr,
	IN u32 BlockCnt
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	u32 AdmaDescAddr;

	if((pSdioHostAdapter == NULL) || (BlockCnt > BLK_CNT_REG_MAX))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)(pSdioHostAdapter->AdmaDescTbl)) & 0x3) || (((u32)(pSdioHostAdapter->AdmaDescTbl->Addr1)) & 0x3))
		return HAL_ERR_PARA;

	AdmaDescAddr = (u32)(pSdioHostAdapter->AdmaDescTbl);

	if(pSdioHostAdapter->IsSdhc)
	{
		/* Unit: block */
		WriteAddr /= 512;
	}

	// wait until card exit busy status
	ret = SdioHostChkDAT0SingleLevel(500);
	if(ret != HAL_OK){
		DBG_SDIO_ERR("card busy TIMEOUT\n");
		return ret;
	}

retry:
	SdioHostDataTransferConfig(AdmaDescAddr, WRITE_OP, BlockCnt, DATA_BLK_LEN);

	if(BlockCnt > 1)
	{
		/* Multiple block */
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkDataLineActive(100);
		if(ret != HAL_OK)
			return ret;

		/* CMD25 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferCompleteFlg = 0;
		pSdioHostAdapter->ErrIntFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_W; // write or read blocks

		
		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = WITH_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_WRITE_MULTIPLE_BLOCK;
		Cmd.Arg = (u32)WriteAddr;

		SdioHostSendCmd(&Cmd);

		ret = SdioHostChkCmdComplete(Data, 500);
		if(ret != HAL_OK)
        {
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
			HalSdioHostGetCardStatusRtl8195a(Data);
			DBG_SDIO_ERR("DAT0 0x%04X(%d)\n",HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_DAT0_SIGNAL_LEVEL,pSdioHostAdapter->CardCurState);
			DBG_SDIO_ERR("write %d blocks command fail:0x%02X\n",BlockCnt, ret);

			return ret;
        }

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
		if(pSdioHostAdapter->Response[0] & R1_WP_VIOLATION)
		{
			DBG_SDIO_ERR("Write protect violation !!\n");
			return HAL_ERR_PARA;
		}

#if 0
        ret = SdioHostChkXferComplete(Data, 8000);
        if(ret != HAL_OK)
        {
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }

            return ret;
        }

		if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT5)
			return HAL_ERR_UNKNOWN;
		if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT4)
			return HAL_TIMEOUT;
#endif
	}
	else
	{
		/* Single block */
		ret = SdioHostChkCmdInhibitCMD(100);
		if(ret != HAL_OK)
			return ret;
		ret = SdioHostChkDataLineActive(100);
		if(ret != HAL_OK)
			return ret;

		/* CMD24 */
		pSdioHostAdapter->CmdCompleteFlg = 0;
		pSdioHostAdapter->XferCompleteFlg = 0;
		pSdioHostAdapter->XferType = SDIO_XFER_W; // write or read blocks
	
		Cmd.CmdFmt.RespType = RSP_LEN_48;
		Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
		Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
		Cmd.CmdFmt.DataPresent = WITH_DATA;
		Cmd.CmdFmt.CmdType = NORMAL;
		Cmd.CmdFmt.CmdIdx = CMD_WRITE_BLOCK;
		Cmd.Arg = (u32)WriteAddr;

		SdioHostSendCmd(&Cmd);
		ret = SdioHostChkCmdComplete(Data, 500);
		if(ret != HAL_OK)
        {
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
			
			HalSdioHostGetCardStatusRtl8195a(Data);
			DBG_SDIO_ERR("DAT0 0x%04X(%d)\n",HAL_SDIO_HOST_READ32(REG_SDIO_HOST_PRESENT_STATE) & PRES_STATE_DAT0_SIGNAL_LEVEL,pSdioHostAdapter->CardCurState);
			DBG_SDIO_ERR("write %d blocks command fail:0x%02X\n",BlockCnt, ret);
			
            return ret;
        }

		SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
		if(pSdioHostAdapter->Response[0] & R1_WP_VIOLATION)
		{
			DBG_SDIO_ERR("Write protect violation !!\n");
			return HAL_ERR_PARA;
		}
#if 0
		ret = SdioHostChkXferComplete(Data, 5000);
		if(ret != HAL_OK)
		{
            if(ret == HAL_SDH_RECOVERED)
            {
                goto retry;
            }
            else
            {
                if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
                {
                    HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
                
                    ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                    if(ret != HAL_OK)
                        DBG_SDIO_ERR("Stop transmission error!\n");
                }
            }

			return HAL_ERR_UNKNOWN;
		}
#endif
	}

	return ret;
}


HAL_Status
HalSdioHostStopTransferRtl8195a(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;
	
	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkCmdInhibitDAT(100);
	if(ret != HAL_OK)
		return ret;
	/* CMD12 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;
	pSdioHostAdapter->XferType = SDIO_XFER_NOR;

	Cmd.CmdFmt.RespType = RSP_LEN_48_CHK_BUSY;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = ABORT;
	Cmd.CmdFmt.CmdIdx = CMD_STOP_TRANSMISSION;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);
	
//	ret = SdioHostChkCmdComplete(Data, 50);
//	if(ret != HAL_OK)
//		return ret;
//	ret = SdioHostChkXferComplete(Data, 5000);
//	if(ret != HAL_OK)
//		return ret;

	return ret;
}


HAL_Status
HalSdioHostGetCardStatusRtl8195a(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD13 */
	pSdioHostAdapter->CmdCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_STATUS;
	Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_SEND_STATUS)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	pSdioHostAdapter->CardStatus = pSdioHostAdapter->Response[0];
	pSdioHostAdapter->CardCurState = ((pSdioHostAdapter->CardStatus) >> 9) & 0xF;

	return ret;
}


HAL_Status
HalSdioHostGetSdStatusRtl8195a(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	ADMA2_DESC_FMT *tmpAdmaDesc = pSdioHostAdapter->AdmaDescTbl;


	if((pSdioHostAdapter == NULL) || (tmpAdmaDesc == NULL))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)tmpAdmaDesc) & 0x3) || (((u32)(pSdioHostAdapter->SdStatus)) & 0x3))
		return HAL_ERR_PARA;

	SdioHostDataTransferConfig((u32)tmpAdmaDesc, READ_OP, 1, SD_STATUS_LEN);
	tmpAdmaDesc->Attrib1.Valid = 1;
	tmpAdmaDesc->Attrib1.End = 1;
	tmpAdmaDesc->Attrib1.Int = 0;
	tmpAdmaDesc->Attrib1.Act1 = 0;
	tmpAdmaDesc->Attrib1.Act2 = 1;
	tmpAdmaDesc->Len1 = SD_STATUS_LEN;
	tmpAdmaDesc->Addr1 = (u32)(pSdioHostAdapter->SdStatus);

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD55 */
	pSdioHostAdapter->CmdCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_APP_CMD;
	Cmd.Arg = (pSdioHostAdapter->RCA) << 16;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_APP_CMD)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}
	if(!(pSdioHostAdapter->Response[0] & BIT5))
	{
		DBG_SDIO_ERR("ACMD isn't expected!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkDataLineActive(100);
	if(ret != HAL_OK)
		return ret;

	/* ACMD13 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = WITH_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_SEND_STATUS;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
			
	ret = SdioHostChkXferComplete(Data, 5000);

	if(ret != HAL_OK)
	{
        if(ret != HAL_SDH_RECOVERED)
        {
            if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
            {
                HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
            
                ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                if(ret != HAL_OK)
                    DBG_SDIO_ERR("Stop transmission error!\n");
            }
        }

		return HAL_ERR_UNKNOWN;
	}

	return ret;
}


HAL_Status
HalSdioHostChangeSdClockRtl8195a(
	IN VOID *Data,
	IN u8 Frequency
)
{
    PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
    HAL_Status ret = HAL_OK;
    u8 StatusData[SWITCH_FN_STATUS_LEN];

    
    if((pSdioHostAdapter == NULL) || (Frequency < SD_CLK_5_2MHZ) || (Frequency > SD_CLK_41_6MHZ))
    {
        return HAL_ERR_PARA;
    }
    if(Frequency == (pSdioHostAdapter->CurrSdClk))
    {
        DBG_SDIO_INFO("Current SDCLK frequency is already the specified value...\n");
        return HAL_OK;
    }

    if(Frequency == SD_CLK_41_6MHZ)
    {
        /* Check if this card supports CMD6 */
        ret = SdioHostGetSCR(Data);
        if(ret != HAL_OK)
            return ret;

        if((pSdioHostAdapter->SdSpecVer) >= SD_VER_110)
        {
            /* Check if this card supports "High-Speed" */
            ret = SdioHostSwitchFunction(Data, CHECK_FN, FN2_KEEP_CURRENT, FN1_KEEP_CURRENT, StatusData);
            if(ret != HAL_OK)
                return ret;

            if(StatusData[13] & SWITCH_FN_GRP1_HIGH_SPEED)
            {
                /* Check if "High-Speed" can be switched */
                ret = SdioHostSwitchFunction(Data, CHECK_FN, FN2_KEEP_CURRENT, FN1_HIGH_SPEED, StatusData);
                if(ret != HAL_OK)
                    return ret;

                if((StatusData[16] & 0xF) == FN1_HIGH_SPEED)
                {
                    /* Check if busy status field exists */
                    if(StatusData[17] == BUSY_STATUS_DEFINED)
                    {
                        if((StatusData[29] & SWITCH_FN_GRP1_HIGH_SPEED) == BUSY_STATUS)
                        {
                            DBG_SDIO_ERR("\"High-Speed Function\" is busy !!\n");
                            return HAL_BUSY;
                        }
                    }

                    /* Switch to High-Speed */
                    ret = SdioHostSwitchFunction(Data, SWITCH_FN, FN2_KEEP_CURRENT, FN1_HIGH_SPEED, StatusData);
                    if(ret != HAL_OK)
                        return ret;

                    if((StatusData[16] & 0xF) == FN1_HIGH_SPEED)
                    {
                        /* Host also changes to High-Speed */
                        ret = SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK);
                        if(ret != HAL_OK)
                        {
                            DBG_SDIO_ERR("Host changes to High-Speed fail !!\n");
                            return ret;
                        }               
                    }
                    else
                    {
                        DBG_SDIO_ERR("Card changes to High-Speed fail !!\n");
                        ret = HAL_ERR_UNKNOWN;
                    }
                }
                else
                {
                    DBG_SDIO_ERR("\"High-Speed\" can't be switched !!\n");
                    return HAL_ERR_UNKNOWN;
                }
            }
            else
                DBG_SDIO_INFO("This card doesn't support \"High-Speed Function\" and can't change to high-speed...\n");
        }
        else
            DBG_SDIO_INFO("This card doesn't support CMD6 and can't change to high-speed...\n");
    }
    else
    {
        /* Change SDCLK to the specified frequency */
        switch(Frequency)
        {
            case SD_CLK_20_8MHZ:
                ret = SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK_DIVIDED_BY_2);
                break;
            case SD_CLK_10_4MHZ:
                ret = SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK_DIVIDED_BY_4);
                break;
            case SD_CLK_5_2MHZ:
                ret = SdioHostSdClkCtrl(pSdioHostAdapter, ENABLE, BASE_CLK_DIVIDED_BY_8);
                break;
            default:
                DBG_SDIO_ERR("Unsupported SDCLK frequency !!\n");
                ret = HAL_ERR_PARA;
                break;
        }

        if(ret != HAL_OK)
        {
            DBG_SDIO_ERR("Host changes clock fail !!\n");
            return ret;
        }
    }


    return ret;
}


HAL_Status
HalSdioHostEraseRtl8195a(
	IN VOID *Data,
	IN u64 StartAddr,
	IN u64 EndAddr
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;

	
	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	if(pSdioHostAdapter->IsSdhc)
	{
		/* Unit: block */
		StartAddr /= 512;
		EndAddr /= 512;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD32 */
	pSdioHostAdapter->CmdCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_ERASE_WR_BLK_START;
	Cmd.Arg = (u32)StartAddr;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_ERASE_WR_BLK_START)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD33 */
	pSdioHostAdapter->CmdCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_ERASE_WR_BLK_END;
	Cmd.Arg = (u32)EndAddr;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(pSdioHostAdapter, 50);
	if(ret != HAL_OK)
		return ret;

	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);

	if((pSdioHostAdapter->Response[1] & 0xFF) != CMD_ERASE_WR_BLK_END)
	{
		DBG_SDIO_ERR("Command index error !!\n");
		return HAL_ERR_UNKNOWN;
	}

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkCmdInhibitDAT(100);
	if(ret != HAL_OK)
		return ret;

	/* CMD38 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;

	Cmd.CmdFmt.RespType = RSP_LEN_48_CHK_BUSY;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = NO_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_ERASE;
	Cmd.Arg = 0;

	SdioHostSendCmd(&Cmd);

	ret = SdioHostChkCmdComplete(Data, 50);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkXferComplete(Data, 5000);
	if(ret != HAL_OK)
		return ret;


	return ret;
}


HAL_Status
HalSdioHostGetWriteProtectRtl8195a(
	IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;


	if(pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	/* Check if in Stand-by state */
	ret = HalSdioHostGetCardStatusRtl8195a(pSdioHostAdapter);
	if(ret == HAL_OK)
	{
		if(pSdioHostAdapter->CardCurState != STAND_BY)
		{
			if((pSdioHostAdapter->CardCurState == TRANSFER) || (pSdioHostAdapter->CardCurState == SENDING_DATA))
			{
				/* De-select card & put it into the stand-by state */
				ret = SdioHostCardSelection(pSdioHostAdapter, DESEL_CARD);
				if(ret != HAL_OK)
					return ret;
			}
			else
			{
				DBG_SDIO_ERR("Wrong card state !!\n");
				return HAL_ERR_UNKNOWN;
			}
		}
		/* Get CSD */
		ret = SdioHostGetCSD(pSdioHostAdapter);
		if(ret == HAL_OK)
		{
			/* CSD bit[12] */
			pSdioHostAdapter->IsWriteProtect = ((pSdioHostAdapter->Csd[14]) >> 4) & 0x1;

			/* Stand-by state -> Transfer state */
			ret = SdioHostCardSelection(pSdioHostAdapter, SEL_CARD);
			if(ret != HAL_OK)
				return ret;
		}
		else
		{
			DBG_SDIO_ERR("Get CSD fail !!\n");
			return ret;
		}
	}
	else
	{
		DBG_SDIO_ERR("Get card status fail !!\n");
		return ret;
	}

	return ret;
}


HAL_Status
HalSdioHostSetWriteProtectRtl8195a(
	IN VOID *Data,
	IN u8 Setting
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;
	SDIO_HOST_CMD Cmd;
	u32 i;
	u8 tmp[CSD_REG_LEN], pos;
	u8 crc = 0x00;
	u8 *msg = tmp;
	ADMA2_DESC_FMT *tmpAdmaDesc = pSdioHostAdapter->AdmaDescTbl;


	if((pSdioHostAdapter == NULL) || (tmpAdmaDesc == NULL))
		return HAL_ERR_PARA;
	/* Check if 4-byte alignment */
	if((((u32)tmpAdmaDesc) & 0x3) || (((u32)tmp) & 0x3))
		return HAL_ERR_PARA;

	for(i = 0; i < (CSD_REG_LEN-1); i++)
	{
		tmp[i] = pSdioHostAdapter->Csd[i];
	}

	/* Set/Reset the "TMP_WRITE_PROTECT" bit */
	if(Setting == SET_WRITE_PROTECT)
		tmp[14] |= BIT4;
	else
		tmp[14] &= (~BIT4);

	/* Calculate CRC7 */
	for(i = 0; i < 15; i++, msg++)
	{
		for(pos = 8; pos > 0; pos--)
		{
			crc <<= 1;
			if((crc >> 7)^(((*msg) >> (pos-1)) & 1))
				crc ^= 0x89;
		}
	}
	/* bit[7:1] is CRC */
	tmp[15] = (crc << 1) | 1;

	SdioHostDataTransferConfig((u32)tmpAdmaDesc, WRITE_OP, 1, CSD_REG_LEN);
	tmpAdmaDesc->Attrib1.Valid = 1;
	tmpAdmaDesc->Attrib1.End = 1;
	tmpAdmaDesc->Attrib1.Int = 0;
	tmpAdmaDesc->Attrib1.Act1 = 0;
	tmpAdmaDesc->Attrib1.Act2 = 1;
	tmpAdmaDesc->Len1 = CSD_REG_LEN;
	tmpAdmaDesc->Addr1 = (u32)tmp;

	ret = SdioHostChkCmdInhibitCMD(100);
	if(ret != HAL_OK)
		return ret;
	ret = SdioHostChkDataLineActive(100);
	if(ret != HAL_OK)
		return ret;
	
	/* CMD27 */
	pSdioHostAdapter->CmdCompleteFlg = 0;
	pSdioHostAdapter->XferCompleteFlg = 0;
	
	Cmd.CmdFmt.RespType = RSP_LEN_48;
	Cmd.CmdFmt.CmdCrcChkEn = ENABLE;
	Cmd.CmdFmt.CmdIdxChkEn = ENABLE;
	Cmd.CmdFmt.DataPresent = WITH_DATA;
	Cmd.CmdFmt.CmdType = NORMAL;
	Cmd.CmdFmt.CmdIdx = CMD_PROGRAM_CSD;
	Cmd.Arg = 0;
	
	SdioHostSendCmd(&Cmd);
	
	ret = SdioHostChkCmdComplete(Data, 50);
	if(ret != HAL_OK)
		return ret;
	
	SdioHostGetResponse(pSdioHostAdapter, Cmd.CmdFmt.RespType);
	if(pSdioHostAdapter->Response[0] & R1_WP_VIOLATION)
	{
		DBG_SDIO_ERR("Write protect violation !!\n");
		return HAL_ERR_PARA;
	}
	
	ret = SdioHostChkXferComplete(Data, 5000);
	if(ret != HAL_OK)
	{
        if(ret != HAL_SDH_RECOVERED)
        {
            if(HAL_SDIO_HOST_READ16(REG_SDIO_HOST_ERROR_INT_STATUS) & BIT9)
            {
                HAL_SDIO_HOST_WRITE16(REG_SDIO_HOST_ERROR_INT_STATUS, BIT9);
            
                ret = HalSdioHostStopTransferRtl8195a(pSdioHostAdapter);
                if(ret != HAL_OK)
                    DBG_SDIO_ERR("Stop transmission error!\n");
            }
        }

		return HAL_ERR_UNKNOWN;
	}

	return ret;
}




