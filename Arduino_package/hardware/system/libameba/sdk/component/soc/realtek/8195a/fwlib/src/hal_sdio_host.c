/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "rtl8195a.h"

#ifdef CONFIG_SDIO_HOST_EN
#include "rtl8195a_sdio_host.h"
#include "hal_sdio_host.h"


extern VOID QueryRegPwrState(u8  FuncIdx, u8* RegState, u8* HwState);


HAL_Status 
HalSdioHostInit(
    IN VOID *Data
)
{
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;
#ifdef CONFIG_SOC_PS_MODULE
	REG_POWER_STATE SdioHostPwrState;
#endif


	if (FunctionChk(SDIOH, S0) == _FALSE)
		return HAL_ERR_UNKNOWN;

	if (pSdioHostAdapter == NULL)
		return HAL_ERR_PARA;

	ret = HalSdioHostInitHostRtl8195a(pSdioHostAdapter);
	if(ret == HAL_OK)	
		ret = HalSdioHostInitCardRtl8195a(pSdioHostAdapter);
	else
		return HAL_ERR_HW;

#ifdef CONFIG_SOC_PS_MODULE
	if(ret == HAL_OK)
	{
		// To register a new peripheral device power state
		SdioHostPwrState.FuncIdx = SDIOH;
		SdioHostPwrState.PwrState = ACT;
		RegPowerState(SdioHostPwrState);
	}
#endif

	return ret;
}


HAL_Status 
HalSdioHostDeInit(
    IN VOID *Data
)
{
#ifdef CONFIG_SOC_PS_MODULE
	REG_POWER_STATE SdioHostPwrState;
	u8 HwState;


	SdioHostPwrState.FuncIdx = SDIOH;
	QueryRegPwrState(SdioHostPwrState.FuncIdx, &(SdioHostPwrState.PwrState), &HwState);

	// if the power state isn't ACT, then switch the power state back to ACT first
	if ((SdioHostPwrState.PwrState != ACT) && (SdioHostPwrState.PwrState != INACT))
	{
		HalSdioHostEnable(Data);
		QueryRegPwrState(SdioHostPwrState.FuncIdx, &(SdioHostPwrState.PwrState), &HwState);
	}

	if (SdioHostPwrState.PwrState == ACT)
	{
		SdioHostPwrState.PwrState = INACT;
		RegPowerState(SdioHostPwrState);
	}	 
#endif
	PHAL_SDIO_HOST_ADAPTER pSdioHostAdapter = (PHAL_SDIO_HOST_ADAPTER) Data;
	HAL_Status ret = HAL_OK;


    if (pSdioHostAdapter == NULL)
        return HAL_ERR_PARA;

	ret = HalSdioHostDeInitRtl8195a(pSdioHostAdapter);

	return ret;
}


HAL_Status 
HalSdioHostEnable(
	IN VOID *Data
)
{
	HAL_Status ret;
#ifdef CONFIG_SOC_PS_MODULE
	REG_POWER_STATE SdioHostPwrState;
#endif


	ret = HalSdioHostEnableRtl8195a(Data);
#ifdef CONFIG_SOC_PS_MODULE
	if (ret == HAL_OK)
	{
		SdioHostPwrState.FuncIdx = SDIOH;
		SdioHostPwrState.PwrState = ACT;
		RegPowerState(SdioHostPwrState);
	}
#endif

	return ret;
}


HAL_Status 
HalSdioHostDisable(
	IN VOID *Data
)
{
	HAL_Status ret;
#ifdef CONFIG_SOC_PS_MODULE
	REG_POWER_STATE SdioHostPwrState;
#endif


	ret = HalSdioHostDisableRtl8195a(Data);
#ifdef CONFIG_SOC_PS_MODULE
	if (ret == HAL_OK)
	{
		SdioHostPwrState.FuncIdx = SDIOH;
		SdioHostPwrState.PwrState = SLPCG;
		RegPowerState(SdioHostPwrState);
	}
#endif

	return ret;
}


VOID
HalSdioHostOpInit(
    IN VOID *Data
)
{
	PHAL_SDIO_HOST_OP pHalSdioHostOp = (PHAL_SDIO_HOST_OP) Data;


	pHalSdioHostOp->HalSdioHostInitHost = HalSdioHostInitHostRtl8195a;
	pHalSdioHostOp->HalSdioHostInitCard = HalSdioHostInitCardRtl8195a;
	pHalSdioHostOp->HalSdioHostDeInit = HalSdioHostDeInitRtl8195a;	
	pHalSdioHostOp->HalSdioHostRegIrq = HalSdioHostIrqInitRtl8195a;
	pHalSdioHostOp->HalSdioHostReadBlocksDma = HalSdioHostReadBlocksDmaRtl8195a;
	pHalSdioHostOp->HalSdioHostWriteBlocksDma = HalSdioHostWriteBlocksDmaRtl8195a;	
	pHalSdioHostOp->HalSdioHostStopTransfer = HalSdioHostStopTransferRtl8195a;
	pHalSdioHostOp->HalSdioHostGetCardStatus = HalSdioHostGetCardStatusRtl8195a;
	pHalSdioHostOp->HalSdioHostGetSdStatus = HalSdioHostGetSdStatusRtl8195a;
	pHalSdioHostOp->HalSdioHostChangeSdClock = HalSdioHostChangeSdClockRtl8195a;
	pHalSdioHostOp->HalSdioHostErase = HalSdioHostEraseRtl8195a;
	pHalSdioHostOp->HalSdioHostGetWriteProtect = HalSdioHostGetWriteProtectRtl8195a;
	pHalSdioHostOp->HalSdioHostSetWriteProtect = HalSdioHostSetWriteProtectRtl8195a;
}

#endif  // end of "#ifdef CONFIG_SDIO_HOST_EN"


